// ===== WAVETABLE OSCILLATOR WITH CROSS-PHASE MODULATION =====

OscXPM : UGen {
	*ar { |oscPhase, modPhase, 
	      oscPmIndex = 0, modPmIndex = 0,
		  oscPmDamping = 1, modPmDamping = 1,
		  oscBufNum, oscNumCycles = 1, oscCyclePos = 0,
		  modBufNum, modNumCycles = 1, modCyclePos = 0,  
		  oversample = 0|

		if(oscBufNum.isNil) { Error("OscXPM: Invalid osc buffer").throw };
		if(modBufNum.isNil) { Error("OscXPM: Invalid mod buffer").throw };

		^this.multiNew('audio',
			oscPhase, modPhase, 
			oscPmIndex, modPmIndex,
			oscPmDamping, modPmDamping,
			oscBufNum, oscNumCycles, oscCyclePos,
			modBufNum, modNumCycles, modCyclePos,
			oversample)
	}
}

// ===== PULSAR OSCILLATOR WITH CROSS-PHASE MODULATION =====

PulsarXPM : UGen {
	*ar { |trig, triggerFreq, subSampleOffset = 0,
		  oscFreq = 440, modFreq = 440,
		  oscPmIndex = 0, modPmIndex = 0,
		  oscPmDamping = 1, modPmDamping = 1,
		  oscBufNum, oscNumCycles = 1, oscCyclePos = 0,
		  modBufNum, modNumCycles = 1, modCyclePos = 0,
		  envBufNum, envNumCycles = 1, envCyclePos = 0,
		  oversample = 0|

		if(oscBufNum.isNil) { Error("PulsarXPM: Invalid osc buffer").throw };
		if(modBufNum.isNil) { Error("PulsarXPM: Invalid mod buffer").throw };
		if(envBufNum.isNil) { Error("PulsarXPM: Invalid env buffer").throw };

		^this.multiNew('audio',
			trig, triggerFreq, subSampleOffset,
			oscFreq, modFreq,
			oscPmIndex, modPmIndex,
			oscPmDamping, modPmDamping,
			oscBufNum, oscNumCycles, oscCyclePos,
			modBufNum, modNumCycles, modCyclePos,
			envBufNum, envNumCycles, envCyclePos,
			oversample)
	}
}