#include "Oscs.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable *ft;

// ===== DUAL WAVETABLE OSCILLATOR =====

DualOscOS::DualOscOS() : m_sampleRate(static_cast<float>(sampleRate()))
{
    m_oversamplingA.reset(m_sampleRate);
    m_oversamplingB.reset(m_sampleRate);
    m_dualOsc.reset();
    mCalcFunc = make_calc_function<DualOscOS, &DualOscOS::next_aa>();
    next_aa(1);
}

DualOscOS::~DualOscOS() = default;

bool DualOscOS::getBufferData(OscUtils::BufUnit& bufUnit, float bufNum, int nSamples,
                              const float*& bufData, int& tableSize, const char* oscName) {
    const SndBuf* buf;
    const auto verify_buf = bufUnit.GetTable(mWorld, mParent, bufNum, nSamples, buf, bufData, tableSize);
    if (!verify_buf) {
        if (!bufUnit.m_buf_failed) {
            Print("DualOscOS: buffer %s not found\n", oscName);
            bufUnit.m_buf_failed = true;
        }
        return false;
    }
    bufUnit.m_buf_failed = false;
    return true;
}

void DualOscOS::next_aa(int nSamples) {

    // Audio-rate parameters
    const float* phaseA = in(PhaseA);
    const float* phaseB = in(PhaseB);
    const float* cyclePosA = in(CyclePosA);
    const float* cyclePosB = in(CyclePosB);
    const float* pmIndexA = in(PMIndexA);
    const float* pmIndexB = in(PMIndexB);
    const float* pmFilterRatioA = in(PMFilterRatioA);
    const float* pmFilterRatioB = in(PMFilterRatioB);
    
    // Control-rate parameters
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
    int tableSizeA, tableSizeB;
    
    if (!getBufferData(m_bufUnitA, bufNumA, nSamples, bufDataA, tableSizeA, "OscA") ||
        !getBufferData(m_bufUnitB, bufNumB, nSamples, bufDataB, tableSizeB, "OscB")) {
        ClearUnitOutputs(this, nSamples);
        return;
    }
    
    // Pre-calculate constants
    const int cycleSamplesA = tableSizeA / static_cast<int>(numCyclesA);
    const int cycleSamplesB = tableSizeB / static_cast<int>(numCyclesB);
    const int numCyclesIntA = static_cast<int>(numCyclesA);
    const int numCyclesIntB = static_cast<int>(numCyclesB);
    
    // Set oversampling factor for both channels
    m_oversamplingA.setOversamplingIndex(oversampleIndex);
    m_oversamplingB.setOversamplingIndex(oversampleIndex);
    
    if (oversampleIndex == 0) {
        // No oversampling - direct processing
        for (int i = 0; i < nSamples; ++i) {
            const float slopeA = m_rampToSlopeA.process(phaseA[i]);
            const float slopeB = m_rampToSlopeB.process(phaseB[i]);
            
            auto result = m_dualOsc.process(
                phaseA[i], phaseB[i], cyclePosA[i], cyclePosB[i],
                slopeA, slopeB, pmIndexA[i], pmIndexB[i],
                pmFilterRatioA[i], pmFilterRatioB[i],
                bufDataA, tableSizeA, cycleSamplesA, numCyclesIntA,
                bufDataB, tableSizeB, cycleSamplesB, numCyclesIntB,
                m_sincTable
            );
            
            outputA[i] = result.oscA;
            outputB[i] = result.oscB;
        }
    } else {
        // Oversampling enabled
        const int osRatioA = m_oversamplingA.getOversamplingRatio();
        const int osRatioB = m_oversamplingB.getOversamplingRatio();
        const float invOsRatioA = 1.0f / static_cast<float>(osRatioA);
        const float invOsRatioB = 1.0f / static_cast<float>(osRatioB);
        
        for (int i = 0; i < nSamples; ++i) {
            const float slopeA = m_rampToSlopeA.process(phaseA[i]);
            const float slopeB = m_rampToSlopeB.process(phaseB[i]);
            
            // Calculate phase difference per oversampled sample
            const float phaseDiffA = slopeA * invOsRatioA;
            const float phaseDiffB = slopeB * invOsRatioB;
            
            // Prepare oversampling buffers
            m_oversamplingA.upsample(0.0f);
            m_oversamplingB.upsample(0.0f);
            float* osBufferA = m_oversamplingA.getOSBuffer();
            float* osBufferB = m_oversamplingB.getOSBuffer();
            
            float osPhaseA = phaseA[i];
            float osPhaseB = phaseB[i];
            
            for (int k = 0; k < osRatioA; k++) {
                // Increment phases first
                osPhaseA += phaseDiffA;
                osPhaseB += phaseDiffB;
                
                // Then process with wrapped phases
                auto result = m_dualOsc.process(
                    sc_wrap(osPhaseA, 0.0f, 1.0f), sc_wrap(osPhaseB, 0.0f, 1.0f),
                    cyclePosA[i], cyclePosB[i],
                    slopeA, slopeB, pmIndexA[i], pmIndexB[i],
                    pmFilterRatioA[i], pmFilterRatioB[i],
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
}

// ===== SINGLE WAVETABLE OSCILLATOR =====

SingleOscOS::SingleOscOS() : m_sampleRate(static_cast<float>(sampleRate()))
{
    m_oversampling.reset(m_sampleRate);
    mCalcFunc = make_calc_function<SingleOscOS, &SingleOscOS::next_aa>();
    next_aa(1);
}

SingleOscOS::~SingleOscOS() = default;

bool SingleOscOS::getBufferData(OscUtils::BufUnit& bufUnit, float bufNum, int nSamples,
                              const float*& bufData, int& tableSize, const char* oscName) {
    const SndBuf* buf;
    const auto verify_buf = bufUnit.GetTable(mWorld, mParent, bufNum, nSamples, buf, bufData, tableSize);
    if (!verify_buf) {
        if (!bufUnit.m_buf_failed) {
            Print("SingleOscOS: buffer %s not found\n", oscName);
            bufUnit.m_buf_failed = true;
        }
        return false;
    }
    bufUnit.m_buf_failed = false;
    return true;
}

void SingleOscOS::next_aa(int nSamples) {
    
    // Audio-rate parameters
    const float* phase = in(Phase);
    const float* cyclePos = in(CyclePos);

    // Control-rate parameters
    const float numCycles = sc_max(in0(NumCycles), 1.0f);
    const float bufNum = in0(BufNum);
    const int oversampleIndex = sc_clip(static_cast<int>(in0(Oversample)), 0, 4);
    
    // Output pointer
    float* output = out(Out);
    
    // Get buffer data
    const float* bufData;
    int tableSize;
    
    if (!getBufferData(m_bufUnit, bufNum, nSamples, bufData, tableSize, "SingleOscOS")) {
        ClearUnitOutputs(this, nSamples);
        return;
    }
    
    // Pre-calculate constants
    const int cycleSamples = tableSize / static_cast<int>(numCycles);
    const int numCyclesInt = static_cast<int>(numCycles);
    
    // Set oversampling factor
    m_oversampling.setOversamplingIndex(oversampleIndex);
    
    if (oversampleIndex == 0) {
        // No oversampling - direct processing
        for (int i = 0; i < nSamples; ++i) {
            const float slope = m_rampToSlope.process(phase[i]);
            
            output[i] = OscUtils::wavetableInterpolate(
            phase[i], bufData, tableSize, 
            cycleSamples, numCyclesInt, cyclePos[i], 
            slope, m_sincTable);
        }
    } else {
        // Oversampling enabled
        const int osRatio = m_oversampling.getOversamplingRatio();
        const float invOsRatio = 1.0f / static_cast<float>(osRatio);
        
        for (int i = 0; i < nSamples; ++i) {
            const float slope = m_rampToSlope.process(phase[i]);
            
            // Calculate phase difference per oversampled sample
            const float phaseDiff = slope * invOsRatio;
            
            // Prepare oversampling buffer
            m_oversampling.upsample(0.0f);
            float* osBuffer = m_oversampling.getOSBuffer();
            
            float osPhase = phase[i];
            
            for (int k = 0; k < osRatio; k++) {
                // Increment phase first
                osPhase += phaseDiff;
                
                // Then process with wrapped phase
                osBuffer[k] = OscUtils::wavetableInterpolate(
                sc_wrap(osPhase, 0.0f, 1.0f), bufData, tableSize, 
                cycleSamples, numCyclesInt, cyclePos[i], 
                slope, m_sincTable);
            }
            
            output[i] = m_oversampling.downsample();
        }
    }
}

PluginLoad(OscUGens)
{
    ft = inTable;
    registerUnit<DualOscOS>(ft, "DualOscOS", false);
    registerUnit<SingleOscOS>(ft, "SingleOscOS", false);
}