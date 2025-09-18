ShiftRegisterUgen : MultiOutUGen {
    *ar { |trig, chance, length, rotate, reset = 0|
		if(trig.rate != 'audio') { trig = K2A.ar(trig) };
		if(chance.rate != 'audio') { chance = K2A.ar(chance) };
		if(length.rate != 'audio') { length = K2A.ar(length) };
		if(rotate.rate != 'audio') { rotate = K2A.ar(rotate) };
        ^this.multiNew('audio', trig, chance, length, rotate, reset);
    }

    init { arg ... theInputs;
        inputs = theInputs;
        ^this.initOutputs(2, rate);
    }

    checkInputs {
        ^this.checkValidInputs;
    }
}

ShiftRegister {
	*ar { |trig, chance, length, rotate, reset = 0|
		var register = ShiftRegisterUgen.ar(trig, chance, length, rotate, reset);
		^(
			bit3: register[0],
			bit8: register[1]
		);
	}
}