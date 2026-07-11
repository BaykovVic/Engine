#pragma once

#include <cstdint>

namespace sky::core {

/// Deterministic random number generator (PCG32, O'Neill's minimal variant).
/// The managed SkyEngine.Random implements the exact same algorithm: the same
/// seed produces the same sequence on both sides of the boundary, which is
/// what makes gameplay randomness replayable. Do not "improve" the constants
/// or the seeding procedure without changing both sides and their shared
/// reference-vector tests.
class Pcg32 {
public:
    explicit Pcg32(std::uint64_t seed = 0, std::uint64_t sequence = 54) {
        reseed(seed, sequence);
    }

    void reseed(std::uint64_t seed, std::uint64_t sequence = 54) {
        state_ = 0;
        increment_ = (sequence << 1u) | 1u;
        next();
        state_ += seed;
        next();
    }

    std::uint32_t next() {
        const std::uint64_t old = state_;
        state_ = old * 6364136223846793005ULL + increment_;
        const auto xorshifted =
            static_cast<std::uint32_t>(((old >> 18u) ^ old) >> 27u);
        const auto rot = static_cast<std::uint32_t>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
    }

    /// Uniform float in [0, 1) from the top 24 bits (fits a float mantissa
    /// exactly, so the value is identical to the managed computation).
    float nextFloat() {
        return static_cast<float>(next() >> 8u) * (1.0f / 16777216.0f);
    }

    /// Uniform integer in [minInclusive, maxExclusive). Plain modulo: the
    /// slight bias is irrelevant for gameplay, cross-language identity isn't.
    std::int32_t range(std::int32_t minInclusive, std::int32_t maxExclusive) {
        if (maxExclusive <= minInclusive) {
            return minInclusive;
        }
        const auto span =
            static_cast<std::uint32_t>(maxExclusive - minInclusive);
        return minInclusive + static_cast<std::int32_t>(next() % span);
    }

private:
    std::uint64_t state_ = 0;
    std::uint64_t increment_ = 0;
};

} // namespace sky::core
