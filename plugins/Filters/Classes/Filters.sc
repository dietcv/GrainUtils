// ===== DISPERSER =====

Disperser : UGen {
	*ar { |input, freq, resonance, mix, feedback|
		^this.multiNew('audio', input, freq, resonance, mix, feedback)
	}

	checkInputs {
		^this.checkValidInputs
	}
}

// ===== STATE VARIABLE FILTER =====

SimperSVF : UGen {
    *ar { |input, freq = 440, resonance = 0, type = 0|
 
        ^this.multiNew('audio', input, freq, resonance, type)
    }

	checkInputs {
		^this.checkValidInputs
	}
}