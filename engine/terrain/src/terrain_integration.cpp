#include <algorithm>
#include <cmath>

#include "sky/terrain/terrain_integration.hpp"

namespace sky::terrain {

physics::ColliderDesc makeTerrainCollider(const TerrainDataset& dataset) {
    physics::ColliderDesc desc;
    desc.shape = physics::ColliderShape::TerrainHeightfield;
    desc.heightfield.resolution = dataset.resolution;
    desc.heightfield.scale = dataset.worldScale;
    desc.heightfield.heights = dataset.heights;
    return desc;
}

namespace {

core::Vec3 vertexAt(const TerrainDataset& dataset, std::uint32_t x, std::uint32_t z) {
    return {static_cast<float>(x) * dataset.worldScale.x,
            dataset.heights[z * dataset.resolution + x] * dataset.worldScale.y,
            static_cast<float>(z) * dataset.worldScale.z};
}

/// Vertex normal via central differences over the height grid.
core::Vec3 normalAt(const TerrainDataset& dataset, std::uint32_t x, std::uint32_t z) {
    const auto resolution = dataset.resolution;
    const auto height = [&](std::int64_t sx, std::int64_t sz) {
        sx = std::clamp<std::int64_t>(sx, 0, resolution - 1);
        sz = std::clamp<std::int64_t>(sz, 0, resolution - 1);
        return dataset.heights[static_cast<std::size_t>(sz) * resolution +
                               static_cast<std::size_t>(sx)] *
               dataset.worldScale.y;
    };
    const float dx = (height(x + 1, z) - height(static_cast<std::int64_t>(x) - 1, z)) /
                     (2.0f * dataset.worldScale.x);
    const float dz = (height(x, z + 1) - height(x, static_cast<std::int64_t>(z) - 1)) /
                     (2.0f * dataset.worldScale.z);
    const float length = std::sqrt(dx * dx + 1.0f + dz * dz);
    return {-dx / length, 1.0f / length, -dz / length};
}

void appendVertex(std::vector<float>& mesh, const core::Vec3& position,
                  const core::Vec3& normal) {
    mesh.insert(mesh.end(),
                {position.x, position.y, position.z, normal.x, normal.y, normal.z});
}

} // namespace

std::vector<float> buildTerrainMesh(const TerrainDataset& dataset) {
    std::vector<float> mesh;
    const auto resolution = dataset.resolution;
    if (resolution < 2 ||
        dataset.heights.size() < static_cast<std::size_t>(resolution) * resolution) {
        return mesh;
    }
    // Two triangles per grid cell, 6 floats per vertex.
    mesh.reserve(static_cast<std::size_t>(resolution - 1) * (resolution - 1) * 6 * 6);
    for (std::uint32_t z = 0; z + 1 < resolution; ++z) {
        for (std::uint32_t x = 0; x + 1 < resolution; ++x) {
            const auto v00 = vertexAt(dataset, x, z);
            const auto v10 = vertexAt(dataset, x + 1, z);
            const auto v01 = vertexAt(dataset, x, z + 1);
            const auto v11 = vertexAt(dataset, x + 1, z + 1);
            const auto n00 = normalAt(dataset, x, z);
            const auto n10 = normalAt(dataset, x + 1, z);
            const auto n01 = normalAt(dataset, x, z + 1);
            const auto n11 = normalAt(dataset, x + 1, z + 1);

            appendVertex(mesh, v00, n00);
            appendVertex(mesh, v01, n01);
            appendVertex(mesh, v11, n11);

            appendVertex(mesh, v00, n00);
            appendVertex(mesh, v11, n11);
            appendVertex(mesh, v10, n10);
        }
    }
    return mesh;
}

} // namespace sky::terrain
