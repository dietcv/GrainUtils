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
            output[i] = OscUtils::wavetableInterpolate(
                phase, bufData, tableSize, 
                cycleSamples, numCyclesInt, cyclePosVal, 
                spacing1, spacing2, crossfade,
                m_sincTable);
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
            float osPhase = phase;
            
            for (int k = 0; k < m_osRatio; k++) {
                
                // Clamp upsampled values
                m_osCyclePosBuffer[k] = sc_clip(m_osCyclePosBuffer[k], 0.0f, 1.0f);
                
                // Increment oversampled phase
                osPhase += osSlope;
                
                // Process wavetable oscillator with upsampled parameter values
                m_outputOSBuffer[k] = OscUtils::wavetableInterpolate(
                    sc_frac(osPhase), bufData, tableSize, 
                    cycleSamples, numCyclesInt, m_osCyclePosBuffer[k], 
                    spacing1, spacing2, crossfade,
                    m_sincTable);
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
                bufDataA, tableSizeA, cycleSamplesA, numCyclesIntA,
                bufDataB, tableSizeB, cycleSamplesB, numCyclesIntB,
                m_sincTable
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
            float osPhaseA = phaseA;
            float osPhaseB = phaseB;
            
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
                    bufDataA, tableSizeA, cycleSamplesA, numCyclesIntA,
                    bufDataB, tableSizeB, cycleSamplesB, numCyclesIntB,
                    m_sincTable
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

PluginLoad(OscUGens)
{
    ft = inTable;
    registerUnit<DualOscOS>(ft, "DualOscOS", false);
    registerUnit<SingleOscOS>(ft, "SingleOscOS", false);
}