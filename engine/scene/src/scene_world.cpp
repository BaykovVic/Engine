#include <algorithm>
#include <unordered_map>

#include "sky/scene/scene_world.hpp"
#include "sky/serialization/byte_stream.hpp"

namespace sky::scene {
namespace {

constexpr const char* kSceneSchemaId = "sky.scene";
// 1.0: hierarchy + transforms + component type ids.
// 1.1: adds per-component field values.
// 1.2: string fields carry the asset GUID (0 = not an asset reference), so
//      renamed sources re-resolve on load.
constexpr serialization::SchemaVersion kSceneSchemaLegacy{1, 0};
constexpr serialization::SchemaVersion kSceneSchemaV11{1, 1};
constexpr serialization::SchemaVersion kSceneSchemaVersion{1, 2};
constexpr std::uint32_t kNoParent = 0xFFFFFFFFu;
constexpr double kPhysicsFixedStep = 1.0 / 60.0;

enum class FieldTag : std::uint32_t {
    Float = 0,
    Int = 1,
    Bool = 2,
    String = 3,
    Vec3 = 4,
};

void writeField(serialization::ByteWriter& writer, const std::string& name,
                const component::FieldValue& value, std::uint64_t assetGuid) {
    writer.writeString(name);
    if (const auto* f = std::get_if<float>(&value)) {
        writer.writeU32(static_cast<std::uint32_t>(FieldTag::Float));
        writer.writeF32(*f);
    } else if (const auto* i = std::get_if<std::int64_t>(&value)) {
        writer.writeU32(static_cast<std::uint32_t>(FieldTag::Int));
        writer.writeU64(static_cast<std::uint64_t>(*i));
    } else if (const auto* b = std::get_if<bool>(&value)) {
        writer.writeU32(static_cast<std::uint32_t>(FieldTag::Bool));
        writer.writeU32(*b ? 1 : 0);
    } else if (const auto* s = std::get_if<std::string>(&value)) {
        writer.writeU32(static_cast<std::uint32_t>(FieldTag::String));
        writer.writeString(*s);
        // 1.2: the GUID rides along so a renamed source still resolves.
        writer.writeU64(assetGuid);
    } else if (const auto* v = std::get_if<core::Vec3>(&value)) {
        writer.writeU32(static_cast<std::uint32_t>(FieldTag::Vec3));
        writer.writeF32(v->x);
        writer.writeF32(v->y);
        writer.writeF32(v->z);
    }
}

struct LoadedField {
    std::string name;
    component::FieldValue value;
    std::uint64_t assetGuid = 0; // strings only; 0 = not an asset reference
};

std::optional<LoadedField> readField(serialization::ByteReader& reader) {
    const auto name = reader.readString();
    const auto tag = reader.readU32();
    if (!name || !tag) {
        return std::nullopt;
    }
    switch (static_cast<FieldTag>(*tag)) {
        case FieldTag::Float:
            if (const auto value = reader.readF32()) {
                return LoadedField{*name, component::FieldValue{*value}};
            }
            break;
        case FieldTag::Int:
            if (const auto value = reader.readU64()) {
                return LoadedField{
                    *name,
                    component::FieldValue{static_cast<std::int64_t>(*value)}};
            }
            break;
        case FieldTag::Bool:
            if (const auto value = reader.readU32()) {
                return LoadedField{*name, component::FieldValue{*value != 0}};
            }
            break;
        case FieldTag::String: {
            auto value = reader.readString();
            const auto guid = reader.readU64();
            if (value && guid) {
                return LoadedField{*name,
                                   component::FieldValue{std::move(*value)},
                                   *guid};
            }
            break;
        }
        case FieldTag::Vec3: {
            const auto x = reader.readF32(), y = reader.readF32(), z = reader.readF32();
            if (x && y && z) {
                return LoadedField{*name,
                                   component::FieldValue{core::Vec3{*x, *y, *z}}};
            }
            break;
        }
    }
    return std::nullopt;
}

/// 1.0 -> 1.1: a zero field count is inserted after every component type id.
std::optional<std::vector<std::byte>> migrateSceneV10ToV11(
    const std::vector<std::byte>& payload) {
    serialization::ByteReader reader(payload);
    serialization::ByteWriter writer;

    const auto sceneName = reader.readString();
    const auto objectCount = reader.readU32();
    if (!sceneName || !objectCount) {
        return std::nullopt;
    }
    writer.writeString(*sceneName);
    writer.writeU32(*objectCount);

    for (std::uint32_t i = 0; i < *objectCount; ++i) {
        const auto parentIndex = reader.readU32();
        const auto name = reader.readString();
        if (!parentIndex || !name) {
            return std::nullopt;
        }
        writer.writeU32(*parentIndex);
        writer.writeString(*name);
        for (int f = 0; f < 10; ++f) {
            const auto value = reader.readF32();
            if (!value) {
                return std::nullopt;
            }
            writer.writeF32(*value);
        }
        const auto componentCount = reader.readU32();
        if (!componentCount) {
            return std::nullopt;
        }
        writer.writeU32(*componentCount);
        for (std::uint32_t c = 0; c < *componentCount; ++c) {
            const auto typeId = reader.readString();
            if (!typeId) {
                return std::nullopt;
            }
            writer.writeString(*typeId);
            writer.writeU32(0); // no stored fields in 1.0
        }
    }
    return writer.takeBuffer();
}

/// 1.1 -> 1.2: a zero asset GUID is appended after every string field value.
std::optional<std::vector<std::byte>> migrateSceneV11ToV12(
    const std::vector<std::byte>& payload) {
    serialization::ByteReader reader(payload);
    serialization::ByteWriter writer;

    const auto sceneName = reader.readString();
    const auto objectCount = reader.readU32();
    if (!sceneName || !objectCount) {
        return std::nullopt;
    }
    writer.writeString(*sceneName);
    writer.writeU32(*objectCount);

    for (std::uint32_t i = 0; i < *objectCount; ++i) {
        const auto parentIndex = reader.readU32();
        const auto name = reader.readString();
        if (!parentIndex || !name) {
            return std::nullopt;
        }
        writer.writeU32(*parentIndex);
        writer.writeString(*name);
        for (int f = 0; f < 10; ++f) {
            const auto value = reader.readF32();
            if (!value) {
                return std::nullopt;
            }
            writer.writeF32(*value);
        }
        const auto componentCount = reader.readU32();
        if (!componentCount) {
            return std::nullopt;
        }
        writer.writeU32(*componentCount);
        for (std::uint32_t c = 0; c < *componentCount; ++c) {
            const auto typeId = reader.readString();
            const auto fieldCount = reader.readU32();
            if (!typeId || !fieldCount) {
                return std::nullopt;
            }
            writer.writeString(*typeId);
            writer.writeU32(*fieldCount);
            for (std::uint32_t f = 0; f < *fieldCount; ++f) {
                const auto fieldName = reader.readString();
                const auto tag = reader.readU32();
                if (!fieldName || !tag) {
                    return std::nullopt;
                }
                writer.writeString(*fieldName);
                writer.writeU32(*tag);
                switch (static_cast<FieldTag>(*tag)) {
                    case FieldTag::Float: {
                        const auto v = reader.readF32();
                        if (!v) {
                            return std::nullopt;
                        }
                        writer.writeF32(*v);
                        break;
                    }
                    case FieldTag::Int: {
                        const auto v = reader.readU64();
                        if (!v) {
                            return std::nullopt;
                        }
                        writer.writeU64(*v);
                        break;
                    }
                    case FieldTag::Bool: {
                        const auto v = reader.readU32();
                        if (!v) {
                            return std::nullopt;
                        }
                        writer.writeU32(*v);
                        break;
                    }
                    case FieldTag::String: {
                        auto v = reader.readString();
                        if (!v) {
                            return std::nullopt;
                        }
                        writer.writeString(*v);
                        writer.writeU64(0); // no GUID recorded in 1.1
                        break;
                    }
                    case FieldTag::Vec3: {
                        const auto x = reader.readF32(), y = reader.readF32(),
                                   z = reader.readF32();
                        if (!x || !y || !z) {
                            return std::nullopt;
                        }
                        writer.writeF32(*x);
                        writer.writeF32(*y);
                        writer.writeF32(*z);
                        break;
                    }
                    default:
                        return std::nullopt;
                }
            }
        }
    }
    return writer.takeBuffer();
}

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
    ~SceneWorldImpl() override { finishPendingPhysics(); }

