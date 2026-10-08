#include "SC_PlugIn.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <new>

static InterfaceTable* ft;

namespace
{
constexpr int maxOctaves = 32;

enum InputIndex
{
    InputFreq = 0,
    InputOctaves,
    InputPersistence,
    InputSeed,
    InputInitialPhase
};

struct PerlinNoise : public Unit
{
    double phase = 0.0;
    float sampleRate = 44100.0f;
    int activeOctaves = 1;
    int pendingOctaves = 1;
};

enum Input2DIndex
{
    Input2DXFreq = 0,
    Input2DYFreq,
    Input2DXPhase,
    Input2DYPhase,
    Input2DOctaves,
    Input2DPersistence,
    Input2DSeed
};

struct PerlinNoise2D : public Unit
{
    double xphase = 0.0;
    double yphase = 0.0;
    float sampleRate = 44100.0f;
    int activeOctaves = 1;
    int pendingOctaves = 1;
};

enum Input3DIndex
{
    Input3DXFreq = 0,
    Input3DYFreq,
    Input3DZFreq,
    Input3DXPhase,
    Input3DYPhase,
    Input3DZPhase,
    Input3DOctaves,
    Input3DPersistence,
    Input3DSeed
};

struct PerlinNoise3D : public Unit
{
    double xphase = 0.0;
    double yphase = 0.0;
    double zphase = 0.0;
    float sampleRate = 44100.0f;
    int activeOctaves = 1;
    int pendingOctaves = 1;
};

inline float inputAt(Unit* unit, int inputIndex, int sampleIndex) noexcept
{
    return INRATE(inputIndex) == calc_FullRate ? IN(inputIndex)[sampleIndex] : IN0(inputIndex);
}

inline int clampOctaves(float value) noexcept
{
    if (!std::isfinite(value))
        return 1;

    return static_cast<int>(std::clamp(std::floor(value + 0.5f), 1.0f, static_cast<float>(maxOctaves)));
}

inline float clampPersistence(float value) noexcept
{
    if (!std::isfinite(value))
        return 0.5f;

    return std::clamp(value, 0.0f, 1.0f);
}

inline float wrapPhase(float value) noexcept
{
    if (!std::isfinite(value))
        return 0.0f;

    value -= std::floor(value);
    return value < 0.0f ? value + 1.0f : value;
}

inline uint32_t seedFromFloat(float value) noexcept
{
    if (!std::isfinite(value))
        return 0u;

    // Reduce before converting so even very large finite float seeds are defined.
    const auto asInt = static_cast<int64_t>(std::fmod(static_cast<double>(std::floor(value + 0.5f)), 4294967296.0));
    return static_cast<uint32_t>(asInt);
}

inline uint64_t splitmix64(uint64_t x) noexcept
{
    x += 0x9e3779b97f4a7c15ull;
    x = (x ^ (x >> 30u)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27u)) * 0x94d049bb133111ebull;
    return x ^ (x >> 31u);
}

inline float gradient(uint64_t latticeIndex, uint32_t seed, uint64_t period) noexcept
{
    const auto wrappedIndex = period > 0 ? latticeIndex % period : latticeIndex;
    const auto hash = splitmix64(wrappedIndex ^ (static_cast<uint64_t>(seed) * 0x9e3779b185ebca87ull));
    const auto unit = static_cast<float>((hash >> 40u) & 0xffffffu) * (1.0f / 16777215.0f);
    return unit * 2.0f - 1.0f;
}

inline uint64_t wrapIndex(int64_t value, uint64_t period) noexcept
{
    if (period == 0)
        return static_cast<uint64_t>(value);

    auto wrapped = value % static_cast<int64_t>(period);
    if (wrapped < 0)
        wrapped += static_cast<int64_t>(period);

    return static_cast<uint64_t>(wrapped);
}

