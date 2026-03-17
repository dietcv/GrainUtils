Disperser : UGen {
	*ar { |input, freq, resonance, mix, feedback|
		^this.multiNew('audio', input, freq, resonance, mix, feedback)
	}
	checkInputs {
		^this.checkValidInputs
	}
}