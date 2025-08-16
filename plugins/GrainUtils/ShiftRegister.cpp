#include "ShiftRegister.hpp"

// ===== SHIFT REGISTER =====

ShiftRegister::ShiftRegister() : m_sampleRate(static_cast<float>(sampleRate()))
{
    mCalcFunc = make_calc_function<ShiftRegister, &ShiftRegister::next_aa>();
    next_aa(1);
}

ShiftRegister::~ShiftRegister() = default;

void ShiftRegister::next_aa(int inNumSamples) {

    // Audio-rate parameters
    const float *freq = in(Freq);
    const float *chance = in(Chance);
    const float *length = in(Length);
    const float *rotate = in(Rotate);
    const float *feedback = in(Feedback);
    const float *feedbackSource = in(FeedbackSource);

    // Control-rate parameters
    const int seed = sc_clip(static_cast<int>(in0(Seed)), 0, MAX_DATA_VALUE);
    const bool resetTrigger = in0(Reset) > 0.5f;
   
    // Get output pointers
    float *out3Bit = out(Out3Bit);
    float *out8Bit = out(Out8Bit);
    float *outRamp = out(OutRamp);
   
    for (int i = 0; i < inNumSamples; ++i) {
 
        // Sample audio-rate parameters per-sample
        const float freqVal = freq[i];
        const float chanceVal = sc_clip(chance[i], 0.0f, 1.0f);
        const int lengthVal = sc_clip(static_cast<int>(length[i]), 1, MAX_LENGTH);
        const int rotationVal = sc_clip(static_cast<int>(rotate[i]), -MAX_LENGTH, MAX_LENGTH);
        const float fmIndex = sc_clip(feedback[i], 0.0f, 2.0f);
        const int fmodSource = sc_clip(static_cast<int>(feedbackSource[i]), 0, 1);
   
        // 1. Get modulation value from previous sample
        float modValue;
        switch (fmodSource) {
            case 0:  // 3-bit feedback for FM
                modValue = m_shiftRegister.m_prevOut3Bit;
                break;
            case 1:  // 8-bit feedback for FM
                modValue = m_shiftRegister.m_prevOut8Bit;
                break;
            default:
                modValue = m_shiftRegister.m_prevOut3Bit;
        }
         
        // 2. Calculate modulated frequency
        float fmod = std::pow(2.0f, (modValue * 2.0f - 1.0f) * fmIndex);
        float modulatedFreq = freqVal * fmod;
       
        // 3. Process timing
        auto event = m_scheduler.process(
            modulatedFreq, 
            resetTrigger, 
            m_sampleRate
        );

        // 4. Process shift register
        m_shiftRegister.process(
            event.trigger, 
            chanceVal, 
            lengthVal, 
            rotationVal, 
            seed
        );

        // 5. Output
        outRamp[i] = event.phase;
        out3Bit[i] = m_shiftRegister.m_currentOut3Bit;
        out8Bit[i] = m_shiftRegister.m_currentOut8Bit;
    }
}