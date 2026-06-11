#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "sky/ecs/ecs_world.hpp"

namespace sky::ecs {
namespace {

class EcsWorldImpl final : public EcsWorld {
public:
    // IEcsWorld

    EntityId createEntity() override {
        const EntityId entity{nextId_++};
        alive_.insert(entity.value);
        return entity;
    }

    void destroyEntity(EntityId entity) override {
        if (alive_.erase(entity.value) == 0) {
            return;
        }
        for (auto& [type, store] : stores_) {
            store->remove(entity);
        }
    }

    bool isAlive(EntityId entity) const override { return alive_.contains(entity.value); }

    IEcsComponentStore& store(std::type_index componentType) override {
        const auto it = stores_.find(componentType);
        if (it == stores_.end()) {
            throw std::out_of_range("ECS store not registered for component type");
        }
        return *it->second;
    }

    // IEcsSystemScheduler

    void registerSystem(IEcsSystem& system) override { systems_.push_back(&system); }

    void unregisterSystem(IEcsSystem& system) override { std::erase(systems_, &system); }

    void tick(double deltaSeconds) override {
        for (auto* system : systems_) {
            system->update(deltaSeconds);
        }
    }

    // IEcsQueryService

    std::vector<EntityId> entitiesWith(
        const std::vector<std::type_index>& componentTypes) const override {
        std::vector<EntityId> result;
        for (const auto id : alive_) {
            const EntityId entity{id};
            const bool matches = std::ranges::all_of(componentTypes, [&](std::type_index type) {
                const auto it = stores_.find(type);
                return it != stores_.end() && it->second->has(entity);
            });
            if (matches) {
                result.push_back(entity);
            }
        }
        return result;
    }

protected:
    StoreMap& stores() override { return stores_; }

private:
    std::uint64_t nextId_ = 1;
    std::unordered_set<std::uint64_t> alive_;
    StoreMap stores_;
    std::vector<IEcsSystem*> systems_;
};

} // namespace

std::unique_ptr<EcsWorld> createEcsWorld() {
    return std::make_unique<EcsWorldImpl>();
}

} // namespace sky::ecs
