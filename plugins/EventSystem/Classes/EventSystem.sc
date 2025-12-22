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

// ===== VOICE ALLOCATOR =====

VoiceAllocatorUGen : MultiOutUGen {
	*ar { |numChannels, trig, rate, subSampleOffset|
		^this.multiNew('audio', numChannels, trig, rate, subSampleOffset)
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(inputs[0] * 2, rate);  // inputs[0] is numChannels
	}

	checkInputs {
		^this.checkValidInputs
	}
}

VoiceAllocator {
	*ar { |numChannels, trig, rate, subSampleOffset|
		var voices = VoiceAllocatorUGen.ar(numChannels, trig, rate, subSampleOffset);
		^(
			phases: voices[0..numChannels - 1],
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
	*ar { |phase, ratio, reset = 0, autosync = 1, threshold = 0.01|
		^this.multiNew('audio', phase, ratio, reset, autosync, threshold)
	}

	checkInputs {
		^this.checkValidInputs
	}
}