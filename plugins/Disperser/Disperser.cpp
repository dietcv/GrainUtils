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
    
    // Check which inputs are audio-rate
    isFreqAudioRate = isAudioRateIn(Freq);
    isResonanceAudioRate = isAudioRateIn(Resonance);
    isMixAudioRate = isAudioRateIn(Mix);
    isFeedbackAudioRate = isAudioRateIn(Feedback);
   
    mCalcFunc = make_calc_function<Disperser, &Disperser::next>();
    next(1);
}

Disperser::~Disperser() = default;

void Disperser::next(int nSamples) {
    // Audio-rate input
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
        // Get current parameter values (audio-rate or interpolated control-rate)
        float freq = isFreqAudioRate ? 
            sc_clip(in(Freq)[i], 20.0f, m_sampleRate * 0.49f) : 
            slopedFreq.consume();
            
        float resonance = isResonanceAudioRate ? 
            sc_clip(in(Resonance)[i], 0.0f, 1.0f) : 
            slopedResonance.consume();
            
        float mix = isMixAudioRate ? 
            sc_clip(in(Mix)[i], 0.0f, 1.0f) : 
            slopedMix.consume();
            
        float feedback = isFeedbackAudioRate ? 
            sc_clip(in(Feedback)[i], 0.0f, 0.99f) : 
            slopedFeedback.consume();
        
        // Add feedback to input
        float inputWithFeedback = input[i] + m_feedbackState;
        
        // DC block
        float dcBlocked = m_dcBlocker.processHighpass(inputWithFeedback, 3.0f, m_sampleRate);
        
        // Process through disperser
        float processed = disperser.process(
            dcBlocked,
            freq,
            resonance,
            m_sampleRate
        );
        
        // Crossfade between dry and processed signal
        float output = Utils::lerp(input[i], processed, mix);
        
        // Write output
        outbuf[i] = output;
        
        // Calculate feedback for next sample
        m_feedbackState = std::tanh(output * feedback);
        m_feedbackState = zapgremlins(m_feedbackState);
    }
   
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    freqPast = isFreqAudioRate ? in(Freq)[nSamples - 1] : slopedFreq.value;
    resonancePast = isResonanceAudioRate ? in(Resonance)[nSamples - 1] : slopedResonance.value;
    mixPast = isMixAudioRate ? in(Mix)[nSamples - 1] : slopedMix.value;
    feedbackPast = isFeedbackAudioRate ? in(Feedback)[nSamples - 1] : slopedFeedback.value;
}

PluginLoad(GrainUtilsUGens) {
    ft = inTable;
    registerUnit<Disperser>(ft, "Disperser", false);
}