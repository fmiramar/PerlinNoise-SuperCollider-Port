(
var pages = ["Classes/PerlinNoise", "Classes/PerlinNoise2D", "Classes/PerlinNoise3D", "Guides/PerlinNoisePorts"];
var target = thisProcess.argv[0];
var source;
var check = { |condition, message|
	if(condition.not) { ("FAIL: " ++ message).postln; 1.exit };
};
if(target.notNil) { SCDoc.helpTargetDir = target };
SCDoc.verbosity = 2;
SCDoc.indexAllDocuments(true);
pages.do { |path|
	var document = SCDoc.documents[path];
	var tree;
	check.(document.notNil, "missing help " ++ path);
	tree = SCDoc.parseDoc(document);
	check.(tree.notNil, "cannot parse " ++ path);
	SCDoc.renderer.renderToFile(document.destPath, document, tree);
	check.(File.exists(document.destPath), "cannot render " ++ path);
	("RENDERED " ++ path).postln;
};
// Compile the actual examples from the installed 3D help without playing audio.
source = File.readAllString(SCDoc.documents["Classes/PerlinNoise3D"].fullPath);
source.findAll("code::\n").do { |offset|
	var start = offset + 7;
	var end = source.find("\n::", offset: start);
	var example = source.copyRange(start, end - 1);
	check.(example.compile.notNil, "3D help example did not compile");
};
"PERLIN_NOISE_SCDOC_OK".postln;
0.exit;
)
