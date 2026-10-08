# PerlinNoise

Standalone SuperCollider UGens for periodic Perlin-noise/fBm oscillator synthesis, based on Artem Popov's [Using Perlin noise in sound synthesis](https://lac.linuxaudio.org/2018/pages/event/14/) (Linux Audio Conference 2018). This established Hodgepodge project keeps the multidimensional adaptations alongside the original oscillator; it uses small standalone C++17 DSP implementations rather than a synthesizer framework.

## UGens

- `PerlinNoise` generates a pitched, seamless 1D gradient-noise waveform. It hashes periodic lattice slopes and sums up to 32 fBm octaves, unlike vanilla `LFNoise1`'s random breakpoints.
- `PerlinNoise2D` scans a periodic 2D noise tile with independent x/y speeds and initial phases. It interpolates four unit-gradient dot products per octave, allowing fixed or moving slices.
- `PerlinNoise3D` scans a periodic 3D noise volume with independent x/y/z speeds and initial phases. It interpolates eight corner dot products using twelve normalized cube-edge gradients, following [Ken Perlin's improved-noise technique](https://cs.nyu.edu/~perlin/noise/) with seeded hashing and octave-dependent periods.

All three expose `.ar`, support multichannel expansion, and produce one channel per instance. “3D” describes the noise coordinates, not spatial audio. Fixing the third coordinate gives a slice of the 3D field, not the same output as `PerlinNoise2D`.

```supercollider
(
{
    var sig = PerlinNoise3D.ar(110, 0.17, 0.11,
        xphase: 0.1, yphase: 0.35, zphase: 0.6,
        octaves: 6, persistence: 0.5, seed: 12);
    var env = EnvGen.kr(Env.linen(0.02, 8, 0.2), doneAction: 2);
    (LeakDC.ar(sig) * env * 0.15) ! 2
}.play;
)
```

## Build and install

Use CMake, a C++17 compiler, and a SuperCollider source checkout containing `tools/cmake_gen`. Configure out of tree with an explicit SDK/source path:

```sh
cmake -S . -B build -DSC_PATH=/path/to/supercollider -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix /path/to/SuperCollider/Extensions
```

The installed `PerlinNoise` extension contains the server plugin, all three classes, and the `PerlinNoisePorts` guide. Recompile the class library and restart the server after installation. Enable `-DSUPERNOVA=ON` to also build the Supernova plugin; on macOS set `-DCMAKE_OSX_ARCHITECTURES=x86_64` or `arm64` explicitly for architecture-specific builds.

`tests/run_smoke.sh /path/to/build /path/to/Extensions/PerlinNoise` checks the installed extension with offline audio renders and SCDoc indexing/rendering. It requires `sclang`, `scsynth`, and `rg` (ripgrep) on PATH; `SC_CLASSLIB` and `SC_PLUGIN_DIR` select an alternate SuperCollider installation.

## Behavior and limitations

- Lacunarity is fixed at 2; octave count is limited to 1–32. Persistence sets successive octave amplitudes and the sum is amplitude-normalized.
- The multidimensional ports accept negative axis speeds. Phases are initialization-only coordinates; a zero speed holds that slice.
- Octave changes wait for wrap boundaries. In 3D, all moving axes must wrap in the same sample; stationary axes are ignored. Unrelated speeds or offsets can leave a change pending indefinitely, and fixed slices need not be zero at a boundary, so changes can still click. The existing 2D latch specifically requires an x wrap, plus a y wrap if y moves.
- Seed and persistence changes are immediate. Use external envelopes or crossfades for smooth structural changes; no envelope is built in.
- There is no oversampling or band limiting. High octaves can alias, and 3D evaluates eight corners per octave, increasing CPU cost. Fixed slices can have DC offset; use `LeakDC` for audio patches.
- This is experimental DSP, not a sample-identical port of the cited implementations. There is no project release CI yet.

## Source and License

The sources above document the synthesis and gradient-noise techniques. The 3D implementation does not copy the reference permutation table or include third-party runtime code; it is an original adaptation of the documented techniques.

This project is licensed under the **GPL-3.0-or-later** license. It links against the SuperCollider plugin API, which is also distributed under the GPL-3.0 license. See the `LICENSE` file for details.