    explicit SceneWorldImpl(const SceneWorldDeps& deps) : deps_(deps) {
        if (deps_.migrations != nullptr) {
            deps_.migrations->registerMigration(kSceneSchemaId, kSceneSchemaLegacy,
                                                kSceneSchemaV11,
                                                migrateSceneV10ToV11);
            deps_.migrations->registerMigration(kSceneSchemaId, kSceneSchemaV11,
                                                kSceneSchemaVersion,
                                                migrateSceneV11ToV12);
        }
    }

    // ISceneRepository

    SceneHandle createScene(const SceneDescriptor& descriptor) override {
        const SceneHandle handle{nextId_++};
        scenes_.emplace(handle.value, SceneRecord{descriptor});
        return handle;
    }

    SceneHandle loadScene(const std::filesystem::path& path) override {
        auto blob = deps_.storage.read(path);
        if (!blob || blob->schemaId != kSceneSchemaId) {
            return SceneHandle::invalid();
        }
        // Legacy files go through the registered migration chain first.
        if (blob->version != kSceneSchemaVersion) {
            if (deps_.migrations == nullptr) {
                return SceneHandle::invalid();
            }
            auto migrated = deps_.migrations->migrate(*blob, kSceneSchemaVersion);
            if (!migrated) {
                return SceneHandle::invalid();
            }
            blob = std::move(migrated);
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
                const auto component = deps_.componentAttachment.attach(object, *typeId);
                const auto fieldCount = reader.readU32();
                if (!fieldCount) {
                    unloadScene(handle);
                    return SceneHandle::invalid();
                }
                for (std::uint32_t f = 0; f < *fieldCount; ++f) {
                    auto field = readField(reader);
                    if (!field) {
                        unloadScene(handle);
                        return SceneHandle::invalid();
                    }
                    // The GUID wins over the stored path: a source renamed
                    // since the save re-resolves to its current ref.
                    if (field->assetGuid != 0 && deps_.guidToRef) {
                        auto current = deps_.guidToRef(field->assetGuid);
                        if (!current.empty()) {
                            field->value =
                                component::FieldValue{std::move(current)};
                        }
                    }
                    if (deps_.componentData != nullptr && component.isValid()) {
                        deps_.componentData->setField(component, field->name,
                                                      std::move(field->value));
                    }
                }
            }
        }
        return handle;
    }

