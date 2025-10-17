// ===== DUAL WAVETABLE OSCILLATOR =====

DualOscOS : MultiOutUGen {
	*ar { |bufnumA, phaseA, numCyclesA = 1, cyclePosA = 0,
		  bufnumB, phaseB, numCyclesB = 1, cyclePosB = 0,
		  pmIndexA = 0, pmIndexB = 0,
		  pmFilterRatioA = 1, pmFilterRatioB = 1,
		  oversample = 0|

		// Ensure audio rate for phase inputs
		if(phaseA.rate != 'audio') { phaseA = K2A.ar(phaseA) };
		if(phaseB.rate != 'audio') { phaseB = K2A.ar(phaseB) };

		// Ensure audio rate for cyclePos inputs
		if(cyclePosA.rate != 'audio') { cyclePosA = K2A.ar(cyclePosA) };
		if(cyclePosB.rate != 'audio') { cyclePosB = K2A.ar(cyclePosB) };

		// Ensure audio rate for PM parameters
		if(pmIndexA.rate != 'audio') { pmIndexA = K2A.ar(pmIndexA) };
		if(pmIndexB.rate != 'audio') { pmIndexB = K2A.ar(pmIndexB) };
		if(pmFilterRatioA.rate != 'audio') { pmFilterRatioA = K2A.ar(pmFilterRatioA) };
		if(pmFilterRatioB.rate != 'audio') { pmFilterRatioB = K2A.ar(pmFilterRatioB) };

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

		if(phase.rate!='audio'){phase = K2A.ar(phase)};
		if(cyclePos.rate!='audio'){cyclePos = K2A.ar(cyclePos)};
		if(bufnum.isNil) {"Invalid buffer".throw};

		^this.multiNew('audio', bufnum, phase, numCycles, cyclePos, oversample)
	}
}
