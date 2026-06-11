#include <algorithm>
#include <unordered_map>

#include "sky/scene/scene_world.hpp"
#include "sky/serialization/byte_stream.hpp"

namespace sky::scene {
namespace {

constexpr const char* kSceneSchemaId = "sky.scene";
constexpr serialization::SchemaVersion kSceneSchemaVersion{1, 0};
constexpr std::uint32_t kNoParent = 0xFFFFFFFFu;

struct SceneRecord {
    SceneDescriptor descriptor;
    SceneState state = SceneState::Loaded;
    std::vector<object::ObjectHandle> rootObjects;
};

void writeTransform(serialization::ByteWriter& writer, const core::Transform& transform) {
    writer.writeF32(transform.position.x);
    writer.writeF32(transform.position.y);
    writer.writeF32(transform.position.z);
    writer.writeF32(transform.rotation.x);
    writer.writeF32(transform.rotation.y);
    writer.writeF32(transform.rotation.z);
    writer.writeF32(transform.rotation.w);
    writer.writeF32(transform.scale.x);
    writer.writeF32(transform.scale.y);
    writer.writeF32(transform.scale.z);
}

bool readTransform(serialization::ByteReader& reader, core::Transform& transform) {
    const auto px = reader.readF32(), py = reader.readF32(), pz = reader.readF32();
    const auto rx = reader.readF32(), ry = reader.readF32(), rz = reader.readF32();
    const auto rw = reader.readF32();
    const auto sx = reader.readF32(), sy = reader.readF32(), sz = reader.readF32();
    if (!px || !py || !pz || !rx || !ry || !rz || !rw || !sx || !sy || !sz) {
        return false;
    }
    transform = {{*px, *py, *pz}, {*rx, *ry, *rz, *rw}, {*sx, *sy, *sz}};
    return true;
}

class SceneWorldImpl final : public SceneWorld {
public:
    explicit SceneWorldImpl(const SceneWorldDeps& deps) : deps_(deps) {}

    // ISceneRepository

    SceneHandle createScene(const SceneDescriptor& descriptor) override {
        const SceneHandle handle{nextId_++};
        scenes_.emplace(handle.value, SceneRecord{descriptor});
        return handle;
    }

    SceneHandle loadScene(const std::filesystem::path& path) override {
        const auto blob = deps_.storage.read(path);
        if (!blob || blob->schemaId != kSceneSchemaId ||
            blob->version != kSceneSchemaVersion) {
            return SceneHandle::invalid();
        }
        serialization::ByteReader reader(blob->payload);
        const auto sceneName = reader.readString();
        const auto objectCount = reader.readU32();
        if (!sceneName || !objectCount) {
            return SceneHandle::invalid();
        }

        const SceneHandle handle = createScene({*sceneName, path});
        std::vector<object::ObjectHandle> byIndex;
        byIndex.reserve(*objectCount);

        for (std::uint32_t i = 0; i < *objectCount; ++i) {
            const auto parentIndex = reader.readU32();
            const auto name = reader.readString();
            core::Transform transform;
            if (!parentIndex || !name || !readTransform(reader, transform)) {
                unloadScene(handle);
                return SceneHandle::invalid();
            }
            const auto object = deps_.objectFactory.createObject(*name);
            deps_.hierarchy.setLocalTransform(object, transform);
            byIndex.push_back(object);

            // Objects are stored in pre-order, so a parent always precedes
            // its children.
            if (*parentIndex == kNoParent) {
                addRootObject(handle, object);
            } else if (*parentIndex < byIndex.size()) {
                deps_.hierarchy.setParent(object, byIndex[*parentIndex]);
            }

            const auto componentCount = reader.readU32();
            if (!componentCount) {
                unloadScene(handle);
                return SceneHandle::invalid();
            }
            for (std::uint32_t c = 0; c < *componentCount; ++c) {
                const auto typeId = reader.readString();
                if (!typeId) {
                    unloadScene(handle);
                    return SceneHandle::invalid();
                }
                deps_.componentAttachment.attach(object, *typeId);
            }
        }
        return handle;
    }

    bool saveScene(SceneHandle scene) override {
        const auto* record = find(scene);
        if (record == nullptr || record->descriptor.path.empty()) {
            return false;
        }

        // Flatten the hierarchy in pre-order and remember each object's index
        // so parent links can be stored as indices.
        std::vector<object::ObjectHandle> order;
        std::unordered_map<std::uint64_t, std::uint32_t> indexOf;
        for (const auto root : record->rootObjects) {
            flatten(root, order, indexOf);
        }

        serialization::ByteWriter writer;
        writer.writeString(record->descriptor.name);
        writer.writeU32(static_cast<std::uint32_t>(order.size()));
        for (const auto object : order) {
            const auto parent = deps_.hierarchy.parentOf(object);
            const auto parentIt = indexOf.find(parent.value);
            writer.writeU32(parentIt != indexOf.end() ? parentIt->second : kNoParent);
            writer.writeString(deps_.objectQuery.nameOf(object));
            writeTransform(writer, deps_.hierarchy.localTransform(object));

            const auto components = deps_.componentQuery.componentsOf(object);
            writer.writeU32(static_cast<std::uint32_t>(components.size()));
            for (const auto component : components) {
                writer.writeString(deps_.componentQuery.descriptorOf(component).typeId);
            }
        }

        return deps_.storage.write(
            record->descriptor.path,
            {kSceneSchemaId, kSceneSchemaVersion, writer.takeBuffer()});
    }

