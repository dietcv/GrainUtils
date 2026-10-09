#include "Oscs.hpp"
#include "SC_PlugIn.hpp"

extern InterfaceTable* ft;

// ===== WAVETABLE OSCILLATOR WITH CROSS-PHASE MODULATION =====

OscXPM::OscXPM() :
    m_sampleRate(static_cast<float>(sampleRate())), 
    m_oversampleIndex(sc_clip(static_cast<int>(in0(Oversample)), 0, 4)),
    m_osRatio(1 << m_oversampleIndex)
{
    // Check which inputs are audio-rate
    isOscPmIndexAudioRate = isAudioRateIn(OscPmIndex);
    isModPmIndexAudioRate = isAudioRateIn(ModPmIndex);
    isOscPmDampingAudioRate = isAudioRateIn(OscPmDamping);
    isModPmDampingAudioRate = isAudioRateIn(ModPmDamping);
    isOscCyclePosAudioRate = isAudioRateIn(OscCyclePos);
    isModCyclePosAudioRate = isAudioRateIn(ModCyclePos);

    // Allocate oversampling buffer
    if (m_oversampleIndex > 0) {
        BufferUtils::allocBuffer(this, mWorld, m_osRatio, m_outputOSBuffer);
    }
    
    // Set calc function & compute initial sample
    set_calc_function<OscXPM, &OscXPM::next>();
}

OscXPM::~OscXPM() {
    RTFree(mWorld, m_outputOSBuffer);
}

