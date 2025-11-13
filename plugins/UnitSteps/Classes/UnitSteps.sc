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

UnitUSRUgen : MultiOutUGen {
    *ar { |phase, chance, length, rotate, interp = 0, reset = 0|
		if(phase.rate != 'audio') { phase = K2A.ar(phase) };
		if(chance.rate != 'audio') { chance = K2A.ar(chance) };
		if(length.rate != 'audio') { length = K2A.ar(length) };
		if(rotate.rate != 'audio') { rotate = K2A.ar(rotate) };
        ^this.multiNew('audio', phase, chance, length, rotate, interp, reset);
    }

    init { arg ... theInputs;
        inputs = theInputs;
        ^this.initOutputs(2, rate);
    }

    checkInputs {
        ^this.checkValidInputs;
    }
}

UnitUSR {
	*ar { |phase, chance, length, rotate, interp = 0, reset = 0|
		var register = UnitUSRUgen.ar(phase, chance, length, rotate, interp, reset);
		^(
			bit3: register[0],
			bit8: register[1]
		);
	}
}