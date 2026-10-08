(
var path = thisProcess.argv[0];
var file = SoundFile.new;
var data, channels, activeFrames;
var check = { |condition, message|
	if(condition.not) { ("FAIL: " ++ message).postln; 1.exit };
};
check.(file.openRead(path), "cannot open rendered audio");
check.(file.numChannels == 13, "expected thirteen diagnostic channels");
check.(file.numFrames >= file.sampleRate, "render too short");
activeFrames = (file.sampleRate * 0.9).asInteger;
data = FloatArray.newClear(file.numFrames * file.numChannels);
file.readData(data);
file.close;
check.(data.every { |value| value.isNaN.not and: { value.abs <= 0.10001 } }, "finite bounded audio");
channels = Array.fill(13, { |channel|
	FloatArray.fill(activeFrames, { |frame| data[(frame * 13) + channel] })
});
12.do { |channel|
	check.(channels[channel].abs.maxItem > 0.001, "silent channel " ++ channel);
};
check.((channels[2] - channels[3]).abs.maxItem == 0, "seeded instances must match exactly");
check.((channels[2] - channels[4]).abs.maxItem > 0.001, "seed must affect output");
check.((channels[2] - channels[5]).abs.maxItem > 0.001, "z coordinate must affect output");
check.((channels[10] - channels[11]).abs.maxItem > 0.001, "multichannel slices must differ");
check.(channels[12].abs.maxItem == 0, "stationary lattice vertex must be zero");
128.do { |i|
	check.(channels[2][i] == channels[2][i + 64], "3D waveform must repeat every 64 samples");
};
("PERLIN_NOISE_AUDIO_OK " ++ path).postln;
0.exit;
)
