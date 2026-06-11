#include <map>

#include "sky/editor/viewport/tool_command_bus.hpp"

namespace sky::editor {
namespace {

class ToolCommandBusImpl final : public ToolCommandBus {
public:
    // IToolCommandBus

    bool execute(const ToolCommand& command) override {
        const auto it = handlers_.find(command.commandId);
        return it != handlers_.end() && it->second(command);
    }

    bool undo() override { return undo_ != nullptr && undo_(); }
    bool redo() override { return redo_ != nullptr && redo_(); }

    // ToolCommandBus

    bool registerHandler(const std::string& commandId, Handler handler) override {
        if (commandId.empty() || handler == nullptr) {
            return false;
        }
        return handlers_.emplace(commandId, std::move(handler)).second;
    }

    std::vector<std::string> registeredCommands() const override {
        std::vector<std::string> ids;
        ids.reserve(handlers_.size());
        for (const auto& [id, handler] : handlers_) {
            ids.push_back(id);
        }
        return ids;
    }

    void setUndoDelegate(HistoryDelegate undo) override { undo_ = std::move(undo); }
    void setRedoDelegate(HistoryDelegate redo) override { redo_ = std::move(redo); }

private:
    std::map<std::string, Handler> handlers_;
    HistoryDelegate undo_;
    HistoryDelegate redo_;
};

} // namespace

std::unique_ptr<ToolCommandBus> createToolCommandBus() {
    return std::make_unique<ToolCommandBusImpl>();
}

} // namespace sky::editor
