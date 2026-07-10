#pragma once

#include <vector>

#include "sky/physics/physics.hpp"
#include "sky/terrain/terrain.hpp"

namespace sky::terrain {

/// Builds the physics collider for a terrain dataset (the Terrain ->
/// Physics Engine integration edge). Attach it to a static body placed at
/// the terrain's world origin.
physics::ColliderDesc makeTerrainCollider(const TerrainDataset& dataset);

/// Builds the render mesh for a terrain dataset: interleaved position(3) +
/// normal(3) triangles, ready for IRenderResourceFactory::createMeshFromData
/// (the Terrain -> Rendering Abstraction integration edge).
std::vector<float> buildTerrainMesh(const TerrainDataset& dataset);

} // namespace sky::terrain
