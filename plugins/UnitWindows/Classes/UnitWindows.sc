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