#pragma once

#include <filesystem>
#include <functional>
#include <memory>

#include "sky/serialization/serialization.hpp"
#include "sky/terrain/terrain.hpp"

namespace sky::terrain {

/// In-memory implementation of Terrain and Landscape: owns terrain datasets
/// and notifies rendering/physics hooks when data changes.
class TerrainWorld : public ITerrainService,
                     public ITerrainQueryService,
                     public ITerrainPersistenceContract {
public:
    ~TerrainWorld() override = default;

    using ChangedHook = std::function<void(TerrainHandle)>;

    /// Rendering and physics subscribe here to rebuild their terrain
    /// representations after edits, generation or load.
    virtual void onTerrainChanged(ChangedHook hook) = 0;

    /// Assigns the on-disk location used by ITerrainPersistenceContract.
    virtual void setStoragePath(TerrainHandle terrain,
                                const std::filesystem::path& path) = 0;
};

std::unique_ptr<TerrainWorld> createTerrainWorld(
    serialization::ISerializationBackend& storage);

} // namespace sky::terrain
