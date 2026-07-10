#pragma once

#include <string>
#include <vector>

#include "sky/mapgen/map_generation.hpp"
#include "sky/object/object_model.hpp"
#include "sky/scene/scene_world.hpp"
#include "sky/terrain/terrain.hpp"

namespace sky::mapgen {

/// Applies a generation result to the world (the Map Generation -> Terrain
/// and Landscape -> Scene System integration edges): replaces the target
/// terrain dataset and materializes every placement as a scene root object.
/// Generation itself never owns persistent terrain or scene storage.
std::vector<object::ObjectHandle> materializeGenerationResult(
    const IGenerationResult& result, terrain::ITerrainService& terrainService,
    terrain::TerrainHandle targetTerrain, scene::SceneWorld& scenes,
    scene::SceneHandle scene, object::IObjectFactory& objectFactory,
    object::IObjectHierarchyAccess& hierarchy,
    const std::string& objectNamePrefix = "Generated");

} // namespace sky::mapgen
