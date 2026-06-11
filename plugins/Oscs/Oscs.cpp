#include "Oscs.hpp"
#include "SC_PlugIn.hpp"

extern InterfaceTable* ft;

// ===== SINGLE WAVETABLE OSCILLATOR =====

SingleOscOS::SingleOscOS() : 
    m_sampleRate(static_cast<float>(sampleRate())),
    m_oversampleIndex(sc_clip(static_cast<int>(in0(Oversample)), 0, 4)),
    m_osRatio(1 << m_oversampleIndex)
{
    // Initialize parameter cache
    cyclePosPast = sc_clip(in0(CyclePos), 0.0f, 1.0f);
    
    // Check which inputs are audio-rate
    isCyclePosAudioRate = isAudioRateIn(CyclePos);
    
    // Initialize oversampling
    if (m_oversampleIndex > 0) {
        auto unit = this;

        // Allocate oversampling buffers
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_outputOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_cyclePosOSBuffer);

        // Setup oversampling filters
        m_outputOversampling.init(m_osRatio, m_sampleRate, m_outputOSBuffer);
        m_cyclePosOversampling.init(m_osRatio, m_sampleRate, m_cyclePosOSBuffer);
    }
    
    // Set calc function & compute initial sample
    set_calc_function<SingleOscOS, &SingleOscOS::next>();
}

SingleOscOS::~SingleOscOS() {
    RTFree(mWorld, m_outputOSBuffer);
    RTFree(mWorld, m_cyclePosOSBuffer);
}