inline void gradient2D(int64_t xIndex, int64_t yIndex, uint32_t seed, uint64_t period, float& gx, float& gy) noexcept
{
    const auto x = wrapIndex(xIndex, period);
    const auto y = wrapIndex(yIndex, period);
    const auto mixed = x * 0x9e3779b185ebca87ull ^ y * 0xc2b2ae3d27d4eb4full ^ static_cast<uint64_t>(seed) * 0x165667b19e3779f9ull;
    const auto hash = splitmix64(mixed);
    const auto angleUnit = static_cast<float>((hash >> 40u) & 0xffffffu) * (1.0f / 16777216.0f);
    const auto angle = angleUnit * 6.28318530717958647692f;

    gx = std::cos(angle);
    gy = std::sin(angle);
}

inline float fade(float t) noexcept
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

inline float lerp(float a, float b, float x) noexcept
{
    return a + x * (b - a);
}

inline float perlin1D(double x, uint32_t seed, uint64_t period) noexcept
{
    const auto xFloor = std::floor(x);
    const auto xi = static_cast<uint64_t>(xFloor);
    const auto xf = static_cast<float>(x - xFloor);

    const auto g0 = gradient(xi, seed, period);
    const auto g1 = gradient(xi + 1u, seed, period);
    const auto d0 = xf;
    const auto d1 = xf - 1.0f;

    return lerp(g0 * d0, g1 * d1, fade(xf));
}

inline float perlin2D(double x, double y, uint32_t seed, uint64_t period) noexcept
{
    const auto xFloor = std::floor(x);
    const auto yFloor = std::floor(y);
    const auto xi = static_cast<int64_t>(xFloor);
    const auto yi = static_cast<int64_t>(yFloor);
    const auto xf = static_cast<float>(x - xFloor);
    const auto yf = static_cast<float>(y - yFloor);

    float gx00 = 0.0f;
    float gy00 = 0.0f;
    float gx10 = 0.0f;
    float gy10 = 0.0f;
    float gx01 = 0.0f;
    float gy01 = 0.0f;
    float gx11 = 0.0f;
    float gy11 = 0.0f;

    gradient2D(xi, yi, seed, period, gx00, gy00);
    gradient2D(xi + 1, yi, seed, period, gx10, gy10);
    gradient2D(xi, yi + 1, seed, period, gx01, gy01);
    gradient2D(xi + 1, yi + 1, seed, period, gx11, gy11);

    const auto v00 = gx00 * xf + gy00 * yf;
    const auto v10 = gx10 * (xf - 1.0f) + gy10 * yf;
    const auto v01 = gx01 * xf + gy01 * (yf - 1.0f);
    const auto v11 = gx11 * (xf - 1.0f) + gy11 * (yf - 1.0f);

    const auto u = fade(xf);
    const auto v = fade(yf);
    return lerp(lerp(v00, v10, u), lerp(v01, v11, u), v);
}

