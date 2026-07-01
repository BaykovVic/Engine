#include "editor_commands.hpp"

namespace sky::editor {

void UndoStack::push(std::unique_ptr<IEditorCommand> command) {
    redoList_.clear();
    undoList_.push_back(std::move(command));
    if (undoList_.size() > capacity_) {
        undoList_.erase(undoList_.begin());
    }
    notify();
}

bool UndoStack::undo() {
    if (undoList_.empty()) {
        return false;
    }
    auto command = std::move(undoList_.back());
    undoList_.pop_back();
    command->undo(context_);
    redoList_.push_back(std::move(command));
    notify();
    return true;
}

bool UndoStack::redo() {
    if (redoList_.empty()) {
        return false;
    }
    auto command = std::move(redoList_.back());
    redoList_.pop_back();
    command->redo(context_);
    undoList_.push_back(std::move(command));
    notify();
    return true;
}

namespace {

class TransformCommand final : public IEditorCommand {
public:
    TransformCommand(object::ObjectHandle object, core::Transform before,
                     core::Transform after)
        : object_(object), before_(before), after_(after) {}

    std::string label() const override { return "Transform"; }

    void undo(EditorContext& context) override { apply(context, before_); }
    void redo(EditorContext& context) override { apply(context, after_); }

private:
    void apply(EditorContext& context, const core::Transform& transform) {
        if (context.objects->exists(object_)) {
            context.objects->setLocalTransform(object_, transform);
        }
    }

    object::ObjectHandle object_;
    core::Transform before_;
    core::Transform after_;
};

class RenameCommand final : public IEditorCommand {
public:
    RenameCommand(object::ObjectHandle object, std::string before, std::string after)
        : object_(object), before_(std::move(before)), after_(std::move(after)) {}

    std::string label() const override { return "Rename"; }

    void undo(EditorContext& context) override {
        context.objects->renameObject(object_, before_);
    }
    void redo(EditorContext& context) override {
        context.objects->renameObject(object_, after_);
    }

private:
    object::ObjectHandle object_;
    std::string before_;
    std::string after_;
};

class CreateCommand final : public IEditorCommand {
public:
    CreateCommand(object::ObjectHandle created, std::string name, core::Vec3 position,
                  bool isCrate)
        : created_(created), name_(std::move(name)), position_(position),
          isCrate_(isCrate) {}

    std::string label() const override { return isCrate_ ? "Create Cube" : "Create Empty"; }

    void undo(EditorContext& context) override { context.destroyObject(created_); }

    void redo(EditorContext& context) override {
        created_ = isCrate_ ? context.createCrate(name_, position_)
                            : context.createEmpty(name_);
    }

private:
    object::ObjectHandle created_;
    std::string name_;
    core::Vec3 position_;
    bool isCrate_;
};

class DuplicateCommand final : public IEditorCommand {
public:
    DuplicateCommand(object::ObjectHandle source, object::ObjectHandle copy)
        : source_(source), copy_(copy) {}

    std::string label() const override { return "Duplicate"; }

    void undo(EditorContext& context) override { context.destroyObject(copy_); }

    void redo(EditorContext& context) override {
        copy_ = context.duplicateObject(source_);
    }

private:
    object::ObjectHandle source_;
    object::ObjectHandle copy_;
};

/// Create, captured as a snapshot so redo restores the exact object regardless
/// of how it was originally created (primitive, model, drag-drop).
class CreateSnapshotCommand final : public IEditorCommand {
public:
    CreateSnapshotCommand(ObjectSnapshot snapshot, object::ObjectHandle parent,
                          object::ObjectHandle object)
        : snapshot_(std::move(snapshot)), parent_(parent), object_(object) {}

    std::string label() const override { return "Create " + snapshot_.name; }

    void undo(EditorContext& context) override { context.destroyObject(object_); }

    void redo(EditorContext& context) override {
        object_ = context.restoreObject(snapshot_, parent_);
    }

private:
    ObjectSnapshot snapshot_;
    object::ObjectHandle parent_;
    object::ObjectHandle object_;
};

class DeleteCommand final : public IEditorCommand {
public:
    DeleteCommand(ObjectSnapshot snapshot, object::ObjectHandle parent,
                  object::ObjectHandle object)
        : snapshot_(std::move(snapshot)), parent_(parent), object_(object) {}

    std::string label() const override { return "Delete " + snapshot_.name; }

    void undo(EditorContext& context) override {
        object_ = context.restoreObject(snapshot_, parent_);
    }

    void redo(EditorContext& context) override { context.destroyObject(object_); }

private:
    ObjectSnapshot snapshot_;
    object::ObjectHandle parent_;
    object::ObjectHandle object_;
};

class ReparentCommand final : public IEditorCommand {
public:
    ReparentCommand(object::ObjectHandle object, object::ObjectHandle oldParent,
                    object::ObjectHandle newParent, core::Transform oldLocal)
        : object_(object), oldParent_(oldParent), newParent_(newParent),
          oldLocal_(oldLocal) {}

    std::string label() const override { return "Reparent"; }

