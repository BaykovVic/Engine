#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "sky/asset/asset_system.hpp"
#include "sky/core/math.hpp"
#include "sky/terrain/terrain.hpp"

namespace sky::mapgen {

/// Reproducible generation settings: the same profile and seed always
/// produce the same output.
struct GenerationProfile {
    std::string profileId;
    std::uint64_t seed = 0;
    std::uint32_t mapSize = 0;
    std::vector<std::string> enabledStages;
};

struct GenerationRequest {
    GenerationProfile profile;
    terrain::TerrainHandle targetTerrain;
};

/// One object placement produced by generation, to be materialized in the
/// scene by the Scene System / Object Model.
struct GeneratedPlacement {
    asset::AssetId asset;
    core::Transform transform;
};

/// Map Generation contract: the output of a generation run. Generation does
/// not own persistent scene or terrain storage — it produces data that the
/// owning modules apply.
class IGenerationResult {
public:
    virtual ~IGenerationResult() = default;

    [[nodiscard]] virtual bool succeeded() const = 0;
    [[nodiscard]] virtual const terrain::TerrainDataset& terrainOutput() const = 0;
    [[nodiscard]] virtual const std::vector<GeneratedPlacement>& placements() const = 0;
};

/// Map Generation contract: orchestration of the procedural pipeline.
class IGenerationPipeline {
public:
    virtual ~IGenerationPipeline() = default;

    [[nodiscard]] virtual std::vector<std::string> availableStages() const = 0;
    virtual std::unique_ptr<IGenerationResult> generate(const GenerationRequest& request) = 0;
};

} // namespace sky::mapgen
