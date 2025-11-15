UnitTriangle : UGen {
    *ar { |phase, skew = 0.5|
        ^this.multiNew('audio', phase, skew)
    }
}

UnitKink : UGen {
    *ar { |phase, skew = 0.5|
        ^this.multiNew('audio', phase, skew)
    }
}

UnitCubic : UGen {
    *ar { |phase, index = 0|
        ^this.multiNew('audio', phase, index)
    }
}