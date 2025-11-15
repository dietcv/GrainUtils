JCurve : UGen {
	*ar { |phase, shape = 0.5|
		^this.multiNew('audio', phase, shape)
	}
}

SCurve : UGen {
	*ar { |phase, shape = 0.5, inflection = 0.5|
		^this.multiNew('audio', phase, shape, inflection)
	}
}