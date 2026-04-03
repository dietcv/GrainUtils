#include "Oscs.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable *ft;

// ===== HELPER FUNCTION =====

static bool getBufferData(OscUtils::BufUnit& bufUnit, float bufNum, int nSamples,
                         World* world, Graph* parent,
                         const float*& bufData, int& tableSize, const char* oscName) {
    const SndBuf* buf;
    const auto verify_buf = bufUnit.GetTable(world, parent, bufNum, nSamples, buf, bufData, tableSize);
    if (!verify_buf) {
        if (!bufUnit.m_buf_failed) {
            Print("%s: buffer not found\n", oscName);
            bufUnit.m_buf_failed = true;
        }
        return false;
    }
    bufUnit.m_buf_failed = false;
    return true;
}

// ===== SINGLE WAVETABLE OSCILLATOR =====

SingleOscOS::SingleOscOS() : m_sampleRate(static_cast<float>(sampleRate()))
{
    // Initialize parameter cache
    cyclePosPast = sc_clip(in0(CyclePos), 0.0f, 1.0f);
    
    // Check which inputs are audio-rate
    isCyclePosAudioRate = isAudioRateIn(CyclePos);
    
    // Initialize oversampling
    m_outputOversampling.reset(m_sampleRate);
    m_cyclePosOversampling.reset(m_sampleRate);
    
    // Setup oversampling index
    m_oversampleIndex = sc_clip(static_cast<int>(in0(Oversample)), 0, 4);
    m_outputOversampling.setOversamplingIndex(m_oversampleIndex);
    m_cyclePosOversampling.setOversamplingIndex(m_oversampleIndex);
    
    // Store ratio and buffer pointers
    m_osRatio = m_outputOversampling.getOversamplingRatio();
    m_outputOSBuffer = m_outputOversampling.getOSBuffer();
    m_osCyclePosBuffer = m_cyclePosOversampling.getOSBuffer();
    
    mCalcFunc = make_calc_function<SingleOscOS, &SingleOscOS::next>();
    next(1);
}

SingleOscOS::~SingleOscOS() = default;

void SingleOscOS::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    auto slopedCyclePos = makeSlope(sc_clip(in0(CyclePos), 0.0f, 1.0f), cyclePosPast);

    // Control-rate parameters (settings)
    const float numCycles = sc_max(in0(NumCycles), 1.0f);
    const float bufNum = in0(BufNum);

    // Output pointer
    float* output = out(Out);
    
    // Get buffer data
    const float* bufData;
    int tableSize;
    
    if (!getBufferData(m_bufUnit, bufNum, nSamples, mWorld, mParent, bufData, tableSize, "SingleOscOS")) {
        ClearUnitOutputs(this, nSamples);
        return;
    }
    
    // Pre-calculate constants
    const int cycleSamples = tableSize / static_cast<int>(numCycles);
    const int numCyclesInt = static_cast<int>(numCycles);
    
    if (m_oversampleIndex == 0) {

        for (int i = 0; i < nSamples; ++i) {
            
            // Wrap phase between 0 and 1
            float phase = sc_frac(phaseIn[i]);
            
            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePosVal = isCyclePosAudioRate ? 
                sc_clip(in(CyclePos)[i], 0.0f, 1.0f) : 
                slopedCyclePos.consume();
            
            // Calculate slope
            float slope = m_rampToSlope.process(phase);
            
            // Calculate mipmap parameters (use ceil for no oversampling)
            const float rangeSize = static_cast<float>(cycleSamples);
            const float samplesPerFrame = std::abs(slope) * rangeSize;
            const float octave = sc_max(0.0f, sc_log2(samplesPerFrame));
            const int layer = static_cast<int>(sc_ceil(octave));

            // Calculate spacings for adjacent mipmap levels
            const int spacing1 = 1 << layer;
            const int spacing2 = spacing1 << 1;
            const float crossfade = sc_frac(octave);
            
            // Process wavetable oscillator
            output[i] = OscUtils::wavetableOsc(
                phase, bufData, 
                cycleSamples, numCyclesInt, cyclePosVal, 
                spacing1, spacing2, crossfade
            );
        }
    } else {

        for (int i = 0; i < nSamples; ++i) {
            
            // Wrap phase between 0 and 1
            float phase = sc_frac(phaseIn[i]);
            
            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePosVal = isCyclePosAudioRate ? 
                sc_clip(in(CyclePos)[i], 0.0f, 1.0f) : 
                slopedCyclePos.consume();
            
            // Calculate slope
            float slope = m_rampToSlope.process(phase);
            
            // Calculate mipmap parameters (use floor for oversampling)
            const float rangeSize = static_cast<float>(cycleSamples);
            const float samplesPerFrame = std::abs(slope) * rangeSize;
            const float octave = sc_max(0.0f, sc_log2(samplesPerFrame));
            const int layer = static_cast<int>(sc_floor(octave));
            
            // Calculate spacings for adjacent mipmap levels
            const int spacing1 = 1 << layer;
            const int spacing2 = spacing1 << 1;
            const float crossfade = sc_frac(octave);
            
            // Upsample parameter values
            m_cyclePosOversampling.upsample(cyclePosVal);

            // Initialize phase and slope for oversampling
            float osSlope = slope / static_cast<float>(m_osRatio);
            float osPhase = phase - slope;
            
            for (int k = 0; k < m_osRatio; k++) {
                
                // Clamp upsampled values
                m_osCyclePosBuffer[k] = sc_clip(m_osCyclePosBuffer[k], 0.0f, 1.0f);
                
                // Increment oversampled phase
                osPhase += osSlope;
                
                // Process wavetable oscillator with upsampled parameter values
                m_outputOSBuffer[k] = OscUtils::wavetableOsc(
                    sc_frac(osPhase), bufData, 
                    cycleSamples, numCyclesInt, m_osCyclePosBuffer[k], 
                    spacing1, spacing2, crossfade
                );
            }
            
            // Downsample output
            output[i] = m_outputOversampling.downsample();
        }
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    cyclePosPast = isCyclePosAudioRate ? 
        sc_clip(in(CyclePos)[nSamples - 1], 0.0f, 1.0f) : 
        slopedCyclePos.value;
}

// ===== DUAL WAVETABLE OSCILLATOR =====

DualOscOS::DualOscOS() : m_sampleRate(static_cast<float>(sampleRate()))
{
    // Initialize parameter cache
    cyclePosAPast = sc_clip(in0(CyclePosA), 0.0f, 1.0f);
    cyclePosBPast = sc_clip(in0(CyclePosB), 0.0f, 1.0f);
    pmIndexAPast = sc_clip(in0(PMIndexA), 0.0f, 10.0f);
    pmIndexBPast = sc_clip(in0(PMIndexB), 0.0f, 10.0f);
    pmFilterRatioAPast = sc_clip(in0(PMFilterRatioA), 1.0f, 10.0f);
    pmFilterRatioBPast = sc_clip(in0(PMFilterRatioB), 1.0f, 10.0f);
    
    // Check which inputs are audio-rate
    isCyclePosAAudioRate = isAudioRateIn(CyclePosA);
    isCyclePosBAAudioRate = isAudioRateIn(CyclePosB);
    isPMIndexAAudioRate = isAudioRateIn(PMIndexA);
    isPMIndexBAudioRate = isAudioRateIn(PMIndexB);
    isPMFilterRatioAAudioRate = isAudioRateIn(PMFilterRatioA);
    isPMFilterRatioBAudioRate = isAudioRateIn(PMFilterRatioB);
    
    // Initialize oversampling
    m_outputOversamplingA.reset(m_sampleRate);
    m_outputOversamplingB.reset(m_sampleRate);
    m_cyclePosAOversampling.reset(m_sampleRate);
    m_cyclePosBOversampling.reset(m_sampleRate);
    m_pmIndexAOversampling.reset(m_sampleRate);
    m_pmIndexBOversampling.reset(m_sampleRate);
    m_pmFilterRatioAOversampling.reset(m_sampleRate);
    m_pmFilterRatioBOversampling.reset(m_sampleRate);
    
    // Setup oversampling index
    m_oversampleIndex = sc_clip(static_cast<int>(in0(Oversample)), 0, 4);
    m_outputOversamplingA.setOversamplingIndex(m_oversampleIndex);
    m_outputOversamplingB.setOversamplingIndex(m_oversampleIndex);
    m_cyclePosAOversampling.setOversamplingIndex(m_oversampleIndex);
    m_cyclePosBOversampling.setOversamplingIndex(m_oversampleIndex);
    m_pmIndexAOversampling.setOversamplingIndex(m_oversampleIndex);
    m_pmIndexBOversampling.setOversamplingIndex(m_oversampleIndex);
    m_pmFilterRatioAOversampling.setOversamplingIndex(m_oversampleIndex);
    m_pmFilterRatioBOversampling.setOversamplingIndex(m_oversampleIndex);
    
    // Store ratio and buffer pointers
    m_osRatio = m_outputOversamplingA.getOversamplingRatio();
    m_outputOSBufferA = m_outputOversamplingA.getOSBuffer();
    m_outputOSBufferB = m_outputOversamplingB.getOSBuffer();
    m_osCyclePosABuffer = m_cyclePosAOversampling.getOSBuffer();
    m_osCyclePosBBuffer = m_cyclePosBOversampling.getOSBuffer();
    m_osPMIndexABuffer = m_pmIndexAOversampling.getOSBuffer();
    m_osPMIndexBBuffer = m_pmIndexBOversampling.getOSBuffer();
    m_osPMFilterRatioABuffer = m_pmFilterRatioAOversampling.getOSBuffer();
    m_osPMFilterRatioBBuffer = m_pmFilterRatioBOversampling.getOSBuffer();
    
    mCalcFunc = make_calc_function<DualOscOS, &DualOscOS::next>();
    next(1);
}

