#pragma once

#include <memory>
#include <unordered_map>

#include "sky/ecs/ecs.hpp"

namespace sky::ecs {

/// Typed component storage. The current layout is a hash map keyed by
/// entity; data-oriented chunked storage replaces it behind the same
/// contract.
template <typename T>
class TypedComponentStore final : public IEcsComponentStore {
public:
    std::type_index componentType() const override { return typeid(T); }

    bool has(EntityId entity) const override { return data_.contains(entity.value); }

    void remove(EntityId entity) override { data_.erase(entity.value); }

    std::size_t count() const override { return data_.size(); }

    T& set(EntityId entity, T value) {
        return data_.insert_or_assign(entity.value, std::move(value)).first->second;
    }

    T* get(EntityId entity) {
        const auto it = data_.find(entity.value);
        return it != data_.end() ? &it->second : nullptr;
    }

private:
    std::unordered_map<std::uint64_t, T> data_;
};

/// In-memory implementation of the ECS Layer: isolated entity world plus
/// system scheduling. Data flows to/from the Object Model only through the
/// explicit sync contract.
class EcsWorld : public IEcsWorld, public IEcsSystemScheduler, public IEcsQueryService {
public:
    ~EcsWorld() override = default;

    /// Registers (or returns the existing) typed store for T.
    template <typename T>
    TypedComponentStore<T>& storeFor() {
        auto& slot = stores()[std::type_index(typeid(T))];
        if (!slot) {
            slot = std::make_unique<TypedComponentStore<T>>();
        }
        return static_cast<TypedComponentStore<T>&>(*slot);
    }

protected:
    using StoreMap =
        std::unordered_map<std::type_index, std::unique_ptr<IEcsComponentStore>>;

    virtual StoreMap& stores() = 0;
};

std::unique_ptr<EcsWorld> createEcsWorld();

} // namespace sky::ecs
