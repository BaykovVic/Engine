#pragma once

#include <functional>
#include <string>
#include <vector>

#include "sky/asset/asset_system.hpp"
#include "sky/component/component_model.hpp"
#include "sky/object/object_model.hpp"

namespace sky::editor {

/// What the user currently has selected across hierarchy, viewport and
/// asset browser. Owned by Editor Tools.
struct SelectionContext {
    std::vector<object::ObjectHandle> objects;
    std::vector<component::ComponentHandle> components;
    std::vector<asset::AssetId> assets;
};

/// The inspector's current focus, derived from the selection.
struct InspectorContext {
    SelectionContext selection;
    std::string activeTab;
};

/// An authoring command issued by a tool. All editor mutations of project
/// and scene data travel through the command bus — never by reaching into
/// module internals — so undo/redo stays possible.
struct ToolCommand {
    std::string commandId;
    std::string payload;
};

/// Editor Tools contract: dispatch of authoring commands into engine core.
class IToolCommandBus {
public:
    virtual ~IToolCommandBus() = default;

    virtual bool execute(const ToolCommand& command) = 0;
    virtual bool undo() = 0;
    virtual bool redo() = 0;
};

/// Editor Tools contract: shared selection state for hierarchy, inspector,
/// viewport and asset browser.
class ISelectionService {
public:
    virtual ~ISelectionService() = default;

    using SelectionChanged = std::function<void(const SelectionContext&)>;

    virtual void select(SelectionContext selection) = 0;
    [[nodiscard]] virtual const SelectionContext& current() const = 0;
    virtual void onSelectionChanged(SelectionChanged callback) = 0;
};

/// A viewport manipulation tool (move/rotate/scale gizmo, terrain brush).
class IGizmoTool {
public:
    virtual ~IGizmoTool() = default;

    [[nodiscard]] virtual std::string toolId() const = 0;
    virtual void activate(const SelectionContext& selection) = 0;
    virtual void deactivate() = 0;
};

} // namespace sky::editor
