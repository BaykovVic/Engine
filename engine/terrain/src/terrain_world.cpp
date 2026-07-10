#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "sky/serialization/byte_stream.hpp"
#include "sky/terrain/terrain_world.hpp"

namespace sky::terrain {
namespace {

constexpr const char* kTerrainSchemaId = "sky.terrain";
constexpr serialization::SchemaVersion kTerrainSchemaVersion{1, 0};

struct TerrainRecord {
    TerrainDataset dataset;
    std::filesystem::path storagePath;
};

class TerrainWorldImpl final : public TerrainWorld {
public:
    explicit TerrainWorldImpl(serialization::ISerializationBackend& storage)
        : storage_(storage) {}

    // ITerrainService

    TerrainHandle createTerrain(const TerrainDataset& dataset) override {
        if (dataset.resolution == 0 ||
            dataset.heights.size() !=
                static_cast<std::size_t>(dataset.resolution) * dataset.resolution) {
            return TerrainHandle::invalid();
        }
        const TerrainHandle handle{nextId_++};
        terrains_.emplace(handle.value, TerrainRecord{dataset, {}});
        notify(handle);
        return handle;
    }

    void destroyTerrain(TerrainHandle terrain) override { terrains_.erase(terrain.value); }

    void applyEdit(TerrainHandle terrain, const TerrainEdit& edit) override {
        auto* record = find(terrain);
        if (record == nullptr || edit.radius <= 0.0f) {
            return;
        }
        auto& dataset = record->dataset;
        const auto resolution = dataset.resolution;
        const float spacingX = dataset.worldScale.x;
        const float spacingZ = dataset.worldScale.z;

        std::vector<float> smoothed;
        if (edit.operation == "smooth") {
            smoothed = dataset.heights;
        }

        for (std::uint32_t z = 0; z < resolution; ++z) {
            for (std::uint32_t x = 0; x < resolution; ++x) {
                const float worldX = static_cast<float>(x) * spacingX;
                const float worldZ = static_cast<float>(z) * spacingZ;
                const float dx = worldX - edit.center.x;
                const float dz = worldZ - edit.center.z;
                const float distance = std::sqrt(dx * dx + dz * dz);
                if (distance > edit.radius) {
                    continue;
                }
                // Smooth cosine falloff towards the brush edge.
                const float falloff =
                    0.5f * (1.0f + std::cos(distance / edit.radius * 3.14159265f));
                const auto index = z * resolution + x;
                if (edit.operation == "raise") {
                    dataset.heights[index] += edit.strength * falloff;
                } else if (edit.operation == "lower") {
                    dataset.heights[index] -= edit.strength * falloff;
                } else if (edit.operation == "smooth") {
                    smoothed[index] = blendWithNeighbours(dataset, x, z, falloff);
                }
            }
        }
        if (edit.operation == "smooth") {
            dataset.heights = std::move(smoothed);
        }
        notify(terrain);
    }

    void replaceDataset(TerrainHandle terrain, TerrainDataset dataset) override {
        if (auto* record = find(terrain)) {
            record->dataset = std::move(dataset);
            notify(terrain);
        }
    }

    // ITerrainQueryService

    const TerrainDataset& dataset(TerrainHandle terrain) const override {
        static const TerrainDataset kEmpty{};
        const auto* record = find(terrain);
        return record != nullptr ? record->dataset : kEmpty;
    }

    float heightAt(TerrainHandle terrain, float x, float z) const override {
        const auto* record = find(terrain);
        if (record == nullptr) {
            return 0.0f;
        }
        const auto& dataset = record->dataset;
        const auto resolution = dataset.resolution;

        // Bilinear interpolation over the height grid, clamped to its edge.
        const float gx = std::clamp(x / dataset.worldScale.x, 0.0f,
                                    static_cast<float>(resolution - 1));
        const float gz = std::clamp(z / dataset.worldScale.z, 0.0f,
                                    static_cast<float>(resolution - 1));
        const auto x0 = static_cast<std::uint32_t>(gx);
        const auto z0 = static_cast<std::uint32_t>(gz);
        const auto x1 = std::min(x0 + 1, resolution - 1);
        const auto z1 = std::min(z0 + 1, resolution - 1);
        const float fx = gx - static_cast<float>(x0);
        const float fz = gz - static_cast<float>(z0);

        const auto sample = [&](std::uint32_t sx, std::uint32_t sz) {
            return dataset.heights[sz * resolution + sx];
        };
        const float top = sample(x0, z0) * (1.0f - fx) + sample(x1, z0) * fx;
        const float bottom = sample(x0, z1) * (1.0f - fx) + sample(x1, z1) * fx;
        return (top * (1.0f - fz) + bottom * fz) * dataset.worldScale.y;
    }

