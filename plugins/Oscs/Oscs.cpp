#include "Oscs.hpp"
#include "SC_PlugIn.hpp"

extern InterfaceTable* ft;

// ===== SINGLE WAVETABLE OSCILLATOR =====

SingleOscOS::SingleOscOS() :
    m_oversampleIndex(sc_clip(static_cast<int>(in0(Oversample)), 0, 4)),
    m_osRatio(1 << m_oversampleIndex)
{
    // Check which inputs are audio-rate
    isCyclePosAudioRate = isAudioRateIn(CyclePos);
    
    // Allocate oversampling buffer
    if (m_oversampleIndex > 0) {
        BufferUtils::allocBuffer(this, mWorld, m_osRatio, m_outputOSBuffer);
    }
    
    // Set calc function & compute initial sample
    set_calc_function<SingleOscOS, &SingleOscOS::next>();
}

SingleOscOS::~SingleOscOS() {
    RTFree(mWorld, m_outputOSBuffer);
}

void SingleOscOS::next(int nSamples) {
    
    // Audio-rate input
    const float* phaseIn = in(Phase);
    
    // Control-rate parameters with smooth interpolation
    m_cyclePosInterp.update(sc_clip(in0(CyclePos), 0.0f, 1.0f), nSamples);

    // Control-rate parameters (settings, no interpolation)
    float bufNum = in0(BufNum);
    int numCycles = sc_max(static_cast<int>(in0(NumCycles)), 1);

    // Output pointer
    float* output = out(Out);

    // Get wavetable data
    auto oscTable = m_oscWavetable.update(this, mWorld, nSamples, bufNum, numCycles, "SingleOscOS");
    if (!oscTable.data) return;

    if (m_oversampleIndex == 0) {

        for (int i = 0; i < nSamples; ++i) {
            
            // Wrap phase between 0 and 1
            float phase = sc_frac(phaseIn[i]);
            
            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePos = isCyclePosAudioRate ? 
                sc_clip(in(CyclePos)[i], 0.0f, 1.0f) : 
                m_cyclePosInterp.process();

            // Derive slope from incoming ramp
            float slope = static_cast<float>(m_rampToSlope.process(static_cast<double>(phase)));

            // Process wavetable oscillator
            output[i] = OscUtils::wavetableOsc(
                phase, slope, 
                oscTable, cyclePos
            );
        }
    } else {

        for (int i = 0; i < nSamples; ++i) {
            
            // Wrap phase between 0 and 1
            float phase = sc_frac(phaseIn[i]);
            
            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePos = isCyclePosAudioRate ? 
                sc_clip(in(CyclePos)[i], 0.0f, 1.0f) : 
                m_cyclePosInterp.process();

            // Derive slope from incoming ramp
            float slope = static_cast<float>(m_rampToSlope.process(static_cast<double>(phase)));

            // Prepare phase and slope for oversampling
            float osSlope = slope / static_cast<float>(m_osRatio);
            float osPhase = phase - slope;

            // Store parameter values for oversampling
            m_osCyclePosInterp.update(cyclePos);
            
            for (int k = 0; k < m_osRatio; k++) {
                
                // Calculate fractional position for interpolation
                float frac = static_cast<float>(k + 1) / static_cast<float>(m_osRatio);
                
                // Interpolate parameter values
                float osCyclePos = m_osCyclePosInterp.process(frac);

                // Increment phase
                osPhase += osSlope;
                
                // Process wavetable oscillator
                m_outputOSBuffer[k] = OscUtils::wavetableOsc(
                    sc_frac(osPhase), osSlope, 
                    oscTable, osCyclePos
                );
            }
            
            // Downsample output
            output[i] = m_outputOversampling.downsample(m_outputOSBuffer, m_osRatio);
        }
    }
    
}

// ===== DUAL WAVETABLE OSCILLATOR =====

DualOscOS::DualOscOS() :
    m_sampleRate(static_cast<float>(sampleRate())), 
    m_oversampleIndex(sc_clip(static_cast<int>(in0(Oversample)), 0, 4)),
    m_osRatio(1 << m_oversampleIndex)
{
    // Check which inputs are audio-rate
    isCyclePosAAudioRate = isAudioRateIn(CyclePosA);
    isCyclePosBAAudioRate = isAudioRateIn(CyclePosB);
    isXmIndexAAudioRate = isAudioRateIn(XmIndexA);
    isXmIndexBAudioRate = isAudioRateIn(XmIndexB);
    isXmFltRatioAAudioRate = isAudioRateIn(XmFltRatioA);
    isXmFltRatioBAudioRate = isAudioRateIn(XmFltRatioB);

    // Allocate oversampling buffers
    if (m_oversampleIndex > 0) {
        BufferUtils::allocBuffer(this, mWorld, m_osRatio, m_outputOSBufferA);
        BufferUtils::allocBuffer(this, mWorld, m_osRatio, m_outputOSBufferB);
    }
    
    // Set calc function & compute initial sample
    set_calc_function<DualOscOS, &DualOscOS::next>();
}

DualOscOS::~DualOscOS() {
    RTFree(mWorld, m_outputOSBufferA);
    RTFree(mWorld, m_outputOSBufferB);
}

