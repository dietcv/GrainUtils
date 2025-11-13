#include "Disperser.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

Disperser::Disperser() : m_sampleRate(static_cast<float>(sampleRate()))
{
    // Initialize parameter cache
    freqPast = in0(Freq);
    resonancePast = in0(Resonance);
    mixPast = in0(Mix);
    feedbackPast = in0(Feedback);
   
    mCalcFunc = make_calc_function<Disperser, &Disperser::next>();
    next(1);
}

Disperser::~Disperser() = default;

void Disperser::next(int nSamples) {
    // Audio-rate parameters
    const float* input = in(Input);
   
    // Control-rate parameters with smooth interpolation
    auto slopedFreq = makeSlope(sc_clip(in0(Freq), 20.0f, m_sampleRate * 0.49f), freqPast);
    auto slopedResonance = makeSlope(sc_clip(in0(Resonance), 0.0f, 1.0f), resonancePast);
    auto slopedMix = makeSlope(sc_clip(in0(Mix), 0.0f, 1.0f), mixPast);
    auto slopedFeedback = makeSlope(sc_clip(in0(Feedback), 0.0f, 0.99f), feedbackPast);
   
    // Output pointer
    float* outbuf = out(Out);
   
    // Process audio
    for (int i = 0; i < nSamples; ++i) {
        // Add feedback to input
        float inputWithFeedback = input[i] + m_feedbackState;
        
        // DC block
        float dcBlocked = m_dcBlocker.processHighpass(inputWithFeedback, 3.0f, m_sampleRate);
        
        // Process through disperser
        float processed = disperser.process(
            dcBlocked,
            slopedFreq.consume(),
            slopedResonance.consume(),
            m_sampleRate
        );
        
        // Crossfade between dry and processed signal
        float output = Utils::lerp(input[i], processed, slopedMix.consume());
        
        // Write output
        outbuf[i] = output;
        
        // Calculate feedback for next sample
        m_feedbackState = std::tanh(output * slopedFeedback.consume());
        m_feedbackState = zapgremlins(m_feedbackState);
    }
   
    // Update parameter cache
    freqPast = slopedFreq.value;
    resonancePast = slopedResonance.value;
    mixPast = slopedMix.value;
    feedbackPast = slopedFeedback.value;
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<Disperser>(ft, "Disperser", false);
}