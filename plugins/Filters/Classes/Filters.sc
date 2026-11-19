// ===== PHASER =====

PhaserSVF : UGen {
	*ar { |input, freq = 440, resonance = 0, feedback = 0, mix = 0.5|
		^this.multiNew('audio', input, freq, resonance, feedback, mix)
	}

	checkInputs {
		^this.checkValidInputs
	}
}

// ===== MORPHING STATE VARIABLE FILTER =====

MorphSVF : UGen {
    *ar { |input, freq = 440, resonance = 0, shape = 0|
        ^this.multiNew('audio', input, freq, resonance, shape)
    }

	checkInputs {
		^this.checkValidInputs
	}
}