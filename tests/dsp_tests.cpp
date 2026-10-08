// Exercise the actual plugin implementation, including SC input-rate dispatch.
#include "../src/PerlinNoise.cpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void near(double actual, double expected, double tolerance, const char* message)
{
    require(std::isfinite(actual) && std::abs(actual - expected) <= tolerance, message);
}

struct Fixture {
    static constexpr int blockSize = 64;
    PerlinNoise3D unit {};
    Rate rate {};
    std::array<Wire, 9> wires {};
    std::array<Wire*, 9> wirePtrs {};
    std::array<std::array<float, blockSize>, 9> inputs {};
    std::array<float*, 9> inputPtrs {};
    std::array<float, blockSize> output {};
    float* outputPtr = output.data();

    explicit Fixture(double sampleRate = 48000.0)
    {
        rate.mSampleRate = sampleRate;
        for (int i = 0; i < 9; ++i) {
            wires[i].mCalcRate = calc_ScalarRate;
            wirePtrs[i] = &wires[i];
            inputPtrs[i] = inputs[i].data();
        }
        unit.mInput = wirePtrs.data();
        unit.mInBuf = inputPtrs.data();
        unit.mOutBuf = &outputPtr;
        unit.mRate = &rate;
        inputs[Input3DXFreq][0] = 110.0f;
        inputs[Input3DXPhase][0] = 0.125f;
        inputs[Input3DYPhase][0] = 0.375f;
        inputs[Input3DZPhase][0] = 0.625f;
        inputs[Input3DOctaves][0] = 6.0f;
        inputs[Input3DPersistence][0] = 0.5f;
        inputs[Input3DSeed][0] = 12.0f;
    }

    void init() { PerlinNoise3D_Ctor(&unit); }
    void next(int count = blockSize) { PerlinNoise3D_next(&unit, count); }
};

void fieldProperties()
{
    double seedDifference = 0.0;
    double zDifference = 0.0;
    for (uint32_t seed = 0; seed < 12; ++seed) {
        for (int octave = 0; octave < 32; ++octave) {
            const uint64_t period = 1ull << octave;
            const double size = static_cast<double>(period);
            const auto value = perlin3D(0.125, 0.375, 0.625, seed, period);
            near(perlin3D(0.125 + size, 0.375, 0.625, seed, period), value, 1e-6, "x periodicity");
            near(perlin3D(0.125, 0.375 - size, 0.625, seed, period), value, 1e-6, "negative y periodicity");
            near(perlin3D(0.125, 0.375, 0.625 + size, seed, period), value, 1e-6, "z periodicity");
            near(perlin3D(size, 0, -size, seed, period), 0, 0, "lattice vertex zero");
        }
        for (int axis = 0; axis < 3; ++axis) {
            double a[3] = {0.125, 0.375, 0.625};
            double b[3] = {0.125, 0.375, 0.625};
            a[axis] = 1e-5;
            b[axis] = 8.0 - 1e-5;
            near(perlin3D(a[0], a[1], a[2], seed, 8),
                 perlin3D(b[0], b[1], b[2], seed, 8), 5e-5, "continuous volume seams");
        }
        for (int i = 0; i < 100; ++i) {
            const double x = i / 100.0;
            for (const int octaves : {1, 6, 32}) {
                for (const float persistence : {0.0f, 0.5f, 1.0f}) {
                    const auto value = fbm3D(x, 0.375, 0.625, octaves, persistence, seed);
                    require(std::isfinite(value) && std::abs(value) <= 1.0f, "bounded finite fBm");
                    near(fbm3D(x, 0.375, 0.625, octaves, persistence, seed), value, 0, "determinism");
                    if (persistence == 0.0f)
                        near(value, fbm3D(x, 0.375, 0.625, 1, 0, seed), 0, "zero persistence removes upper octaves");
                }
            }
            seedDifference += std::abs(fbm3D(x, 0.375, 0.625, 6, 0.5f, seed)
                - fbm3D(x, 0.375, 0.625, 6, 0.5f, seed + 1));
            zDifference += std::abs(fbm3D(x, 0.375, 0.625, 6, 0.5f, seed)
                - fbm3D(x, 0.375, 0.25, 6, 0.5f, seed));
        }
    }
    require(seedDifference > 1.0, "seed changes the field");
    require(zDifference > 1.0, "third coordinate changes the field");
}

