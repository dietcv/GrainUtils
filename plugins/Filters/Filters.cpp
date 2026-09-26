#include "Filters.hpp"
#include "SC_PlugIn.hpp"

extern InterfaceTable* ft;

// ===== DISPERSER =====

Disperser::Disperser() : 
    m_sampleRate(static_cast<float>(sampleRate()))
{
    // Check which inputs are audio-rate
    isFreqAudioRate = isAudioRateIn(Freq);
    isResonanceAudioRate = isAudioRateIn(Resonance);
    isMixAudioRate = isAudioRateIn(Mix);
    isFeedbackAudioRate = isAudioRateIn(Feedback);
   
    // Set calc function & compute initial sample
    set_calc_function<Disperser, &Disperser::next>();
}

Disperser::~Disperser() = default;

void Disperser::next(int nSamples) {
    
    // Audio-rate input
    const float* input = in(Input);
   
    // Control-rate parameters with smooth interpolation
    m_freqInterp.update(sc_clip(in0(Freq), 20.0f, m_sampleRate * 0.49f), nSamples);
    m_resonanceInterp.update(sc_clip(in0(Resonance), 0.0f, 1.0f), nSamples);
    m_mixInterp.update(sc_clip(in0(Mix), 0.0f, 1.0f), nSamples);
    m_feedbackInterp.update(sc_clip(in0(Feedback), 0.0f, 0.99f), nSamples);
   
    // Output pointer
    float* outbuf = out(Out);
   
    // Process audio
    for (int i = 0; i < nSamples; ++i) {
        
        // Get current parameter values (audio-rate or interpolated control-rate)
        float freq = isFreqAudioRate ? 
            sc_clip(in(Freq)[i], 20.0f, m_sampleRate * 0.49f) : 
            m_freqInterp.process();
            
        float resonance = isResonanceAudioRate ? 
            sc_clip(in(Resonance)[i], 0.0f, 1.0f) : 
            m_resonanceInterp.process();
            
        float mix = isMixAudioRate ? 
            sc_clip(in(Mix)[i], 0.0f, 1.0f) : 
            m_mixInterp.process();
            
        float feedback = isFeedbackAudioRate ? 
            sc_clip(in(Feedback)[i], 0.0f, 0.99f) : 
            m_feedbackInterp.process();
        
        // Add feedback to input
        float inputWithFeedback = input[i] + m_feedbackState;
        
        // DC block
        float dcBlocked = m_dcBlocker.process(inputWithFeedback, m_sampleRate);
        
        // Process audio through allpass chain
        float processed = m_allpassChain.process(
            dcBlocked,
            freq,
            resonance,
            m_sampleRate
        );
        
        // Crossfade between dry and processed signal
        float output = lininterp(mix, input[i], processed);
        
        // Write output
        outbuf[i] = output;
        
        // Calculate feedback for next sample
        m_feedbackState = std::tanh(output * feedback);
        m_feedbackState = zapgremlins(m_feedbackState);
    }
}

// ===== MORPHING FILTER =====
 
MorphSVF::MorphSVF() : 
    m_sampleRate(static_cast<float>(sampleRate()))
{
    // Check which inputs are audio-rate
    isFreqAudioRate = isAudioRateIn(Freq);
    isResonanceAudioRate = isAudioRateIn(Resonance);
    isShapeAudioRate = isAudioRateIn(Shape);
 
    // Set calc function & compute initial sample
    set_calc_function<MorphSVF, &MorphSVF::next>();
}
 
MorphSVF::~MorphSVF() = default;
 
void MorphSVF::next(int nSamples) {
    
    // Audio-rate input
    const float* input = in(Input);
 
    // Control-rate parameters with smooth interpolation
    m_freqInterp.update(sc_clip(in0(Freq), 20.0f, m_sampleRate * 0.49f), nSamples);
    m_resonanceInterp.update(sc_clip(in0(Resonance), 0.0f, 1.0f), nSamples);
    m_shapeInterp.update(sc_clip(in0(Shape), 0.0f, 1.0f), nSamples);
 
    // Output pointer
    float* outbuf = out(Out);
 
    // Process audio
    for (int i = 0; i < nSamples; ++i) {
 
        // Get current parameter values (audio-rate or interpolated control-rate)
        float freq = isFreqAudioRate ?
            sc_clip(in(Freq)[i], 20.0f, m_sampleRate * 0.49f) :
            m_freqInterp.process();
 
        float resonance = isResonanceAudioRate ?
            sc_clip(in(Resonance)[i], 0.0f, 1.0f) :
            m_resonanceInterp.process();
 
        float shape = isShapeAudioRate ?
            sc_clip(in(Shape)[i], 0.0f, 1.0f) :
            m_shapeInterp.process();
 
        // Process audio through morphing filter
        outbuf[i] = m_morphingFilter.process(input[i], freq, resonance, shape, m_sampleRate);
    }
}

void Filters_setup()
{
    registerUnit<Disperser>(ft, "Disperser", false);
    registerUnit<MorphSVF>(ft, "MorphSVF", false);
}