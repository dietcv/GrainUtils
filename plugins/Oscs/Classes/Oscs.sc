// ===== DUAL WAVETABLE OSCILLATOR =====

DualOscOS : MultiOutUGen {
	*ar { |bufnumA, phaseA, numCyclesA = 1, cyclePosA = 0,
		  bufnumB, phaseB, numCyclesB = 1, cyclePosB = 0,
		  xmIndexA = 0, xmIndexB = 0,
		  xmFltRatioA = 1, xmFltRatioB = 1,
		  oversample = 0|

		// Validate buffers
		if(bufnumA.isNil) { Error("DualOscOS: Invalid buffer A").throw };
		if(bufnumB.isNil) { Error("DualOscOS: Invalid buffer B").throw };

		^this.multiNew('audio',
			bufnumA, phaseA, numCyclesA, cyclePosA,
			bufnumB, phaseB, numCyclesB, cyclePosB,
			xmIndexA, xmIndexB, xmFltRatioA, xmFltRatioB,
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

PulsarOS : UGen {
	*ar { |trig, triggerFreq, subSampleOffset = 0,
		  oscFreq = 440, modFreq = 0, modIndex = 0,
		  oscBuffer, oscNumCycles = 1, oscCyclePos = 0,
		  envBuffer, envNumCycles = 1, envCyclePos = 0,
		  modBuffer, modNumCycles = 1, modCyclePos = 0,
		  oversample = 0|

		if(oscBuffer.isNil) { Error("PulsarOS: Invalid osc buffer").throw };
		if(envBuffer.isNil) { Error("PulsarOS: Invalid env buffer").throw };
		if(modBuffer.isNil) { Error("PulsarOS: Invalid mod buffer").throw };

		^this.multiNew('audio',
			trig, triggerFreq, subSampleOffset,
			oscFreq, modFreq, modIndex,
			oscBuffer, oscNumCycles, oscCyclePos,
			envBuffer, envNumCycles, envCyclePos,
			modBuffer, modNumCycles, modCyclePos,
			oversample)
	}
}

// ===== DUAL PULSAR OSCILLATOR =====

DualPulsarOS : UGen {
	*ar { |trig, triggerFreq, subSampleOffset = 0,
		  oscFreq = 440, modFreq = 440,
		  oscXmIndex = 0, modXmIndex = 0,
		  oscXmFltRatio = 1, modXmFltRatio = 1,
		  oscWarp = 0.5, modWarp = 0.5,
		  oscBuffer, oscNumCycles = 1, oscCyclePos = 0,
		  modBuffer, modNumCycles = 1, modCyclePos = 0,
		  envSkew = 0.5, envIndex = 0,
		  oversample = 0|

		if(oscBuffer.isNil) { Error("DualPulsarOS: Invalid osc buffer").throw };
		if(modBuffer.isNil) { Error("DualPulsarOS: Invalid mod buffer").throw };
		
		^this.multiNew('audio',
			trig, triggerFreq, subSampleOffset,
			oscFreq, modFreq,
			oscXmIndex, modXmIndex,
			oscXmFltRatio, modXmFltRatio,
			oscWarp, modWarp,
			oscBuffer, oscNumCycles, oscCyclePos,
			modBuffer, modNumCycles, modCyclePos,
			envSkew, envIndex,
			oversample)
	}
}
