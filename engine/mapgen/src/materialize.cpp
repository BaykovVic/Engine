#include "sky/mapgen/materialize.hpp"

namespace sky::mapgen {

std::vector<object::ObjectHandle> materializeGenerationResult(
    const IGenerationResult& result, terrain::ITerrainService& terrainService,
    terrain::TerrainHandle targetTerrain, scene::SceneWorld& scenes,
    scene::SceneHandle scene, object::IObjectFactory& objectFactory,
    object::IObjectHierarchyAccess& hierarchy,
    const std::string& objectNamePrefix) {
    std::vector<object::ObjectHandle> created;
    if (!result.succeeded()) {
        return created;
    }

    if (targetTerrain.isValid() && result.terrainOutput().resolution > 0) {
        terrainService.replaceDataset(targetTerrain, result.terrainOutput());
    }

    created.reserve(result.placements().size());
    std::size_t index = 0;
    for (const auto& placement : result.placements()) {
        const auto object = objectFactory.createObject(
            objectNamePrefix + " " + std::to_string(++index));
        hierarchy.setLocalTransform(object, placement.transform);
        scenes.addRootObject(scene, object);
        created.push_back(object);
    }
    return created;
}

} // namespace sky::mapgen
