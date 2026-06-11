#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "sky/core/runtime_services.hpp"

namespace sky::core {
namespace {

class EventBus final : public IEventBus {
public:
    EventSubscription subscribe(std::string_view topic, Handler handler) override {
        const std::scoped_lock lock(mutex_);
        const EventSubscription subscription{nextId_++};
        topics_[std::string(topic)].push_back({subscription.id, std::move(handler)});
        return subscription;
    }

    void unsubscribe(EventSubscription subscription) override {
        const std::scoped_lock lock(mutex_);
        for (auto& [topic, handlers] : topics_) {
            std::erase_if(handlers, [&](const Entry& entry) {
                return entry.id == subscription.id;
            });
        }
    }

    void publish(const EventEnvelope& event) override {
        // Copy handlers out so subscribers may (un)subscribe during dispatch.
        std::vector<Handler> handlers;
        {
            const std::scoped_lock lock(mutex_);
            const auto it = topics_.find(std::string(event.topic));
            if (it == topics_.end()) {
                return;
            }
            handlers.reserve(it->second.size());
            for (const auto& entry : it->second) {
                handlers.push_back(entry.handler);
            }
        }
        for (const auto& handler : handlers) {
            handler(event);
        }
    }

private:
    struct Entry {
        std::uint64_t id;
        Handler handler;
    };

    std::mutex mutex_;
    std::uint64_t nextId_ = 1;
    std::unordered_map<std::string, std::vector<Entry>> topics_;
};

} // namespace

std::unique_ptr<IEventBus> createEventBus() {
    return std::make_unique<EventBus>();
}

} // namespace sky::core
