#include "Distortion.hpp"
#include "SC_PlugIn.hpp"

static InterfaceTable* ft;

// ===== BUCHLA 259 WAVEFOLDER =====

BuchlaFold::BuchlaFold() : m_sampleRate(static_cast<float>(sampleRate()))
{
    // Initialize parameter cache
    drivePast = sc_clip(in0(Drive), 0.0f, 10.0f);
    
    // Check which inputs are audio-rate
    isDriveAudioRate = isAudioRateIn(Drive);
    
    // Initialize oversampling
    m_outputOversampling.reset(m_sampleRate);
    m_driveOversampling.reset(m_sampleRate);
    
    // Setup oversampling index
    m_oversampleIndex = sc_clip(static_cast<int>(in0(Oversample)), 0, 4);
    m_outputOversampling.setOversamplingIndex(m_oversampleIndex);
    m_driveOversampling.setOversamplingIndex(m_oversampleIndex);
    
    // Store ratio and buffer pointers
    m_osRatio = m_outputOversampling.getOversamplingRatio();
    m_outputOSBuffer = m_outputOversampling.getOSBuffer();
    m_osDriveBuffer = m_driveOversampling.getOSBuffer();
    
    mCalcFunc = make_calc_function<BuchlaFold, &BuchlaFold::next>();
    next(1);
}

BuchlaFold::~BuchlaFold() = default;

void BuchlaFold::next(int nSamples) {
    
    // Audio-rate input
    const float* input = in(Input);
    
    // Control-rate parameters with smooth interpolation
    auto slopedDrive = makeSlope(sc_clip(in0(Drive), 0.0f, 10.0f), drivePast);
    
    // Output pointer
    float* output = out(Out);
    
    if (m_oversampleIndex == 0) {

        for (int i = 0; i < nSamples; ++i) {
            
            // Get current parameter values (audio-rate or interpolated control-rate)
            float driveVal = isDriveAudioRate ? 
                sc_clip(in(Drive)[i], 0.0f, 10.0f) : 
                slopedDrive.consume();
            
            output[i] = m_folder.process(input[i], driveVal);
        }
    } else {

        for (int i = 0; i < nSamples; ++i) {
            
            // Get current parameter values (audio-rate or interpolated control-rate)
            float driveVal = isDriveAudioRate ? 
                sc_clip(in(Drive)[i], 0.0f, 10.0f) : 
                slopedDrive.consume();
            
            // Upsample input and parameter values
            m_outputOversampling.upsample(input[i]);
            m_driveOversampling.upsample(driveVal);
            
            for (int k = 0; k < m_osRatio; ++k) {
                
                // Clamp upsampled values
                m_osDriveBuffer[k] = sc_clip(m_osDriveBuffer[k], 0.0f, 10.0f);
                
                // Process wavefolder with upsampled parameter values
                m_outputOSBuffer[k] = m_folder.process(m_outputOSBuffer[k], m_osDriveBuffer[k]);
            }
            
            // Downsample output
            output[i] = m_outputOversampling.downsample();
        }
    }
    
    // Update parameter cache (use last value if audio-rate, otherwise slope value)
    drivePast = isDriveAudioRate ? 
        sc_clip(in(Drive)[nSamples - 1], 0.0f, 10.0f) : 
        slopedDrive.value;
}

PluginLoad(DistortionUGens) {
    ft = inTable;
    registerUnit<BuchlaFold>(ft, "BuchlaFold", false);
}