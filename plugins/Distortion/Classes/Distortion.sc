// ===== BUCHLA 259 WAVEFOLDER =====

BuchlaFold : UGen {
	*ar { |input, drive = 0, oversample = 0|
		^this.multiNew('audio', input, drive, oversample)
	}
}

// ===== SERGE WAVEFOLDER =====

SergeFold : UGen {
    *ar { |input, drive = 0, oversample = 0|
        ^this.multiNew('audio', input, drive, oversample)
    }
}