void OscXPM::next(int nSamples) {

    // Audio-rate inputs
    const float* oscPhaseIn = in(OscPhase);
    const float* modPhaseIn = in(ModPhase);
    
    // Control-rate parameters with smooth interpolation
    m_oscPmIndexInterp.update(sc_clip(in0(OscPmIndex), 0.0f, 10.0f), nSamples);
    m_modPmIndexInterp.update(sc_clip(in0(ModPmIndex), 0.0f, 10.0f), nSamples);
    m_oscPmDampingInterp.update(sc_clip(in0(OscPmDamping), 0.0f, 1.0f), nSamples);
    m_modPmDampingInterp.update(sc_clip(in0(ModPmDamping), 0.0f, 1.0f), nSamples);
    m_oscCyclePosInterp.update(sc_clip(in0(OscCyclePos), 0.0f, 1.0f), nSamples);
    m_modCyclePosInterp.update(sc_clip(in0(ModCyclePos), 0.0f, 1.0f), nSamples);
    
    // Control-rate parameters (settings, no interpolation)
    float oscBufNum = in0(OscBufNum);
    float modBufNum = in0(ModBufNum);
    int oscNumCycles = sc_max(static_cast<int>(in0(OscNumCycles)), 1);
    int modNumCycles = sc_max(static_cast<int>(in0(ModNumCycles)), 1);

    // Output pointer
    float* output = out(Out);

    // Get wavetable data
    auto oscTable = m_oscWavetable.update(this, mWorld, nSamples, oscBufNum, oscNumCycles, "OscXPM osc");
    auto modTable = m_modWavetable.update(this, mWorld, nSamples, modBufNum, modNumCycles, "OscXPM mod");
    if (!oscTable.data || !modTable.data) return;

    if (m_oversampleIndex == 0) {

        for (int i = 0; i < nSamples; ++i) {

            // Wrap phases between 0 and 1
            float oscPhase = sc_frac(oscPhaseIn[i]);
            float modPhase = sc_frac(modPhaseIn[i]);

            // Get current parameter values (audio-rate or interpolated control-rate)
            float oscPmIndex = isOscPmIndexAudioRate ? 
                sc_clip(in(OscPmIndex)[i], 0.0f, 10.0f) : 
                m_oscPmIndexInterp.process();

            float modPmIndex = isModPmIndexAudioRate ? 
                sc_clip(in(ModPmIndex)[i], 0.0f, 10.0f) : 
                m_modPmIndexInterp.process();

            float oscPmDamping = isOscPmDampingAudioRate ? 
                sc_clip(in(OscPmDamping)[i], 0.0f, 1.0f) : 
                m_oscPmDampingInterp.process();

            float modPmDamping = isModPmDampingAudioRate ? 
                sc_clip(in(ModPmDamping)[i], 0.0f, 1.0f) : 
                m_modPmDampingInterp.process();

            float oscCyclePos = isOscCyclePosAudioRate ?
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) : 
                m_oscCyclePosInterp.process();

            float modCyclePos = isModCyclePosAudioRate ? 
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) : 
                m_modCyclePosInterp.process();

            // Derive slopes from incoming ramps
            float oscSlope = static_cast<float>(m_oscRampToSlope.process(static_cast<double>(oscPhase)));
            float modSlope = static_cast<float>(m_modRampToSlope.process(static_cast<double>(modPhase)));

            // Process wavetable oscillator with cross-phase modulation
            output[i] = m_xpmOsc.process(
                oscPhase, modPhase,
                oscSlope, modSlope,
                oscPmIndex, modPmIndex,
                oscPmDamping, modPmDamping,
                oscCyclePos, modCyclePos,
                oscTable, modTable,
                m_sampleRate
            );
        }
    } else {

        for (int i = 0; i < nSamples; ++i) {

            // Wrap phases between 0 and 1
            float oscPhase = sc_frac(oscPhaseIn[i]);
            float modPhase = sc_frac(modPhaseIn[i]);

            // Get current parameter values (audio-rate or interpolated control-rate)
            float oscPmIndex = isOscPmIndexAudioRate ? 
                sc_clip(in(OscPmIndex)[i], 0.0f, 10.0f) : 
                m_oscPmIndexInterp.process();

            float modPmIndex = isModPmIndexAudioRate ? 
                sc_clip(in(ModPmIndex)[i], 0.0f, 10.0f) : 
                m_modPmIndexInterp.process();

            float oscPmDamping = isOscPmDampingAudioRate ? 
                sc_clip(in(OscPmDamping)[i], 0.0f, 1.0f) : 
                m_oscPmDampingInterp.process();

            float modPmDamping = isModPmDampingAudioRate ? 
                sc_clip(in(ModPmDamping)[i], 0.0f, 1.0f) : 
                m_modPmDampingInterp.process();

            float oscCyclePos = isOscCyclePosAudioRate ? 
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) : 
                m_oscCyclePosInterp.process();

            float modCyclePos = isModCyclePosAudioRate ? 
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) : 
                m_modCyclePosInterp.process();

            // Derive slopes from incoming ramps
            float oscSlope = static_cast<float>(m_oscRampToSlope.process(static_cast<double>(oscPhase)));
            float modSlope = static_cast<float>(m_modRampToSlope.process(static_cast<double>(modPhase)));

            // Prepare phases and slopes for oversampling
            float osOscSlope = oscSlope / static_cast<float>(m_osRatio);
            float osModSlope = modSlope / static_cast<float>(m_osRatio);
            float osOscPhase = oscPhase - oscSlope;
            float osModPhase = modPhase - modSlope;

            // Store parameter values for oversampling
            m_osOscPmIndexInterp.update(oscPmIndex);
            m_osModPmIndexInterp.update(modPmIndex);
            m_osOscPmDampingInterp.update(oscPmDamping);
            m_osModPmDampingInterp.update(modPmDamping);
            m_osOscCyclePosInterp.update(oscCyclePos);
            m_osModCyclePosInterp.update(modCyclePos);
            
            for (int k = 0; k < m_osRatio; k++) {
                
                // Calculate fractional position for interpolation
                float frac = static_cast<float>(k + 1) / static_cast<float>(m_osRatio);
                
                // Interpolate parameter values
                float osOscPmIndex = m_osOscPmIndexInterp.process(frac);
                float osModPmIndex = m_osModPmIndexInterp.process(frac);
                float osOscPmDamping = m_osOscPmDampingInterp.process(frac);
                float osModPmDamping = m_osModPmDampingInterp.process(frac);
                float osOscCyclePos = m_osOscCyclePosInterp.process(frac);
                float osModCyclePos = m_osModCyclePosInterp.process(frac);

                // Increment phases
                osOscPhase += osOscSlope;
                osModPhase += osModSlope;
                
                // Process wavetable oscillator with cross-phase modulation
                m_outputOSBuffer[k] = m_xpmOsc.process(
                    sc_frac(osOscPhase), sc_frac(osModPhase),
                    osOscSlope, osModSlope,
                    osOscPmIndex, osModPmIndex,
                    osOscPmDamping, osModPmDamping,
                    osOscCyclePos, osModCyclePos,
                    oscTable, modTable,
                    m_sampleRate
                );
            }
            
            // Downsample output
            output[i] = m_outputOversampling.downsample(m_outputOSBuffer, m_osRatio);
        }
    }

}