void DualOscOS::next(int nSamples) {

    // Audio-rate inputs
    const float* phaseAIn = in(PhaseA);
    const float* phaseBIn = in(PhaseB);
    
    // Control-rate parameters with smooth interpolation
    m_cyclePosAInterp.update(sc_clip(in0(CyclePosA), 0.0f, 1.0f), nSamples);
    m_cyclePosBInterp.update(sc_clip(in0(CyclePosB), 0.0f, 1.0f), nSamples);
    m_xmIndexAInterp.update(sc_clip(in0(XmIndexA), 0.0f, 10.0f), nSamples);
    m_xmIndexBInterp.update(sc_clip(in0(XmIndexB), 0.0f, 10.0f), nSamples);
    m_xmFltRatioAInterp.update(sc_clip(in0(XmFltRatioA), 0.0f, 10.0f), nSamples);
    m_xmFltRatioBInterp.update(sc_clip(in0(XmFltRatioB), 0.0f, 10.0f), nSamples);
    
    // Control-rate parameters (settings, no interpolation)
    float bufNumA = in0(BufNumA);
    float bufNumB = in0(BufNumB);
    int numCyclesA = sc_max(static_cast<int>(in0(NumCyclesA)), 1);
    int numCyclesB = sc_max(static_cast<int>(in0(NumCyclesB)), 1);

    // Output pointers
    float* outputA = out(OutA);
    float* outputB = out(OutB);

    // Get wavetable data
    auto oscTableA = m_oscWavetableA.update(this, mWorld, nSamples, bufNumA, numCyclesA, "DualOscOS oscA");
    auto oscTableB = m_oscWavetableB.update(this, mWorld, nSamples, bufNumB, numCyclesB, "DualOscOS oscB");
    if (!oscTableA.data || !oscTableB.data) return;

    if (m_oversampleIndex == 0) {

        for (int i = 0; i < nSamples; ++i) {

            // Wrap phases between 0 and 1
            float phaseA = sc_frac(phaseAIn[i]);
            float phaseB = sc_frac(phaseBIn[i]);

            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePosA = isCyclePosAAudioRate ?
                sc_clip(in(CyclePosA)[i], 0.0f, 1.0f) : 
                m_cyclePosAInterp.process();

            float cyclePosB = isCyclePosBAAudioRate ? 
                sc_clip(in(CyclePosB)[i], 0.0f, 1.0f) : 
                m_cyclePosBInterp.process();

            float xmIndexA = isXmIndexAAudioRate ? 
                sc_clip(in(XmIndexA)[i], 0.0f, 10.0f) : 
                m_xmIndexAInterp.process();

            float xmIndexB = isXmIndexBAudioRate ? 
                sc_clip(in(XmIndexB)[i], 0.0f, 10.0f) : 
                m_xmIndexBInterp.process();

            float xmFltRatioA = isXmFltRatioAAudioRate ? 
                sc_clip(in(XmFltRatioA)[i], 1.0f, 10.0f) : 
                m_xmFltRatioAInterp.process();

            float xmFltRatioB = isXmFltRatioBAudioRate ? 
                sc_clip(in(XmFltRatioB)[i], 1.0f, 10.0f) : 
                m_xmFltRatioBInterp.process();

            // Derive slopes from incoming ramps
            float slopeA = static_cast<float>(m_rampToSlopeA.process(static_cast<double>(phaseA)));
            float slopeB = static_cast<float>(m_rampToSlopeB.process(static_cast<double>(phaseB)));

            // Process wavetable oscillator with cross modulation
            auto result = m_dualOsc.process(
                phaseA, phaseB,
                slopeA, slopeB,
                xmIndexA, xmIndexB,
                xmFltRatioA, xmFltRatioB,
                cyclePosA, cyclePosB,
                oscTableA, oscTableB,
                m_sampleRate
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
            float cyclePosA = isCyclePosAAudioRate ? 
                sc_clip(in(CyclePosA)[i], 0.0f, 1.0f) : 
                m_cyclePosAInterp.process();

            float cyclePosB = isCyclePosBAAudioRate ? 
                sc_clip(in(CyclePosB)[i], 0.0f, 1.0f) : 
                m_cyclePosBInterp.process();

            float xmIndexA = isXmIndexAAudioRate ? 
                sc_clip(in(XmIndexA)[i], 0.0f, 10.0f) : 
                m_xmIndexAInterp.process();

            float xmIndexB = isXmIndexBAudioRate ? 
                sc_clip(in(XmIndexB)[i], 0.0f, 10.0f) : 
                m_xmIndexBInterp.process();

            float xmFltRatioA = isXmFltRatioAAudioRate ? 
                sc_clip(in(XmFltRatioA)[i], 1.0f, 10.0f) : 
                m_xmFltRatioAInterp.process();

            float xmFltRatioB = isXmFltRatioBAudioRate ? 
                sc_clip(in(XmFltRatioB)[i], 1.0f, 10.0f) : 
                m_xmFltRatioBInterp.process();

            // Derive slopes from incoming ramps
            float slopeA = static_cast<float>(m_rampToSlopeA.process(static_cast<double>(phaseA)));
            float slopeB = static_cast<float>(m_rampToSlopeB.process(static_cast<double>(phaseB)));

            // Prepare phases and slopes for oversampling
            float osSlopeA = slopeA / static_cast<float>(m_osRatio);
            float osSlopeB = slopeB / static_cast<float>(m_osRatio);
            float osPhaseA = phaseA - slopeA;
            float osPhaseB = phaseB - slopeB;

            // Store parameter values for oversampling
            m_osCyclePosAInterp.update(cyclePosA);
            m_osCyclePosBInterp.update(cyclePosB);
            m_osXmIndexAInterp.update(xmIndexA);
            m_osXmIndexBInterp.update(xmIndexB);
            m_osXmFltRatioAInterp.update(xmFltRatioA);
            m_osXmFltRatioBInterp.update(xmFltRatioB);
            
            for (int k = 0; k < m_osRatio; k++) {
                
                // Calculate fractional position for interpolation
                float frac = static_cast<float>(k + 1) / static_cast<float>(m_osRatio);
                
                // Interpolate parameter values
                float osCyclePosA = m_osCyclePosAInterp.process(frac);
                float osCyclePosB = m_osCyclePosBInterp.process(frac);
                float osXmIndexA = m_osXmIndexAInterp.process(frac);
                float osXmIndexB = m_osXmIndexBInterp.process(frac);
                float osXmFltRatioA = m_osXmFltRatioAInterp.process(frac);
                float osXmFltRatioB = m_osXmFltRatioBInterp.process(frac);

                // Increment phases
                osPhaseA += osSlopeA;
                osPhaseB += osSlopeB;
                
                // Process wavetable oscillator with cross modulation
                auto result = m_dualOsc.process(
                    sc_frac(osPhaseA), sc_frac(osPhaseB),
                    osSlopeA, osSlopeB,
                    osXmIndexA, osXmIndexB,
                    osXmFltRatioA, osXmFltRatioB,
                    osCyclePosA, osCyclePosB,
                    oscTableA, oscTableB,
                    m_sampleRate
                );
                
                m_outputOSBufferA[k] = result.oscA;
                m_outputOSBufferB[k] = result.oscB;
            }
            
            // Downsample outputs
            outputA[i] = m_outputOversamplingA.downsample(m_outputOSBufferA, m_osRatio);
            outputB[i] = m_outputOversamplingB.downsample(m_outputOSBufferB, m_osRatio);
        }
    }

}

// ===== PULSAR OSCILLATOR =====
 
PulsarOS::PulsarOS() : 
    m_sampleRate(static_cast<float>(sampleRate())),
    m_oversampleIndex(sc_clip(static_cast<int>(in0(Oversample)), 0, 4)),
    m_osRatio(1 << m_oversampleIndex)
{
    // Check which inputs are audio-rate
    isTriggerAudioRate = isAudioRateIn(Trigger);
    isTriggerFreqAudioRate = isAudioRateIn(TriggerFreq);
    isSubSampleOffsetAudioRate = isAudioRateIn(SubSampleOffset);
    isOscFreqAudioRate = isAudioRateIn(OscFreq);
    isModFreqAudioRate = isAudioRateIn(ModFreq);
    isPmIndexAudioRate = isAudioRateIn(PmIndex);
    isOscCyclePosAudioRate = isAudioRateIn(OscCyclePos);
    isEnvCyclePosAudioRate = isAudioRateIn(EnvCyclePos);
    isModCyclePosAudioRate = isAudioRateIn(ModCyclePos);
    
    // Allocate oversampling buffer
    if (m_oversampleIndex > 0) {
        BufferUtils::allocBuffer(this, mWorld, m_osRatio, m_outputOSBuffer);
    }
    
    // Set calc function & compute initial sample
    set_calc_function<PulsarOS, &PulsarOS::next>();
    
    // Reset state after priming
    m_allocator.reset();
    m_trigger.reset();
}
 
PulsarOS::~PulsarOS() {
    RTFree(mWorld, m_outputOSBuffer);
}
 
void PulsarOS::next(int nSamples) {
    
    // Control-rate parameters with smooth interpolation
    m_oscCyclePosInterp.update(sc_clip(in0(OscCyclePos), 0.0f, 1.0f), nSamples);
    m_envCyclePosInterp.update(sc_clip(in0(EnvCyclePos), 0.0f, 1.0f), nSamples);
    m_modCyclePosInterp.update(sc_clip(in0(ModCyclePos), 0.0f, 1.0f), nSamples);
    
    // Control-rate parameters (settings, no interpolation)
    float oscBufNum = in0(OscBufNum);
    float modBufNum = in0(ModBufNum);
    float envBufNum = in0(EnvBufNum);
    int oscNumCycles = sc_max(static_cast<int>(in0(OscNumCycles)), 1);
    int modNumCycles = sc_max(static_cast<int>(in0(ModNumCycles)), 1);
    int envNumCycles = sc_max(static_cast<int>(in0(EnvNumCycles)), 1);
    
    // Output pointer
    float* output = out(Out);

    // Get wavetable data
    auto oscTable = m_oscWavetable.update(this, mWorld, nSamples, oscBufNum, oscNumCycles, "PulsarOS osc");
    auto modTable = m_modWavetable.update(this, mWorld, nSamples, modBufNum, modNumCycles, "PulsarOS mod");
    auto envTable = m_envWavetable.update(this, mWorld, nSamples, envBufNum, envNumCycles, "PulsarOS env");
    if (!oscTable.data || !modTable.data || !envTable.data) return;
    
    if (m_oversampleIndex == 0) {
 
        for (int i = 0; i < nSamples; ++i) {
            
            // Trigger input (audio-rate or control-rate)
            bool trigger = isTriggerAudioRate ? 
                m_trigger.process(in(Trigger)[i]) : 
                m_trigger.process(in0(Trigger));
            
            // Get current parameter values (no interpolation - latched per trigger)
            float triggerFreq = isTriggerFreqAudioRate ? 
                sc_clip(in(TriggerFreq)[i], 0.0f, m_sampleRate * 0.49f) : 
                sc_clip(in0(TriggerFreq), 0.0f, m_sampleRate * 0.49f);
            
            float subSampleOffset = isSubSampleOffsetAudioRate ? 
                in(SubSampleOffset)[i] : 
                in0(SubSampleOffset);
 
            float oscFreq = isOscFreqAudioRate ? 
                sc_clip(in(OscFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(OscFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f); 
            
            float modFreq = isModFreqAudioRate ? 
                sc_clip(in(ModFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) : 
                sc_clip(in0(ModFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float pmIndex = isPmIndexAudioRate ? 
                sc_clip(in(PmIndex)[i], 0.0f, 10.0f) : 
                sc_clip(in0(PmIndex), 0.0f, 10.0f);
            
            // Get current parameter values (audio-rate or interpolated control-rate)            
            float oscCyclePos = isOscCyclePosAudioRate ?
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) :
                m_oscCyclePosInterp.process();
            
            float envCyclePos = isEnvCyclePosAudioRate ?
                sc_clip(in(EnvCyclePos)[i], 0.0f, 1.0f) :
                m_envCyclePosInterp.process();
 
            float modCyclePos = isModCyclePosAudioRate ?
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) :
                m_modCyclePosInterp.process();
            
            // 1. Process voice allocator
            auto voices = m_allocator.process(
                NUM_VOICES,
                trigger,
                triggerFreq,
                subSampleOffset,
                m_sampleRate
            );
            
            // 2. Process all grains
            float sum = 0.0f;
            for (int g = 0; g < NUM_VOICES; ++g) {

                // Trigger new grain if needed and store graindata
                if (voices.triggers[g]) {
                    m_grainData[g].oscFreq = oscFreq;
                    m_grainData[g].modFreq = modFreq;
                    m_grainData[g].pmIndex = pmIndex;
                    m_grainData[g].sampleCount = subSampleOffset;
                    m_pmOscs[g].reset();
                }
                
                // Process grain if voice is active
                if (voices.gates[g]) {

                    // Calculate slopes
                    float modSlope = m_grainData[g].modFreq / m_sampleRate;
                    float oscSlope = m_grainData[g].oscFreq / m_sampleRate;

                    // Calculate phases
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * modSlope));
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * oscSlope));
                    float envPhase = voices.phases[g];

                    // Phase-modulation index scaling: normalize by modulator, scale by carrier
                    float pmScaleRatio = 0.0f;
                    if (sc_abs(modSlope) > Utils::SAFE_DENOM_EPSILON) {
                        pmScaleRatio = sc_abs(oscSlope / modSlope);
                    }

                    // Scale phase-modulation index
                    float pmIndexScaled = m_grainData[g].pmIndex * pmScaleRatio;

                    // Process wavetable oscillator with phase modulation
                    float grainOsc = m_pmOscs[g].process(
                        oscPhase, modPhase,
                        oscSlope, modSlope,
                        pmIndexScaled,
                        oscCyclePos, modCyclePos,
                        oscTable, modTable,
                        m_sampleRate
                    );

                    // Process wavetable oscillator for grain envelope
                    float grainWindow = OscUtils::wavetableOsc(
                        envPhase, voices.slopes[g], 
                        envTable, envCyclePos
                    );
                    
                    // Accumulate grain output
                    sum += grainOsc * grainWindow;
 
                    // Increment sample count
                    m_grainData[g].sampleCount++;
                }
            }
            
            // 3. DC block output
            output[i] = m_dcBlocker.process(sum, m_sampleRate);
        }
    } else {
 
        for (int i = 0; i < nSamples; ++i) {
            
            // Trigger input (audio-rate or control-rate)
            bool trigger = isTriggerAudioRate ? 
                m_trigger.process(in(Trigger)[i]) : 
                m_trigger.process(in0(Trigger));
            
            // Get current parameter values (no interpolation - latched per trigger)
            float triggerFreq = isTriggerFreqAudioRate ? 
                sc_clip(in(TriggerFreq)[i], 0.0f, m_sampleRate * 0.49f) : 
                sc_clip(in0(TriggerFreq), 0.0f, m_sampleRate * 0.49f);
            
            float subSampleOffset = isSubSampleOffsetAudioRate ? 
                in(SubSampleOffset)[i] : 
                in0(SubSampleOffset);
 
            float oscFreq = isOscFreqAudioRate ? 
                sc_clip(in(OscFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(OscFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f); 
            
            float modFreq = isModFreqAudioRate ? 
                sc_clip(in(ModFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) : 
                sc_clip(in0(ModFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float pmIndex = isPmIndexAudioRate ? 
                sc_clip(in(PmIndex)[i], 0.0f, 10.0f) : 
                sc_clip(in0(PmIndex), 0.0f, 10.0f);
            
            // Get current parameter values (audio-rate or interpolated control-rate)            
            float oscCyclePos = isOscCyclePosAudioRate ?
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) :
                m_oscCyclePosInterp.process();
            
            float envCyclePos = isEnvCyclePosAudioRate ?
                sc_clip(in(EnvCyclePos)[i], 0.0f, 1.0f) :
                m_envCyclePosInterp.process();
 
            float modCyclePos = isModCyclePosAudioRate ?
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) :
                m_modCyclePosInterp.process();
        
            // 1. Process voice allocator
            auto voices = m_allocator.process(
                NUM_VOICES,
                trigger,
                triggerFreq,
                subSampleOffset,
                m_sampleRate
            );
            
            // 2. Store parameter values for oversampling
            m_osOscCyclePosInterp.update(oscCyclePos);
            m_osEnvCyclePosInterp.update(envCyclePos);
            m_osModCyclePosInterp.update(modCyclePos);
            
            // 3. Clear OS buffer
            memset(m_outputOSBuffer, 0, m_osRatio * sizeof(float));
            
            // 4. Process all grains
            for (int g = 0; g < NUM_VOICES; ++g) {

                // Trigger new grain if needed and store graindata
                if (voices.triggers[g]) {
                    m_grainData[g].oscFreq = oscFreq;
                    m_grainData[g].modFreq = modFreq;
                    m_grainData[g].pmIndex = pmIndex;
                    m_grainData[g].sampleCount = subSampleOffset;
                    m_pmOscs[g].reset();
                }
                
                // Process grain if voice is active
                if (voices.gates[g]) {

                    // Calculate slopes
                    float modSlope = m_grainData[g].modFreq / m_sampleRate;
                    float oscSlope = m_grainData[g].oscFreq / m_sampleRate;
                    float envSlope = voices.slopes[g];

                    // Calculate phases
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * modSlope));
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * oscSlope));
                    float envPhase = voices.phases[g];

                    // Phase-modulation index scaling: normalize by modulator, scale by carrier
                    float pmScaleRatio = 0.0f;
                    if (sc_abs(modSlope) > Utils::SAFE_DENOM_EPSILON) {
                        pmScaleRatio = sc_abs(oscSlope / modSlope);
                    }

                    // Scale phase-modulation index
                    float pmIndexScaled = m_grainData[g].pmIndex * pmScaleRatio;

                    // Prepare phases and slopes for oversampling
                    float osModSlope = modSlope / static_cast<float>(m_osRatio);
                    float osOscSlope = oscSlope / static_cast<float>(m_osRatio);
                    float osEnvSlope = envSlope / static_cast<float>(m_osRatio);
                    float osModPhase = modPhase - modSlope;
                    float osOscPhase = oscPhase - oscSlope;
                    float osEnvPhase = envPhase - envSlope;
                    
                    for (int k = 0; k < m_osRatio; k++) {
                        
                        // Calculate fractional position for interpolation
                        float frac = static_cast<float>(k + 1) / static_cast<float>(m_osRatio);
                        
                        // Interpolate parameter values
                        float osOscCyclePos = m_osOscCyclePosInterp.process(frac);
                        float osEnvCyclePos = m_osEnvCyclePosInterp.process(frac);
                        float osModCyclePos = m_osModCyclePosInterp.process(frac);

                        // Increment phases
                        osModPhase += osModSlope;
                        osOscPhase += osOscSlope;
                        osEnvPhase += osEnvSlope;
                        
                        // Process wavetable oscillator with phase modulation
                        float grainOsc = m_pmOscs[g].process(
                            sc_frac(osOscPhase), sc_frac(osModPhase),
                            osOscSlope, osModSlope,
                            pmIndexScaled,
                            osOscCyclePos, osModCyclePos,
                            oscTable, modTable,
                            m_sampleRate
                        );
                        
                        // Process wavetable oscillator for grain envelope
                        float grainWindow = OscUtils::wavetableOsc(
                            osEnvPhase, osEnvSlope, 
                            envTable, osEnvCyclePos
                        );
                        
                        // Accumulate grain output
                        m_outputOSBuffer[k] += grainOsc * grainWindow;
                    }
 
                    // Increment sample count
                    m_grainData[g].sampleCount++;
                }
            }

            // 5. Downsample and DC block output
            float downsampled = m_outputOversampling.downsample(m_outputOSBuffer, m_osRatio);
            output[i] = m_dcBlocker.process(downsampled, m_sampleRate);
        }
    }
}

