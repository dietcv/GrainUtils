JCurve : UGen {
	*ar { |phase, shape = 0.5|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(shape.rate!='audio'){shape = K2A.ar(shape)};
		^this.multiNew('audio', phase, shape)
	}
}

SCurve : UGen {
	*ar { |phase, shape = 0.5, inflection = 0.5|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(shape.rate!='audio'){shape = K2A.ar(shape)};
		if(inflection.rate!='audio'){ inflection = K2A.ar( inflection)};
		^this.multiNew('audio', phase, shape, inflection)
	}
}