// ===== PULSAR OSCILLATOR WITH CROSS-PHASE MODULATION =====
 
PulsarXPM::PulsarXPM() :
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
    isOscPmIndexAudioRate = isAudioRateIn(OscPmIndex);
    isModPmIndexAudioRate = isAudioRateIn(ModPmIndex);
    isOscPmDampingAudioRate = isAudioRateIn(OscPmDamping);
    isModPmDampingAudioRate = isAudioRateIn(ModPmDamping);
    isOscCyclePosAudioRate = isAudioRateIn(OscCyclePos);
    isModCyclePosAudioRate = isAudioRateIn(ModCyclePos);
    isEnvCyclePosAudioRate = isAudioRateIn(EnvCyclePos);
 
    // Allocate oversampling buffer
    if (m_oversampleIndex > 0) {
        BufferUtils::allocBuffer(this, mWorld, m_osRatio, m_outputOSBuffer);
    }
 
    // Set calc function & compute initial sample
    set_calc_function<PulsarXPM, &PulsarXPM::next>();
 
    // Reset state after priming
    m_allocator.reset();
    m_trigger.reset();
}
 
PulsarXPM::~PulsarXPM() {
    RTFree(mWorld, m_outputOSBuffer);
}
 
void PulsarXPM::next(int nSamples) {
 
    // Control-rate parameters with smooth interpolation
    m_oscPmIndexInterp.update(sc_clip(in0(OscPmIndex), 0.0f, 10.0f), nSamples);
    m_modPmIndexInterp.update(sc_clip(in0(ModPmIndex), 0.0f, 10.0f), nSamples);
    m_oscPmDampingInterp.update(sc_clip(in0(OscPmDamping), 0.0f, 1.0f), nSamples);
    m_modPmDampingInterp.update(sc_clip(in0(ModPmDamping), 0.0f, 1.0f), nSamples);
    m_oscCyclePosInterp.update(sc_clip(in0(OscCyclePos), 0.0f, 1.0f), nSamples);
    m_modCyclePosInterp.update(sc_clip(in0(ModCyclePos), 0.0f, 1.0f), nSamples);
    m_envCyclePosInterp.update(sc_clip(in0(EnvCyclePos), 0.0f, 1.0f), nSamples);
 
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
    auto oscTable = m_oscWavetable.update(this, mWorld, nSamples, oscBufNum, oscNumCycles, "PulsarXPM osc");
    auto modTable = m_modWavetable.update(this, mWorld, nSamples, modBufNum, modNumCycles, "PulsarXPM mod");
    auto envTable = m_envWavetable.update(this, mWorld, nSamples, envBufNum, envNumCycles, "PulsarXPM env");
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
 
            // Get current parameter values (audio-rate or interpolated control-rate)
            float oscPmIndex = isOscPmIndexAudioRate ?
                sc_clip(in(OscPmIndex)[i], 0.0f, 10.0f) :
                m_oscPmIndexInterp.process();
 
            float modPmIndex = isModPmIndexAudioRate ?
                sc_clip(in(ModPmIndex)[i], 0.0f, 10.0f) :
                m_modPmIndexInterp.process();
 
            float oscPmDamping = isOscPmDampingAudioRate ?
                sc_clip(in(OscPmDamping)[i], 0.0f, 1.0f) :
                m_oscPmDampingInterp.process();
 
            float modPmDamping = isModPmDampingAudioRate ?
                sc_clip(in(ModPmDamping)[i], 0.0f, 1.0f) :
                m_modPmDampingInterp.process();
 
            float oscCyclePos = isOscCyclePosAudioRate ?
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) :
                m_oscCyclePosInterp.process();
 
            float modCyclePos = isModCyclePosAudioRate ?
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) :
                m_modCyclePosInterp.process();
 
            float envCyclePos = isEnvCyclePosAudioRate ?
                sc_clip(in(EnvCyclePos)[i], 0.0f, 1.0f) :
                m_envCyclePosInterp.process();
 
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
                    m_grainData[g].sampleCount = subSampleOffset;
                    m_xpmOscs[g].reset();
                }
 
                // Process grain if voice is active
                if (voices.gates[g]) {

                    // Calculate slopes
                    float oscSlope = m_grainData[g].oscFreq / m_sampleRate;
                    float modSlope = m_grainData[g].modFreq / m_sampleRate;
                    float envSlope = voices.slopes[g];

                    // Calculate phases
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * oscSlope));
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * modSlope));
                    float envPhase = static_cast<float>(voices.phases[g]);

                    // Calculate frequency ratios for cross-phase modulation indices
                    float oscModRatio = 0.0f;
                    if (sc_abs(modSlope) > Utils::SAFE_DENOM_EPSILON) {
                        oscModRatio = sc_abs(oscSlope / modSlope);
                    }
                    float modOscRatio = 0.0f;
                    if (sc_abs(oscSlope) > Utils::SAFE_DENOM_EPSILON) {
                        modOscRatio = sc_abs(modSlope / oscSlope);
                    }

                    // Scale cross-phase modulation indices
                    float oscPmIndexScaled = oscPmIndex * oscModRatio;
                    float modPmIndexScaled = modPmIndex * modOscRatio;

                    // Process wavetable oscillator with cross-phase modulation
                    float grainOsc = m_xpmOscs[g].process(
                        oscPhase, modPhase,
                        oscSlope, modSlope,
                        oscPmIndexScaled, modPmIndexScaled,
                        oscPmDamping, modPmDamping,
                        oscCyclePos, modCyclePos,
                        oscTable, modTable,
                        m_sampleRate
                    );
 
                    // Process wavetable oscillator for grain envelope
                    float grainWindow = OscUtils::wavetableOsc(
                        envPhase, envSlope, 
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
 
            // Get current parameter values (audio-rate or interpolated control-rate)
            float oscPmIndex = isOscPmIndexAudioRate ?
                sc_clip(in(OscPmIndex)[i], 0.0f, 10.0f) :
                m_oscPmIndexInterp.process();
 
            float modPmIndex = isModPmIndexAudioRate ?
                sc_clip(in(ModPmIndex)[i], 0.0f, 10.0f) :
                m_modPmIndexInterp.process();
 
            float oscPmDamping = isOscPmDampingAudioRate ?
                sc_clip(in(OscPmDamping)[i], 0.0f, 1.0f) :
                m_oscPmDampingInterp.process();
 
            float modPmDamping = isModPmDampingAudioRate ?
                sc_clip(in(ModPmDamping)[i], 0.0f, 1.0f) :
                m_modPmDampingInterp.process();
 
            float oscCyclePos = isOscCyclePosAudioRate ?
                sc_clip(in(OscCyclePos)[i], 0.0f, 1.0f) :
                m_oscCyclePosInterp.process();
 
            float modCyclePos = isModCyclePosAudioRate ?
                sc_clip(in(ModCyclePos)[i], 0.0f, 1.0f) :
                m_modCyclePosInterp.process();
 
            float envCyclePos = isEnvCyclePosAudioRate ?
                sc_clip(in(EnvCyclePos)[i], 0.0f, 1.0f) :
                m_envCyclePosInterp.process();
 
            // 1. Process voice allocator
            auto voices = m_allocator.process(
                NUM_VOICES,
                trigger,
                triggerFreq,
                subSampleOffset,
                m_sampleRate
            );
 
            // 2. Store parameter values for oversampling
            m_osOscPmIndexInterp.update(oscPmIndex);
            m_osModPmIndexInterp.update(modPmIndex);
            m_osOscPmDampingInterp.update(oscPmDamping);
            m_osModPmDampingInterp.update(modPmDamping);
            m_osOscCyclePosInterp.update(oscCyclePos);
            m_osModCyclePosInterp.update(modCyclePos);
            m_osEnvCyclePosInterp.update(envCyclePos);
 
            // 3. Clear OS buffer
            memset(m_outputOSBuffer, 0, m_osRatio * sizeof(float));
 
            // 4. Process all grains
            for (int g = 0; g < NUM_VOICES; ++g) {

                // Trigger new grain if needed and store graindata
                if (voices.triggers[g]) {
                    m_grainData[g].oscFreq = oscFreq;
                    m_grainData[g].modFreq = modFreq;
                    m_grainData[g].sampleCount = subSampleOffset;
                    m_xpmOscs[g].reset();
                }

                // Process grain if voice is active
                if (voices.gates[g]) {

                    // Calculate slopes
                    float oscSlope = m_grainData[g].oscFreq / m_sampleRate;
                    float modSlope = m_grainData[g].modFreq / m_sampleRate;
                    float envSlope = voices.slopes[g];

                    // Calculate phases
                    float oscPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * oscSlope));
                    float modPhase = static_cast<float>(sc_frac(m_grainData[g].sampleCount * modSlope));
                    float envPhase = static_cast<float>(voices.phases[g]);

                    // Calculate frequency ratios for cross-phase modulation indices
                    float oscModRatio = 0.0f;
                    if (sc_abs(modSlope) > Utils::SAFE_DENOM_EPSILON) {
                        oscModRatio = sc_abs(oscSlope / modSlope);
                    }
                    float modOscRatio = 0.0f;
                    if (sc_abs(oscSlope) > Utils::SAFE_DENOM_EPSILON) {
                        modOscRatio = sc_abs(modSlope / oscSlope);
                    }

                    // Prepare phases and slopes for oversampling
                    float osOscSlope = oscSlope / static_cast<float>(m_osRatio);
                    float osModSlope = modSlope / static_cast<float>(m_osRatio);
                    float osEnvSlope = envSlope / static_cast<float>(m_osRatio);
                    float osOscPhase = oscPhase - oscSlope;
                    float osModPhase = modPhase - modSlope;
                    float osEnvPhase = envPhase - envSlope;
 
                    for (int k = 0; k < m_osRatio; k++) {
 
                        // Calculate fractional position for interpolation
                        float frac = static_cast<float>(k + 1) / static_cast<float>(m_osRatio);
                        
                        // Interpolate parameter values
                        float osOscPmIndex = m_osOscPmIndexInterp.process(frac);
                        float osModPmIndex = m_osModPmIndexInterp.process(frac);
                        float osOscPmDamping = m_osOscPmDampingInterp.process(frac);
                        float osModPmDamping = m_osModPmDampingInterp.process(frac);
                        float osOscCyclePos = m_osOscCyclePosInterp.process(frac);
                        float osModCyclePos = m_osModCyclePosInterp.process(frac);
                        float osEnvCyclePos = m_osEnvCyclePosInterp.process(frac);

                        // Scale cross-phase modulation indices
                        float osOscPmIndexScaled = osOscPmIndex * oscModRatio;
                        float osModPmIndexScaled = osModPmIndex * modOscRatio;

                        // Increment grain phases
                        osOscPhase += osOscSlope;
                        osModPhase += osModSlope;
                        osEnvPhase += osEnvSlope;
 
                        // Process wavetable oscillator with cross-phase modulation
                        float grainOsc = m_xpmOscs[g].process(
                            sc_frac(osOscPhase), sc_frac(osModPhase),
                            osOscSlope, osModSlope,
                            osOscPmIndexScaled, osModPmIndexScaled,
                            osOscPmDamping, osModPmDamping,
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

void Oscs_setup()
{
    registerUnit<OscXPM>(ft, "OscXPM", false);
    registerUnit<PulsarXPM>(ft, "PulsarXPM", false);
}