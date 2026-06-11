#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "sky/core/handle.hpp"
#include "sky/core/math.hpp"

namespace sky::terrain {

struct TerrainTag {};
using TerrainHandle = core::Handle<TerrainTag>;

/// Persistent terrain data: a heightfield split into chunks, understood
/// identically by rendering, physics and serialization.
struct TerrainDataset {
    std::uint32_t resolution = 0;
    std::uint32_t chunkSize = 0;
    core::Vec3 worldScale{1.0f, 1.0f, 1.0f};
    std::vector<float> heights;
};

/// One terrain edit operation (raise/lower/smooth/paint) applied by editor
/// tools or by Map Generation.
struct TerrainEdit {
    core::Vec3 center;
    float radius = 1.0f;
    float strength = 1.0f;
    std::string operation;
};

/// Terrain and Landscape contract: terrain lifecycle and editing. Notifies
/// physics and rendering hooks when data changes.
class ITerrainService {
public:
    virtual ~ITerrainService() = default;

    virtual TerrainHandle createTerrain(const TerrainDataset& dataset) = 0;
    virtual void destroyTerrain(TerrainHandle terrain) = 0;
    virtual void applyEdit(TerrainHandle terrain, const TerrainEdit& edit) = 0;
    /// Replaces the dataset wholesale, e.g. with a Map Generation result.
    virtual void replaceDataset(TerrainHandle terrain, TerrainDataset dataset) = 0;
};

/// Terrain and Landscape contract: read-only access to terrain data.
class ITerrainQueryService {
public:
    virtual ~ITerrainQueryService() = default;

    [[nodiscard]] virtual const TerrainDataset& dataset(TerrainHandle terrain) const = 0;
    [[nodiscard]] virtual float heightAt(TerrainHandle terrain, float x, float z) const = 0;
};

/// Terrain and Landscape contract: round-trip persistence of terrain state
/// through Serialization and Persistence.
class ITerrainPersistenceContract {
public:
    virtual ~ITerrainPersistenceContract() = default;

    virtual bool save(TerrainHandle terrain) = 0;
    virtual TerrainHandle load(const std::string& terrainAssetPath) = 0;
};

} // namespace sky::terrain