    void unloadScene(SceneHandle scene) override {
        const auto it = scenes_.find(scene.value);
        if (it == scenes_.end()) {
            return;
        }
        if (activeScene_ == scene) {
            deactivate(scene);
            activeScene_ = SceneHandle::invalid();
        }
        for (const auto root : it->second.rootObjects) {
            deps_.objectFactory.destroyObject(root);
        }
        scenes_.erase(it);
    }

    // ISceneRuntime

    void activate(SceneHandle scene) override {
        auto* record = find(scene);
        if (record == nullptr) {
            return;
        }
        record->state = SceneState::RuntimeActive;
        activeScene_ = scene;
        context_ = {scene, SceneState::RuntimeActive, record->rootObjects};
    }

    void deactivate(SceneHandle scene) override {
        auto* record = find(scene);
        if (record == nullptr) {
            return;
        }
        record->state = SceneState::Loaded;
        if (activeScene_ == scene) {
            context_ = {scene, SceneState::Loaded, record->rootObjects};
        }
    }

    const SceneRuntimeContext& activeContext() const override { return context_; }

    void tick(double deltaSeconds) override {
        // Frame order per the architecture data flow: object/component edits
        // are immediate, then physics (fixed step), then ECS systems.
        // Scripting callbacks slot in once the Scripting Boundary lands.
        if (context_.state != SceneState::RuntimeActive) {
            return;
        }
        if (deps_.physicsWorld != nullptr) {
            constexpr double kFixedStep = 1.0 / 60.0;
            if (deps_.physicsSync != nullptr) {
                deps_.physicsSync->pushKinematicState();
            }
            physicsAccumulator_ += deltaSeconds;
            while (physicsAccumulator_ >= kFixedStep) {
                deps_.physicsWorld->step(kFixedStep);
                physicsAccumulator_ -= kFixedStep;
            }
            if (deps_.physicsSync != nullptr) {
                deps_.physicsSync->pullSimulationResults();
            }
        }
        if (deps_.ecsScheduler != nullptr) {
            deps_.ecsScheduler->tick(deltaSeconds);
        }
    }

    // ISceneQueryService

    std::vector<SceneHandle> loadedScenes() const override {
        std::vector<SceneHandle> result;
        result.reserve(scenes_.size());
        for (const auto& [id, record] : scenes_) {
            result.push_back(SceneHandle{id});
        }
        return result;
    }

    const SceneDescriptor& descriptor(SceneHandle scene) const override {
        static const SceneDescriptor kEmpty{};
        const auto* record = find(scene);
        return record != nullptr ? record->descriptor : kEmpty;
    }

    SceneHandle sceneOf(object::ObjectHandle object) const override {
        // Scene membership is defined by the root: walk up the hierarchy.
        object::ObjectHandle root = object;
        while (deps_.hierarchy.parentOf(root).isValid()) {
            root = deps_.hierarchy.parentOf(root);
        }
        for (const auto& [id, record] : scenes_) {
            if (std::ranges::find(record.rootObjects, root) != record.rootObjects.end()) {
                return SceneHandle{id};
            }
        }
        return SceneHandle::invalid();
    }

    // SceneWorld

    void addRootObject(SceneHandle scene, object::ObjectHandle object) override {
        if (auto* record = find(scene)) {
            record->rootObjects.push_back(object);
            if (activeScene_ == scene) {
                context_.rootObjects = record->rootObjects;
            }
        }
    }

private:
    SceneRecord* find(SceneHandle scene) {
        const auto it = scenes_.find(scene.value);
        return it != scenes_.end() ? &it->second : nullptr;
    }

    const SceneRecord* find(SceneHandle scene) const {
        const auto it = scenes_.find(scene.value);
        return it != scenes_.end() ? &it->second : nullptr;
    }

    void flatten(object::ObjectHandle object, std::vector<object::ObjectHandle>& order,
                 std::unordered_map<std::uint64_t, std::uint32_t>& indexOf) const {
        indexOf.emplace(object.value, static_cast<std::uint32_t>(order.size()));
        order.push_back(object);
        for (const auto child : deps_.hierarchy.childrenOf(object)) {
            flatten(child, order, indexOf);
        }
    }

    SceneWorldDeps deps_;
    std::uint64_t nextId_ = 1;
    std::unordered_map<std::uint64_t, SceneRecord> scenes_;
    SceneHandle activeScene_;
    SceneRuntimeContext context_;
    double physicsAccumulator_ = 0.0;
};

} // namespace

std::unique_ptr<SceneWorld> createSceneWorld(const SceneWorldDeps& deps) {
    return std::make_unique<SceneWorldImpl>(deps);
}

} // namespace sky::scene
