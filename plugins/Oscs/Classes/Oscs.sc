// ===== DUAL WAVETABLE OSCILLATOR =====

DualOscOS : MultiOutUGen {
	*ar { |bufnumA, phaseA, numCyclesA = 1, cyclePosA = 0,
		  bufnumB, phaseB, numCyclesB = 1, cyclePosB = 0,
		  pmIndexA = 0, pmIndexB = 0,
		  pmFilterRatioA = 1, pmFilterRatioB = 1,
		  oversample = 0|

		// Validate buffers
		if(bufnumA.isNil) { Error("DualOscOS: Invalid buffer A").throw };
		if(bufnumB.isNil) { Error("DualOscOS: Invalid buffer B").throw };

		^this.multiNew('audio',
			bufnumA, phaseA, numCyclesA, cyclePosA,
			bufnumB, phaseB, numCyclesB, cyclePosB,
			pmIndexA, pmIndexB, pmFilterRatioA, pmFilterRatioB,
			oversample)
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}
}

// ===== SINGLE WAVETABLE OSCILLATOR =====

SingleOscOS : UGen {
	*ar { |bufnum, phase, numCycles = 1, cyclePos = 0, oversample = 0|

		// Validate buffer
		if(bufnum.isNil) { Error("SingleOscOS: Invalid buffer").throw };

		^this.multiNew('audio', bufnum, phase, numCycles, cyclePos, oversample)
	}
}

// ===== PULSAR OSCILLATOR =====

PulsarOS : MultiOutUGen {
	*ar { |numChannels = 1,
		  trig, triggerFreq, subSampleOffset = 0,
		  grainFreq = 440, modFreq = 0, modIndex = 0, pan = 0, amp = 1,
		  oscBuffer, oscNumCycles = 1, oscCyclePos = 0,
		  envBuffer, envNumCycles = 1, envCyclePos = 0,
		  modBuffer, modNumCycles = 1, modCyclePos = 0,
		  oversample = 0|

		if(oscBuffer.isNil) { Error("PulsarOS: Invalid osc buffer").throw };
		if(envBuffer.isNil) { Error("PulsarOS: Invalid env buffer").throw };
		if(modBuffer.isNil) { Error("PulsarOS: Invalid mod buffer").throw };

		^this.multiNew('audio',
			numChannels,
			trig, triggerFreq, subSampleOffset,
			grainFreq, modFreq, modIndex, pan, amp,
			oscBuffer, oscNumCycles, oscCyclePos,
			envBuffer, envNumCycles, envCyclePos,
			modBuffer, modNumCycles, modCyclePos,
			oversample)
	}
	
	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(inputs[0], rate);
	}
}

// ===== DUAL PULSAR OSCILLATOR =====

DualPulsarOS : UGen {
	*ar { |trig, triggerFreq, subSampleOffset = 0,
		  oscFreq = 440, modFreq = 440,
		  pmIndexOsc = 0, pmIndexMod = 0,
		  pmFilterRatioOsc = 1, pmFilterRatioMod = 1,
		  warpOsc = 0.5, warpMod = 0.5,
		  oscBuffer, oscNumCycles = 1, oscCyclePos = 0,
		  modBuffer, modNumCycles = 1, modCyclePos = 0,
		  skew = 0.5, index = 0,
		  oversample = 0|

		if(oscBuffer.isNil) { Error("DualPulsarOS: Invalid osc buffer").throw };
		if(modBuffer.isNil) { Error("DualPulsarOS: Invalid mod buffer").throw };
		
		^this.multiNew('audio',
			trig, triggerFreq, subSampleOffset,
			oscFreq, modFreq,
			pmIndexOsc, pmIndexMod,
			pmFilterRatioOsc, pmFilterRatioMod,
			warpOsc, warpMod,
			oscBuffer, oscNumCycles, oscCyclePos,
			modBuffer, modNumCycles, modCyclePos,
			skew, index,
			oversample)
	}
}