    void undo(EditorContext& context) override {
        context.reparent(object_, oldParent_);
        // reparent preserves the world position; restore the exact original
        // local transform instead.
        context.objects->setLocalTransform(object_, oldLocal_);
    }

    void redo(EditorContext& context) override { context.reparent(object_, newParent_); }

private:
    object::ObjectHandle object_;
    object::ObjectHandle oldParent_;
    object::ObjectHandle newParent_;
    core::Transform oldLocal_;
};

class FieldCommand final : public IEditorCommand {
public:
    FieldCommand(component::ComponentHandle component, std::string fieldName,
                 component::FieldValue before, component::FieldValue after)
        : component_(component), fieldName_(std::move(fieldName)),
          before_(std::move(before)), after_(std::move(after)) {}

    std::string label() const override { return "Edit " + fieldName_; }

    void undo(EditorContext& context) override {
        context.components->setField(component_, fieldName_, before_);
    }
    void redo(EditorContext& context) override {
        context.components->setField(component_, fieldName_, after_);
    }

private:
    component::ComponentHandle component_;
    std::string fieldName_;
    component::FieldValue before_;
    component::FieldValue after_;
};

class MaterialEditCommand final : public IEditorCommand {
public:
    MaterialEditCommand(rendering::MaterialHandle material,
                        rendering::MaterialDesc before, rendering::MaterialDesc after)
        : material_(material), before_(std::move(before)), after_(std::move(after)) {}

    std::string label() const override { return "Edit Material " + after_.name; }

    void undo(EditorContext& context) override {
        context.materials->updateMaterial(material_, before_);
    }
    void redo(EditorContext& context) override {
        context.materials->updateMaterial(material_, after_);
    }

private:
    rendering::MaterialHandle material_;
    rendering::MaterialDesc before_;
    rendering::MaterialDesc after_;
};

class MaterialCreateCommand final : public IEditorCommand {
public:
    explicit MaterialCreateCommand(rendering::MaterialDesc desc)
        : desc_(std::move(desc)) {}

    std::string label() const override { return "Create Material"; }

    void undo(EditorContext& context) override {
        if (const auto handle = context.materials->findMaterial(desc_.name)) {
            // Keep the latest state so redo restores what the user last saw.
            desc_ = context.materials->material(*handle);
            context.materials->removeMaterial(*handle);
        }
    }
    void redo(EditorContext& context) override {
        context.materials->createMaterial(desc_);
    }

private:
    rendering::MaterialDesc desc_;
};

} // namespace

std::unique_ptr<IEditorCommand> makeTransformCommand(object::ObjectHandle object,
                                                     const core::Transform& before,
                                                     const core::Transform& after) {
    return std::make_unique<TransformCommand>(object, before, after);
}

std::unique_ptr<IEditorCommand> makeRenameCommand(object::ObjectHandle object,
                                                  std::string before, std::string after) {
    return std::make_unique<RenameCommand>(object, std::move(before), std::move(after));
}

std::unique_ptr<IEditorCommand> makeCreateCommand(object::ObjectHandle created,
                                                  std::string name, core::Vec3 position,
                                                  bool isCrate) {
    return std::make_unique<CreateCommand>(created, std::move(name), position, isCrate);
}

std::unique_ptr<IEditorCommand> makeDuplicateCommand(object::ObjectHandle source,
                                                     object::ObjectHandle copy) {
    return std::make_unique<DuplicateCommand>(source, copy);
}

std::unique_ptr<IEditorCommand> makeCreateSnapshotCommand(EditorContext& context,
                                                          object::ObjectHandle created) {
    return std::make_unique<CreateSnapshotCommand>(
        context.snapshotObject(created), context.objects->parentOf(created), created);
}

std::unique_ptr<IEditorCommand> makeDeleteCommand(EditorContext& context,
                                                  object::ObjectHandle object) {
    return std::make_unique<DeleteCommand>(context.snapshotObject(object),
                                           context.objects->parentOf(object), object);
}

std::unique_ptr<IEditorCommand> makeReparentCommand(object::ObjectHandle object,
                                                    object::ObjectHandle oldParent,
                                                    object::ObjectHandle newParent,
                                                    const core::Transform& oldLocal) {
    return std::make_unique<ReparentCommand>(object, oldParent, newParent, oldLocal);
}

std::unique_ptr<IEditorCommand> makeFieldCommand(component::ComponentHandle component,
                                                 std::string fieldName,
                                                 component::FieldValue before,
                                                 component::FieldValue after) {
    return std::make_unique<FieldCommand>(component, std::move(fieldName),
                                          std::move(before), std::move(after));
}

std::unique_ptr<IEditorCommand> makeMaterialEditCommand(
    rendering::MaterialHandle material, rendering::MaterialDesc before,
    rendering::MaterialDesc after) {
    return std::make_unique<MaterialEditCommand>(material, std::move(before),
                                                 std::move(after));
}

std::unique_ptr<IEditorCommand> makeMaterialCreateCommand(rendering::MaterialDesc desc) {
    return std::make_unique<MaterialCreateCommand>(std::move(desc));
}

} // namespace sky::editor
