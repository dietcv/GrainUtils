GrainDelay : UGen {
    *ar { |input, triggerRate = 10, overlap = 1, 
        delayTime = 0.2, grainRate = 1.0, mix = 0.5, 
        feedback = 0.0, damping = 0.7, freeze = 0, reset = 0|
        
        ^this.multiNew('audio', input, triggerRate, overlap, 
            delayTime, grainRate, mix, feedback, damping, freeze, reset)
    }
}