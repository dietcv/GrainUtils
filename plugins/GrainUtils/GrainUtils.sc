// ===== EVENT SCHEDULER =====

EventSchedulerUGen : MultiOutUGen {
	*ar { |triggerRate, reset|
		if(triggerRate.rate != 'audio') { triggerRate = K2A.ar(triggerRate) };
		if(reset.rate != 'audio') { reset = T2A.ar(reset) };
		^this.multiNew('audio', triggerRate, reset)
	}

	init { |... theInputs|
		inputs = theInputs;
		^this.initOutputs(3, rate);
	}

	checkInputs {
		^this.checkValidInputs
	}
}

EventScheduler {
	*ar { |triggerRate, reset|
		var events = EventSchedulerUGen.ar(triggerRate, reset);
		^(
			trigger: events[0],
			rate: events[1],
			subSampleOffset: events[2]
		);
	}
}

// ===== VOICE ALLOCATOR =====

VoiceAllocatorUGen : MultiOutUGen {
	*ar { |numChannels, trig, rate, subSampleOffset|
		if(trig.rate != 'audio') { trig = T2A.ar(trig) };
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
		if(trig.rate != 'audio') { trig = T2A.ar(trig) };
		if(rate.rate != 'audio') { rate = K2A.ar(rate) };
		if(subSampleOffset.rate != 'audio') { subSampleOffset = K2A.ar(subSampleOffset) };
		^this.multiNew('audio', trig, rate, subSampleOffset)
	}

	checkInputs {
		^this.checkValidInputs
	}
}

// ===== HELPER FUNCTIONS =====

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

SCurve : UGen {
	*ar { |phase, shape = 0.5, inflection = 0.5|
		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(shape.rate!='audio'){shape = K2A.ar(shape)};
		if(inflection.rate!='audio'){ inflection = K2A.ar( inflection)};
		^this.multiNew('audio', phase, shape, inflection)
	}
}