DualOscOS::~DualOscOS() = default;

void DualOscOS::next(int nSamples) {

    // Audio-rate inputs
    const float* phaseAIn = in(PhaseA);
    const float* phaseBIn = in(PhaseB);
    
    // Control-rate parameters with smooth interpolation
    auto slopedCyclePosA = makeSlope(sc_clip(in0(CyclePosA), 0.0f, 1.0f), cyclePosAPast);
    auto slopedCyclePosB = makeSlope(sc_clip(in0(CyclePosB), 0.0f, 1.0f), cyclePosBPast);
    auto slopedPMIndexA = makeSlope(sc_clip(in0(PMIndexA), 0.0f, 10.0f), pmIndexAPast);
    auto slopedPMIndexB = makeSlope(sc_clip(in0(PMIndexB), 0.0f, 10.0f), pmIndexBPast);
    auto slopedPMFilterRatioA = makeSlope(sc_clip(in0(PMFilterRatioA), 1.0f, 10.0f), pmFilterRatioAPast);
    auto slopedPMFilterRatioB = makeSlope(sc_clip(in0(PMFilterRatioB), 1.0f, 10.0f), pmFilterRatioBPast);
    
    // Control-rate parameters (settings)
    const float numCyclesA = sc_max(in0(NumCyclesA), 1.0f);
    const float numCyclesB = sc_max(in0(NumCyclesB), 1.0f);
    const float bufNumA = in0(BufNumA);
    const float bufNumB = in0(BufNumB);

    // Output pointers
    float* outputA = out(OutA);
    float* outputB = out(OutB);
    
    // Get buffer data
    const float* bufDataA;
    const float* bufDataB;
    int tableSizeA;
    int tableSizeB;
    
    if (!getBufferData(m_bufUnitA, bufNumA, nSamples, mWorld, mParent, bufDataA, tableSizeA, "DualOscOS OscA") ||
        !getBufferData(m_bufUnitB, bufNumB, nSamples, mWorld, mParent, bufDataB, tableSizeB, "DualOscOS OscB")) {
        ClearUnitOutputs(this, nSamples);
        return;
    }
    
    // Pre-calculate constants
    const int cycleSamplesA = tableSizeA / static_cast<int>(numCyclesA);
    const int cycleSamplesB = tableSizeB / static_cast<int>(numCyclesB);
    const int numCyclesIntA = static_cast<int>(numCyclesA);
    const int numCyclesIntB = static_cast<int>(numCyclesB);
    
    if (m_oversampleIndex == 0) {

        for (int i = 0; i < nSamples; ++i) {

            // Wrap phases between 0 and 1
            float phaseA = sc_frac(phaseAIn[i]);
            float phaseB = sc_frac(phaseBIn[i]);

            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePosAVal = isCyclePosAAudioRate ? 
                sc_clip(in(CyclePosA)[i], 0.0f, 1.0f) : 
                slopedCyclePosA.consume();

            float cyclePosBVal = isCyclePosBAAudioRate ? 
                sc_clip(in(CyclePosB)[i], 0.0f, 1.0f) : 
                slopedCyclePosB.consume();

            float pmIndexAVal = isPMIndexAAudioRate ? 
                sc_clip(in(PMIndexA)[i], 0.0f, 10.0f) : 
                slopedPMIndexA.consume();

            float pmIndexBVal = isPMIndexBAudioRate ? 
                sc_clip(in(PMIndexB)[i], 0.0f, 10.0f) : 
                slopedPMIndexB.consume();

            float pmFilterRatioAVal = isPMFilterRatioAAudioRate ? 
                sc_clip(in(PMFilterRatioA)[i], 1.0f, 10.0f) : 
                slopedPMFilterRatioA.consume();

            float pmFilterRatioBVal = isPMFilterRatioBAudioRate ? 
                sc_clip(in(PMFilterRatioB)[i], 1.0f, 10.0f) : 
                slopedPMFilterRatioB.consume();
            
            // Calculate slopes
            float slopeA = m_rampToSlopeA.process(phaseA);
            float slopeB = m_rampToSlopeB.process(phaseB);
            
            // Calculate mipmap parameters for oscillator A (use ceil for no oversampling)
            const float rangeSizeA = static_cast<float>(cycleSamplesA);
            const float samplesPerFrameA = std::abs(slopeA) * rangeSizeA;
            const float octaveA = sc_max(0.0f, sc_log2(samplesPerFrameA));
            const int layerA = static_cast<int>(sc_ceil(octaveA));

            // Calculate spacings for adjacent mipmap levels for oscillator A
            const int spacing1A = 1 << layerA;
            const int spacing2A = spacing1A << 1;
            const float crossfadeA = sc_frac(octaveA);
            
            // Calculate mipmap parameters for oscillator B (use ceil for no oversampling)
            const float rangeSizeB = static_cast<float>(cycleSamplesB);
            const float samplesPerFrameB = std::abs(slopeB) * rangeSizeB;
            const float octaveB = sc_max(0.0f, sc_log2(samplesPerFrameB));
            const int layerB = static_cast<int>(sc_ceil(octaveB));

            // Calculate spacings for adjacent mipmap levels for oscillator B
            const int spacing1B = 1 << layerB;
            const int spacing2B = spacing1B << 1;
            const float crossfadeB = sc_frac(octaveB);
            
            // Process dual wavetable oscillator
            auto result = m_dualOsc.process(
                phaseA, phaseB, cyclePosAVal, cyclePosBVal,
                slopeA, slopeB, pmIndexAVal, pmIndexBVal,
                pmFilterRatioAVal, pmFilterRatioBVal,
                spacing1A, spacing2A, crossfadeA,
                spacing1B, spacing2B, crossfadeB,
                bufDataA, cycleSamplesA, numCyclesIntA,
                bufDataB, cycleSamplesB, numCyclesIntB
            );
            
            outputA[i] = result.oscA;
            outputB[i] = result.oscB;
        }
    } else {

        for (int i = 0; i < nSamples; ++i) {

            // Wrap phases between 0 and 1
            float phaseA = sc_frac(phaseAIn[i]);
            float phaseB = sc_frac(phaseBIn[i]);

            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePosAVal = isCyclePosAAudioRate ? 
                sc_clip(in(CyclePosA)[i], 0.0f, 1.0f) : 
                slopedCyclePosA.consume();

            float cyclePosBVal = isCyclePosBAAudioRate ? 
                sc_clip(in(CyclePosB)[i], 0.0f, 1.0f) : 
                slopedCyclePosB.consume();

            float pmIndexAVal = isPMIndexAAudioRate ? 
                sc_clip(in(PMIndexA)[i], 0.0f, 10.0f) : 
                slopedPMIndexA.consume();

            float pmIndexBVal = isPMIndexBAudioRate ? 
                sc_clip(in(PMIndexB)[i], 0.0f, 10.0f) : 
                slopedPMIndexB.consume();

            float pmFilterRatioAVal = isPMFilterRatioAAudioRate ? 
                sc_clip(in(PMFilterRatioA)[i], 1.0f, 10.0f) : 
                slopedPMFilterRatioA.consume();

            float pmFilterRatioBVal = isPMFilterRatioBAudioRate ? 
                sc_clip(in(PMFilterRatioB)[i], 1.0f, 10.0f) : 
                slopedPMFilterRatioB.consume();

            // Calculate slopes
            float slopeA = m_rampToSlopeA.process(phaseA);
            float slopeB = m_rampToSlopeB.process(phaseB);
            
            // Calculate mipmap parameters for oscillator A (use floor for oversampling)
            const float rangeSizeA = static_cast<float>(cycleSamplesA);
            const float samplesPerFrameA = std::abs(slopeA) * rangeSizeA;
            const float octaveA = sc_max(0.0f, sc_log2(samplesPerFrameA));
            const int layerA = static_cast<int>(sc_floor(octaveA));

            // Calculate spacings for adjacent mipmap levels for oscillator A
            const int spacing1A = 1 << layerA;
            const int spacing2A = spacing1A << 1;
            const float crossfadeA = sc_frac(octaveA);
            
            // Calculate mipmap parameters for oscillator B (use floor for oversampling)
            const float rangeSizeB = static_cast<float>(cycleSamplesB);
            const float samplesPerFrameB = std::abs(slopeB) * rangeSizeB;
            const float octaveB = sc_max(0.0f, sc_log2(samplesPerFrameB));
            const int layerB = static_cast<int>(sc_floor(octaveB));

            // Calculate spacings for adjacent mipmap levels for oscillator B
            const int spacing1B = 1 << layerB;
            const int spacing2B = spacing1B << 1;
            const float crossfadeB = sc_frac(octaveB);
            
            // Upsample parameter values
            m_cyclePosAOversampling.upsample(cyclePosAVal);
            m_cyclePosBOversampling.upsample(cyclePosBVal);
            m_pmIndexAOversampling.upsample(pmIndexAVal);
            m_pmIndexBOversampling.upsample(pmIndexBVal);
            m_pmFilterRatioAOversampling.upsample(pmFilterRatioAVal);
            m_pmFilterRatioBOversampling.upsample(pmFilterRatioBVal);

            // Initialize phases and slopes for oversampling
            float osSlopeA = slopeA / static_cast<float>(m_osRatio);
            float osSlopeB = slopeB / static_cast<float>(m_osRatio);
            float osPhaseA = phaseA - slopeA;
            float osPhaseB = phaseB - slopeB;
            
            for (int k = 0; k < m_osRatio; k++) {
                
                // Clamp upsampled values
                m_osCyclePosABuffer[k] = sc_clip(m_osCyclePosABuffer[k], 0.0f, 1.0f);
                m_osCyclePosBBuffer[k] = sc_clip(m_osCyclePosBBuffer[k], 0.0f, 1.0f);
                m_osPMIndexABuffer[k] = sc_clip(m_osPMIndexABuffer[k], 0.0f, 10.0f);
                m_osPMIndexBBuffer[k] = sc_clip(m_osPMIndexBBuffer[k], 0.0f, 10.0f);
                m_osPMFilterRatioABuffer[k] = sc_clip(m_osPMFilterRatioABuffer[k], 1.0f, 10.0f);
                m_osPMFilterRatioBBuffer[k] = sc_clip(m_osPMFilterRatioBBuffer[k], 1.0f, 10.0f);
                
                // Increment oversampled phases
                osPhaseA += osSlopeA;
                osPhaseB += osSlopeB;
                
                // Process dual wavetable oscillator with upsampled parameter values
                auto result = m_dualOsc.process(
                    sc_frac(osPhaseA), sc_frac(osPhaseB),
                    m_osCyclePosABuffer[k], m_osCyclePosBBuffer[k],
                    osSlopeA, osSlopeB,
                    m_osPMIndexABuffer[k], m_osPMIndexBBuffer[k],
                    m_osPMFilterRatioABuffer[k], m_osPMFilterRatioBBuffer[k],
                    spacing1A, spacing2A, crossfadeA,
                    spacing1B, spacing2B, crossfadeB,
                    bufDataA, cycleSamplesA, numCyclesIntA,
                    bufDataB, cycleSamplesB, numCyclesIntB
                );
                
                m_outputOSBufferA[k] = result.oscA;
                m_outputOSBufferB[k] = result.oscB;
            }
            
            // Downsample outputs
            outputA[i] = m_outputOversamplingA.downsample();
            outputB[i] = m_outputOversamplingB.downsample();
        }
    }

    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    cyclePosAPast = isCyclePosAAudioRate ? 
        sc_clip(in(CyclePosA)[nSamples - 1], 0.0f, 1.0f) : 
        slopedCyclePosA.value;

    cyclePosBPast = isCyclePosBAAudioRate ? 
        sc_clip(in(CyclePosB)[nSamples - 1], 0.0f, 1.0f) : 
        slopedCyclePosB.value;

    pmIndexAPast = isPMIndexAAudioRate ? 
        sc_clip(in(PMIndexA)[nSamples - 1], 0.0f, 10.0f) : 
        slopedPMIndexA.value;

    pmIndexBPast = isPMIndexBAudioRate ? 
        sc_clip(in(PMIndexB)[nSamples - 1], 0.0f, 10.0f) : 
        slopedPMIndexB.value;

    pmFilterRatioAPast = isPMFilterRatioAAudioRate ? 
        sc_clip(in(PMFilterRatioA)[nSamples - 1], 1.0f, 10.0f) : 
        slopedPMFilterRatioA.value;

    pmFilterRatioBPast = isPMFilterRatioBAudioRate ? 
        sc_clip(in(PMFilterRatioB)[nSamples - 1], 1.0f, 10.0f) : 
        slopedPMFilterRatioB.value;
}

