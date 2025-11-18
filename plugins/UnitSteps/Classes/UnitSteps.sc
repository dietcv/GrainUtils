UnitStep : UGen {
    *ar { |phase, interp = 0|
        ^this.multiNew('audio', phase, interp)
    }
}

UnitWalk : UGen {
    *ar { |phase, step = 0.2, interp = 0|
        ^this.multiNew('audio', phase, step, interp)
    }
}

UnitRegisterUgen : MultiOutUGen {
    *ar { |phase, chance, length, rotate, interp = 0, reset = 0|
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

UnitRegister {
	*ar { |phase, chance, length, rotate, interp = 0, reset = 0|
		var register = UnitRegisterUgen.ar(phase, chance, length, rotate, interp, reset);
		^(
			bit3: register[0],
			bit8: register[1]
		);
	}
}