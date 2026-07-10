#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <typeindex>
#include <vector>

namespace sky::ecs {

/// Identity of an ECS entity. The ECS world is isolated from the Object
/// Model: data flows between them only through an explicit sync contract.
struct EntityId {
    std::uint64_t value = 0;

    [[nodiscard]] bool isValid() const noexcept { return value != 0; }
    auto operator<=>(const EntityId&) const = default;
};

/// Type-erased storage of one ECS component type. Concrete data-oriented
/// layouts (SoA chunks) live behind this contract.
class IEcsComponentStore {
public:
    virtual ~IEcsComponentStore() = default;

    [[nodiscard]] virtual std::type_index componentType() const = 0;
    [[nodiscard]] virtual bool has(EntityId entity) const = 0;
    virtual void remove(EntityId entity) = 0;
    [[nodiscard]] virtual std::size_t count() const = 0;
};

/// A system executed by the scheduler each frame over matching entities.
class IEcsSystem {
public:
    virtual ~IEcsSystem() = default;

    [[nodiscard]] virtual std::string name() const = 0;
    virtual void update(double deltaSeconds) = 0;
};

/// ECS Layer contract: the entity world — entity creation and component
/// store access.
class IEcsWorld {
public:
    virtual ~IEcsWorld() = default;

    virtual EntityId createEntity() = 0;
    virtual void destroyEntity(EntityId entity) = 0;
    [[nodiscard]] virtual bool isAlive(EntityId entity) const = 0;
    virtual IEcsComponentStore& store(std::type_index componentType) = 0;
};

/// ECS Layer contract: system registration and frame scheduling.
class IEcsSystemScheduler {
public:
    virtual ~IEcsSystemScheduler() = default;

    virtual void registerSystem(IEcsSystem& system) = 0;
    virtual void unregisterSystem(IEcsSystem& system) = 0;
    virtual void tick(double deltaSeconds) = 0;
};

/// ECS Layer contract: read-only entity queries.
class IEcsQueryService {
public:
    virtual ~IEcsQueryService() = default;

    [[nodiscard]] virtual std::vector<EntityId> entitiesWith(
        const std::vector<std::type_index>& componentTypes) const = 0;
};

} // namespace sky::ecs