    bool saveScene(SceneHandle scene) override {
        auto* record = find(scene);
        if (record == nullptr || record->descriptor.path.empty()) {
            return false;
        }
        return writeScene(*record, object::ObjectHandle::invalid());
    }

    bool saveSceneAs(SceneHandle scene, const std::filesystem::path& path,
                     object::ObjectHandle excludeRoot) override {
        auto* record = find(scene);
        if (record == nullptr || path.empty()) {
            return false;
        }
        record->descriptor.path = path;
        return writeScene(*record, excludeRoot);
    }

    std::vector<object::ObjectHandle> rootObjectsOf(SceneHandle scene) const override {
        const auto* record = find(scene);
        return record != nullptr ? record->rootObjects
                                 : std::vector<object::ObjectHandle>{};
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
        // Land any in-flight async step before anyone reads final state.
        finishPendingPhysics();
        record->state = SceneState::Loaded;
        if (activeScene_ == scene) {
            context_ = {scene, SceneState::Loaded, record->rootObjects};
        }
    }

    const SceneRuntimeContext& activeContext() const override { return context_; }

    void tick(double deltaSeconds) override {
        // Frame order per the architecture data flow: object/component edits
        // are immediate, then physics (fixed step), then ECS systems.
        // With a physics job scheduler attached the frame model flips to
        // Unigine-style async: the PREVIOUS frame's simulation lands first,
        // systems and scripts run against it, and this frame's steps are
        // scheduled at the end — overlapping the caller's render.
        if (context_.state != SceneState::RuntimeActive) {
            return;
        }
        // FixedUpdate scripts pin the scene to synchronous stepping: their
        // callbacks interleave with the steps and may touch anything.
        const bool wantsFixed =
            deps_.wantsFixedUpdate && deps_.wantsFixedUpdate();
        const bool asyncPhysics =
            deps_.physicsWorld != nullptr && physicsJobs_ != nullptr && !wantsFixed;
        if (asyncPhysics) {
            finishPendingPhysics();
        } else if (deps_.physicsWorld != nullptr) {
            // A step scheduled before FixedUpdate scripts appeared may still
            // be in flight — land it before stepping on this thread.
            finishPendingPhysics();
            if (deps_.physicsSync != nullptr) {
                deps_.physicsSync->pushKinematicState();
            }
            physicsAccumulator_ += deltaSeconds;
            while (physicsAccumulator_ >= kPhysicsFixedStep) {
                if (deps_.fixedUpdate) {
                    deps_.fixedUpdate(kPhysicsFixedStep);
                }
                deps_.physicsWorld->step(kPhysicsFixedStep);
                physicsAccumulator_ -= kPhysicsFixedStep;
            }
            if (deps_.physicsSync != nullptr) {
                deps_.physicsSync->pullSimulationResults();
            }
        }
        if (deps_.ecsScheduler != nullptr) {
            // Explicit object <-> ECS sync brackets the system tick, exactly
            // like the physics sync brackets the simulation step.
            if (deps_.ecsSync != nullptr) {
                deps_.ecsSync->pushAuthoringState();
            }
            deps_.ecsScheduler->tick(deltaSeconds);
            if (deps_.ecsSync != nullptr) {
                deps_.ecsSync->pullEcsResults();
            }
        }
        // Managed scripts run last in the frame, per the architecture's
        // runtime-frame data flow. In async mode the caller runs its own
        // script layer after this and then calls schedulePhysics().
        if (deps_.scriptBridge != nullptr) {
            deps_.scriptBridge->dispatchAll(scripting::ScriptLifecycleEvent::OnUpdate,
                                            deltaSeconds);
        }
    }