// ===== DUAL PULSAR OSCILLATOR =====
 
DualPulsarOS::DualPulsarOS() :
    m_sampleRate(static_cast<float>(sampleRate())),
    m_oversampleIndex(sc_clip(static_cast<int>(in0(Oversample)), 0, 4)),
    m_osRatio(1 << m_oversampleIndex)
{
    // Check which inputs are audio-rate
    isTriggerAudioRate = isAudioRateIn(Trigger);
    isTriggerFreqAudioRate = isAudioRateIn(TriggerFreq);
    isSubSampleOffsetAudioRate = isAudioRateIn(SubSampleOffset);
    isOscFreqAudioRate = isAudioRateIn(OscFreq);
    isModFreqAudioRate = isAudioRateIn(ModFreq);
    isOscXmIndexAudioRate = isAudioRateIn(OscXmIndex);
    isModXmIndexAudioRate = isAudioRateIn(ModXmIndex);
    isOscXmFltRatioAudioRate = isAudioRateIn(OscXmFltRatio);
    isModXmFltRatioAudioRate = isAudioRateIn(ModXmFltRatio);
    isOscWarpAudioRate = isAudioRateIn(OscWarp);
    isModWarpAudioRate = isAudioRateIn(ModWarp);
    isOscCyclePosAudioRate = isAudioRateIn(OscCyclePos);
    isModCyclePosAudioRate = isAudioRateIn(ModCyclePos);
    isEnvSkewAudioRate = isAudioRateIn(EnvSkew);
    isEnvIndexAudioRate = isAudioRateIn(EnvIndex);
 
    // Allocate oversampling buffer
    if (m_oversampleIndex > 0) {
        BufferUtils::allocBuffer(this, mWorld, m_osRatio, m_outputOSBuffer);
    }
 
    // Set calc function & compute initial sample
    set_calc_function<DualPulsarOS, &DualPulsarOS::next>();
 
    // Reset state after priming
    m_allocator.reset();
    m_trigger.reset();
}
 
