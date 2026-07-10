#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "sky/editor/tools/editor_tools.hpp"

namespace sky::editor {

/// Concrete IToolCommandBus from the modules doc: editor tools execute
/// authoring commands by id through this bus instead of reaching into
/// engine internals; undo/redo delegate to the editor history.
class ToolCommandBus : public IToolCommandBus {
public:
    ~ToolCommandBus() override = default;

    using Handler = std::function<bool(const ToolCommand&)>;
    using HistoryDelegate = std::function<bool()>;

    virtual bool registerHandler(const std::string& commandId, Handler handler) = 0;
    [[nodiscard]] virtual std::vector<std::string> registeredCommands() const = 0;
    virtual void setUndoDelegate(HistoryDelegate undo) = 0;
    virtual void setRedoDelegate(HistoryDelegate redo) = 0;
};

std::unique_ptr<ToolCommandBus> createToolCommandBus();

} // namespace sky::editor
