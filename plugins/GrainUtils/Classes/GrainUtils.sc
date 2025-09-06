// ===== GRAIN DELAY =====

GrainDelay : UGen {
	*ar { |input = 0, triggerRate = 10, overlap = 1, 
		delayTime = 0.2, grainRate = 1.0, mix = 0.5, 
		feedback = 0.3, damping = 0.7, freeze = 0, reset = 0|
		
		if(triggerRate.rate != 'audio') { triggerRate = K2A.ar(triggerRate) };
		if(overlap.rate != 'audio') { overlap = K2A.ar(overlap) };
		if(delayTime.rate != 'audio') { delayTime = K2A.ar(delayTime) };
		if(grainRate.rate != 'audio') { grainRate = K2A.ar(grainRate) };
		
		^this.multiNew('audio', input, triggerRate, overlap, 
			delayTime, grainRate, mix, feedback, damping, freeze, reset)
	}
}

// ===== SCHEDULER CYCLE =====

SchedulerCycleUGen : MultiOutUGen {
	*ar { |rate, reset = 0|
		if(rate.rate != 'audio') { rate = K2A.ar(rate) };
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
		if(trig.rate != 'audio') { trig = K2A.ar(trig) };
		if(duration.rate != 'audio') { duration = K2A.ar(duration) };
		if(cycles.rate != 'audio') { cycles = K2A.ar(cycles) };
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
		if(trig.rate != 'audio') { trig = K2A.ar(trig) };
		if(rate.rate != 'audio') { rate = K2A.ar(rate) };
		if(subSampleOffset.rate != 'audio') { subSampleOffset = K2A.ar(subSampleOffset) };
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
		if(trig.rate != 'audio') { trig = K2A.ar(trig) };
		if(rate.rate != 'audio') { rate = K2A.ar(rate) };
		if(subSampleOffset.rate != 'audio') { subSampleOffset = K2A.ar(subSampleOffset) };
		^this.multiNew('audio', trig, rate, subSampleOffset)
	}

	checkInputs {
		^this.checkValidInputs
	}
}

// ===== RAMP ACCUMULATOR =====

RampAccumulator : UGen {
	*ar { |trig, subSampleOffset|
		if(trig.rate != 'audio') { trig = K2A.ar(trig) };
		if(subSampleOffset.rate != 'audio') { subSampleOffset = K2A.ar(subSampleOffset) };
		^this.multiNew('audio', trig, subSampleOffset)
	}

	checkInputs {
		^this.checkValidInputs
	}
}

// ===== SHIFT REGISTER =====

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

PlanckWindow : UGen {
    *ar { |phase, skew = 0.5, width = 0, index = 0|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(skew.rate!='audio'){skew = K2A.ar(skew)};
		if(width.rate!='audio'){width = K2A.ar(width)};
		if(index.rate!='audio'){index = K2A.ar(index)};
        ^this.multiNew('audio', phase, skew, width, index)
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