// ===== PULSAR OSCILLATOR =====

PulsarOS::PulsarOS() : 
    m_sampleRate(static_cast<float>(sampleRate())),
    m_sampleDur(static_cast<float>(sampleDur())),
    m_numChannels(sc_clip(static_cast<int>(in0(NumChannels)), 1, MAX_CHANNELS))
{
    // Initialize parameter cache
    oscCyclePosPast = sc_clip(in0(OscCyclePos), 0.0f, 1.0f);
    envCyclePosPast = sc_clip(in0(EnvCyclePos), 0.0f, 1.0f);
    modCyclePosPast = sc_clip(in0(ModCyclePos), 0.0f, 1.0f);
    
    // Check which inputs are audio-rate
    isTriggerAudioRate = isAudioRateIn(Trigger);
    isTriggerFreqAudioRate = isAudioRateIn(TriggerFreq);
    isSubSampleOffsetAudioRate = isAudioRateIn(SubSampleOffset);
    isGrainFreqAudioRate = isAudioRateIn(GrainFreq);
    isModFreqAudioRate = isAudioRateIn(ModFreq);
    isModIndexAudioRate = isAudioRateIn(ModIndex);
    isPanAudioRate = isAudioRateIn(Pan);
    isAmpAudioRate = isAudioRateIn(Amp);
    isOscCyclePosAudioRate = isAudioRateIn(OscCyclePos);
    isEnvCyclePosAudioRate = isAudioRateIn(EnvCyclePos);
    isModCyclePosAudioRate = isAudioRateIn(ModCyclePos);
    
    // Initialize oversampling
    m_outputOversamplingL.reset(m_sampleRate);
    m_outputOversamplingR.reset(m_sampleRate);
    m_oscCyclePosOversampling.reset(m_sampleRate);
    m_envCyclePosOversampling.reset(m_sampleRate);
    m_modCyclePosOversampling.reset(m_sampleRate);
    
    // Setup oversampling index
    m_oversampleIndex = sc_clip(static_cast<int>(in0(Oversample)), 0, 4);
    m_outputOversamplingL.setOversamplingIndex(m_oversampleIndex);
    m_outputOversamplingR.setOversamplingIndex(m_oversampleIndex);
    m_oscCyclePosOversampling.setOversamplingIndex(m_oversampleIndex);
    m_envCyclePosOversampling.setOversamplingIndex(m_oversampleIndex);
    m_modCyclePosOversampling.setOversamplingIndex(m_oversampleIndex);
    
    // Store ratio and buffer pointers
    m_osRatio = m_outputOversamplingL.getOversamplingRatio();
    m_outputOSBufferL = m_outputOversamplingL.getOSBuffer();
    m_outputOSBufferR = m_outputOversamplingR.getOSBuffer();
    m_osOscCyclePosBuffer = m_oscCyclePosOversampling.getOSBuffer();
    m_osEnvCyclePosBuffer = m_envCyclePosOversampling.getOSBuffer();
    m_osModCyclePosBuffer = m_modCyclePosOversampling.getOSBuffer();
    
    mCalcFunc = make_calc_function<PulsarOS, &PulsarOS::next>();
    next(1);
    
    // Reset state after priming
    m_allocator.reset();
    m_trigger.reset();
}

PulsarOS::~PulsarOS() = default;

