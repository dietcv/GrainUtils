HanningWindow : UGen {
    *ar { |phase, skew = 0.5|
        ^this.multiNew('audio', phase, skew)
    }
}

GaussianWindow : UGen {
    *ar { |phase, skew = 0.5, index = 0|
        ^this.multiNew('audio', phase, skew, index)
    }
}

TrapezoidalWindow : UGen {
    *ar { |phase, skew = 0.5, width = 0.5, duty = 1|
        ^this.multiNew('audio', phase, skew, width, duty)
    }
}

TukeyWindow : UGen {
    *ar { |phase, skew = 0.5, width = 0.5|
        ^this.multiNew('audio', phase, skew, width)
    }
}

ExponentialWindow : UGen {
	*ar { |phase, skew = 0.5, shape = 0.5|
		^this.multiNew('audio', phase, skew, shape)
	}
}