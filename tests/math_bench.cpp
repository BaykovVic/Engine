// Manual micro-benchmark of the core math hot operations. Not part of the
// ctest schedule — build target sky_math_bench and run it by hand. Only a
// Release build (-O3) is meaningful; unoptimized numbers are ~10x higher.
//
// Recorded result (GCC 13.3, -O3, x86-64/SSE2 baseline): hand-written SSE2
// intrinsics for these AoS single-value operations were 1.4-2x SLOWER than
// the code the compiler generates from the scalar constexpr formulas
// (lane assembly for the 12-byte Vec3 and store round-trips eat the win).
// That experiment is why sky/core/math.hpp stays scalar; a SIMD attempt
// must beat these numbers first:
//   quat*quat ~1.5 ns/op, rotate ~2.0 ns/op,
//   compose ~4.9 ns/op, invCompose ~4.8 ns/op.

#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

#include "sky/core/math.hpp"

using namespace sky::core;
using Clock = std::chrono::steady_clock;

int main() {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> d(-2.0f, 2.0f);
    const int N = 4096;
    std::vector<Quat> qa(N), qb(N);
    std::vector<Vec3> va(N);
    std::vector<Transform> ta(N), tb(N);
    for (int i = 0; i < N; ++i) {
        float x = d(rng), y = d(rng), z = d(rng), w = d(rng) + 2.5f;
        float n = std::sqrt(x * x + y * y + z * z + w * w);
        qa[i] = {x / n, y / n, z / n, w / n};
        x = d(rng); y = d(rng); z = d(rng); w = d(rng) + 2.5f;
        n = std::sqrt(x * x + y * y + z * z + w * w);
        qb[i] = {x / n, y / n, z / n, w / n};
        va[i] = {d(rng), d(rng), d(rng)};
        ta[i] = {{d(rng), d(rng), d(rng)}, qa[i], {1.5f, 0.5f, 2.0f}};
        tb[i] = {{d(rng), d(rng), d(rng)}, qb[i], {1.0f, 2.0f, 0.5f}};
    }
    const int REPS = 20000;
    volatile float sink = 0.0f;

    const auto bench = [&](const char* name, auto&& fn) {
        fn(1); // warmup
        const auto t0 = Clock::now();
        fn(REPS);
        const auto dt = std::chrono::duration<double>(Clock::now() - t0).count();
        std::printf("%-12s %7.2f ns/op\n", name, dt / (double(REPS) * N) * 1e9);
    };

    bench("quat*quat", [&](int reps) {
        float acc = 0;
        for (int r = 0; r < reps; ++r)
            for (int i = 0; i < N; ++i) {
                const auto q = qa[i] * qb[i];
                acc += q.x + q.w;
            }
        sink = acc;
    });
    bench("rotate", [&](int reps) {
        float acc = 0;
        for (int r = 0; r < reps; ++r)
            for (int i = 0; i < N; ++i) {
                const auto v = rotate(qa[i], va[i]);
                acc += v.x + v.z;
            }
        sink = acc;
    });
    bench("compose", [&](int reps) {
        float acc = 0;
        for (int r = 0; r < reps; ++r)
            for (int i = 0; i < N; ++i) {
                const auto t = compose(ta[i], tb[i]);
                acc += t.position.x + t.rotation.w;
            }
        sink = acc;
    });
    bench("invCompose", [&](int reps) {
        float acc = 0;
        for (int r = 0; r < reps; ++r)
            for (int i = 0; i < N; ++i) {
                const auto t = invCompose(ta[i], tb[i]);
                acc += t.position.y + t.scale.z;
            }
        sink = acc;
    });
    return int(sink) * 0;
}
