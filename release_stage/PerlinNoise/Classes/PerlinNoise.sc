PerlinNoise : UGen {
	*ar { arg freq = 440.0, octaves = 8, persistence = 0.5, seed = 0, iphase = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', freq, octaves, persistence, seed, iphase).madd(mul, add)
	}
}