    // ITerrainPersistenceContract

    bool save(TerrainHandle terrain) override {
        const auto* record = find(terrain);
        if (record == nullptr || record->storagePath.empty()) {
            return false;
        }
        const auto& dataset = record->dataset;
        serialization::ByteWriter writer;
        writer.writeU32(dataset.resolution);
        writer.writeU32(dataset.chunkSize);
        writer.writeF32(dataset.worldScale.x);
        writer.writeF32(dataset.worldScale.y);
        writer.writeF32(dataset.worldScale.z);
        for (const float height : dataset.heights) {
            writer.writeF32(height);
        }
        return storage_.write(record->storagePath, {kTerrainSchemaId,
                                                    kTerrainSchemaVersion,
                                                    writer.takeBuffer()});
    }

    TerrainHandle load(const std::string& terrainAssetPath) override {
        const auto blob = storage_.read(terrainAssetPath);
        if (!blob || blob->schemaId != kTerrainSchemaId ||
            blob->version != kTerrainSchemaVersion) {
            return TerrainHandle::invalid();
        }
        serialization::ByteReader reader(blob->payload);
        TerrainDataset dataset;
        const auto resolution = reader.readU32();
        const auto chunkSize = reader.readU32();
        const auto sx = reader.readF32(), sy = reader.readF32(), sz = reader.readF32();
        if (!resolution || !chunkSize || !sx || !sy || !sz) {
            return TerrainHandle::invalid();
        }
        dataset.resolution = *resolution;
        dataset.chunkSize = *chunkSize;
        dataset.worldScale = {*sx, *sy, *sz};
        const auto count = static_cast<std::size_t>(*resolution) * *resolution;
        dataset.heights.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            const auto height = reader.readF32();
            if (!height) {
                return TerrainHandle::invalid();
            }
            dataset.heights.push_back(*height);
        }
        const auto handle = createTerrain(dataset);
        if (handle.isValid()) {
            setStoragePath(handle, terrainAssetPath);
        }
        return handle;
    }

    // TerrainWorld

    void onTerrainChanged(ChangedHook hook) override { hooks_.push_back(std::move(hook)); }

    void setStoragePath(TerrainHandle terrain, const std::filesystem::path& path) override {
        if (auto* record = find(terrain)) {
            record->storagePath = path;
        }
    }

private:
    TerrainRecord* find(TerrainHandle terrain) {
        const auto it = terrains_.find(terrain.value);
        return it != terrains_.end() ? &it->second : nullptr;
    }

    const TerrainRecord* find(TerrainHandle terrain) const {
        const auto it = terrains_.find(terrain.value);
        return it != terrains_.end() ? &it->second : nullptr;
    }

    static float blendWithNeighbours(const TerrainDataset& dataset, std::uint32_t x,
                                     std::uint32_t z, float falloff) {
        const auto resolution = dataset.resolution;
        float sum = 0.0f;
        int samples = 0;
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dx = -1; dx <= 1; ++dx) {
                const auto nx = static_cast<std::int64_t>(x) + dx;
                const auto nz = static_cast<std::int64_t>(z) + dz;
                if (nx < 0 || nz < 0 || nx >= resolution || nz >= resolution) {
                    continue;
                }
                sum += dataset.heights[static_cast<std::size_t>(nz) * resolution +
                                       static_cast<std::size_t>(nx)];
                ++samples;
            }
        }
        const float average = sum / static_cast<float>(samples);
        const float current = dataset.heights[static_cast<std::size_t>(z) * resolution + x];
        return current + (average - current) * falloff;
    }

    void notify(TerrainHandle terrain) {
        for (const auto& hook : hooks_) {
            hook(terrain);
        }
    }

    serialization::ISerializationBackend& storage_;
    std::uint64_t nextId_ = 1;
    std::unordered_map<std::uint64_t, TerrainRecord> terrains_;
    std::vector<ChangedHook> hooks_;
};

} // namespace

std::unique_ptr<TerrainWorld> createTerrainWorld(
    serialization::ISerializationBackend& storage) {
    return std::make_unique<TerrainWorldImpl>(storage);
}

} // namespace sky::terrain
