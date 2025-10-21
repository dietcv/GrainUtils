// ===== UNIT SHAPERS =====

UnitTriangle : UGen {
    *ar { |phase, skew = 0.5|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(skew.rate!='audio'){skew = K2A.ar(skew)};
        ^this.multiNew('audio', phase, skew)
    }
}

UnitKink : UGen {
    *ar { |phase, skew = 0.5|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(skew.rate!='audio'){skew = K2A.ar(skew)};
        ^this.multiNew('audio', phase, skew)
    }
}

UnitCubic : UGen {
    *ar { |phase, index = 0|
        if(phase.rate != 'audio') { phase = K2A.ar(phase) };
        if(index.rate != 'audio') { index = K2A.ar(index) };
        ^this.multiNew('audio', phase, index)
    }
}

UnitStep : UGen {
    *ar { |phase, interp = 0|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
        ^this.multiNew('audio', phase, interp)
    }
}

UnitWalk : UGen {
    *ar { |phase, step = 0.2, interp = 0|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(step.rate!='audio'){step = K2A.ar(step)};
        ^this.multiNew('audio', phase, step, interp)
    }
}

// ===== WINDOW FUNCTIONS =====

HanningWindow : UGen {
    *ar { |phase, skew = 0.5|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(skew.rate!='audio'){skew = K2A.ar(skew)};
        ^this.multiNew('audio', phase, skew)
    }
}

GaussianWindow : UGen {
    *ar { |phase, skew = 0.5, index = 0|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(skew.rate!='audio'){skew = K2A.ar(skew)};
		if(index.rate!='audio'){index = K2A.ar(index)};
        ^this.multiNew('audio', phase, skew, index)
    }
}

TrapezoidalWindow : UGen {
    *ar { |phase, skew = 0.5, width = 0.5, duty = 1|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(skew.rate!='audio'){skew = K2A.ar(skew)};
		if(width.rate!='audio'){width = K2A.ar(width)};
		if(duty.rate!='audio'){duty = K2A.ar(duty)};
        ^this.multiNew('audio', phase, skew, width, duty)
    }
}

TukeyWindow : UGen {
    *ar { |phase, skew = 0.5, width = 0.5|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(skew.rate!='audio'){skew = K2A.ar(skew)};
		if(width.rate!='audio'){width = K2A.ar(width)};
        ^this.multiNew('audio', phase, skew, width)
    }
}

ExponentialWindow : UGen {
	*ar { |phase, skew = 0.5, shape = 0.5|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(skew.rate!='audio'){skew = K2A.ar(skew)};
		if(shape.rate!='audio'){shape = K2A.ar(shape)};
		^this.multiNew('audio', phase, skew, shape)
	}
}

// ===== INTERP FUNCTIONS =====

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