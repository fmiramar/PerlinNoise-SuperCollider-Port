PerlinNoise2D : UGen {
	*ar { arg xfreq = 440.0, yfreq = 0.0, xphase = 0.0, yphase = 0.0, octaves = 8, persistence = 0.5, seed = 0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', xfreq, yfreq, xphase, yphase, octaves, persistence, seed).madd(mul, add)
	}
}