void PulsarOS::next(int nSamples) {
    
    // Control-rate parameters with smooth interpolation
    auto slopedOscCyclePos = makeSlope(sc_clip(in0(OscCyclePos), 0.0f, 1.0f), oscCyclePosPast);
    auto slopedEnvCyclePos = makeSlope(sc_clip(in0(EnvCyclePos), 0.0f, 1.0f), envCyclePosPast);
    auto slopedModCyclePos = makeSlope(sc_clip(in0(ModCyclePos), 0.0f, 1.0f), modCyclePosPast);
    
    // Control-rate parameters (settings, no interpolation)
    float oscBufNum = in0(OscBuffer);
    float envBufNum = in0(EnvBuffer);
    float modBufNum = in0(ModBuffer);
    const float oscNumCycles = sc_max(in0(OscNumCycles), 1.0f);
    const float envNumCycles = sc_max(in0(EnvNumCycles), 1.0f);
    const float modNumCycles = sc_max(in0(ModNumCycles), 1.0f);
    
    // Output pointers
    float* outputL = out(OutL);
    float* outputR = (m_numChannels > 1) ? out(OutR) : nullptr;
    
    // Get buffer data
    const float* oscBufData;
    const float* envBufData;
    const float* modBufData;
    int oscTableSize, envTableSize, modTableSize;
    
    if (!getBufferData(m_oscBufUnit, oscBufNum, nSamples, mWorld, mParent, 
                       oscBufData, oscTableSize, "PulsarOS osc") ||
        !getBufferData(m_envBufUnit, envBufNum, nSamples, mWorld, mParent, 
                       envBufData, envTableSize, "PulsarOS env") ||
        !getBufferData(m_modBufUnit, modBufNum, nSamples, mWorld, mParent, 
                       modBufData, modTableSize, "PulsarOS mod")) {
        ClearUnitOutputs(this, nSamples);
        return;
    }
    
    // Pre-calculate constants
    const int oscCycleSamples = oscTableSize / static_cast<int>(oscNumCycles);
    const int oscNumCyclesInt = static_cast<int>(oscNumCycles);
    const int envCycleSamples = envTableSize / static_cast<int>(envNumCycles);
    const int envNumCyclesInt = static_cast<int>(envNumCycles);
    const int modCycleSamples = modTableSize / static_cast<int>(modNumCycles);
    const int modNumCyclesInt = static_cast<int>(modNumCycles);
    
    if (m_oversampleIndex == 0) {

        for (int i = 0; i < nSamples; ++i) {
            
            // Trigger input (audio-rate or control-rate)
            bool trigger = isTriggerAudioRate ? 
                m_trigger.process(in(Trigger)[i]) : 
                m_trigger.process(in0(Trigger));
            
            // Get current parameter values (no interpolation - latched per trigger)
            float triggerFreq = isTriggerFreqAudioRate ? 
                sc_clip(in(TriggerFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) : 
                sc_clip(in0(TriggerFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
            
            float offset = isSubSampleOffsetAudioRate ? 
                in(SubSampleOffset)[i] : 
                in0(SubSampleOffset);

            float grainFreq = isGrainFreqAudioRate ? 
                sc_clip(in(GrainFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(GrainFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f); 
            
            float modFreq = isModFreqAudioRate ? 
                sc_clip(in(ModFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) : 
                sc_clip(in0(ModFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);

            float modIndex = isModIndexAudioRate ? 
                sc_clip(in(ModIndex)[i], 0.0f, 10.0f) : 
                sc_clip(in0(ModIndex), 0.0f, 10.0f);

            float pan = isPanAudioRate ? 
                sc_clip(in(Pan)[i], -1.0f, 1.0f) : 
                sc_clip(in0(Pan), -1.0f, 1.0f);

            float amp = isAmpAudioRate ? 
                sc_clip(in(Amp)[i], 0.0f, 1.0f) : 
                sc_clip(in0(Amp), 0.0f, 1.0f);
            
            // Get current parameter values (audio-rate or interpolated control-rate)            
            float oscCyclePosVal = isOscCyclePosAudioRate ?
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) :
                slopedOscCyclePos.consume();
            
            float envCyclePosVal = isEnvCyclePosAudioRate ?
                sc_clip(in(EnvCyclePos)[i], 0.0f, 1.0f) :
                slopedEnvCyclePos.consume();

            float modCyclePosVal = isModCyclePosAudioRate ?
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) :
                slopedModCyclePos.consume();
            
            // 1. Process voice allocation
            m_allocator.process(
                trigger,
                triggerFreq,
                offset,
                m_sampleRate
            );
            
            // 2. Process all grains
            float sumL = 0.0f;
            float sumR = 0.0f;
            for (int g = 0; g < NUM_VOICES; ++g) {
                
                // Trigger new grain if needed and store graindata
                if (m_allocator.triggers[g]) {
                    m_grainData[g].grainFreq = grainFreq;
                    m_grainData[g].modFreq = modFreq;
                    m_grainData[g].modIndex = modIndex;
                    m_grainData[g].pan = pan;
                    m_grainData[g].amp = amp;
                    m_grainData[g].sampleCount = offset;
                    m_pmFilters[g].reset();
                }
                
                // Process grain if voice is active
                if (m_allocator.isActive[g]) {
                    
                    // Calculate slopes
                    const float oscSlope = m_grainData[g].grainFreq * m_sampleDur;
                    const float envSlope = static_cast<float>(m_allocator.localSlopes[g]);
                    const float modSlope = m_grainData[g].modFreq * m_sampleDur;

                    // Accumulate osc and mod phases
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(oscSlope)));
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(modSlope)));

                    // Calculate mipmap parameters for mod (use ceil for no oversampling)
                    const float modRangeSize = static_cast<float>(modCycleSamples);
                    const float modSamplesPerFrame = std::abs(modSlope) * modRangeSize;
                    const float modOctave = sc_max(0.0f, sc_log2(modSamplesPerFrame));
                    const int modLayer = static_cast<int>(sc_ceil(modOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for mod
                    const int modSpacing1 = 1 << modLayer;
                    const int modSpacing2 = modSpacing1 << 1;
                    const float modCrossfade = sc_frac(modOctave);
                    
                    // Process mod wavetable oscillator
                    float modOsc = OscUtils::wavetableOsc(
                        modPhase, modBufData, 
                        modCycleSamples, modNumCyclesInt, modCyclePosVal,
                        modSpacing1, modSpacing2, modCrossfade
                    );

                    // Calculate mipmap parameters for osc (use ceil for no oversampling)
                    const float oscRangeSize = static_cast<float>(oscCycleSamples);
                    const float oscSamplesPerFrame = std::abs(oscSlope) * oscRangeSize;
                    const float oscOctave = sc_max(0.0f, sc_log2(oscSamplesPerFrame));
                    const int oscLayer = static_cast<int>(sc_ceil(oscOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for osc
                    const int oscSpacing1 = 1 << oscLayer;
                    const int oscSpacing2 = oscSpacing1 << 1;
                    const float oscCrossfade = sc_frac(oscOctave);

                    // Calculate mod scale ratio for PM
                    float modScaleRatio = 0.0f;
                    if (sc_abs(modSlope) > Utils::SAFE_DENOM_EPSILON) {
                        modScaleRatio = sc_abs(oscSlope / modSlope);
                    }

                    // Apply Phase Modulation
                    float modFiltered = m_pmFilters[g].processLowpass(modOsc, modSlope);
                    float modScaled = modFiltered / Utils::TWO_PI * modScaleRatio;
                    float modulatedOscPhase = sc_frac(oscPhase + (modScaled * m_grainData[g].modIndex));
                    
                    // Process osc wavetable oscillator
                    float grainOsc = OscUtils::wavetableOsc(
                        modulatedOscPhase, oscBufData, 
                        oscCycleSamples, oscNumCyclesInt, oscCyclePosVal,
                        oscSpacing1, oscSpacing2, oscCrossfade
                    );
                    
                    // Calculate mipmap parameters for env (use ceil for no oversampling)
                    const float envRangeSize = static_cast<float>(envCycleSamples);
                    const float envSamplesPerFrame = std::abs(envSlope) * envRangeSize;
                    const float envOctave = sc_max(0.0f, sc_log2(envSamplesPerFrame));
                    const int envLayer = static_cast<int>(sc_ceil(envOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for env
                    const int envSpacing1 = 1 << envLayer;
                    const int envSpacing2 = envSpacing1 << 1;
                    const float envCrossfade = sc_frac(envOctave);
                    
                    // Process env wavetable oscillator
                    float grainWindow = OscUtils::wavetableOsc(
                        m_allocator.phases[g], envBufData, 
                        envCycleSamples, envNumCyclesInt, envCyclePosVal,
                        envSpacing1, envSpacing2, envCrossfade
                    );
                    
                    // Accumulate grain output with panning
                    float grainOut = grainOsc * grainWindow * m_grainData[g].amp;
                    if (m_numChannels > 1) {
                        auto panGains = Utils::EqualPowerPan{}.process(m_grainData[g].pan);
                        sumL += grainOut * panGains.left;
                        sumR += grainOut * panGains.right;
                    } else {
                        sumL += grainOut;
                    }

                    // Increment sample count
                    m_grainData[g].sampleCount++;
                }
            }
            
            // 3. DC block output
            outputL[i] = m_dcBlockerL.processHighpass(sumL, 3.0f, m_sampleRate);
            if (m_numChannels > 1) {
                outputR[i] = m_dcBlockerR.processHighpass(sumR, 3.0f, m_sampleRate);
            }
        }
    } else {

        for (int i = 0; i < nSamples; ++i) {
            
            // Trigger input (audio-rate or control-rate)
            bool trigger = isTriggerAudioRate ? 
                m_trigger.process(in(Trigger)[i]) : 
                m_trigger.process(in0(Trigger));
            
            // Get current parameter values (no interpolation - latched per trigger)
            float triggerFreq = isTriggerFreqAudioRate ? 
                sc_clip(in(TriggerFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) : 
                sc_clip(in0(TriggerFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
            
            float offset = isSubSampleOffsetAudioRate ? 
                in(SubSampleOffset)[i] : 
                in0(SubSampleOffset);

            float grainFreq = isGrainFreqAudioRate ? 
                sc_clip(in(GrainFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(GrainFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f); 
            
            float modFreq = isModFreqAudioRate ? 
                sc_clip(in(ModFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) : 
                sc_clip(in0(ModFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);

            float modIndex = isModIndexAudioRate ? 
                sc_clip(in(ModIndex)[i], 0.0f, 10.0f) : 
                sc_clip(in0(ModIndex), 0.0f, 10.0f);

            float pan = isPanAudioRate ? 
                sc_clip(in(Pan)[i], -1.0f, 1.0f) : 
                sc_clip(in0(Pan), -1.0f, 1.0f);

            float amp = isAmpAudioRate ? 
                sc_clip(in(Amp)[i], 0.0f, 1.0f) : 
                sc_clip(in0(Amp), 0.0f, 1.0f);
            
            // Get current parameter values (audio-rate or interpolated control-rate)            
            float oscCyclePosVal = isOscCyclePosAudioRate ?
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) :
                slopedOscCyclePos.consume();
            
            float envCyclePosVal = isEnvCyclePosAudioRate ?
                sc_clip(in(EnvCyclePos)[i], 0.0f, 1.0f) :
                slopedEnvCyclePos.consume();

            float modCyclePosVal = isModCyclePosAudioRate ?
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) :
                slopedModCyclePos.consume();
        
            // 1. Process voice allocation
            m_allocator.process(
                trigger,
                triggerFreq,
                offset,
                m_sampleRate
            );
            
            // 2. Upsample parameter values
            m_oscCyclePosOversampling.upsample(oscCyclePosVal);
            m_envCyclePosOversampling.upsample(envCyclePosVal);
            m_modCyclePosOversampling.upsample(modCyclePosVal);
            
            // 3. Clear OS buffers
            for (int k = 0; k < m_osRatio; k++) {
                m_outputOSBufferL[k] = 0.0f;
            }
            if (m_numChannels > 1) {
                for (int k = 0; k < m_osRatio; k++) {
                    m_outputOSBufferR[k] = 0.0f;
                }
            }
            
            // 4. Process all grains
            for (int g = 0; g < NUM_VOICES; ++g) {
                
                // Trigger new grain if needed and store graindata
                if (m_allocator.triggers[g]) {
                    m_grainData[g].grainFreq = grainFreq;
                    m_grainData[g].modFreq = modFreq;
                    m_grainData[g].modIndex = modIndex;
                    m_grainData[g].pan = pan;
                    m_grainData[g].amp = amp;
                    m_grainData[g].sampleCount = offset;
                    m_pmFilters[g].reset();
                }
                
                // Process grain if voice is active
                if (m_allocator.isActive[g]) {
                    
                    // Calculate slopes
                    const float oscSlope = m_grainData[g].grainFreq * m_sampleDur;
                    const float envSlope = static_cast<float>(m_allocator.localSlopes[g]);
                    const float modSlope = m_grainData[g].modFreq * m_sampleDur;

                    // Accumulate osc and mod phases
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(oscSlope)));
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(modSlope)));

                    // Calculate mipmap parameters for mod (use floor for oversampling)
                    const float modRangeSize = static_cast<float>(modCycleSamples);
                    const float modSamplesPerFrame = std::abs(modSlope) * modRangeSize;
                    const float modOctave = sc_max(0.0f, sc_log2(modSamplesPerFrame));
                    const int modLayer = static_cast<int>(sc_floor(modOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for mod
                    const int modSpacing1 = 1 << modLayer;
                    const int modSpacing2 = modSpacing1 << 1;
                    const float modCrossfade = sc_frac(modOctave);

                    // Calculate mipmap parameters for osc (use floor for oversampling)
                    const float oscRangeSize = static_cast<float>(oscCycleSamples);
                    const float oscSamplesPerFrame = std::abs(oscSlope) * oscRangeSize;
                    const float oscOctave = sc_max(0.0f, sc_log2(oscSamplesPerFrame));
                    const int oscLayer = static_cast<int>(sc_floor(oscOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for osc
                    const int oscSpacing1 = 1 << oscLayer;
                    const int oscSpacing2 = oscSpacing1 << 1;
                    const float oscCrossfade = sc_frac(oscOctave);
                    
                    // Initialize mod phase and slope for oversampling
                    float osModSlope = modSlope / static_cast<float>(m_osRatio);
                    float osModPhase = modPhase - modSlope;
                    
                    // Initialize osc phase and slope for oversampling
                    float osOscSlope = oscSlope / static_cast<float>(m_osRatio);
                    float osOscPhase = oscPhase - oscSlope;
                    
                    // Initialize env phase and slope for oversampling
                    float osEnvSlope = envSlope / static_cast<float>(m_osRatio);
                    float osEnvPhase = m_allocator.phases[g] - envSlope;
                    
                    // Calculate mipmap parameters for env (use floor for oversampling)
                    const float envRangeSize = static_cast<float>(envCycleSamples);
                    const float envSamplesPerFrame = std::abs(envSlope) * envRangeSize;
                    const float envOctave = sc_max(0.0f, sc_log2(envSamplesPerFrame));
                    const int envLayer = static_cast<int>(sc_floor(envOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for env
                    const int envSpacing1 = 1 << envLayer;
                    const int envSpacing2 = envSpacing1 << 1;
                    const float envCrossfade = sc_frac(envOctave);

                    // Calculate pan gains
                    auto panGains = Utils::EqualPowerPan{}.process(m_grainData[g].pan);

                    // Calculate mod scale ratio for PM
                    float modScaleRatio = 0.0f;
                    if (sc_abs(modSlope) > Utils::SAFE_DENOM_EPSILON) {
                        modScaleRatio = sc_abs(oscSlope / modSlope);
                    }
                    
                    for (int k = 0; k < m_osRatio; k++) {
                        
                        // Clamp upsampled values
                        m_osOscCyclePosBuffer[k] = sc_clip(m_osOscCyclePosBuffer[k], 0.0f, 1.0f);
                        m_osEnvCyclePosBuffer[k] = sc_clip(m_osEnvCyclePosBuffer[k], 0.0f, 1.0f);
                        m_osModCyclePosBuffer[k] = sc_clip(m_osModCyclePosBuffer[k], 0.0f, 1.0f);
                        
                        // Increment oversampled phases
                        osModPhase += osModSlope;
                        osOscPhase += osOscSlope;
                        osEnvPhase += osEnvSlope;
                        
                        // Process mod wavetable oscillator
                        float modOsc = OscUtils::wavetableOsc(
                            sc_frac(osModPhase), modBufData, 
                            modCycleSamples, modNumCyclesInt, m_osModCyclePosBuffer[k],
                            modSpacing1, modSpacing2, modCrossfade
                        );
                        
                        // Apply Phase Modulation
                        float modFiltered = m_pmFilters[g].processLowpass(modOsc, osModSlope);
                        float modScaled = modFiltered / Utils::TWO_PI * modScaleRatio;
                        float modulatedOscPhase = sc_frac(osOscPhase + (modScaled * m_grainData[g].modIndex));
                        
                        // Process osc wavetable oscillator
                        float grainOsc = OscUtils::wavetableOsc(
                            modulatedOscPhase, oscBufData, 
                            oscCycleSamples, oscNumCyclesInt, m_osOscCyclePosBuffer[k],
                            oscSpacing1, oscSpacing2, oscCrossfade
                        );
                        
                        // Process env wavetable oscillator
                        float grainWindow = OscUtils::wavetableOsc(
                            osEnvPhase, envBufData, 
                            envCycleSamples, envNumCyclesInt, m_osEnvCyclePosBuffer[k],
                            envSpacing1, envSpacing2, envCrossfade
                        );
                        
                        // Accumulate grain output with panning
                        float grainOut = grainOsc * grainWindow * m_grainData[g].amp;
                        if (m_numChannels > 1) {
                            m_outputOSBufferL[k] += grainOut * panGains.left;
                            m_outputOSBufferR[k] += grainOut * panGains.right;
                        } else {
                            m_outputOSBufferL[k] += grainOut;
                        }
                    }

                    // Increment sample count
                    m_grainData[g].sampleCount++;
                }
            }
            
            // 5. Downsample and DC block output
            outputL[i] = m_dcBlockerL.processHighpass(m_outputOversamplingL.downsample(), 3.0f, m_sampleRate);
            if (m_numChannels > 1) {
                outputR[i] = m_dcBlockerR.processHighpass(m_outputOversamplingR.downsample(), 3.0f, m_sampleRate);
            }
        }
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    oscCyclePosPast = isOscCyclePosAudioRate ? 
        sc_clip(in(OscCyclePos)[nSamples - 1], 0.0f, 1.0f) : slopedOscCyclePos.value;
        
    envCyclePosPast = isEnvCyclePosAudioRate ? 
        sc_clip(in(EnvCyclePos)[nSamples - 1], 0.0f, 1.0f) : slopedEnvCyclePos.value;

    modCyclePosPast = isModCyclePosAudioRate ? 
        sc_clip(in(ModCyclePos)[nSamples - 1], 0.0f, 1.0f) : slopedModCyclePos.value;
}

// ===== DUAL PULSAR OSCILLATOR =====
 
DualPulsarOS::DualPulsarOS() :
    m_sampleRate(static_cast<float>(sampleRate())),
    m_sampleDur(static_cast<float>(sampleDur()))
{
    // Initialize parameter cache (sloped params)
    oscCyclePosPast = sc_clip(in0(OscCyclePos), 0.0f, 1.0f);
    modCyclePosPast = sc_clip(in0(ModCyclePos), 0.0f, 1.0f);
    skewPast = sc_clip(in0(Skew), 0.0f, 1.0f);
    indexPast = sc_clip(in0(Index), 0.0f, 10.0f);
 
    // Check which inputs are audio-rate
    isTriggerAudioRate = isAudioRateIn(Trigger);
    isTriggerFreqAudioRate = isAudioRateIn(TriggerFreq);
    isSubSampleOffsetAudioRate = isAudioRateIn(SubSampleOffset);
    isOscFreqAudioRate = isAudioRateIn(OscFreq);
    isModFreqAudioRate = isAudioRateIn(ModFreq);
    isPmIndexOscAudioRate = isAudioRateIn(PmIndexOsc);
    isPmIndexModAudioRate = isAudioRateIn(PmIndexMod);
    isPmFilterRatioOscAudioRate = isAudioRateIn(PmFilterRatioOsc);
    isPmFilterRatioModAudioRate = isAudioRateIn(PmFilterRatioMod);
    isWarpOscAudioRate = isAudioRateIn(WarpOsc);
    isWarpModAudioRate = isAudioRateIn(WarpMod);
    isOscCyclePosAudioRate = isAudioRateIn(OscCyclePos);
    isModCyclePosAudioRate = isAudioRateIn(ModCyclePos);
    isSkewAudioRate = isAudioRateIn(Skew);
    isIndexAudioRate = isAudioRateIn(Index);
 
    // Initialize oversampling
    m_outputOversampling.reset(m_sampleRate);
    m_oscCyclePosOversampling.reset(m_sampleRate);
    m_modCyclePosOversampling.reset(m_sampleRate);
    m_skewOversampling.reset(m_sampleRate);
    m_indexOversampling.reset(m_sampleRate);
 
    // Setup oversampling index
    m_oversampleIndex = sc_clip(static_cast<int>(in0(Oversample)), 0, 4);
    m_outputOversampling.setOversamplingIndex(m_oversampleIndex);
    m_oscCyclePosOversampling.setOversamplingIndex(m_oversampleIndex);
    m_modCyclePosOversampling.setOversamplingIndex(m_oversampleIndex);
    m_skewOversampling.setOversamplingIndex(m_oversampleIndex);
    m_indexOversampling.setOversamplingIndex(m_oversampleIndex);
 
    // Store ratio and buffer pointers
    m_osRatio = m_outputOversampling.getOversamplingRatio();
    m_outputOSBuffer = m_outputOversampling.getOSBuffer();
    m_osOscCyclePosBuffer = m_oscCyclePosOversampling.getOSBuffer();
    m_osModCyclePosBuffer = m_modCyclePosOversampling.getOSBuffer();
    m_osSkewBuffer = m_skewOversampling.getOSBuffer();
    m_osIndexBuffer = m_indexOversampling.getOSBuffer();
 
    mCalcFunc = make_calc_function<DualPulsarOS, &DualPulsarOS::next>();
    next(1);
 
    // Reset state after priming
    m_allocator.reset();
    m_trigger.reset();
}
 
DualPulsarOS::~DualPulsarOS() = default;
 
void DualPulsarOS::next(int nSamples) {
 
    // Control-rate parameters with smooth interpolation (sloped params)
    auto slopedOscCyclePos = makeSlope(sc_clip(in0(OscCyclePos), 0.0f, 1.0f), oscCyclePosPast);
    auto slopedModCyclePos = makeSlope(sc_clip(in0(ModCyclePos), 0.0f, 1.0f), modCyclePosPast);
    auto slopedSkew = makeSlope(sc_clip(in0(Skew), 0.0f, 1.0f), skewPast);
    auto slopedIndex = makeSlope(sc_clip(in0(Index), 0.0f, 10.0f), indexPast);
 
    // Control-rate parameters (settings, no interpolation)
    float oscBufNum = in0(OscBuffer);
    float modBufNum = in0(ModBuffer);
    const float oscNumCycles = sc_max(in0(OscNumCycles), 1.0f);
    const float modNumCycles = sc_max(in0(ModNumCycles), 1.0f);
 
    // Output pointer
    float* output = out(Out);
 
    // Get buffer data
    const float* oscBufData;
    const float* modBufData;
    int oscTableSize, modTableSize;
 
    if (!getBufferData(m_oscBufUnit, oscBufNum, nSamples, mWorld, mParent,
                       oscBufData, oscTableSize, "DualPulsarOS osc") ||
        !getBufferData(m_modBufUnit, modBufNum, nSamples, mWorld, mParent,
                       modBufData, modTableSize, "DualPulsarOS mod")) {
        ClearUnitOutputs(this, nSamples);
        return;
    }
 
    // Pre-calculate constants
    const int oscCycleSamples = oscTableSize / static_cast<int>(oscNumCycles);
    const int oscNumCyclesInt = static_cast<int>(oscNumCycles);
    const int modCycleSamples = modTableSize / static_cast<int>(modNumCycles);
    const int modNumCyclesInt = static_cast<int>(modNumCycles);
 
    if (m_oversampleIndex == 0) {
 
        for (int i = 0; i < nSamples; ++i) {
 
            // Trigger input (audio-rate or control-rate)
            bool trigger = isTriggerAudioRate ?
                m_trigger.process(in(Trigger)[i]) :
                m_trigger.process(in0(Trigger));
 
            // Get current parameter values (no interpolation - latched per trigger)
            float triggerFreq = isTriggerFreqAudioRate ?
                sc_clip(in(TriggerFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(TriggerFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float offset = isSubSampleOffsetAudioRate ?
                in(SubSampleOffset)[i] :
                in0(SubSampleOffset);
 
            float oscFreq = isOscFreqAudioRate ?
                sc_clip(in(OscFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(OscFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float modFreq = isModFreqAudioRate ?
                sc_clip(in(ModFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(ModFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float pmIndexOsc = isPmIndexOscAudioRate ?
                sc_clip(in(PmIndexOsc)[i], 0.0f, 10.0f) :
                sc_clip(in0(PmIndexOsc), 0.0f, 10.0f);
 
            float pmIndexMod = isPmIndexModAudioRate ?
                sc_clip(in(PmIndexMod)[i], 0.0f, 10.0f) :
                sc_clip(in0(PmIndexMod), 0.0f, 10.0f);
 
            float pmFilterRatioOsc = isPmFilterRatioOscAudioRate ?
                sc_clip(in(PmFilterRatioOsc)[i], 1.0f, 10.0f) :
                sc_clip(in0(PmFilterRatioOsc), 1.0f, 10.0f);
 
            float pmFilterRatioMod = isPmFilterRatioModAudioRate ?
                sc_clip(in(PmFilterRatioMod)[i], 1.0f, 10.0f) :
                sc_clip(in0(PmFilterRatioMod), 1.0f, 10.0f);
 
            float warpOsc = isWarpOscAudioRate ?
                sc_clip(in(WarpOsc)[i], 0.0f, 1.0f) :
                sc_clip(in0(WarpOsc), 0.0f, 1.0f);
 
            float warpMod = isWarpModAudioRate ?
                sc_clip(in(WarpMod)[i], 0.0f, 1.0f) :
                sc_clip(in0(WarpMod), 0.0f, 1.0f);
 
            // Get current parameter values (audio-rate or interpolated control-rate)
            float oscCyclePosVal = isOscCyclePosAudioRate ?
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) :
                slopedOscCyclePos.consume();
 
            float modCyclePosVal = isModCyclePosAudioRate ?
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) :
                slopedModCyclePos.consume();
 
            float skewVal = isSkewAudioRate ?
                sc_clip(in(Skew)[i], 0.0f, 1.0f) :
                slopedSkew.consume();
 
            float indexVal = isIndexAudioRate ?
                sc_clip(in(Index)[i], 0.0f, 10.0f) :
                slopedIndex.consume();
 
            // 1. Process voice allocation
            m_allocator.process(
                trigger,
                triggerFreq,
                offset,
                m_sampleRate
            );
 
            // 2. Process all grains
            float sum = 0.0f;
            for (int g = 0; g < NUM_VOICES; ++g) {
 
                // Trigger new grain if needed and store graindata
                if (m_allocator.triggers[g]) {
                    m_grainData[g].oscFreq = oscFreq;
                    m_grainData[g].modFreq = modFreq;
                    m_grainData[g].pmIndexOsc = pmIndexOsc;
                    m_grainData[g].pmIndexMod = pmIndexMod;
                    m_grainData[g].pmFilterRatioOsc = pmFilterRatioOsc;
                    m_grainData[g].pmFilterRatioMod = pmFilterRatioMod;
                    m_grainData[g].warpOsc = warpOsc;
                    m_grainData[g].warpMod = warpMod;
                    m_grainData[g].sampleCount = offset;
                    m_dualOscs[g].reset();
                }
 
                // Process grain if voice is active
                if (m_allocator.isActive[g]) {
 
                    // Calculate slopes
                    const float oscSlope = m_grainData[g].oscFreq * m_sampleDur;
                    const float modSlope = m_grainData[g].modFreq * m_sampleDur;
                    const float envSlope = static_cast<float>(m_allocator.localSlopes[g]);
 
                    // Derive osc and mod phases from sample count
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(oscSlope)));
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(modSlope)));
 
                    // Phase increment distortion ratios
                    float phsIncRatioOsc = 0.0f;
                    float phsIncRatioMod = 0.0f;
                    if (sc_abs(envSlope) > Utils::SAFE_DENOM_EPSILON) {
                        phsIncRatioOsc = sc_abs(oscSlope / envSlope);
                        phsIncRatioMod = sc_abs(modSlope / envSlope);
                    }

                    // Apply Phase increment distortion
                    float phsIncDistOsc = Easing::Interp::jCurve(m_allocator.phases[g], m_grainData[g].warpOsc, Easing::Cores::cubic) - m_allocator.phases[g];
                    float oscPhaseDistorted = sc_frac(oscPhase + (phsIncDistOsc * phsIncRatioOsc));
 
                    float phsIncDistMod = Easing::Interp::jCurve(m_allocator.phases[g], m_grainData[g].warpMod, Easing::Cores::cubic) - m_allocator.phases[g];
                    float modPhaseDistorted = sc_frac(modPhase + (phsIncDistMod * phsIncRatioMod));
 
                    // Calculate mipmap parameters for osc (use ceil for no oversampling)
                    const float oscRangeSize = static_cast<float>(oscCycleSamples);
                    const float oscSamplesPerFrame = std::abs(oscSlope) * oscRangeSize;
                    const float oscOctave = sc_max(0.0f, sc_log2(oscSamplesPerFrame));
                    const int oscLayer = static_cast<int>(sc_ceil(oscOctave));
 
                    // Calculate spacings for adjacent mipmap levels for osc
                    const int oscSpacing1 = 1 << oscLayer;
                    const int oscSpacing2 = oscSpacing1 << 1;
                    const float oscCrossfade = sc_frac(oscOctave);
 
                    // Calculate mipmap parameters for mod (use ceil for no oversampling)
                    const float modRangeSize = static_cast<float>(modCycleSamples);
                    const float modSamplesPerFrame = std::abs(modSlope) * modRangeSize;
                    const float modOctave = sc_max(0.0f, sc_log2(modSamplesPerFrame));
                    const int modLayer = static_cast<int>(sc_ceil(modOctave));
 
                    // Calculate spacings for adjacent mipmap levels for mod
                    const int modSpacing1 = 1 << modLayer;
                    const int modSpacing2 = modSpacing1 << 1;
                    const float modCrossfade = sc_frac(modOctave);
 
                    // Process cross-modulated dual oscillator
                    auto result = m_dualOscs[g].process(
                        oscPhaseDistorted, modPhaseDistorted,
                        oscCyclePosVal, modCyclePosVal,
                        oscSlope, modSlope,
                        m_grainData[g].pmIndexOsc, m_grainData[g].pmIndexMod,
                        m_grainData[g].pmFilterRatioOsc, m_grainData[g].pmFilterRatioMod,
                        oscSpacing1, oscSpacing2, oscCrossfade,
                        modSpacing1, modSpacing2, modCrossfade,
                        oscBufData, oscCycleSamples, oscNumCyclesInt,
                        modBufData, modCycleSamples, modNumCyclesInt
                    );
 
                    // Apply gaussian window
                    float grainWindow = WindowFunctions::gaussianWindow(
                        m_allocator.phases[g], skewVal, indexVal);
 
                    sum += result.oscA * grainWindow;
 
                    // Increment sample count
                    m_grainData[g].sampleCount++;
                }
            }
 
            // 3. DC block output
            output[i] = m_dcBlocker.processHighpass(sum, 3.0f, m_sampleRate);
        }
    } else {
 
        for (int i = 0; i < nSamples; ++i) {
 
            // Trigger input (audio-rate or control-rate)
            bool trigger = isTriggerAudioRate ?
                m_trigger.process(in(Trigger)[i]) :
                m_trigger.process(in0(Trigger));
 
            // Get current parameter values (no interpolation - latched per trigger)
            float triggerFreq = isTriggerFreqAudioRate ?
                sc_clip(in(TriggerFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(TriggerFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float offset = isSubSampleOffsetAudioRate ?
                in(SubSampleOffset)[i] :
                in0(SubSampleOffset);
 
            float oscFreq = isOscFreqAudioRate ?
                sc_clip(in(OscFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(OscFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float modFreq = isModFreqAudioRate ?
                sc_clip(in(ModFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(ModFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float pmIndexOsc = isPmIndexOscAudioRate ?
                sc_clip(in(PmIndexOsc)[i], 0.0f, 10.0f) :
                sc_clip(in0(PmIndexOsc), 0.0f, 10.0f);
 
            float pmIndexMod = isPmIndexModAudioRate ?
                sc_clip(in(PmIndexMod)[i], 0.0f, 10.0f) :
                sc_clip(in0(PmIndexMod), 0.0f, 10.0f);
 
            float pmFilterRatioOsc = isPmFilterRatioOscAudioRate ?
                sc_clip(in(PmFilterRatioOsc)[i], 1.0f, 10.0f) :
                sc_clip(in0(PmFilterRatioOsc), 1.0f, 10.0f);
 
            float pmFilterRatioMod = isPmFilterRatioModAudioRate ?
                sc_clip(in(PmFilterRatioMod)[i], 1.0f, 10.0f) :
                sc_clip(in0(PmFilterRatioMod), 1.0f, 10.0f);
 
            float warpOsc = isWarpOscAudioRate ?
                sc_clip(in(WarpOsc)[i], 0.0f, 1.0f) :
                sc_clip(in0(WarpOsc), 0.0f, 1.0f);
 
            float warpMod = isWarpModAudioRate ?
                sc_clip(in(WarpMod)[i], 0.0f, 1.0f) :
                sc_clip(in0(WarpMod), 0.0f, 1.0f);
 
            // Get current parameter values (audio-rate or interpolated control-rate)
            float oscCyclePosVal = isOscCyclePosAudioRate ?
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) :
                slopedOscCyclePos.consume();
 
            float modCyclePosVal = isModCyclePosAudioRate ?
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) :
                slopedModCyclePos.consume();
 
            float skewVal = isSkewAudioRate ?
                sc_clip(in(Skew)[i], 0.0f, 1.0f) :
                slopedSkew.consume();
 
            float indexVal = isIndexAudioRate ?
                sc_clip(in(Index)[i], 0.0f, 10.0f) :
                slopedIndex.consume();
 
            // 1. Process voice allocation
            m_allocator.process(
                trigger,
                triggerFreq,
                offset,
                m_sampleRate
            );
 
            // 2. Upsample parameter values
            m_oscCyclePosOversampling.upsample(oscCyclePosVal);
            m_modCyclePosOversampling.upsample(modCyclePosVal);
            m_skewOversampling.upsample(skewVal);
            m_indexOversampling.upsample(indexVal);
 
            // 3. Clear OS buffer
            for (int k = 0; k < m_osRatio; k++) {
                m_outputOSBuffer[k] = 0.0f;
            }
 
            // 4. Process all grains
            for (int g = 0; g < NUM_VOICES; ++g) {
 
                // Trigger new grain if needed and store graindata
                if (m_allocator.triggers[g]) {
                    m_grainData[g].oscFreq = oscFreq;
                    m_grainData[g].modFreq = modFreq;
                    m_grainData[g].pmIndexOsc = pmIndexOsc;
                    m_grainData[g].pmIndexMod = pmIndexMod;
                    m_grainData[g].pmFilterRatioOsc = pmFilterRatioOsc;
                    m_grainData[g].pmFilterRatioMod = pmFilterRatioMod;
                    m_grainData[g].warpOsc = warpOsc;
                    m_grainData[g].warpMod = warpMod;
                    m_grainData[g].sampleCount = offset;
                    m_dualOscs[g].reset();
                }
 
                // Process grain if voice is active
                if (m_allocator.isActive[g]) {
 
                    // Calculate slopes
                    const float oscSlope = m_grainData[g].oscFreq * m_sampleDur;
                    const float modSlope = m_grainData[g].modFreq * m_sampleDur;
                    const float envSlope = static_cast<float>(m_allocator.localSlopes[g]);
 
                    // Derive osc and mod phases from sample count
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(oscSlope)));
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(modSlope)));
 
                    // Calculate mipmap parameters for osc (use floor for oversampling)
                    const float oscRangeSize = static_cast<float>(oscCycleSamples);
                    const float oscSamplesPerFrame = std::abs(oscSlope) * oscRangeSize;
                    const float oscOctave = sc_max(0.0f, sc_log2(oscSamplesPerFrame));
                    const int oscLayer = static_cast<int>(sc_floor(oscOctave));
 
                    // Calculate spacings for adjacent mipmap levels for osc
                    const int oscSpacing1 = 1 << oscLayer;
                    const int oscSpacing2 = oscSpacing1 << 1;
                    const float oscCrossfade = sc_frac(oscOctave);
 
                    // Calculate mipmap parameters for mod (use floor for oversampling)
                    const float modRangeSize = static_cast<float>(modCycleSamples);
                    const float modSamplesPerFrame = std::abs(modSlope) * modRangeSize;
                    const float modOctave = sc_max(0.0f, sc_log2(modSamplesPerFrame));
                    const int modLayer = static_cast<int>(sc_floor(modOctave));
 
                    // Calculate spacings for adjacent mipmap levels for mod
                    const int modSpacing1 = 1 << modLayer;
                    const int modSpacing2 = modSpacing1 << 1;
                    const float modCrossfade = sc_frac(modOctave);
 
                    // Initialize osc phase and slope for oversampling
                    float osOscSlope = oscSlope / static_cast<float>(m_osRatio);
                    float osOscPhase = oscPhase - oscSlope;
 
                    // Initialize mod phase and slope for oversampling
                    float osModSlope = modSlope / static_cast<float>(m_osRatio);
                    float osModPhase = modPhase - modSlope;
 
                    // Initialize env phase and slope for oversampling
                    float osEnvSlope = envSlope / static_cast<float>(m_osRatio);
                    float osEnvPhase = m_allocator.phases[g] - envSlope;
 
                    // Phase increment distortion ratios
                    float phsIncRatioOsc = 0.0f;
                    float phsIncRatioMod = 0.0f;
                    if (sc_abs(envSlope) > Utils::SAFE_DENOM_EPSILON) {
                        phsIncRatioOsc = sc_abs(oscSlope / envSlope);
                        phsIncRatioMod = sc_abs(modSlope / envSlope);
                    }
 
                    for (int k = 0; k < m_osRatio; k++) {
 
                        // Clamp upsampled values
                        m_osOscCyclePosBuffer[k] = sc_clip(m_osOscCyclePosBuffer[k], 0.0f, 1.0f);
                        m_osModCyclePosBuffer[k] = sc_clip(m_osModCyclePosBuffer[k], 0.0f, 1.0f);
                        m_osSkewBuffer[k] = sc_clip(m_osSkewBuffer[k], 0.0f, 1.0f);
                        m_osIndexBuffer[k] = sc_clip(m_osIndexBuffer[k], 0.0f, 10.0f);
 
                        // Increment oversampled phases
                        osOscPhase += osOscSlope;
                        osModPhase += osModSlope;
                        osEnvPhase += osEnvSlope;
 
                        // Apply phase increment distortion
                        float phsIncDistOsc = Easing::Interp::jCurve(osEnvPhase, m_grainData[g].warpOsc, Easing::Cores::cubic) - osEnvPhase;
                        float osOscPhaseDistorted = sc_frac(osOscPhase + (phsIncDistOsc * phsIncRatioOsc));
 
                        float phsIncDistMod = Easing::Interp::jCurve(osEnvPhase, m_grainData[g].warpMod, Easing::Cores::cubic) - osEnvPhase;
                        float osModPhaseDistorted = sc_frac(osModPhase + (phsIncDistMod * phsIncRatioMod));
 
                        // Process cross-modulated dual oscillator at oversampled rate
                        auto result = m_dualOscs[g].process(
                            osOscPhaseDistorted, osModPhaseDistorted,
                            m_osOscCyclePosBuffer[k], m_osModCyclePosBuffer[k],
                            osOscSlope, osModSlope,
                            m_grainData[g].pmIndexOsc, m_grainData[g].pmIndexMod,
                            m_grainData[g].pmFilterRatioOsc, m_grainData[g].pmFilterRatioMod,
                            oscSpacing1, oscSpacing2, oscCrossfade,
                            modSpacing1, modSpacing2, modCrossfade,
                            oscBufData, oscCycleSamples, oscNumCyclesInt,
                            modBufData, modCycleSamples, modNumCyclesInt
                        );
 
                        // Apply gaussian window with upsampled skew and index
                        float grainWindow = WindowFunctions::gaussianWindow(
                            osEnvPhase, m_osSkewBuffer[k], m_osIndexBuffer[k]);
 
                        m_outputOSBuffer[k] += result.oscA * grainWindow;
                    }
 
                    // Increment sample count
                    m_grainData[g].sampleCount++;
                }
            }
 
            // 5. Downsample and DC block output
            output[i] = m_dcBlocker.processHighpass(m_outputOversampling.downsample(), 3.0f, m_sampleRate);
        }
    }
 
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    oscCyclePosPast = isOscCyclePosAudioRate ?
        sc_clip(in(OscCyclePos)[nSamples - 1], 0.0f, 1.0f) : slopedOscCyclePos.value;
 
    modCyclePosPast = isModCyclePosAudioRate ?
        sc_clip(in(ModCyclePos)[nSamples - 1], 0.0f, 1.0f) : slopedModCyclePos.value;
 
    skewPast = isSkewAudioRate ?
        sc_clip(in(Skew)[nSamples - 1], 0.0f, 1.0f) : slopedSkew.value;
 
    indexPast = isIndexAudioRate ?
        sc_clip(in(Index)[nSamples - 1], 0.0f, 10.0f) : slopedIndex.value;
}

PluginLoad(OscUGens)
{
    ft = inTable;
    registerUnit<DualOscOS>(ft, "DualOscOS", false);
    registerUnit<SingleOscOS>(ft, "SingleOscOS", false);
    registerUnit<PulsarOS>(ft, "PulsarOS", false);
    registerUnit<DualPulsarOS>(ft, "DualPulsarOS", false);
}