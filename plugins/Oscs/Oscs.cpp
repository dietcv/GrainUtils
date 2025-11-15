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
    cyclePosPast = in0(CyclePos);
    
    // Check which inputs are audio-rate
    isCyclePosAudioRate = isAudioRateIn(CyclePos);
    
    m_oversampling.reset(m_sampleRate);
    
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
    const int oversampleIndex = sc_clip(static_cast<int>(in0(Oversample)), 0, 4);
    
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
    
    if (oversampleIndex == 0) {
        // No oversampling - direct processing
        for (int i = 0; i < nSamples; ++i) {
            
            // Wrap phase between 0 and 1
            float phase = sc_frac(phaseIn[i]);
            
            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePosVal = isCyclePosAudioRate ? 
                sc_clip(in(CyclePos)[i], 0.0f, 1.0f) : slopedCyclePos.consume();
            
            const float slope = m_rampToSlope.process(phase);
            
            output[i] = OscUtils::wavetableInterpolate(
                phase, 
                bufData, tableSize, 
                cycleSamples, numCyclesInt, cyclePosVal, 
                slope, m_sincTable);
        }
    } else {
        // Set oversampling factor
        m_oversampling.setOversamplingIndex(oversampleIndex);

        // Prepare oversampling buffer
        const int osRatio = m_oversampling.getOversamplingRatio();
        float* osBuffer = m_oversampling.getOSBuffer();

        for (int i = 0; i < nSamples; ++i) {
            
            // Wrap phase between 0 and 1
            float phase = sc_frac(phaseIn[i]);
            
            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePosVal = isCyclePosAudioRate ? 
                sc_clip(in(CyclePos)[i], 0.0f, 1.0f) : slopedCyclePos.consume();
            
            const float slope = m_rampToSlope.process(phase);

            // Calculate phase difference per oversampled sample
            const float phaseDiff = slope / static_cast<float>(osRatio);
            
            m_oversampling.upsample(0.0f);
            float osPhase = phase;
            
            for (int k = 0; k < osRatio; k++) {
                // Increment phase
                osPhase += phaseDiff;
                
                // Process with wrapped phase
                osBuffer[k] = OscUtils::wavetableInterpolate(
                    sc_frac(osPhase), 
                    bufData, tableSize, 
                    cycleSamples, numCyclesInt, cyclePosVal, 
                    slope, m_sincTable);
            }
            
            output[i] = m_oversampling.downsample();
        }
    }
    
    // Update parameter cache
    cyclePosPast = isCyclePosAudioRate ? in(CyclePos)[nSamples - 1] : slopedCyclePos.value;
}

// ===== DUAL WAVETABLE OSCILLATOR =====