DualPulsarOS::~DualPulsarOS() {
    RTFree(mWorld, m_outputOSBuffer);
}
 
void DualPulsarOS::next(int nSamples) {
 
    // Control-rate parameters with smooth interpolation
    m_oscCyclePosInterp.update(sc_clip(in0(OscCyclePos), 0.0f, 1.0f), nSamples);
    m_modCyclePosInterp.update(sc_clip(in0(ModCyclePos), 0.0f, 1.0f), nSamples);
    m_envSkewInterp.update(sc_clip(in0(EnvSkew), 0.0f, 1.0f), nSamples);
    m_envIndexInterp.update(sc_clip(in0(EnvIndex), 0.0f, 10.0f), nSamples);
 
    // Control-rate parameters (settings, no interpolation)
    float oscBufNum = in0(OscBufNum);
    float modBufNum = in0(ModBufNum);
    int oscNumCycles = sc_max(static_cast<int>(in0(OscNumCycles)), 1);
    int modNumCycles = sc_max(static_cast<int>(in0(ModNumCycles)), 1);
 
    // Output pointer
    float* output = out(Out);

    // Get wavetable data
    auto oscTable = m_oscWavetable.update(this, mWorld, nSamples, oscBufNum, oscNumCycles, "DualPulsarOS osc");
    auto modTable = m_modWavetable.update(this, mWorld, nSamples, modBufNum, modNumCycles, "DualPulsarOS mod");
    if (!oscTable.data || !modTable.data) return;

    if (m_oversampleIndex == 0) {
 
        for (int i = 0; i < nSamples; ++i) {
 
            // Trigger input (audio-rate or control-rate)
            bool trigger = isTriggerAudioRate ?
                m_trigger.process(in(Trigger)[i]) :
                m_trigger.process(in0(Trigger));
 
            // Get current parameter values (no interpolation - latched per trigger)
            float triggerFreq = isTriggerFreqAudioRate ?
                sc_clip(in(TriggerFreq)[i], 0.0f, m_sampleRate * 0.49f) :
                sc_clip(in0(TriggerFreq), 0.0f, m_sampleRate * 0.49f);
 
            float subSampleOffset = isSubSampleOffsetAudioRate ?
                in(SubSampleOffset)[i] :
                in0(SubSampleOffset);
 
            float oscFreq = isOscFreqAudioRate ?
                sc_clip(in(OscFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(OscFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float modFreq = isModFreqAudioRate ?
                sc_clip(in(ModFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(ModFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float oscXmIndex = isOscXmIndexAudioRate ?
                sc_clip(in(OscXmIndex)[i], 0.0f, 10.0f) :
                sc_clip(in0(OscXmIndex), 0.0f, 10.0f);
 
            float modXmIndex = isModXmIndexAudioRate ?
                sc_clip(in(ModXmIndex)[i], 0.0f, 10.0f) :
                sc_clip(in0(ModXmIndex), 0.0f, 10.0f);
 
            float oscXmFltRatio = isOscXmFltRatioAudioRate ?
                sc_clip(in(OscXmFltRatio)[i], 1.0f, 10.0f) :
                sc_clip(in0(OscXmFltRatio), 1.0f, 10.0f);
 
            float modXmFltRatio = isModXmFltRatioAudioRate ?
                sc_clip(in(ModXmFltRatio)[i], 1.0f, 10.0f) :
                sc_clip(in0(ModXmFltRatio), 1.0f, 10.0f);
 
            float oscWarp = isOscWarpAudioRate ?
                sc_clip(in(OscWarp)[i], 0.0f, 1.0f) :
                sc_clip(in0(OscWarp), 0.0f, 1.0f);
 
            float modWarp = isModWarpAudioRate ?
                sc_clip(in(ModWarp)[i], 0.0f, 1.0f) :
                sc_clip(in0(ModWarp), 0.0f, 1.0f);
 
            // Get current parameter values (audio-rate or interpolated control-rate)
            float oscCyclePos = isOscCyclePosAudioRate ?
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) :
                m_oscCyclePosInterp.process();
 
            float modCyclePos = isModCyclePosAudioRate ?
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) :
                m_modCyclePosInterp.process();
 
            float envSkew = isEnvSkewAudioRate ?
                sc_clip(in(EnvSkew)[i], 0.0f, 1.0f) :
                m_envSkewInterp.process();
 
            float envIndex = isEnvIndexAudioRate ?
                sc_clip(in(EnvIndex)[i], 0.0f, 10.0f) :
                m_envIndexInterp.process();
 
            // 1. Process voice allocator
            auto voices = m_allocator.process(
                NUM_VOICES,
                trigger,
                triggerFreq,
                subSampleOffset,
                m_sampleRate
            );
 
            // 2. Process all grains
            float sum = 0.0f;
            for (int g = 0; g < NUM_VOICES; ++g) {

                // Trigger new grain if needed and store graindata
                if (voices.triggers[g]) {
                    m_grainData[g].oscFreq = oscFreq;
                    m_grainData[g].modFreq = modFreq;
                    m_grainData[g].oscXmIndex = oscXmIndex;
                    m_grainData[g].modXmIndex = modXmIndex;
                    m_grainData[g].oscXmFltRatio = oscXmFltRatio;
                    m_grainData[g].modXmFltRatio = modXmFltRatio;
                    m_grainData[g].oscWarp = oscWarp;
                    m_grainData[g].modWarp = modWarp;
                    m_grainData[g].sampleCount = subSampleOffset;
                    m_dualOscs[g].reset();
                }
 
                // Process grain if voice is active
                if (voices.gates[g]) {

                    // Calculate slopes
                    float oscSlope = m_grainData[g].oscFreq / m_sampleRate;
                    float modSlope = m_grainData[g].modFreq / m_sampleRate;
                    float envSlope = voices.slopes[g];

                    // Calculate phases
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * modSlope));
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * oscSlope));
                    float envPhase = voices.phases[g];

                    // Cross-modulation index scaling: normalize by modulator, scale by carrier
                    float oscXmScaleRatio = 0.0f;
                    if (sc_abs(modSlope) > Utils::SAFE_DENOM_EPSILON) {
                        oscXmScaleRatio = sc_abs(oscSlope / modSlope);
                    }
                    float modXmScaleRatio = 0.0f;
                    if (sc_abs(oscSlope) > Utils::SAFE_DENOM_EPSILON) {
                        modXmScaleRatio = sc_abs(modSlope / oscSlope);
                    }

                    // Scale cross-modulation indices
                    float oscXmIndexScaled = m_grainData[g].oscXmIndex * oscXmScaleRatio;
                    float modXmIndexScaled = m_grainData[g].modXmIndex * modXmScaleRatio;

                    // Phase Increment Distortion ratios
                    float oscPhsIncRatio = 0.0f;
                    float modPhsIncRatio = 0.0f;
                    if (sc_abs(envSlope) > Utils::SAFE_DENOM_EPSILON) {
                        oscPhsIncRatio = sc_abs(oscSlope / envSlope);
                        modPhsIncRatio = sc_abs(modSlope / envSlope);
                    }

                    // Apply Phase Increment Distortion
                    float oscPhsIncDist = Easing::Interp::jCurve(envPhase, m_grainData[g].oscWarp, Easing::Cores::cubic) - envPhase;
                    float oscPhaseDistorted = sc_frac(oscPhase + (oscPhsIncDist * oscPhsIncRatio));
 
                    float modPhsIncDist = Easing::Interp::jCurve(envPhase, m_grainData[g].modWarp, Easing::Cores::cubic) - envPhase;
                    float modPhaseDistorted = sc_frac(modPhase + (modPhsIncDist * modPhsIncRatio));

                    // Process wavetable oscillator with cross modulation
                    auto result = m_dualOscs[g].process(
                        oscPhaseDistorted, modPhaseDistorted,
                        oscSlope, modSlope,
                        oscXmIndexScaled, modXmIndexScaled,
                        m_grainData[g].oscXmFltRatio, m_grainData[g].modXmFltRatio,
                        oscCyclePos, modCyclePos,
                        oscTable, modTable,
                        m_sampleRate
                    );
 
                    // Process gaussian window
                    float grainWindow = WindowFunctions::gaussianWindow(
                        envPhase, envSkew, envIndex
                    );
 
                    // Accumulate grain output
                    sum += result.oscA * grainWindow;
 
                    // Increment sample count
                    m_grainData[g].sampleCount++;
                }
            }
 
            // 3. DC block output
            output[i] = m_dcBlocker.process(sum, m_sampleRate);
        }
    } else {
 
        for (int i = 0; i < nSamples; ++i) {
 
            // Trigger input (audio-rate or control-rate)
            bool trigger = isTriggerAudioRate ?
                m_trigger.process(in(Trigger)[i]) :
                m_trigger.process(in0(Trigger));
 
            // Get current parameter values (no interpolation - latched per trigger)
            float triggerFreq = isTriggerFreqAudioRate ?
                sc_clip(in(TriggerFreq)[i], 0.0f, m_sampleRate * 0.49f) :
                sc_clip(in0(TriggerFreq), 0.0f, m_sampleRate * 0.49f);
 
            float subSampleOffset = isSubSampleOffsetAudioRate ?
                in(SubSampleOffset)[i] :
                in0(SubSampleOffset);
 
            float oscFreq = isOscFreqAudioRate ?
                sc_clip(in(OscFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(OscFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float modFreq = isModFreqAudioRate ?
                sc_clip(in(ModFreq)[i], m_sampleRate * -0.49f, m_sampleRate * 0.49f) :
                sc_clip(in0(ModFreq), m_sampleRate * -0.49f, m_sampleRate * 0.49f);
 
            float oscXmIndex = isOscXmIndexAudioRate ?
                sc_clip(in(OscXmIndex)[i], 0.0f, 10.0f) :
                sc_clip(in0(OscXmIndex), 0.0f, 10.0f);
 
            float modXmIndex = isModXmIndexAudioRate ?
                sc_clip(in(ModXmIndex)[i], 0.0f, 10.0f) :
                sc_clip(in0(ModXmIndex), 0.0f, 10.0f);
 
            float oscXmFltRatio = isOscXmFltRatioAudioRate ?
                sc_clip(in(OscXmFltRatio)[i], 1.0f, 10.0f) :
                sc_clip(in0(OscXmFltRatio), 1.0f, 10.0f);
 
            float modXmFltRatio = isModXmFltRatioAudioRate ?
                sc_clip(in(ModXmFltRatio)[i], 1.0f, 10.0f) :
                sc_clip(in0(ModXmFltRatio), 1.0f, 10.0f);
 
            float oscWarp = isOscWarpAudioRate ?
                sc_clip(in(OscWarp)[i], 0.0f, 1.0f) :
                sc_clip(in0(OscWarp), 0.0f, 1.0f);
 
            float modWarp = isModWarpAudioRate ?
                sc_clip(in(ModWarp)[i], 0.0f, 1.0f) :
                sc_clip(in0(ModWarp), 0.0f, 1.0f);
 
            // Get current parameter values (audio-rate or interpolated control-rate)
            float oscCyclePos = isOscCyclePosAudioRate ?
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) :
                m_oscCyclePosInterp.process();
 
            float modCyclePos = isModCyclePosAudioRate ?
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) :
                m_modCyclePosInterp.process();
 
            float envSkew = isEnvSkewAudioRate ?
                sc_clip(in(EnvSkew)[i], 0.0f, 1.0f) :
                m_envSkewInterp.process();
 
            float envIndex = isEnvIndexAudioRate ?
                sc_clip(in(EnvIndex)[i], 0.0f, 10.0f) :
                m_envIndexInterp.process();
 
            // 1. Process voice allocator
            auto voices = m_allocator.process(
                NUM_VOICES,
                trigger,
                triggerFreq,
                subSampleOffset,
                m_sampleRate
            );
 
            // 2. Store parameter values for oversampling
            m_osOscCyclePosInterp.update(oscCyclePos);
            m_osModCyclePosInterp.update(modCyclePos);
            m_osEnvSkewInterp.update(envSkew);
            m_osEnvIndexInterp.update(envIndex);
 
            // 3. Clear OS buffer
            memset(m_outputOSBuffer, 0, m_osRatio * sizeof(float));
 
            // 4. Process all grains
            for (int g = 0; g < NUM_VOICES; ++g) {

                // Trigger new grain if needed and store graindata
                if (voices.triggers[g]) {
                    m_grainData[g].oscFreq = oscFreq;
                    m_grainData[g].modFreq = modFreq;
                    m_grainData[g].oscXmIndex = oscXmIndex;
                    m_grainData[g].modXmIndex = modXmIndex;
                    m_grainData[g].oscXmFltRatio = oscXmFltRatio;
                    m_grainData[g].modXmFltRatio = modXmFltRatio;
                    m_grainData[g].oscWarp = oscWarp;
                    m_grainData[g].modWarp = modWarp;
                    m_grainData[g].sampleCount = subSampleOffset;
                    m_dualOscs[g].reset();
                }

                // Process grain if voice is active
                if (voices.gates[g]) {

                    // Calculate slopes
                    float oscSlope = m_grainData[g].oscFreq / m_sampleRate;
                    float modSlope = m_grainData[g].modFreq / m_sampleRate;
                    float envSlope = voices.slopes[g];

                    // Calculate phases
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * modSlope));
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * oscSlope));
                    float envPhase = voices.phases[g];

                    // Cross-modulation index scaling: normalize by modulator, scale by carrier
                    float oscXmScaleRatio = 0.0f;
                    if (sc_abs(modSlope) > Utils::SAFE_DENOM_EPSILON) {
                        oscXmScaleRatio = sc_abs(oscSlope / modSlope);
                    }
                    float modXmScaleRatio = 0.0f;
                    if (sc_abs(oscSlope) > Utils::SAFE_DENOM_EPSILON) {
                        modXmScaleRatio = sc_abs(modSlope / oscSlope);
                    }

                    // Scale cross-modulation indices
                    float oscXmIndexScaled = m_grainData[g].oscXmIndex * oscXmScaleRatio;
                    float modXmIndexScaled = m_grainData[g].modXmIndex * modXmScaleRatio;

                    // Phase Increment Distortion ratios
                    float oscPhsIncRatio = 0.0f;
                    float modPhsIncRatio = 0.0f;
                    if (sc_abs(envSlope) > Utils::SAFE_DENOM_EPSILON) {
                        oscPhsIncRatio = sc_abs(oscSlope / envSlope);
                        modPhsIncRatio = sc_abs(modSlope / envSlope);
                    }

                    // Prepare phases and slopes for oversampling
                    float osModSlope = modSlope / static_cast<float>(m_osRatio);
                    float osOscSlope = oscSlope / static_cast<float>(m_osRatio);
                    float osEnvSlope = envSlope / static_cast<float>(m_osRatio);
                    float osModPhase = modPhase - modSlope;
                    float osOscPhase = oscPhase - oscSlope;
                    float osEnvPhase = envPhase - envSlope;
 
                    for (int k = 0; k < m_osRatio; k++) {
 
                        // Calculate fractional position for interpolation
                        float frac = static_cast<float>(k + 1) / static_cast<float>(m_osRatio);
                        
                        // Interpolate parameter values
                        float osOscCyclePos = m_osOscCyclePosInterp.process(frac);
                        float osModCyclePos = m_osModCyclePosInterp.process(frac);
                        float osEnvSkew = m_osEnvSkewInterp.process(frac);
                        float osEnvIndex = m_osEnvIndexInterp.process(frac);

                        // Increment phases
                        osModPhase += osModSlope;
                        osOscPhase += osOscSlope;
                        osEnvPhase += osEnvSlope;
 
                        // Apply Phase Increment Distortion
                        float oscPhsIncDist = Easing::Interp::jCurve(osEnvPhase, m_grainData[g].oscWarp, Easing::Cores::cubic) - osEnvPhase;
                        float osOscPhaseDistorted = sc_frac(osOscPhase + (oscPhsIncDist * oscPhsIncRatio));
 
                        float modPhsIncDist = Easing::Interp::jCurve(osEnvPhase, m_grainData[g].modWarp, Easing::Cores::cubic) - osEnvPhase;
                        float osModPhaseDistorted = sc_frac(osModPhase + (modPhsIncDist * modPhsIncRatio));
 
                        // Process wavetable oscillator with cross modulation
                        auto result = m_dualOscs[g].process(
                            osOscPhaseDistorted, osModPhaseDistorted,
                            osOscSlope, osModSlope,
                            oscXmIndexScaled, modXmIndexScaled,
                            m_grainData[g].oscXmFltRatio, m_grainData[g].modXmFltRatio,
                            osOscCyclePos, osModCyclePos,
                            oscTable, modTable,
                            m_sampleRate
                        );
 
                        // Process gaussian window
                        float grainWindow = WindowFunctions::gaussianWindow(
                            osEnvPhase, osEnvSkew, osEnvIndex
                        );
 
                        // Accumulate grain output
                        m_outputOSBuffer[k] += result.oscA * grainWindow;
                    }
 
                    // Increment sample count
                    m_grainData[g].sampleCount++;
                }
            }
 
            // 5. Downsample and DC block output
            float downsampled = m_outputOversampling.downsample(m_outputOSBuffer, m_osRatio);
            output[i] = m_dcBlocker.process(downsampled, m_sampleRate);
        }
    }
}

void Oscs_setup()
{
    registerUnit<SingleOscOS>(ft, "SingleOscOS", false);
    registerUnit<DualOscOS>(ft, "DualOscOS", false);
    registerUnit<PulsarOS>(ft, "PulsarOS", false);
    registerUnit<DualPulsarOS>(ft, "DualPulsarOS", false);
}