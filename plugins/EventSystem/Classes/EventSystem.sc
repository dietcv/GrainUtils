// ===== SCHEDULER CYCLE =====

SchedulerCycleUGen : MultiOutUGen {
	*ar { |rate, reset = 0|
		^this.multiNew('audio', rate, reset)
	}

	init { |... theInputs|
		inputs = theInputs;
		^this.initOutputs(4, rate);
	}

	checkInputs {
		^this.checkValidInputs
	}
}

SchedulerCycle {
	*ar { |rate, reset = 0|
		var events = SchedulerCycleUGen.ar(rate, reset);
		^(
			trigger: events[0],
			rate: events[1],
			subSampleOffset: events[2],
			phase: events[3]
		);
	}
}

// ===== SCHEDULER BURST =====

SchedulerBurstUGen : MultiOutUGen {
	*ar { |trig, duration, cycles = 1|
		^this.multiNew('audio', trig, duration, cycles)
	}

	init { |... theInputs|
		inputs = theInputs;
		^this.initOutputs(4, rate);
	}

	checkInputs {
		^this.checkValidInputs
	}
}

SchedulerBurst {
	*ar { |trig, duration, cycles = 1|
		var events = SchedulerBurstUGen.ar(trig, duration, cycles);
		^(
			trigger: events[0],
			rate: events[1],
			subSampleOffset: events[2],
			phase: events[3]
		);
	}
}

// ===== SCHEDULER BANK =====

SchedulerBankUGen : MultiOutUGen {
    *ar { |numChannels, rate, spread = 0, couple = 0, bias = 0, reset = 0|
        ^this.multiNew(\audio, numChannels, rate, spread, couple, bias, reset);
    }

    init { |...theInputs|
        inputs = theInputs;
        ^this.initOutputs(inputs[0] * 4, rate);
    }

	checkInputs {
		^this.checkValidInputs
	}
}

SchedulerBank {
    *ar { |numChannels, rate, spread = 0, couple = 0, bias = 0, reset = 0|
        var events = SchedulerBankUGen.ar(numChannels, rate, spread, couple, bias, reset);
        ^(
			triggers:         events[0..numChannels - 1],
            rates:            events[numChannels..(2 * numChannels) - 1],
            subSampleOffsets: events[(2 * numChannels)..(3 * numChannels) - 1],
            phases:           events[(3 * numChannels)..(4 * numChannels) - 1]
        );
    }
}

// ===== VOICE ALLOCATOR =====

VoiceAllocatorUGen : MultiOutUGen {
	*ar { |numChannels, trig, rate, subSampleOffset|
		^this.multiNew('audio', numChannels, trig, rate, subSampleOffset)
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(inputs[0] * 2, rate);
	}

	checkInputs {
		^this.checkValidInputs
	}
}

VoiceAllocator {
	*ar { |numChannels, trig, rate, subSampleOffset|
		var voices = VoiceAllocatorUGen.ar(numChannels, trig, rate, subSampleOffset);
		^(
			phases:   voices[0..numChannels - 1],
			triggers: voices[numChannels..numChannels * 2 - 1]
		);
	}
}

// ===== RAMP INTEGRATOR =====

RampIntegrator : UGen {
	*ar { |trig, rate, subSampleOffset|
		^this.multiNew('audio', trig, rate, subSampleOffset)
	}

	checkInputs {
		^this.checkValidInputs
	}
}

// ===== RAMP ACCUMULATOR =====

RampAccumulator : UGen {
	*ar { |trig, subSampleOffset|
		^this.multiNew('audio', trig, subSampleOffset)
	}

	checkInputs {
		^this.checkValidInputs
	}
}

// ===== RAMP DIVIDER =====

RampDivider : UGen {
	*ar { |phase, ratio = 1, reset = 0, mode = 1|
		^this.multiNew('audio', phase, ratio, reset, mode)
	}

	checkInputs {
		^this.checkValidInputs
	}
}