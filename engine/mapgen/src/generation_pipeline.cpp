#include <algorithm>
#include <cmath>

#include "sky/asset/asset_database.hpp"
#include "sky/mapgen/generation_pipeline.hpp"

namespace sky::mapgen {
namespace {

/// SplitMix64: deterministic, seedable and good enough for generation.
std::uint64_t splitMix64(std::uint64_t& state) {
    state += 0x9E3779B97F4A7C15ull;
    std::uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

float hashToUnitFloat(std::uint64_t seed, std::uint64_t x, std::uint64_t z) {
    std::uint64_t state = seed ^ (x * 0x9E3779B97F4A7C15ull) ^ (z * 0xC2B2AE3D27D4EB4Full);
    return static_cast<float>(splitMix64(state) >> 40) /
           static_cast<float>(1ull << 24);
}

/// Value noise: random lattice values, bilinear interpolation between them.
float valueNoise(std::uint64_t seed, float x, float z) {
    const auto x0 = static_cast<std::uint64_t>(std::floor(x));
    const auto z0 = static_cast<std::uint64_t>(std::floor(z));
    const float fx = x - std::floor(x);
    const float fz = z - std::floor(z);
    const float top = hashToUnitFloat(seed, x0, z0) * (1.0f - fx) +
                      hashToUnitFloat(seed, x0 + 1, z0) * fx;
    const float bottom = hashToUnitFloat(seed, x0, z0 + 1) * (1.0f - fx) +
                         hashToUnitFloat(seed, x0 + 1, z0 + 1) * fx;
    return top * (1.0f - fz) + bottom * fz;
}

class GenerationResultImpl final : public IGenerationResult {
public:
    GenerationResultImpl(bool succeeded, terrain::TerrainDataset dataset,
                         std::vector<GeneratedPlacement> placements)
        : succeeded_(succeeded),
          dataset_(std::move(dataset)),
          placements_(std::move(placements)) {}

    bool succeeded() const override { return succeeded_; }
    const terrain::TerrainDataset& terrainOutput() const override { return dataset_; }
    const std::vector<GeneratedPlacement>& placements() const override {
        return placements_;
    }

private:
    bool succeeded_;
    terrain::TerrainDataset dataset_;
    std::vector<GeneratedPlacement> placements_;
};

class GenerationPipelineImpl final : public IGenerationPipeline {
public:
    std::vector<std::string> availableStages() const override {
        return {kStageHeightfield, kStagePlacement};
    }

    std::unique_ptr<IGenerationResult> generate(const GenerationRequest& request) override {
        const auto& profile = request.profile;
        if (profile.mapSize == 0) {
            return std::make_unique<GenerationResultImpl>(
                false, terrain::TerrainDataset{}, std::vector<GeneratedPlacement>{});
        }

        const auto enabled = [&](const char* stage) {
            return std::ranges::find(profile.enabledStages, stage) !=
                   profile.enabledStages.end();
        };

        terrain::TerrainDataset dataset;
        dataset.resolution = profile.mapSize;
        dataset.chunkSize = std::max(1u, profile.mapSize / 4);
        dataset.worldScale = {1.0f, 1.0f, 1.0f};
        dataset.heights.assign(
            static_cast<std::size_t>(profile.mapSize) * profile.mapSize, 0.0f);

        if (enabled(kStageHeightfield)) {
            fillHeightfield(dataset, profile.seed);
        }

        std::vector<GeneratedPlacement> placements;
        if (enabled(kStagePlacement)) {
            placements = scatterObjects(dataset, profile.seed);
        }

        return std::make_unique<GenerationResultImpl>(true, std::move(dataset),
                                                      std::move(placements));
    }

private:
    static void fillHeightfield(terrain::TerrainDataset& dataset, std::uint64_t seed) {
        const auto resolution = dataset.resolution;
        for (std::uint32_t z = 0; z < resolution; ++z) {
            for (std::uint32_t x = 0; x < resolution; ++x) {
                // Two octaves of value noise: broad shapes plus detail.
                const float fx = static_cast<float>(x);
                const float fz = static_cast<float>(z);
                const float broad = valueNoise(seed, fx / 16.0f, fz / 16.0f) * 8.0f;
                const float detail = valueNoise(seed + 1, fx / 4.0f, fz / 4.0f) * 2.0f;
                dataset.heights[z * resolution + x] = broad + detail;
            }
        }
    }

    static std::vector<GeneratedPlacement> scatterObjects(
        const terrain::TerrainDataset& dataset, std::uint64_t seed) {
        std::vector<GeneratedPlacement> placements;
        const auto resolution = dataset.resolution;
        const auto count = std::max(1u, resolution / 4);
        std::uint64_t state = seed ^ 0xA5A5A5A5A5A5A5A5ull;
        placements.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto gx = static_cast<std::uint32_t>(splitMix64(state) % resolution);
            const auto gz = static_cast<std::uint32_t>(splitMix64(state) % resolution);
            GeneratedPlacement placement;
            placement.asset = asset::assetIdFromPath("generated/scatter-object");
            placement.transform.position = {
                static_cast<float>(gx) * dataset.worldScale.x,
                dataset.heights[gz * resolution + gx] * dataset.worldScale.y,
                static_cast<float>(gz) * dataset.worldScale.z};
            placements.push_back(placement);
        }
        return placements;
    }
};

} // namespace

std::unique_ptr<IGenerationPipeline> createGenerationPipeline() {
    return std::make_unique<GenerationPipelineImpl>();
}

} // namespace sky::mapgen
