#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "editor_context.hpp"

namespace sky::editor {

/// One reversible editor action. Convention: the action has already been
/// applied when the command is pushed; undo() reverts it, redo() re-applies.
///
/// Commands that recreate objects (delete/create) produce new handles on
/// redo and keep their internal references updated; commands further down
/// the stack referencing the old handle become no-ops, which is safe.
class IEditorCommand {
public:
    virtual ~IEditorCommand() = default;

    [[nodiscard]] virtual std::string label() const = 0;
    virtual void undo(EditorContext& context) = 0;
    virtual void redo(EditorContext& context) = 0;
};

/// Linear undo history with a redo branch that is discarded on new pushes.
class UndoStack {
public:
    explicit UndoStack(EditorContext& context, std::size_t capacity = 100)
        : context_(context), capacity_(capacity) {}

    void push(std::unique_ptr<IEditorCommand> command);
    bool undo();
    bool redo();

    /// Drops the whole history (e.g. after loading a new scene, whose object
    /// handles make earlier commands meaningless).
    void clear() {
        undoList_.clear();
        redoList_.clear();
        notify();
    }

    [[nodiscard]] bool canUndo() const { return !undoList_.empty(); }
    [[nodiscard]] bool canRedo() const { return !redoList_.empty(); }
    [[nodiscard]] std::string undoLabel() const {
        return canUndo() ? undoList_.back()->label() : std::string{};
    }
    [[nodiscard]] std::string redoLabel() const {
        return canRedo() ? redoList_.back()->label() : std::string{};
    }

    void setOnChanged(std::function<void()> callback) {
        onChanged_ = std::move(callback);
    }

private:
    void notify() {
        if (onChanged_) {
            onChanged_();
        }
    }

    EditorContext& context_;
    std::size_t capacity_;
    std::vector<std::unique_ptr<IEditorCommand>> undoList_;
    std::vector<std::unique_ptr<IEditorCommand>> redoList_;
    std::function<void()> onChanged_;
};

// Factory helpers for the concrete commands.

std::unique_ptr<IEditorCommand> makeTransformCommand(object::ObjectHandle object,
                                                     const core::Transform& before,
                                                     const core::Transform& after);
std::unique_ptr<IEditorCommand> makeRenameCommand(object::ObjectHandle object,
                                                  std::string before,
                                                  std::string after);
/// `isCrate` selects between createEmpty and createCrate on redo.
std::unique_ptr<IEditorCommand> makeCreateCommand(object::ObjectHandle created,
                                                  std::string name, core::Vec3 position,
                                                  bool isCrate);
std::unique_ptr<IEditorCommand> makeDuplicateCommand(object::ObjectHandle source,
                                                     object::ObjectHandle copy);
/// Undo of a just-created object: undo destroys it, redo restores it from a
/// snapshot — works regardless of how it was created.
std::unique_ptr<IEditorCommand> makeCreateSnapshotCommand(EditorContext& context,
                                                          object::ObjectHandle created);
std::unique_ptr<IEditorCommand> makeDeleteCommand(EditorContext& context,
                                                  object::ObjectHandle object);
std::unique_ptr<IEditorCommand> makeReparentCommand(object::ObjectHandle object,
                                                    object::ObjectHandle oldParent,
                                                    object::ObjectHandle newParent,
                                                    const core::Transform& oldLocal);
std::unique_ptr<IEditorCommand> makeFieldCommand(component::ComponentHandle component,
                                                 std::string fieldName,
                                                 component::FieldValue before,
                                                 component::FieldValue after);
std::unique_ptr<IEditorCommand> makeMaterialEditCommand(
    rendering::MaterialHandle material, rendering::MaterialDesc before,
    rendering::MaterialDesc after);
std::unique_ptr<IEditorCommand> makeMaterialCreateCommand(rendering::MaterialDesc desc);

} // namespace sky::editor
