Durn : UGen {
    *new { |chance = 0, size = 8, length = inf|
        ^this.multiNew('demand', chance, size, length)
    }
}