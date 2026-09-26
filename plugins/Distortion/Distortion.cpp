#include "Distortion.hpp"
#include "SC_PlugIn.hpp"

extern InterfaceTable* ft;

// ===== BUCHLA 259 WAVEFOLDER =====
 
BuchlaFold::BuchlaFold() : 
    m_oversampleIndex(sc_clip(static_cast<int>(in0(Oversample)), 0, 4)),
    m_osRatio(1 << m_oversampleIndex)
{
    // Check which inputs are audio-rate
    isDriveAudioRate = isAudioRateIn(Drive);
 
    // Allocate oversampling buffer
    if (m_oversampleIndex > 0) {
        BufferUtils::allocBuffer(this, mWorld, m_osRatio, m_outputOSBuffer);
    }
    
    // Set calc function & compute initial sample
    set_calc_function<BuchlaFold, &BuchlaFold::next>();
}
 
BuchlaFold::~BuchlaFold() {
    RTFree(mWorld, m_outputOSBuffer);
}
 
void BuchlaFold::next(int nSamples) {
    
    // Audio-rate input
    const float* input = in(Input);
    
    // Control-rate parameters with smooth interpolation
    m_driveInterp.update(sc_clip(in0(Drive), 0.0f, 1.0f), nSamples);
    
    // Output pointer
    float* output = out(Out);
    
    if (m_oversampleIndex == 0) {
 
        for (int i = 0; i < nSamples; ++i) {
            
            // Get current parameter values (audio-rate or interpolated control-rate)
            float drive = isDriveAudioRate ? 
                sc_clip(in(Drive)[i], 0.0f, 1.0f) : 
                m_driveInterp.process();
            
            output[i] = m_folder.process(input[i], drive);
        }
    } else {
 
        for (int i = 0; i < nSamples; ++i) {
            
            // Get current parameter values (audio-rate or interpolated control-rate)
            float drive = isDriveAudioRate ? 
                sc_clip(in(Drive)[i], 0.0f, 1.0f) : 
                m_driveInterp.process();
            
            // Upsample input
            m_outputOversampling.upsample(input[i], m_outputOSBuffer, m_osRatio);

            // Latch parameter values for oversampling
            m_osDriveInterp.update(drive);
            
            for (int k = 0; k < m_osRatio; ++k) {

                // Calculate fractional position for interpolation
                float frac = static_cast<float>(k + 1) / static_cast<float>(m_osRatio);

                // Interpolate parameter values
                float osDrive = m_osDriveInterp.process(frac);
                
                // Process wavefolder
                m_outputOSBuffer[k] = m_folder.process(m_outputOSBuffer[k], osDrive);
            }
            
            // Downsample output
            output[i] = m_outputOversampling.downsample(m_outputOSBuffer, m_osRatio);
        }
    }
}

// ===== SERGE WAVEFOLDER =====
 
SergeFold::SergeFold() :
    m_oversampleIndex(sc_clip(static_cast<int>(in0(Oversample)), 0, 4)),
    m_osRatio(1 << m_oversampleIndex)
{
    // Check which inputs are audio-rate
    isDriveAudioRate = isAudioRateIn(Drive);
 
    // Allocate oversampling buffer
    if (m_oversampleIndex > 0) {
        BufferUtils::allocBuffer(this, mWorld, m_osRatio, m_outputOSBuffer);
    }
 
    // Set calc function & compute initial sample
    set_calc_function<SergeFold, &SergeFold::next>();
}
 
SergeFold::~SergeFold() {
    RTFree(mWorld, m_outputOSBuffer);
}
 
void SergeFold::next(int nSamples) {
 
    // Audio-rate input
    const float* input = in(Input);
 
    // Control-rate parameters with smooth interpolation
    m_driveInterp.update(sc_clip(in0(Drive), 0.0f, 1.0f), nSamples);
 
    // Output pointer
    float* output = out(Out);
 
    if (m_oversampleIndex == 0) {
 
        for (int i = 0; i < nSamples; ++i) {
 
            // Get current parameter values (audio-rate or interpolated control-rate)
            float drive = isDriveAudioRate ?
                sc_clip(in(Drive)[i], 0.0f, 1.0f) :
                m_driveInterp.process();
 
            output[i] = m_folder.process(input[i], drive);
        }
    } else {
 
        for (int i = 0; i < nSamples; ++i) {
 
            // Get current parameter values (audio-rate or interpolated control-rate)
            float drive = isDriveAudioRate ?
                sc_clip(in(Drive)[i], 0.0f, 1.0f) :
                m_driveInterp.process();
 
            // Upsample input
            m_outputOversampling.upsample(input[i], m_outputOSBuffer, m_osRatio);

            // Latch parameter values for oversampling
            m_osDriveInterp.update(drive);
 
            for (int k = 0; k < m_osRatio; ++k) {

                // Calculate fractional position for interpolation
                float frac = static_cast<float>(k + 1) / static_cast<float>(m_osRatio);

                // Interpolate parameter values
                float osDrive = m_osDriveInterp.process(frac);
 
                // Process wavefolder
                m_outputOSBuffer[k] = m_folder.process(m_outputOSBuffer[k], osDrive);
            }
 
            // Downsample output
            output[i] = m_outputOversampling.downsample(m_outputOSBuffer, m_osRatio);
        }
    }
}

void Distortion_setup() 
{
    registerUnit<BuchlaFold>(ft, "BuchlaFold", false);
    registerUnit<SergeFold>(ft, "SergeFold", false);
}