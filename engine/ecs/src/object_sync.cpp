#include <unordered_map>

#include "sky/ecs/object_sync.hpp"

namespace sky::ecs {
namespace {

class EcsObjectSyncImpl final : public IEcsObjectSync {
public:
    EcsObjectSyncImpl(EcsWorld& ecs, object::IObjectHierarchyAccess& hierarchy)
        : ecs_(ecs), hierarchy_(hierarchy) {}

    EntityId bind(object::ObjectHandle object) override {
        if (const auto it = byObject_.find(object.value); it != byObject_.end()) {
            return it->second;
        }
        const auto entity = ecs_.createEntity();
        ecs_.storeFor<EcsTransform>().set(entity,
                                          {hierarchy_.localTransform(object)});
        byObject_.emplace(object.value, entity);
        byEntity_.emplace(entity.value, object);
        return entity;
    }

    void unbind(object::ObjectHandle object) override {
        const auto it = byObject_.find(object.value);
        if (it == byObject_.end()) {
            return;
        }
        ecs_.destroyEntity(it->second);
        byEntity_.erase(it->second.value);
        byObject_.erase(it);
    }

    EntityId entityOf(object::ObjectHandle object) const override {
        const auto it = byObject_.find(object.value);
        return it != byObject_.end() ? it->second : EntityId{};
    }

    object::ObjectHandle objectOf(EntityId entity) const override {
        const auto it = byEntity_.find(entity.value);
        return it != byEntity_.end() ? it->second : object::ObjectHandle::invalid();
    }

    void pushAuthoringState() override {
        auto& transforms = ecs_.storeFor<EcsTransform>();
        for (const auto& [objectId, entity] : byObject_) {
            transforms.set(entity,
                           {hierarchy_.localTransform(object::ObjectHandle{objectId})});
        }
    }

    void pullEcsResults() override {
        auto& transforms = ecs_.storeFor<EcsTransform>();
        for (const auto& [objectId, entity] : byObject_) {
            if (const auto* transform = transforms.get(entity)) {
                hierarchy_.setLocalTransform(object::ObjectHandle{objectId},
                                             transform->value);
            }
        }
    }

private:
    EcsWorld& ecs_;
    object::IObjectHierarchyAccess& hierarchy_;
    std::unordered_map<std::uint64_t, EntityId> byObject_;
    std::unordered_map<std::uint64_t, object::ObjectHandle> byEntity_;
};

} // namespace

std::unique_ptr<IEcsObjectSync> createEcsObjectSync(
    EcsWorld& ecs, object::IObjectHierarchyAccess& hierarchy) {
    return std::make_unique<EcsObjectSyncImpl>(ecs, hierarchy);
}

} // namespace sky::ecs
