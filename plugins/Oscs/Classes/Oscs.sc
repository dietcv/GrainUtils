// ===== DUAL WAVETABLE OSCILLATOR =====

DualOscOS : MultiOutUGen {
	*ar { |bufnumA, phaseA, numCyclesA = 1, cyclePosA = 0,
		  bufnumB, phaseB, numCyclesB = 1, cyclePosB = 0,
		  pmIndexA = 0, pmIndexB = 0,
		  pmFilterRatioA = 1, pmFilterRatioB = 1,
		  oversample = 0|

		// Validate buffers
		if(bufnumA.isNil) { "DualOscOS: Invalid buffer A".throw };
		if(bufnumB.isNil) { "DualOscOS: Invalid buffer B".throw };

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
		if(bufnum.isNil) { "SingleOscOS: Invalid buffer".throw };

		^this.multiNew('audio', bufnum, phase, numCycles, cyclePos, oversample)
	}
}