void SingleOscOS::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    auto slopedCyclePos = makeSlope(sc_clip(in0(CyclePos), 0.0f, 1.0f), cyclePosPast);

    // Control-rate parameters (settings, no interpolation)
    float bufNum = in0(BufNum);
    int numCycles = sc_max(static_cast<int>(in0(NumCycles)), 1);

    // Output pointer
    float* output = out(Out);
    
    // Get wavetable data
    auto oscTable = m_oscBufUnit.GetTable(this, bufNum, "SingleOscOS");
    if (!oscTable.valid) { 
        ClearUnitOutputs(this, nSamples); 
        return; 
    }
    
    // Calculate samples per cycle
    int cycleSamples = oscTable.size / numCycles;
    
    if (m_oversampleIndex == 0) {

        for (int i = 0; i < nSamples; ++i) {
            
            // Wrap phase between 0 and 1
            float phase = sc_frac(phaseIn[i]);
            
            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePosVal = isCyclePosAudioRate ? 
                sc_clip(in(CyclePos)[i], 0.0f, 1.0f) : 
                slopedCyclePos.consume();
            
            // Calculate slope
            float slope = static_cast<float>(m_rampToSlope.process(static_cast<double>(phase)));
            
            // Calculate mipmap parameters (use ceil for no oversampling)
            float rangeSize = static_cast<float>(cycleSamples);
            float samplesPerFrame = std::abs(slope) * rangeSize;
            float octave = sc_max(0.0f, sc_log2(samplesPerFrame));
            int layer = static_cast<int>(sc_ceil(octave));

            // Calculate spacings for adjacent mipmap levels
            int spacing1 = 1 << layer;
            int spacing2 = spacing1 << 1;
            float crossfade = sc_frac(octave);
            
            // Process wavetable oscillator
            output[i] = OscUtils::wavetableOsc(
                phase, oscTable.data, 
                cycleSamples, numCycles, cyclePosVal, 
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
            float slope = static_cast<float>(m_rampToSlope.process(static_cast<double>(phase)));
            
            // Calculate mipmap parameters (use floor for oversampling)
            float rangeSize = static_cast<float>(cycleSamples);
            float samplesPerFrame = std::abs(slope) * rangeSize;
            float octave = sc_max(0.0f, sc_log2(samplesPerFrame));
            int layer = static_cast<int>(sc_floor(octave));
            
            // Calculate spacings for adjacent mipmap levels
            int spacing1 = 1 << layer;
            int spacing2 = spacing1 << 1;
            float crossfade = sc_frac(octave);
            
            // Upsample parameter values
            m_cyclePosOversampling.upsample(cyclePosVal);

            // Initialize phase and slope for oversampling
            float osSlope = slope / static_cast<float>(m_osRatio);
            float osPhase = phase - slope;
            
            for (int k = 0; k < m_osRatio; k++) {
                
                // Clamp upsampled values
                m_cyclePosOSBuffer[k] = sc_clip(m_cyclePosOSBuffer[k], 0.0f, 1.0f);
                
                // Increment oversampled phase
                osPhase += osSlope;
                
                // Process wavetable oscillator with upsampled parameter values
                m_outputOSBuffer[k] = OscUtils::wavetableOsc(
                    sc_frac(osPhase), oscTable.data, 
                    cycleSamples, numCycles, m_cyclePosOSBuffer[k], 
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

DualOscOS::DualOscOS() : 
    m_sampleRate(static_cast<float>(sampleRate())),
    m_oversampleIndex(sc_clip(static_cast<int>(in0(Oversample)), 0, 4)),
    m_osRatio(1 << m_oversampleIndex)
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
    if (m_oversampleIndex > 0) {
        auto unit = this;

        // Allocate oversampling buffers
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_outputOSBufferA);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_outputOSBufferB);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_cyclePosAOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_cyclePosBOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_pmIndexAOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_pmIndexBOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_pmFilterRatioAOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_pmFilterRatioBOSBuffer);

        // Setup oversampling filters
        m_outputOversamplingA.init(m_osRatio, m_sampleRate, m_outputOSBufferA);
        m_outputOversamplingB.init(m_osRatio, m_sampleRate, m_outputOSBufferB);
        m_cyclePosAOversampling.init(m_osRatio, m_sampleRate, m_cyclePosAOSBuffer);
        m_cyclePosBOversampling.init(m_osRatio, m_sampleRate, m_cyclePosBOSBuffer);
        m_pmIndexAOversampling.init(m_osRatio, m_sampleRate, m_pmIndexAOSBuffer);
        m_pmIndexBOversampling.init(m_osRatio, m_sampleRate, m_pmIndexBOSBuffer);
        m_pmFilterRatioAOversampling.init(m_osRatio, m_sampleRate, m_pmFilterRatioAOSBuffer);
        m_pmFilterRatioBOversampling.init(m_osRatio, m_sampleRate, m_pmFilterRatioBOSBuffer);
    }
    
    // Set calc function & compute initial sample
    set_calc_function<DualOscOS, &DualOscOS::next>();
}

DualOscOS::~DualOscOS() {
    RTFree(mWorld, m_outputOSBufferA);
    RTFree(mWorld, m_outputOSBufferB);
    RTFree(mWorld, m_cyclePosAOSBuffer);
    RTFree(mWorld, m_cyclePosBOSBuffer);
    RTFree(mWorld, m_pmIndexAOSBuffer);
    RTFree(mWorld, m_pmIndexBOSBuffer);
    RTFree(mWorld, m_pmFilterRatioAOSBuffer);
    RTFree(mWorld, m_pmFilterRatioBOSBuffer);
}

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
    
    // Control-rate parameters (settings, no interpolation)
    float bufNumA = in0(BufNumA);
    float bufNumB = in0(BufNumB);
    int numCyclesA = sc_max(static_cast<int>(in0(NumCyclesA)), 1);
    int numCyclesB = sc_max(static_cast<int>(in0(NumCyclesB)), 1);

    // Output pointers
    float* outputA = out(OutA);
    float* outputB = out(OutB);

    // Get wavetable data
    auto oscTableA = m_oscBufUnitA.GetTable(this, bufNumA, "DualOscOS OscA");
    auto oscTableB = m_oscBufUnitB.GetTable(this, bufNumB, "DualOscOS OscB");
    if (!oscTableA.valid || !oscTableB.valid) {
        ClearUnitOutputs(this, nSamples);
        return;
    }
    
    // Calculate samples per cycle
    int cycleSamplesA = oscTableA.size / numCyclesA;
    int cycleSamplesB = oscTableB.size / numCyclesB;
    
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
            float slopeA = static_cast<float>(m_rampToSlopeA.process(static_cast<double>(phaseA)));
            float slopeB = static_cast<float>(m_rampToSlopeB.process(static_cast<double>(phaseB)));
            
            // Calculate mipmap parameters for oscillator A (use ceil for no oversampling)
            float rangeSizeA = static_cast<float>(cycleSamplesA);
            float samplesPerFrameA = std::abs(slopeA) * rangeSizeA;
            float octaveA = sc_max(0.0f, sc_log2(samplesPerFrameA));
            int layerA = static_cast<int>(sc_ceil(octaveA));

            // Calculate spacings for adjacent mipmap levels for oscillator A
            int spacing1A = 1 << layerA;
            int spacing2A = spacing1A << 1;
            float crossfadeA = sc_frac(octaveA);
            
            // Calculate mipmap parameters for oscillator B (use ceil for no oversampling)
            float rangeSizeB = static_cast<float>(cycleSamplesB);
            float samplesPerFrameB = std::abs(slopeB) * rangeSizeB;
            float octaveB = sc_max(0.0f, sc_log2(samplesPerFrameB));
            int layerB = static_cast<int>(sc_ceil(octaveB));

            // Calculate spacings for adjacent mipmap levels for oscillator B
            int spacing1B = 1 << layerB;
            int spacing2B = spacing1B << 1;
            float crossfadeB = sc_frac(octaveB);
            
            // Process dual wavetable oscillator
            auto result = m_dualOsc.process(
                phaseA, phaseB, cyclePosAVal, cyclePosBVal,
                slopeA, slopeB, pmIndexAVal, pmIndexBVal,
                pmFilterRatioAVal, pmFilterRatioBVal,
                spacing1A, spacing2A, crossfadeA,
                spacing1B, spacing2B, crossfadeB,
                oscTableA.data, cycleSamplesA, numCyclesA,
                oscTableB.data, cycleSamplesB, numCyclesB
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
            float slopeA = static_cast<float>(m_rampToSlopeA.process(static_cast<double>(phaseA)));
            float slopeB = static_cast<float>(m_rampToSlopeB.process(static_cast<double>(phaseB)));
            
            // Calculate mipmap parameters for oscillator A (use floor for oversampling)
            float rangeSizeA = static_cast<float>(cycleSamplesA);
            float samplesPerFrameA = std::abs(slopeA) * rangeSizeA;
            float octaveA = sc_max(0.0f, sc_log2(samplesPerFrameA));
            int layerA = static_cast<int>(sc_floor(octaveA));

            // Calculate spacings for adjacent mipmap levels for oscillator A
            int spacing1A = 1 << layerA;
            int spacing2A = spacing1A << 1;
            float crossfadeA = sc_frac(octaveA);
            
            // Calculate mipmap parameters for oscillator B (use floor for oversampling)
            float rangeSizeB = static_cast<float>(cycleSamplesB);
            float samplesPerFrameB = std::abs(slopeB) * rangeSizeB;
            float octaveB = sc_max(0.0f, sc_log2(samplesPerFrameB));
            int layerB = static_cast<int>(sc_floor(octaveB));

            // Calculate spacings for adjacent mipmap levels for oscillator B
            int spacing1B = 1 << layerB;
            int spacing2B = spacing1B << 1;
            float crossfadeB = sc_frac(octaveB);
            
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
                m_cyclePosAOSBuffer[k] = sc_clip(m_cyclePosAOSBuffer[k], 0.0f, 1.0f);
                m_cyclePosBOSBuffer[k] = sc_clip(m_cyclePosBOSBuffer[k], 0.0f, 1.0f);
                m_pmIndexAOSBuffer[k] = sc_clip(m_pmIndexAOSBuffer[k], 0.0f, 10.0f);
                m_pmIndexBOSBuffer[k] = sc_clip(m_pmIndexBOSBuffer[k], 0.0f, 10.0f);
                m_pmFilterRatioAOSBuffer[k] = sc_clip(m_pmFilterRatioAOSBuffer[k], 1.0f, 10.0f);
                m_pmFilterRatioBOSBuffer[k] = sc_clip(m_pmFilterRatioBOSBuffer[k], 1.0f, 10.0f);
                
                // Increment oversampled phases
                osPhaseA += osSlopeA;
                osPhaseB += osSlopeB;
                
                // Process dual wavetable oscillator with upsampled parameter values
                auto result = m_dualOsc.process(
                    sc_frac(osPhaseA), sc_frac(osPhaseB),
                    m_cyclePosAOSBuffer[k], m_cyclePosBOSBuffer[k],
                    osSlopeA, osSlopeB,
                    m_pmIndexAOSBuffer[k], m_pmIndexBOSBuffer[k],
                    m_pmFilterRatioAOSBuffer[k], m_pmFilterRatioBOSBuffer[k],
                    spacing1A, spacing2A, crossfadeA,
                    spacing1B, spacing2B, crossfadeB,
                    oscTableA.data, cycleSamplesA, numCyclesA,
                    oscTableB.data, cycleSamplesB, numCyclesB
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
    m_oversampleIndex(sc_clip(static_cast<int>(in0(Oversample)), 0, 4)),
    m_osRatio(1 << m_oversampleIndex)
{
    // Initialize parameter cache
    oscCyclePosPast = sc_clip(in0(OscCyclePos), 0.0f, 1.0f);
    envCyclePosPast = sc_clip(in0(EnvCyclePos), 0.0f, 1.0f);
    modCyclePosPast = sc_clip(in0(ModCyclePos), 0.0f, 1.0f);
    
    // Check which inputs are audio-rate
    isTriggerAudioRate = isAudioRateIn(Trigger);
    isTriggerFreqAudioRate = isAudioRateIn(TriggerFreq);
    isSubSampleOffsetAudioRate = isAudioRateIn(SubSampleOffset);
    isOscFreqAudioRate = isAudioRateIn(OscFreq);
    isModFreqAudioRate = isAudioRateIn(ModFreq);
    isModIndexAudioRate = isAudioRateIn(ModIndex);
    isOscCyclePosAudioRate = isAudioRateIn(OscCyclePos);
    isEnvCyclePosAudioRate = isAudioRateIn(EnvCyclePos);
    isModCyclePosAudioRate = isAudioRateIn(ModCyclePos);
    
    // Initialize oversampling
    if (m_oversampleIndex > 0) {
        auto unit = this;

        // Allocate oversampling buffers
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_outputOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_oscCyclePosOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_envCyclePosOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_modCyclePosOSBuffer);

        // Setup oversampling filters
        m_outputOversampling.init(m_osRatio, m_sampleRate, m_outputOSBuffer);
        m_oscCyclePosOversampling.init(m_osRatio, m_sampleRate, m_oscCyclePosOSBuffer);
        m_envCyclePosOversampling.init(m_osRatio, m_sampleRate, m_envCyclePosOSBuffer);
        m_modCyclePosOversampling.init(m_osRatio, m_sampleRate, m_modCyclePosOSBuffer);
    }
    
    // Set calc function & compute initial sample
    set_calc_function<PulsarOS, &PulsarOS::next>();
    
    // Reset state after priming
    m_allocator.reset();
    m_trigger.reset();
}
 
PulsarOS::~PulsarOS() {
    RTFree(mWorld, m_outputOSBuffer);
    RTFree(mWorld, m_oscCyclePosOSBuffer);
    RTFree(mWorld, m_envCyclePosOSBuffer);
    RTFree(mWorld, m_modCyclePosOSBuffer);
}
 
void PulsarOS::next(int nSamples) {
    
    // Control-rate parameters with smooth interpolation
    auto slopedOscCyclePos = makeSlope(sc_clip(in0(OscCyclePos), 0.0f, 1.0f), oscCyclePosPast);
    auto slopedEnvCyclePos = makeSlope(sc_clip(in0(EnvCyclePos), 0.0f, 1.0f), envCyclePosPast);
    auto slopedModCyclePos = makeSlope(sc_clip(in0(ModCyclePos), 0.0f, 1.0f), modCyclePosPast);
    
    // Control-rate parameters (settings, no interpolation)
    float oscBufNum = in0(OscBuffer);
    float envBufNum = in0(EnvBuffer);
    float modBufNum = in0(ModBuffer);
    int oscNumCycles = sc_max(static_cast<int>(in0(OscNumCycles)), 1);
    int envNumCycles = sc_max(static_cast<int>(in0(EnvNumCycles)), 1);
    int modNumCycles = sc_max(static_cast<int>(in0(ModNumCycles)), 1);
    
    // Output pointer
    float* output = out(Out);

    // Get wavetable data
    auto oscTable = m_oscBufUnit.GetTable(this, oscBufNum, "PulsarOS osc");
    auto envTable = m_envBufUnit.GetTable(this, envBufNum, "PulsarOS env");
    auto modTable = m_modBufUnit.GetTable(this, modBufNum, "PulsarOS mod");
    if (!oscTable.valid || !envTable.valid || !modTable.valid) {
        ClearUnitOutputs(this, nSamples);
        return;
    }

    // Calculate samples per cycle
    int oscCycleSamples = oscTable.size / oscNumCycles;
    int envCycleSamples = envTable.size / envNumCycles;
    int modCycleSamples = modTable.size / modNumCycles;
    
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
 
            float modIndex = isModIndexAudioRate ? 
                sc_clip(in(ModIndex)[i], 0.0f, 10.0f) : 
                sc_clip(in0(ModIndex), 0.0f, 10.0f);
            
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
            float sum = 0.0f;
            for (int g = 0; g < NUM_VOICES; ++g) {
                
                // Trigger new grain if needed and store graindata
                if (m_allocator.triggers[g]) {
                    m_grainData[g].oscFreq = oscFreq;
                    m_grainData[g].modFreq = modFreq;
                    m_grainData[g].modIndex = modIndex;
                    m_grainData[g].sampleCount = offset;
                    m_pmFilters[g].reset();
                }
                
                // Process grain if voice is active
                if (m_allocator.isActive[g]) {
                    
                    // Calculate slopes
                    float oscSlope = m_grainData[g].oscFreq * m_sampleDur;
                    float envSlope = static_cast<float>(m_allocator.localSlopes[g]);
                    float modSlope = m_grainData[g].modFreq * m_sampleDur;
 
                    // Accumulate osc and mod phases
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(oscSlope)));
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(modSlope)));
 
                    // Calculate mipmap parameters for mod (use ceil for no oversampling)
                    float modRangeSize = static_cast<float>(modCycleSamples);
                    float modSamplesPerFrame = std::abs(modSlope) * modRangeSize;
                    float modOctave = sc_max(0.0f, sc_log2(modSamplesPerFrame));
                    int modLayer = static_cast<int>(sc_ceil(modOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for mod
                    int modSpacing1 = 1 << modLayer;
                    int modSpacing2 = modSpacing1 << 1;
                    float modCrossfade = sc_frac(modOctave);
                    
                    // Process mod wavetable oscillator
                    float modOsc = OscUtils::wavetableOsc(
                        modPhase, modTable.data, 
                        modCycleSamples, modNumCycles, modCyclePosVal,
                        modSpacing1, modSpacing2, modCrossfade
                    );
 
                    // Calculate mipmap parameters for osc (use ceil for no oversampling)
                    float oscRangeSize = static_cast<float>(oscCycleSamples);
                    float oscSamplesPerFrame = std::abs(oscSlope) * oscRangeSize;
                    float oscOctave = sc_max(0.0f, sc_log2(oscSamplesPerFrame));
                    int oscLayer = static_cast<int>(sc_ceil(oscOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for osc
                    int oscSpacing1 = 1 << oscLayer;
                    int oscSpacing2 = oscSpacing1 << 1;
                    float oscCrossfade = sc_frac(oscOctave);
 
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
                        modulatedOscPhase, oscTable.data, 
                        oscCycleSamples, oscNumCycles, oscCyclePosVal,
                        oscSpacing1, oscSpacing2, oscCrossfade
                    );
                    
                    // Calculate mipmap parameters for env (use ceil for no oversampling)
                    float envRangeSize = static_cast<float>(envCycleSamples);
                    float envSamplesPerFrame = std::abs(envSlope) * envRangeSize;
                    float envOctave = sc_max(0.0f, sc_log2(envSamplesPerFrame));
                    int envLayer = static_cast<int>(sc_ceil(envOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for env
                    int envSpacing1 = 1 << envLayer;
                    int envSpacing2 = envSpacing1 << 1;
                    float envCrossfade = sc_frac(envOctave);
                    
                    // Process env wavetable oscillator
                    float grainWindow = OscUtils::wavetableOsc(
                        m_allocator.phases[g], envTable.data, 
                        envCycleSamples, envNumCycles, envCyclePosVal,
                        envSpacing1, envSpacing2, envCrossfade
                    );
                    
                    // Accumulate grain output
                    sum += grainOsc * grainWindow;
 
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
 
            float modIndex = isModIndexAudioRate ? 
                sc_clip(in(ModIndex)[i], 0.0f, 10.0f) : 
                sc_clip(in0(ModIndex), 0.0f, 10.0f);
            
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
            
            // 3. Clear OS buffer
            memset(m_outputOSBuffer, 0, m_osRatio * sizeof(float));
            
            // 4. Process all grains
            for (int g = 0; g < NUM_VOICES; ++g) {
                
                // Trigger new grain if needed and store graindata
                if (m_allocator.triggers[g]) {
                    m_grainData[g].oscFreq = oscFreq;
                    m_grainData[g].modFreq = modFreq;
                    m_grainData[g].modIndex = modIndex;
                    m_grainData[g].sampleCount = offset;
                    m_pmFilters[g].reset();
                }
                
                // Process grain if voice is active
                if (m_allocator.isActive[g]) {
                    
                    // Calculate slopes
                    float oscSlope = m_grainData[g].oscFreq * m_sampleDur;
                    float envSlope = static_cast<float>(m_allocator.localSlopes[g]);
                    float modSlope = m_grainData[g].modFreq * m_sampleDur;
 
                    // Accumulate osc and mod phases
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(oscSlope)));
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(modSlope)));
 
                    // Calculate mipmap parameters for mod (use floor for oversampling)
                    float modRangeSize = static_cast<float>(modCycleSamples);
                    float modSamplesPerFrame = std::abs(modSlope) * modRangeSize;
                    float modOctave = sc_max(0.0f, sc_log2(modSamplesPerFrame));
                    int modLayer = static_cast<int>(sc_floor(modOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for mod
                    int modSpacing1 = 1 << modLayer;
                    int modSpacing2 = modSpacing1 << 1;
                    float modCrossfade = sc_frac(modOctave);
 
                    // Calculate mipmap parameters for osc (use floor for oversampling)
                    float oscRangeSize = static_cast<float>(oscCycleSamples);
                    float oscSamplesPerFrame = std::abs(oscSlope) * oscRangeSize;
                    float oscOctave = sc_max(0.0f, sc_log2(oscSamplesPerFrame));
                    int oscLayer = static_cast<int>(sc_floor(oscOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for osc
                    int oscSpacing1 = 1 << oscLayer;
                    int oscSpacing2 = oscSpacing1 << 1;
                    float oscCrossfade = sc_frac(oscOctave);
                    
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
                    float envRangeSize = static_cast<float>(envCycleSamples);
                    float envSamplesPerFrame = std::abs(envSlope) * envRangeSize;
                    float envOctave = sc_max(0.0f, sc_log2(envSamplesPerFrame));
                    int envLayer = static_cast<int>(sc_floor(envOctave));
                    
                    // Calculate spacings for adjacent mipmap levels for env
                    int envSpacing1 = 1 << envLayer;
                    int envSpacing2 = envSpacing1 << 1;
                    float envCrossfade = sc_frac(envOctave);
 
                    // Calculate mod scale ratio for PM
                    float modScaleRatio = 0.0f;
                    if (sc_abs(modSlope) > Utils::SAFE_DENOM_EPSILON) {
                        modScaleRatio = sc_abs(oscSlope / modSlope);
                    }
                    
                    for (int k = 0; k < m_osRatio; k++) {
                        
                        // Clamp upsampled values
                        m_oscCyclePosOSBuffer[k] = sc_clip(m_oscCyclePosOSBuffer[k], 0.0f, 1.0f);
                        m_envCyclePosOSBuffer[k] = sc_clip(m_envCyclePosOSBuffer[k], 0.0f, 1.0f);
                        m_modCyclePosOSBuffer[k] = sc_clip(m_modCyclePosOSBuffer[k], 0.0f, 1.0f);
                        
                        // Increment oversampled phases
                        osModPhase += osModSlope;
                        osOscPhase += osOscSlope;
                        osEnvPhase += osEnvSlope;
                        
                        // Process mod wavetable oscillator
                        float modOsc = OscUtils::wavetableOsc(
                            sc_frac(osModPhase), modTable.data, 
                            modCycleSamples, modNumCycles, m_modCyclePosOSBuffer[k],
                            modSpacing1, modSpacing2, modCrossfade
                        );
                        
                        // Apply Phase Modulation
                        float modFiltered = m_pmFilters[g].processLowpass(modOsc, osModSlope);
                        float modScaled = modFiltered / Utils::TWO_PI * modScaleRatio;
                        float modulatedOscPhase = sc_frac(osOscPhase + (modScaled * m_grainData[g].modIndex));
                        
                        // Process osc wavetable oscillator
                        float grainOsc = OscUtils::wavetableOsc(
                            modulatedOscPhase, oscTable.data, 
                            oscCycleSamples, oscNumCycles, m_oscCyclePosOSBuffer[k],
                            oscSpacing1, oscSpacing2, oscCrossfade
                        );
                        
                        // Process env wavetable oscillator
                        float grainWindow = OscUtils::wavetableOsc(
                            osEnvPhase, envTable.data, 
                            envCycleSamples, envNumCycles, m_envCyclePosOSBuffer[k],
                            envSpacing1, envSpacing2, envCrossfade
                        );
                        
                        // Accumulate grain output
                        m_outputOSBuffer[k] += grainOsc * grainWindow;
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
        
    envCyclePosPast = isEnvCyclePosAudioRate ? 
        sc_clip(in(EnvCyclePos)[nSamples - 1], 0.0f, 1.0f) : slopedEnvCyclePos.value;
 
    modCyclePosPast = isModCyclePosAudioRate ? 
        sc_clip(in(ModCyclePos)[nSamples - 1], 0.0f, 1.0f) : slopedModCyclePos.value;
}

// ===== DUAL PULSAR OSCILLATOR =====
 
DualPulsarOS::DualPulsarOS() :
    m_sampleRate(static_cast<float>(sampleRate())),
    m_sampleDur(static_cast<float>(sampleDur())),
    m_oversampleIndex(sc_clip(static_cast<int>(in0(Oversample)), 0, 4)),
    m_osRatio(1 << m_oversampleIndex)
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
    if (m_oversampleIndex > 0) {
        auto unit = this;

        // Allocate oversampling buffers
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_outputOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_oscCyclePosOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_modCyclePosOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_skewOSBuffer);
        PluginUtils::allocBuffer(unit, mWorld, m_osRatio, m_indexOSBuffer);

        // Setup oversampling filters
        m_outputOversampling.init(m_osRatio, m_sampleRate, m_outputOSBuffer);
        m_oscCyclePosOversampling.init(m_osRatio, m_sampleRate, m_oscCyclePosOSBuffer);
        m_modCyclePosOversampling.init(m_osRatio, m_sampleRate, m_modCyclePosOSBuffer);
        m_skewOversampling.init(m_osRatio, m_sampleRate, m_skewOSBuffer);
        m_indexOversampling.init(m_osRatio, m_sampleRate, m_indexOSBuffer);
    }
 
    // Set calc function & compute initial sample
    set_calc_function<DualPulsarOS, &DualPulsarOS::next>();
 
    // Reset state after priming
    m_allocator.reset();
    m_trigger.reset();
}
 
DualPulsarOS::~DualPulsarOS() {
    RTFree(mWorld, m_outputOSBuffer);
    RTFree(mWorld, m_oscCyclePosOSBuffer);
    RTFree(mWorld, m_modCyclePosOSBuffer);
    RTFree(mWorld, m_skewOSBuffer);
    RTFree(mWorld, m_indexOSBuffer);
}
 
void DualPulsarOS::next(int nSamples) {
 
    // Control-rate parameters with smooth interpolation (sloped params)
    auto slopedOscCyclePos = makeSlope(sc_clip(in0(OscCyclePos), 0.0f, 1.0f), oscCyclePosPast);
    auto slopedModCyclePos = makeSlope(sc_clip(in0(ModCyclePos), 0.0f, 1.0f), modCyclePosPast);
    auto slopedSkew = makeSlope(sc_clip(in0(Skew), 0.0f, 1.0f), skewPast);
    auto slopedIndex = makeSlope(sc_clip(in0(Index), 0.0f, 10.0f), indexPast);
 
    // Control-rate parameters (settings, no interpolation)
    float oscBufNum = in0(OscBuffer);
    float modBufNum = in0(ModBuffer);
    int oscNumCycles = sc_max(static_cast<int>(in0(OscNumCycles)), 1);
    int modNumCycles = sc_max(static_cast<int>(in0(ModNumCycles)), 1);
 
    // Output pointer
    float* output = out(Out);

    // Get wavetable data
    auto oscTable = m_oscBufUnit.GetTable(this, oscBufNum, "DualPulsarOS osc");
    auto modTable = m_modBufUnit.GetTable(this, modBufNum, "DualPulsarOS mod");
    if (!oscTable.valid || !modTable.valid) {
        ClearUnitOutputs(this, nSamples);
        return;
    }
 
    // Calculate samples per cycle
    int oscCycleSamples = oscTable.size / oscNumCycles;
    int modCycleSamples = modTable.size / modNumCycles;
 
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
                    float oscSlope = m_grainData[g].oscFreq * m_sampleDur;
                    float modSlope = m_grainData[g].modFreq * m_sampleDur;
                    float envSlope = static_cast<float>(m_allocator.localSlopes[g]);
 
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
                    float oscRangeSize = static_cast<float>(oscCycleSamples);
                    float oscSamplesPerFrame = std::abs(oscSlope) * oscRangeSize;
                    float oscOctave = sc_max(0.0f, sc_log2(oscSamplesPerFrame));
                    int oscLayer = static_cast<int>(sc_ceil(oscOctave));
 
                    // Calculate spacings for adjacent mipmap levels for osc
                    int oscSpacing1 = 1 << oscLayer;
                    int oscSpacing2 = oscSpacing1 << 1;
                    float oscCrossfade = sc_frac(oscOctave);
 
                    // Calculate mipmap parameters for mod (use ceil for no oversampling)
                    float modRangeSize = static_cast<float>(modCycleSamples);
                    float modSamplesPerFrame = std::abs(modSlope) * modRangeSize;
                    float modOctave = sc_max(0.0f, sc_log2(modSamplesPerFrame));
                    int modLayer = static_cast<int>(sc_ceil(modOctave));
 
                    // Calculate spacings for adjacent mipmap levels for mod
                    int modSpacing1 = 1 << modLayer;
                    int modSpacing2 = modSpacing1 << 1;
                    float modCrossfade = sc_frac(modOctave);
 
                    // Process cross-modulated dual oscillator
                    auto result = m_dualOscs[g].process(
                        oscPhaseDistorted, modPhaseDistorted,
                        oscCyclePosVal, modCyclePosVal,
                        oscSlope, modSlope,
                        m_grainData[g].pmIndexOsc, m_grainData[g].pmIndexMod,
                        m_grainData[g].pmFilterRatioOsc, m_grainData[g].pmFilterRatioMod,
                        oscSpacing1, oscSpacing2, oscCrossfade,
                        modSpacing1, modSpacing2, modCrossfade,
                        oscTable.data, oscCycleSamples, oscNumCycles,
                        modTable.data, modCycleSamples, modNumCycles
                    );
 
                    // Process gaussian window
                    float grainWindow = WindowFunctions::gaussianWindow(
                        m_allocator.phases[g], skewVal, indexVal);
 
                    // Accumulate grain output
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
            memset(m_outputOSBuffer, 0, m_osRatio * sizeof(float));
 
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
                    float oscSlope = m_grainData[g].oscFreq * m_sampleDur;
                    float modSlope = m_grainData[g].modFreq * m_sampleDur;
                    float envSlope = static_cast<float>(m_allocator.localSlopes[g]);
 
                    // Derive osc and mod phases from sample count
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(oscSlope)));
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * static_cast<double>(modSlope)));
 
                    // Calculate mipmap parameters for osc (use floor for oversampling)
                    float oscRangeSize = static_cast<float>(oscCycleSamples);
                    float oscSamplesPerFrame = std::abs(oscSlope) * oscRangeSize;
                    float oscOctave = sc_max(0.0f, sc_log2(oscSamplesPerFrame));
                    int oscLayer = static_cast<int>(sc_floor(oscOctave));
 
                    // Calculate spacings for adjacent mipmap levels for osc
                    int oscSpacing1 = 1 << oscLayer;
                    int oscSpacing2 = oscSpacing1 << 1;
                    float oscCrossfade = sc_frac(oscOctave);
 
                    // Calculate mipmap parameters for mod (use floor for oversampling)
                    float modRangeSize = static_cast<float>(modCycleSamples);
                    float modSamplesPerFrame = std::abs(modSlope) * modRangeSize;
                    float modOctave = sc_max(0.0f, sc_log2(modSamplesPerFrame));
                    int modLayer = static_cast<int>(sc_floor(modOctave));
 
                    // Calculate spacings for adjacent mipmap levels for mod
                    int modSpacing1 = 1 << modLayer;
                    int modSpacing2 = modSpacing1 << 1;
                    float modCrossfade = sc_frac(modOctave);
 
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
                        m_oscCyclePosOSBuffer[k] = sc_clip(m_oscCyclePosOSBuffer[k], 0.0f, 1.0f);
                        m_modCyclePosOSBuffer[k] = sc_clip(m_modCyclePosOSBuffer[k], 0.0f, 1.0f);
                        m_skewOSBuffer[k] = sc_clip(m_skewOSBuffer[k], 0.0f, 1.0f);
                        m_indexOSBuffer[k] = sc_clip(m_indexOSBuffer[k], 0.0f, 10.0f);
 
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
                            m_oscCyclePosOSBuffer[k], m_modCyclePosOSBuffer[k],
                            osOscSlope, osModSlope,
                            m_grainData[g].pmIndexOsc, m_grainData[g].pmIndexMod,
                            m_grainData[g].pmFilterRatioOsc, m_grainData[g].pmFilterRatioMod,
                            oscSpacing1, oscSpacing2, oscCrossfade,
                            modSpacing1, modSpacing2, modCrossfade,
                            oscTable.data, oscCycleSamples, oscNumCycles,
                            modTable.data, modCycleSamples, modNumCycles
                        );
 
                        // Process gaussian window with upsampled skew and index
                        float grainWindow = WindowFunctions::gaussianWindow(
                            osEnvPhase, m_skewOSBuffer[k], m_indexOSBuffer[k]);
 
                        // Accumulate grain output
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

void Oscs_setup()
{
    registerUnit<DualOscOS>(ft, "DualOscOS", false);
    registerUnit<SingleOscOS>(ft, "SingleOscOS", false);
    registerUnit<PulsarOS>(ft, "PulsarOS", false);
    registerUnit<DualPulsarOS>(ft, "DualPulsarOS", false);
}