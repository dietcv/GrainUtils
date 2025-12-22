#include "Distortion.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== BUCHLA 259 WAVEFOLDER =====

BuchlaFoldADAA::BuchlaFoldADAA() : m_sampleRate(static_cast<float>(sampleRate()))
{
    // Initialize parameter cache
    drivePast = sc_clip(in0(Drive), 0.0f, 10.0f);
    
    // Check which inputs are audio-rate
    isDriveAudioRate = isAudioRateIn(Drive);
    
    m_oversampling.reset(m_sampleRate);
    
    mCalcFunc = make_calc_function<BuchlaFoldADAA, &BuchlaFoldADAA::next>();
    next(1);
}

BuchlaFoldADAA::~BuchlaFoldADAA() = default;

void BuchlaFoldADAA::next(int nSamples) {
    
    // Audio-rate input
    const float* input = in(Input);
    
    // Control-rate parameters with smooth interpolation
    auto slopedDrive = makeSlope(sc_clip(in0(Drive), 0.0f, 10.0f), drivePast);
    
    // Control-rate parameters (settings)
    const int oversampleIndex = sc_clip(static_cast<int>(in0(Oversample)), 0, 4);
    
    // Output pointer
    float* output = out(Out);
    
    if (oversampleIndex == 0) {
        // No oversampling - direct processing
        for (int i = 0; i < nSamples; ++i) {

            // Get current parameter values (audio-rate or interpolated control-rate)
            float driveVal = isDriveAudioRate ? 
                sc_clip(in(Drive)[i], 0.0f, 10.0f) : 
                slopedDrive.consume();
            
            output[i] = m_folder.process(input[i], driveVal);
        }
    } else {
        // Set oversampling factor
        m_oversampling.setOversamplingIndex(oversampleIndex);
        
        // Prepare oversampling buffer
        const int osRatio = m_oversampling.getOversamplingRatio();
        float* osBuffer = m_oversampling.getOSBuffer();
        
        for (int i = 0; i < nSamples; ++i) {

            // Get current parameter values (audio-rate or interpolated control-rate)
            float driveVal = isDriveAudioRate ? 
                sc_clip(in(Drive)[i], 0.0f, 10.0f) : 
                slopedDrive.consume();
            
            m_oversampling.upsample(input[i]);
            
            for (int k = 0; k < osRatio; ++k) {
                osBuffer[k] = m_folder.process(osBuffer[k], driveVal);
            }
            
            output[i] = m_oversampling.downsample();
        }
    }
    
    // Update parameter cache
    drivePast = isDriveAudioRate ? 
        sc_clip(in(Drive)[nSamples - 1], 0.0f, 10.0f) : 
        slopedDrive.value;
}

PluginLoad(DistortionUGens) {
    ft = inTable;
    registerUnit<BuchlaFoldADAA>(ft, "BuchlaFoldADAA", false);
}