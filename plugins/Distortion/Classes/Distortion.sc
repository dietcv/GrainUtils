BuchlaFoldADAA : UGen {
	*ar { |input, drive, oversample = 0|
		^this.multiNew('audio', input, drive, oversample)
	}
}