(
var oscPath = thisProcess.argv[0];
var score = Score([
	[0.0, [\d_recv, SynthDef(\perlinNoiseSmoke, { |xfreq = 110, yfreq = 0.17, zfreq = -0.11, octaves = 6, persistence = 0.5, seed = 12|
		var periodic = { |seed = 12, zphase = 0.625|
			PerlinNoise3D.ar(SampleRate.ir / 64, 0, 0, 0.125, 0.375, zphase, 6, 0.5, seed)
		};
		var signals = [
			PerlinNoise.ar(220, 8, 0.5, 1234),
			PerlinNoise2D.ar(110, 0.25, 0.1, 0.4, 8, 0.5, 5678),
			periodic.value,
			periodic.value,
			periodic.value(13),
			periodic.value(zphase: 0.25),
			PerlinNoise3D.ar(SinOsc.ar(13).range(90, 150), SinOsc.ar(7) * 0.3,
				SinOsc.ar(11) * 0.2, 0.1, 0.3, 0.6,
				LFPulse.ar(50).range(3, 8), SinOsc.ar(5).range(0.3, 0.7), LFPulse.ar(20).range(12, 14)),
			PerlinNoise3D.ar(0, -90, 0, 0.2, 0.4, 0.6, 6, 0.5, 12),
			PerlinNoise3D.ar(0, 0, -90, 0.2, 0.4, 0.6, 6, 0.5, 12),
			PerlinNoise3D.ar(xfreq, yfreq, zfreq, 0.1, 0.3, 0.6, octaves, persistence, seed)
		] ++ PerlinNoise3D.ar(110, 0.17, 0.11, 0.1, 0.35, [0.2, 0.6], 6, 0.5, 12)
		++ [PerlinNoise3D.ar(0, 0, 0)];
		Out.ar(0, signals * 0.1);
	}).asBytes]],
	[0.0, [\s_new, \perlinNoiseSmoke, 1000, 0, 1]],
	[1.0, [\n_free, 1000]],
	[1.25, [0]]
]);

score.writeOSCFile(oscPath, 0, 1.25);
("WROTE " ++ oscPath).postln;
0.exit;
)
