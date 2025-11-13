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