#include "ShiftRegister.hpp"

// ===== SHIFT REGISTER =====

ShiftRegister::ShiftRegister() : m_sampleRate(static_cast<float>(sampleRate()))
{
    mCalcFunc = make_calc_function<ShiftRegister, &ShiftRegister::next_aa>();
    next_aa(1);
    m_shiftRegister.reset();
}

ShiftRegister::~ShiftRegister() = default;

void ShiftRegister::next_aa(int inNumSamples) {
    RGen& rgen = *mParent->mRGen;

    // Audio-rate parameters
    const float *triggerIn = in(Trigger);
    const float *chanceIn = in(Chance);
    const float *lengthIn = in(Length);
    const float *rotateIn = in(Rotate);

    // Control-rate parameters
    const bool resetTrigger = in0(Reset) > 0.5f;
   
    // Get output pointers
    float *out3Bit = out(Out3Bit);
    float *out8Bit = out(Out8Bit);
   
    for (int i = 0; i < inNumSamples; ++i) {
 
        // Get audio-rate parameters per-sample
        const float trigger = triggerIn[i] > 0.5f;
        const float chance = sc_clip(chanceIn[i], 0.0f, 1.0f);
        const int length = sc_clip(static_cast<int>(lengthIn[i]), 1, MAX_LENGTH);
        const int rotation = sc_clip(static_cast<int>(rotateIn[i]), -MAX_LENGTH, MAX_LENGTH);

        // Process shift register
        m_shiftRegister.process(
            trigger, 
            chance, 
            length, 
            rotation,
            rgen
        );

        // 5. Output
        out3Bit[i] = m_shiftRegister.m_out3Bit;
        out8Bit[i] = m_shiftRegister.m_out8Bit;
    }
}