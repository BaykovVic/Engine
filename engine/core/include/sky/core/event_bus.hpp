#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace sky::core {

/// Subscription token returned by IEventBus; used to unsubscribe.
struct EventSubscription {
    std::uint64_t id = 0;
};

/// Type-erased event payload. Concrete event types are defined by the
/// publishing module; the bus itself stays domain-agnostic.
struct EventEnvelope {
    std::string_view topic;
    const void* payload = nullptr;
};

/// Core Foundation contract: in-process publish/subscribe events, not bound
/// to any specific domain logic.
class IEventBus {
public:
    virtual ~IEventBus() = default;

    using Handler = std::function<void(const EventEnvelope&)>;

    virtual EventSubscription subscribe(std::string_view topic, Handler handler) = 0;
    virtual void unsubscribe(EventSubscription subscription) = 0;
    virtual void publish(const EventEnvelope& event) = 0;
};

} // namespace sky::core
