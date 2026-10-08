(
var path = thisProcess.argv[0];
var parsed = SCDoc.parseFileFull(path);

if(parsed.isNil) {
	("SCDOC_PARSE_FAILED " ++ path).postln;
	1.exit;
} {
	("SCDOC_PARSE_OK " ++ path).postln;
	0.exit;
};
)