    void setPhysicsJobScheduler(core::IJobScheduler* scheduler) override {
        finishPendingPhysics(); // drain under the old scheduler first
        physicsJobs_ = scheduler;
    }

    void schedulePhysics(double deltaSeconds) override {
        if (context_.state != SceneState::RuntimeActive ||
            deps_.physicsWorld == nullptr || physicsJobs_ == nullptr ||
            (deps_.wantsFixedUpdate && deps_.wantsFixedUpdate())) {
            return; // synchronous mode already stepped inside tick()
        }
        beginAsyncPhysics(deltaSeconds);
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

    void setRootObjects(SceneHandle scene,
                        std::vector<object::ObjectHandle> roots) override {
        if (auto* record = find(scene)) {
            record->rootObjects = std::move(roots);
            if (activeScene_ == scene) {
                context_.rootObjects = record->rootObjects;
            }
        }
    }

private:
    /// Serializes a scene record to its descriptor path, optionally skipping
    /// one root subtree (used to keep the editor's terrain fixture, whose
    /// heightfield is not part of the scene schema, out of the file).
    bool writeScene(SceneRecord& record, object::ObjectHandle excludeRoot) {
        std::vector<object::ObjectHandle> order;
        std::unordered_map<std::uint64_t, std::uint32_t> indexOf;
        for (const auto root : record.rootObjects) {
            if (root == excludeRoot) {
                continue;
            }
            flatten(root, order, indexOf);
        }

        serialization::ByteWriter writer;
        writer.writeString(record.descriptor.name);
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
                const auto fields = deps_.componentData != nullptr
                                        ? deps_.componentData->fields(component)
                                        : std::map<std::string, component::FieldValue>{};
                writer.writeU32(static_cast<std::uint32_t>(fields.size()));
                for (const auto& [name, value] : fields) {
                    std::uint64_t guid = 0;
                    if (deps_.refToGuid) {
                        if (const auto* ref = std::get_if<std::string>(&value)) {
                            guid = deps_.refToGuid(*ref);
                        }
                    }
                    writeField(writer, name, value, guid);
                }
            }
        }

        return deps_.storage.write(
            record.descriptor.path,
            {kSceneSchemaId, kSceneSchemaVersion, writer.takeBuffer()});
    }

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

    /// Joins the in-flight step job and applies its results to the objects.
    /// Safe to call in any state; a no-op when nothing is pending.
    void finishPendingPhysics() {
        if (physicsJobPending_) {
            physicsJobs_->wait(physicsJob_);
            physicsJobPending_ = false;
        }
        if (physicsRoundOpen_) {
            physicsRoundOpen_ = false;
            if (deps_.physicsSync != nullptr) {
                deps_.physicsSync->pullSimulationResults();
            }
        }
    }

    /// Pushes the authored state and schedules this frame's fixed steps in
    /// the background. Step count is decided on the main thread, so the
    /// step sequence is identical to the synchronous mode.
    void beginAsyncPhysics(double deltaSeconds) {
        if (deps_.physicsSync != nullptr) {
            deps_.physicsSync->pushKinematicState();
        }
        physicsRoundOpen_ = true;
        physicsAccumulator_ += deltaSeconds;
        int steps = 0;
        while (physicsAccumulator_ >= kPhysicsFixedStep) {
            physicsAccumulator_ -= kPhysicsFixedStep;
            ++steps;
        }
        if (steps == 0) {
            return; // pushed, nothing to simulate this frame
        }
        auto* world = deps_.physicsWorld;
        physicsJob_ = physicsJobs_->schedule([world, steps] {
            for (int i = 0; i < steps; ++i) {
                world->step(kPhysicsFixedStep);
            }
        });
        physicsJobPending_ = true;
    }

    SceneWorldDeps deps_;
    std::uint64_t nextId_ = 1;
    std::unordered_map<std::uint64_t, SceneRecord> scenes_;
    SceneHandle activeScene_;
    SceneRuntimeContext context_;
    double physicsAccumulator_ = 0.0;
    core::IJobScheduler* physicsJobs_ = nullptr;
    core::JobHandle physicsJob_{};
    bool physicsJobPending_ = false;
    bool physicsRoundOpen_ = false;
};

} // namespace

std::unique_ptr<SceneWorld> createSceneWorld(const SceneWorldDeps& deps) {
    return std::make_unique<SceneWorldImpl>(deps);
}

} // namespace sky::scene
