#include "ShiftRegister.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== SHIFT REGISTER =====

ShiftRegister::ShiftRegister() : m_sampleRate(static_cast<float>(sampleRate()))
{
    mCalcFunc = make_calc_function<ShiftRegister, &ShiftRegister::next_aa>();
    next_aa(1);

    // Reset state after priming
    m_shiftRegister.reset();
    m_trigger.reset();
    m_resetTrigger.reset();
}

ShiftRegister::~ShiftRegister() = default;

void ShiftRegister::next_aa(int inNumSamples) {
    RGen& rgen = *mParent->mRGen;

    // Audio-rate parameters
    const float *triggerIn = in(Trigger);
    const float *chanceIn = in(Chance);
    const float *lengthIn = in(Length);
    const float *rotateIn = in(Rotate);
    const float* resetIn = in(Reset);

    // Output pointers
    float *out3Bit = out(Out3Bit);
    float *out8Bit = out(Out8Bit);
   
    for (int i = 0; i < inNumSamples; ++i) {
 
        // Get audio-rate parameters per-sample
        bool trigger = m_trigger.process(triggerIn[i]);
        float chance = sc_clip(chanceIn[i], 0.0f, 1.0f);
        int length = sc_clip(static_cast<int>(lengthIn[i]), 1, MAX_LENGTH);
        int rotation = sc_clip(static_cast<int>(rotateIn[i]), -MAX_LENGTH, MAX_LENGTH);
        bool reset = m_resetTrigger.process(resetIn[i]);

        // Process shift register
        auto output = m_shiftRegister.process(
            trigger,
            reset, 
            chance, 
            length, 
            rotation,
            rgen
        );

        // Write outputs
        out3Bit[i] = output.out3Bit;
        out8Bit[i] = output.out8Bit;
    }
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<ShiftRegister>(ft, "ShiftRegisterUgen", false);
}