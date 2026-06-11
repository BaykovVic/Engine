#include <algorithm>
#include <unordered_map>

#include "sky/object/object_world.hpp"

namespace sky::object {
namespace {

struct ObjectRecord {
    std::string name;
    TransformNode node;
};

class ObjectWorldImpl final : public ObjectWorld {
public:
    // IObjectFactory

    ObjectHandle createObject(const std::string& name) override {
        const ObjectHandle handle{nextId_++};
        objects_.emplace(handle.value, ObjectRecord{name, {}});
        return handle;
    }

    void renameObject(ObjectHandle object, const std::string& name) override {
        if (auto* record = find(object)) {
            record->name = name;
        }
    }

    void destroyObject(ObjectHandle object) override {
        const auto it = objects_.find(object.value);
        if (it == objects_.end()) {
            return;
        }
        // Destroy the subtree; copy children since destruction mutates them.
        const auto children = it->second.node.children;
        for (const auto child : children) {
            destroyObject(child);
        }
        detachFromParent(object);
        objects_.erase(object.value);
    }

    // IObjectHierarchyAccess

    void setParent(ObjectHandle child, ObjectHandle parent) override {
        auto* childRecord = find(child);
        if (childRecord == nullptr || wouldCreateCycle(child, parent)) {
            return;
        }
        detachFromParent(child);
        childRecord->node.parent = parent;
        if (auto* parentRecord = find(parent)) {
            parentRecord->node.children.push_back(child);
        }
    }

    ObjectHandle parentOf(ObjectHandle object) const override {
        const auto* record = find(object);
        return record != nullptr ? record->node.parent : ObjectHandle::invalid();
    }

    std::vector<ObjectHandle> childrenOf(ObjectHandle object) const override {
        const auto* record = find(object);
        return record != nullptr ? record->node.children : std::vector<ObjectHandle>{};
    }

    void setLocalTransform(ObjectHandle object, const core::Transform& transform) override {
        if (auto* record = find(object)) {
            record->node.local = transform;
        }
    }

    core::Transform localTransform(ObjectHandle object) const override {
        const auto* record = find(object);
        return record != nullptr ? record->node.local : core::Transform{};
    }

    core::Transform worldTransform(ObjectHandle object) const override {
        const auto* record = find(object);
        if (record == nullptr) {
            return {};
        }
        if (!record->node.parent.isValid()) {
            return record->node.local;
        }
        return core::compose(worldTransform(record->node.parent), record->node.local);
    }

    // IObjectQueryService

    bool exists(ObjectHandle object) const override {
        return objects_.contains(object.value);
    }

    std::string nameOf(ObjectHandle object) const override {
        const auto* record = find(object);
        return record != nullptr ? record->name : std::string{};
    }

    std::vector<ObjectHandle> findByName(const std::string& name) const override {
        std::vector<ObjectHandle> result;
        for (const auto& [id, record] : objects_) {
            if (record.name == name) {
                result.push_back(ObjectHandle{id});
            }
        }
        return result;
    }

private:
    ObjectRecord* find(ObjectHandle handle) {
        const auto it = objects_.find(handle.value);
        return it != objects_.end() ? &it->second : nullptr;
    }

    const ObjectRecord* find(ObjectHandle handle) const {
        const auto it = objects_.find(handle.value);
        return it != objects_.end() ? &it->second : nullptr;
    }

    void detachFromParent(ObjectHandle child) {
        auto* childRecord = find(child);
        if (childRecord == nullptr || !childRecord->node.parent.isValid()) {
            return;
        }
        if (auto* parentRecord = find(childRecord->node.parent)) {
            std::erase(parentRecord->node.children, child);
        }
        childRecord->node.parent = ObjectHandle::invalid();
    }

    bool wouldCreateCycle(ObjectHandle child, ObjectHandle newParent) const {
        for (ObjectHandle cursor = newParent; cursor.isValid();
             cursor = parentOf(cursor)) {
            if (cursor == child) {
                return true;
            }
        }
        return false;
    }

    std::uint64_t nextId_ = 1;
    std::unordered_map<std::uint64_t, ObjectRecord> objects_;
};

} // namespace

std::unique_ptr<ObjectWorld> createObjectWorld() {
    return std::make_unique<ObjectWorldImpl>();
}

} // namespace sky::object