void inputRates()
{
    constexpr int indices[] = {Input3DXFreq, Input3DYFreq, Input3DZFreq,
        Input3DOctaves, Input3DPersistence, Input3DSeed};
    constexpr float bases[] = {7000, -4000, 3000, 4, 0.5f, 12};
    constexpr float steps[] = {31, -17, 13, 1, 0.005f, 1};
    for (const double sampleRate : {44100.0, 48000.0, 96000.0}) {
        // Every combination of scalar/control and audio rates for live inputs.
        for (int mask = 0; mask < 64; ++mask) {
            Fixture mixed(sampleRate);
            Fixture expanded(sampleRate);
            for (int j = 0; j < 6; ++j) {
                const int index = indices[j];
                const bool audio = (mask & (1 << j)) != 0;
                mixed.wires[index].mCalcRate = audio ? calc_FullRate : (j % 2 ? calc_BufRate : calc_ScalarRate);
                expanded.wires[index].mCalcRate = calc_FullRate;
                for (int i = 0; i < Fixture::blockSize; ++i) {
                    const auto value = bases[j] + (audio ? steps[j] * (i % 5) : 0);
                    mixed.inputs[index][i] = audio || i == 0 ? value : std::numeric_limits<float>::quiet_NaN();
                    expanded.inputs[index][i] = value;
                }
            }
            mixed.init();
            expanded.init();
            const auto first = mixed.output[0];
            mixed.next();
            expanded.next();
            near(mixed.output[0], first, 0, "constructor must not advance phases");
            for (int i = 0; i < Fixture::blockSize; ++i)
                near(mixed.output[i], expanded.output[i], 0, "mixed-rate dispatch matches expanded inputs");
        }
        for (int axis = 0; axis < 3; ++axis) {
            Fixture moving(sampleRate);
            moving.inputs[Input3DXFreq][0] = 0;
            moving.inputs[axis][0] = static_cast<float>(-sampleRate / 8);
            moving.init();
            moving.next();
            for (int i = 0; i < 8; ++i)
                near(moving.output[i], moving.output[i + 8], 0, "reverse scan repeats every eight samples");
            near(moving.unit.xphase, 0.125, 0, "x phase remains wrapped");
            near(moving.unit.yphase, 0.375, 0, "y phase remains wrapped");
            near(moving.unit.zphase, 0.625, 0, "z phase remains wrapped");
        }
    }
}

void latchAndInitialization()
{
    for (int mask = 1; mask < 8; ++mask) {
        for (const int direction : {-1, 1}) {
            Fixture f;
            for (int axis = 0; axis < 3; ++axis) {
                f.inputs[axis][0] = mask & (1 << axis) ? direction * 1920.0f : 0.0f;
                f.inputs[axis + 3][0] = direction > 0 ? 0.95f : 0.05f;
            }
            f.init();
            f.inputs[Input3DOctaves][0] = 12;
            f.next(1);
            require(f.unit.activeOctaves == 6, "octaves wait for wrap");
            f.next(1);
            require(f.unit.activeOctaves == 12, "all moving axes latch together in either direction");
        }
    }
    Fixture partial;
    partial.inputs[Input3DXFreq][0] = 1920;
    partial.inputs[Input3DYFreq][0] = 1920;
    partial.inputs[Input3DXPhase][0] = 0.99f;
    partial.init();
    partial.inputs[Input3DOctaves][0] = 12;
    partial.next(1);
    require(partial.unit.activeOctaves == 6, "partial wrap must not latch");

    Fixture still;
    still.inputs[Input3DXFreq][0] = 0;
    still.inputs[Input3DXPhase][0] = 1.125f;
    still.inputs[Input3DYPhase][0] = -0.625f;
    still.init();
    const auto initial = still.output[0];
    still.inputs[Input3DOctaves][0] = 12;
    still.inputs[Input3DZPhase][0] = 0.25f;
    still.next();
    require(still.unit.activeOctaves == 6, "stationary path retains pending octaves");
    near(still.unit.xphase, 0.125, 0, "initial x wraps");
    near(still.unit.yphase, 0.375, 0, "initial negative y wraps");
    for (const auto value : still.output)
        near(value, initial, 0, "stationary output constant and phases initialization-only");
}

void invalidInputs()
{
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const auto inf = std::numeric_limits<float>::infinity();
    const auto large = std::numeric_limits<float>::max();
    for (const float value : {nan, inf, -inf, large, -large}) {
        Fixture f;
        for (auto& input : f.inputs)
            input.fill(value);
        f.init();
        f.next();
        for (const auto sample : f.output)
            require(std::isfinite(sample) && std::abs(sample) <= 1.0f, "invalid inputs stay finite and bounded");
    }
    near(clampOctaves(-2), 1, 0, "octaves lower limit");
    near(clampOctaves(99), 32, 0, "octaves upper limit");
    near(clampOctaves(4.5f), 5, 0, "octaves rounding");
    near(clampPersistence(-1), 0, 0, "persistence lower limit");
    near(clampPersistence(2), 1, 0, "persistence upper limit");
    near(clampSignedFreq(large, 48000), 48000 * 0.499, 1e-8, "positive frequency limit");
    near(clampSignedFreq(-large, 48000), -48000 * 0.499, 1e-8, "negative frequency limit");
    require(seedFromFloat(-1) == UINT32_MAX, "negative seed wraps");
    require(seedFromFloat(4294967296.0f) == 0, "large seed wraps");
    require(seedFromFloat(nan) == 0, "non-finite seed sanitization");
    // Existing 1D/2D seed mappings remain unchanged within the old defined range.
    for (int i = -10000; i <= 10000; ++i) {
        const float value = i * 0.25f;
        const auto previous = static_cast<uint32_t>(static_cast<int64_t>(std::floor(value + 0.5f)));
        require(seedFromFloat(value) == previous, "legacy seed compatibility");
    }
}
} // namespace

int main()
{
    fieldProperties();
    inputRates();
    latchAndInitialization();
    invalidInputs();
    std::cout << "PERLIN_NOISE_DSP_OK\n";
}