inline float gradientDot3D(int64_t xi, int64_t yi, int64_t zi, uint32_t seed, uint64_t period,
                          float dx, float dy, float dz) noexcept
{
    // The twelve cube-edge directions used by improved Perlin noise, normalized.
    // Seeded coordinate hashing replaces the reference's fixed permutation table.
    constexpr int gradients[12][3] = {
        {1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0},
        {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
        {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}
    };
    const auto mixed = wrapIndex(xi, period) * 0x9e3779b185ebca87ull
        ^ wrapIndex(yi, period) * 0xc2b2ae3d27d4eb4full
        ^ wrapIndex(zi, period) * 0x27d4eb2f165667c5ull
        ^ static_cast<uint64_t>(seed) * 0x165667b19e3779f9ull;
    const auto& g = gradients[splitmix64(mixed) % 12u];
    return (g[0] * dx + g[1] * dy + g[2] * dz) * 0.7071067811865475244f;
}

inline float perlin3D(double x, double y, double z, uint32_t seed, uint64_t period) noexcept
{
    const auto xFloor = std::floor(x);
    const auto yFloor = std::floor(y);
    const auto zFloor = std::floor(z);
    const auto xi = static_cast<int64_t>(xFloor);
    const auto yi = static_cast<int64_t>(yFloor);
    const auto zi = static_cast<int64_t>(zFloor);
    const auto xf = static_cast<float>(x - xFloor);
    const auto yf = static_cast<float>(y - yFloor);
    const auto zf = static_cast<float>(z - zFloor);
    const auto u = fade(xf);
    const auto v = fade(yf);
    const auto w = fade(zf);
    float planes[2];

    for (int dz = 0; dz < 2; ++dz) {
        float rows[2];
        for (int dy = 0; dy < 2; ++dy) {
            const auto left = gradientDot3D(xi, yi + dy, zi + dz, seed, period, xf, yf - dy, zf - dz);
            const auto right = gradientDot3D(xi + 1, yi + dy, zi + dz, seed, period, xf - 1.0f, yf - dy, zf - dz);
            rows[dy] = lerp(left, right, u);
        }
        planes[dz] = lerp(rows[0], rows[1], v);
    }
    return lerp(planes[0], planes[1], w);
}

inline float fbm(double phase, int octaves, float persistence, uint32_t seed) noexcept
{
    float sum = 0.0f;
    float amp = 1.0f;
    float ampSum = 0.0f;

    for (int octave = 0; octave < octaves; ++octave) {
        const auto period = 1ull << octave;
        const auto octaveSeed = seed + static_cast<uint32_t>(octave * 0x68bc21ebu);
        sum += amp * perlin1D(phase * static_cast<double>(period), octaveSeed, period);
        ampSum += amp;
        amp *= persistence;
    }

    if (ampSum <= 0.0f)
        return 0.0f;

    // 1D Perlin has a limited peak range; this keeps typical fBm near SC oscillator levels.
    return std::clamp((sum / ampSum) * 2.0f, -1.0f, 1.0f);
}

inline float fbm2D(double xphase, double yphase, int octaves, float persistence, uint32_t seed) noexcept
{
    float sum = 0.0f;
    float amp = 1.0f;
    float ampSum = 0.0f;

    for (int octave = 0; octave < octaves; ++octave) {
        const auto period = 1ull << octave;
        const auto octaveSeed = seed + static_cast<uint32_t>(octave * 0x68bc21ebu);
        const auto scale = static_cast<double>(period);
        sum += amp * perlin2D(xphase * scale, yphase * scale, octaveSeed, period);
        ampSum += amp;
        amp *= persistence;
    }

    if (ampSum <= 0.0f)
        return 0.0f;

    return std::clamp((sum / ampSum) * 1.6f, -1.0f, 1.0f);
}

inline float fbm3D(double xphase, double yphase, double zphase, int octaves, float persistence, uint32_t seed) noexcept
{
    float sum = 0.0f;
    float amp = 1.0f;
    float ampSum = 0.0f;

    for (int octave = 0; octave < octaves; ++octave) {
        const auto period = 1ull << octave;
        const auto octaveSeed = seed + static_cast<uint32_t>(octave * 0x68bc21ebu);
        const auto scale = static_cast<double>(period);
        sum += amp * perlin3D(xphase * scale, yphase * scale, zphase * scale, octaveSeed, period);
        ampSum += amp;
        amp *= persistence;
    }

    return ampSum > 0.0f ? std::clamp((sum / ampSum) * 1.6f, -1.0f, 1.0f) : 0.0f;
}

inline double clampSignedFreq(float value, double sampleRate) noexcept
{
    auto freq = static_cast<double>(value);
    if (!std::isfinite(freq))
        return 0.0;

    return std::clamp(freq, -sampleRate * 0.499, sampleRate * 0.499);
}

void PerlinNoise_next(PerlinNoise* unit, int inNumSamples)
{
    auto* out = OUT(0);
    auto phase = unit->phase;
    auto activeOctaves = unit->activeOctaves;
    auto pendingOctaves = unit->pendingOctaves;
    const auto sampleRate = static_cast<double>(unit->sampleRate);

    const auto octavesControl = INRATE(InputOctaves) == calc_FullRate;
    const auto persistenceControl = INRATE(InputPersistence) == calc_FullRate;
    const auto seedControl = INRATE(InputSeed) == calc_FullRate;

    const auto octaves0 = clampOctaves(IN0(InputOctaves));
    const auto persistence0 = clampPersistence(IN0(InputPersistence));
    const auto seed0 = seedFromFloat(IN0(InputSeed));

    for (int i = 0; i < inNumSamples; ++i) {
        auto freq = static_cast<double>(inputAt(unit, InputFreq, i));
        if (!std::isfinite(freq))
            freq = 0.0;
        freq = std::clamp(freq, 0.0, sampleRate * 0.499);
        pendingOctaves = octavesControl ? clampOctaves(IN(InputOctaves)[i]) : octaves0;
        const auto persistence = persistenceControl ? clampPersistence(IN(InputPersistence)[i]) : persistence0;
        const auto seed = seedControl ? seedFromFloat(IN(InputSeed)[i]) : seed0;

        out[i] = fbm(phase, activeOctaves, persistence, seed);

        phase += freq / sampleRate;
        if (phase >= 1.0) {
            phase -= std::floor(phase);
            activeOctaves = pendingOctaves;
        }
    }

    unit->phase = phase;
    unit->activeOctaves = activeOctaves;
    unit->pendingOctaves = pendingOctaves;
}

void PerlinNoise_Ctor(PerlinNoise* unit)
{
    new (unit) PerlinNoise;
    unit->sampleRate = static_cast<float>(SAMPLERATE);
    unit->phase = wrapPhase(IN0(InputInitialPhase));
    unit->activeOctaves = clampOctaves(IN0(InputOctaves));
    unit->pendingOctaves = unit->activeOctaves;

    SETCALC(PerlinNoise_next);
    OUT0(0) = fbm(unit->phase, unit->activeOctaves, clampPersistence(IN0(InputPersistence)), seedFromFloat(IN0(InputSeed)));
}

void PerlinNoise2D_next(PerlinNoise2D* unit, int inNumSamples)
{
    auto* out = OUT(0);
    auto xphase = unit->xphase;
    auto yphase = unit->yphase;
    auto activeOctaves = unit->activeOctaves;
    auto pendingOctaves = unit->pendingOctaves;
    const auto sampleRate = static_cast<double>(unit->sampleRate);

    const auto octavesControl = INRATE(Input2DOctaves) == calc_FullRate;
    const auto persistenceControl = INRATE(Input2DPersistence) == calc_FullRate;
    const auto seedControl = INRATE(Input2DSeed) == calc_FullRate;

    const auto octaves0 = clampOctaves(IN0(Input2DOctaves));
    const auto persistence0 = clampPersistence(IN0(Input2DPersistence));
    const auto seed0 = seedFromFloat(IN0(Input2DSeed));

    for (int i = 0; i < inNumSamples; ++i) {
        const auto xfreq = clampSignedFreq(inputAt(unit, Input2DXFreq, i), sampleRate);
        const auto yfreq = clampSignedFreq(inputAt(unit, Input2DYFreq, i), sampleRate);
        pendingOctaves = octavesControl ? clampOctaves(IN(Input2DOctaves)[i]) : octaves0;
        const auto persistence = persistenceControl ? clampPersistence(IN(Input2DPersistence)[i]) : persistence0;
        const auto seed = seedControl ? seedFromFloat(IN(Input2DSeed)[i]) : seed0;

        out[i] = fbm2D(xphase, yphase, activeOctaves, persistence, seed);

        xphase += xfreq / sampleRate;
        yphase += yfreq / sampleRate;

        const auto xWrapped = xphase >= 1.0 || xphase < 0.0;
        const auto yWrapped = yphase >= 1.0 || yphase < 0.0;

        if (xWrapped)
            xphase -= std::floor(xphase);

        if (yWrapped)
            yphase -= std::floor(yphase);

        if ((xWrapped && std::abs(yfreq) < 1.0e-12) || (xWrapped && yWrapped))
            activeOctaves = pendingOctaves;
    }

    unit->xphase = xphase;
    unit->yphase = yphase;
    unit->activeOctaves = activeOctaves;
    unit->pendingOctaves = pendingOctaves;
}

void PerlinNoise2D_Ctor(PerlinNoise2D* unit)
{
    new (unit) PerlinNoise2D;
    unit->sampleRate = static_cast<float>(SAMPLERATE);
    unit->xphase = wrapPhase(IN0(Input2DXPhase));
    unit->yphase = wrapPhase(IN0(Input2DYPhase));
    unit->activeOctaves = clampOctaves(IN0(Input2DOctaves));
    unit->pendingOctaves = unit->activeOctaves;

    SETCALC(PerlinNoise2D_next);
    OUT0(0) = fbm2D(unit->xphase, unit->yphase, unit->activeOctaves, clampPersistence(IN0(Input2DPersistence)), seedFromFloat(IN0(Input2DSeed)));
}

void PerlinNoise3D_next(PerlinNoise3D* unit, int inNumSamples)
{
    auto* out = OUT(0);
    auto xphase = unit->xphase;
    auto yphase = unit->yphase;
    auto zphase = unit->zphase;
    auto activeOctaves = unit->activeOctaves;
    auto pendingOctaves = unit->pendingOctaves;
    const auto sampleRate = static_cast<double>(unit->sampleRate);
    const auto octavesAudio = INRATE(Input3DOctaves) == calc_FullRate;
    const auto persistenceAudio = INRATE(Input3DPersistence) == calc_FullRate;
    const auto seedAudio = INRATE(Input3DSeed) == calc_FullRate;
    const auto octaves0 = clampOctaves(IN0(Input3DOctaves));
    const auto persistence0 = clampPersistence(IN0(Input3DPersistence));
    const auto seed0 = seedFromFloat(IN0(Input3DSeed));

    for (int i = 0; i < inNumSamples; ++i) {
        const auto xfreq = clampSignedFreq(inputAt(unit, Input3DXFreq, i), sampleRate);
        const auto yfreq = clampSignedFreq(inputAt(unit, Input3DYFreq, i), sampleRate);
        const auto zfreq = clampSignedFreq(inputAt(unit, Input3DZFreq, i), sampleRate);
        pendingOctaves = octavesAudio ? clampOctaves(IN(Input3DOctaves)[i]) : octaves0;
        const auto persistence = persistenceAudio ? clampPersistence(IN(Input3DPersistence)[i]) : persistence0;
        const auto seed = seedAudio ? seedFromFloat(IN(Input3DSeed)[i]) : seed0;

        out[i] = fbm3D(xphase, yphase, zphase, activeOctaves, persistence, seed);

        xphase += xfreq / sampleRate;
        yphase += yfreq / sampleRate;
        zphase += zfreq / sampleRate;
        const auto xWrapped = xphase >= 1.0 || xphase < 0.0;
        const auto yWrapped = yphase >= 1.0 || yphase < 0.0;
        const auto zWrapped = zphase >= 1.0 || zphase < 0.0;
        if (xWrapped)
            xphase -= std::floor(xphase);
        if (yWrapped)
            yphase -= std::floor(yphase);
        if (zWrapped)
            zphase -= std::floor(zphase);

        // Apply at a shared wrap of every moving axis; static axes are ignored.
        // Such boundaries need not be zero crossings for arbitrary fixed slices.
        if ((xWrapped || yWrapped || zWrapped)
            && (xWrapped || xfreq == 0.0)
            && (yWrapped || yfreq == 0.0)
            && (zWrapped || zfreq == 0.0))
            activeOctaves = pendingOctaves;
    }

    unit->xphase = xphase;
    unit->yphase = yphase;
    unit->zphase = zphase;
    unit->activeOctaves = activeOctaves;
    unit->pendingOctaves = pendingOctaves;
}

void PerlinNoise3D_Ctor(PerlinNoise3D* unit)
{
    new (unit) PerlinNoise3D;
    unit->sampleRate = static_cast<float>(SAMPLERATE);
    unit->xphase = wrapPhase(IN0(Input3DXPhase));
    unit->yphase = wrapPhase(IN0(Input3DYPhase));
    unit->zphase = wrapPhase(IN0(Input3DZPhase));
    unit->activeOctaves = clampOctaves(IN0(Input3DOctaves));
    unit->pendingOctaves = unit->activeOctaves;

    SETCALC(PerlinNoise3D_next);
    OUT0(0) = fbm3D(unit->xphase, unit->yphase, unit->zphase, unit->activeOctaves,
                   clampPersistence(IN0(Input3DPersistence)), seedFromFloat(IN0(Input3DSeed)));
}
} // namespace

PluginLoad(PerlinNoise)
{
    ft = inTable;
    DefineSimpleUnit(PerlinNoise);
    DefineSimpleUnit(PerlinNoise2D);
    DefineSimpleUnit(PerlinNoise3D);
}