DualOscOS::DualOscOS() : m_sampleRate(static_cast<float>(sampleRate()))
{
    // Initialize parameter cache
    cyclePosAPast = in0(CyclePosA);
    cyclePosBPast = in0(CyclePosB);
    pmIndexAPast = in0(PMIndexA);
    pmIndexBPast = in0(PMIndexB);
    pmFilterRatioAPast = in0(PMFilterRatioA);
    pmFilterRatioBPast = in0(PMFilterRatioB);
    
    // Check which inputs are audio-rate
    isCyclePosAAudioRate = isAudioRateIn(CyclePosA);
    isCyclePosBAAudioRate = isAudioRateIn(CyclePosB);
    isPMIndexAAudioRate = isAudioRateIn(PMIndexA);
    isPMIndexBAudioRate = isAudioRateIn(PMIndexB);
    isPMFilterRatioAAudioRate = isAudioRateIn(PMFilterRatioA);
    isPMFilterRatioBAudioRate = isAudioRateIn(PMFilterRatioB);
    
    m_oversamplingA.reset(m_sampleRate);
    m_oversamplingB.reset(m_sampleRate);
    
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
    auto slopedPMFilterRatioA = makeSlope(sc_clip(in0(PMFilterRatioA), 0.0f, 10.0f), pmFilterRatioAPast);
    auto slopedPMFilterRatioB = makeSlope(sc_clip(in0(PMFilterRatioB), 0.0f, 10.0f), pmFilterRatioBPast);
    
    // Control-rate parameters (settings)
    const float numCyclesA = sc_max(in0(NumCyclesA), 1.0f);
    const float numCyclesB = sc_max(in0(NumCyclesB), 1.0f);
    const float bufNumA = in0(BufNumA);
    const float bufNumB = in0(BufNumB);
    const int oversampleIndex = sc_clip(static_cast<int>(in0(Oversample)), 0, 4);
    
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
    
    if (oversampleIndex == 0) {
        // No oversampling - direct processing
        for (int i = 0; i < nSamples; ++i) {

            // Wrap phases between 0 and 1
            float phaseA = sc_frac(phaseAIn[i]);
            float phaseB = sc_frac(phaseBIn[i]);

            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePosAVal = isCyclePosAAudioRate ? 
                sc_clip(in(CyclePosA)[i], 0.0f, 1.0f) : slopedCyclePosA.consume();

            float cyclePosBVal = isCyclePosBAAudioRate ? 
                sc_clip(in(CyclePosB)[i], 0.0f, 1.0f) : slopedCyclePosB.consume();

            float pmIndexAVal = isPMIndexAAudioRate ? 
                sc_clip(in(PMIndexA)[i], 0.0f, 10.0f) : slopedPMIndexA.consume();

            float pmIndexBVal = isPMIndexBAudioRate ? 
                sc_clip(in(PMIndexB)[i], 0.0f, 10.0f) : slopedPMIndexB.consume();

            float pmFilterRatioAVal = isPMFilterRatioAAudioRate ? 
                sc_clip(in(PMFilterRatioA)[i], 0.0f, 10.0f) : slopedPMFilterRatioA.consume();

            float pmFilterRatioBVal = isPMFilterRatioBAudioRate ? 
                sc_clip(in(PMFilterRatioB)[i], 0.0f, 10.0f) : slopedPMFilterRatioB.consume();
            
            const float slopeA = m_rampToSlopeA.process(phaseA);
            const float slopeB = m_rampToSlopeB.process(phaseB);
            
            auto result = m_dualOsc.process(
                phaseA, phaseB, 
                cyclePosAVal, cyclePosBVal,
                slopeA, slopeB, pmIndexAVal, pmIndexBVal,
                pmFilterRatioAVal, pmFilterRatioBVal,
                bufDataA, tableSizeA, cycleSamplesA, numCyclesIntA,
                bufDataB, tableSizeB, cycleSamplesB, numCyclesIntB,
                m_sincTable
            );
            
            outputA[i] = result.oscA;
            outputB[i] = result.oscB;
        }
    } else {
        // Set oversampling factor for both channels
        m_oversamplingA.setOversamplingIndex(oversampleIndex);
        m_oversamplingB.setOversamplingIndex(oversampleIndex);
        
        // Prepare oversampling buffers
        const int osRatio = m_oversamplingA.getOversamplingRatio();
        float* osBufferA = m_oversamplingA.getOSBuffer();
        float* osBufferB = m_oversamplingB.getOSBuffer();
        
        for (int i = 0; i < nSamples; ++i) {

            // Wrap phases between 0 and 1
            float phaseA = sc_frac(phaseAIn[i]);
            float phaseB = sc_frac(phaseBIn[i]);

            // Get current parameter values (audio-rate or interpolated control-rate)
            float cyclePosAVal = isCyclePosAAudioRate ? 
                sc_clip(in(CyclePosA)[i], 0.0f, 1.0f) : slopedCyclePosA.consume();

            float cyclePosBVal = isCyclePosBAAudioRate ? 
                sc_clip(in(CyclePosB)[i], 0.0f, 1.0f) : slopedCyclePosB.consume();

            float pmIndexAVal = isPMIndexAAudioRate ? 
                sc_clip(in(PMIndexA)[i], 0.0f, 10.0f) : slopedPMIndexA.consume();

            float pmIndexBVal = isPMIndexBAudioRate ? 
                sc_clip(in(PMIndexB)[i], 0.0f, 10.0f) : slopedPMIndexB.consume();

            float pmFilterRatioAVal = isPMFilterRatioAAudioRate ? 
                sc_clip(in(PMFilterRatioA)[i], 0.0f, 10.0f) : slopedPMFilterRatioA.consume();

            float pmFilterRatioBVal = isPMFilterRatioBAudioRate ? 
                sc_clip(in(PMFilterRatioB)[i], 0.0f, 10.0f) : slopedPMFilterRatioB.consume();

            const float slopeA = m_rampToSlopeA.process(phaseA);
            const float slopeB = m_rampToSlopeB.process(phaseB);
            
            // Calculate phase difference per oversampled sample
            const float phaseDiffA = slopeA / static_cast<float>(osRatio);
            const float phaseDiffB = slopeB / static_cast<float>(osRatio);
            
            m_oversamplingA.upsample(0.0f);
            m_oversamplingB.upsample(0.0f);
            float osPhaseA = phaseA;
            float osPhaseB = phaseB;
            
            for (int k = 0; k < osRatio; k++) {
                // Increment phases
                osPhaseA += phaseDiffA;
                osPhaseB += phaseDiffB;
                
                // Process with wrapped phases
                auto result = m_dualOsc.process(
                    sc_frac(osPhaseA), sc_frac(osPhaseB),
                    cyclePosAVal, cyclePosBVal,
                    slopeA, slopeB, pmIndexAVal, pmIndexBVal,
                    pmFilterRatioAVal, pmFilterRatioBVal,
                    bufDataA, tableSizeA, cycleSamplesA, numCyclesIntA,
                    bufDataB, tableSizeB, cycleSamplesB, numCyclesIntB,
                    m_sincTable
                );
                
                osBufferA[k] = result.oscA;
                osBufferB[k] = result.oscB;
            }
            
            outputA[i] = m_oversamplingA.downsample();
            outputB[i] = m_oversamplingB.downsample();
        }
    }
    
    // Update parameter cache
    cyclePosAPast = isCyclePosAAudioRate ? in(CyclePosA)[nSamples - 1] : slopedCyclePosA.value;
    cyclePosBPast = isCyclePosBAAudioRate ? in(CyclePosB)[nSamples - 1] : slopedCyclePosB.value;
    pmIndexAPast = isPMIndexAAudioRate ? in(PMIndexA)[nSamples - 1] : slopedPMIndexA.value;
    pmIndexBPast = isPMIndexBAudioRate ? in(PMIndexB)[nSamples - 1] : slopedPMIndexB.value;
    pmFilterRatioAPast = isPMFilterRatioAAudioRate ? in(PMFilterRatioA)[nSamples - 1] : slopedPMFilterRatioA.value;
    pmFilterRatioBPast = isPMFilterRatioBAudioRate ? in(PMFilterRatioB)[nSamples - 1] : slopedPMFilterRatioB.value;
}

PluginLoad(OscUGens)
{
    ft = inTable;
    registerUnit<DualOscOS>(ft, "DualOscOS", false);
    registerUnit<SingleOscOS>(ft, "SingleOscOS", false);
}