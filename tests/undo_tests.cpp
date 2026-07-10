// Undo/redo of editor commands against the real engine worlds. The command
// layer is Qt-free, so it is verified here without the UI.

#include "editor_commands.hpp"
#include "sky/editor/viewport/tool_command_bus.hpp"
#include "sky_test.hpp"

namespace {

using sky::editor::EditorContext;
using sky::editor::UndoStack;

void testTransformUndoRedo() {
    EditorContext context;
    UndoStack stack(context);

    const auto crates = context.objects->findByName("Crate B");
    CHECK(crates.size() == 1);
    const auto crate = crates.front();

    const auto before = context.objects->localTransform(crate);
    auto after = before;
    after.position.x = 7.0f;
    context.objects->setLocalTransform(crate, after);
    stack.push(sky::editor::makeTransformCommand(crate, before, after));

    CHECK(stack.canUndo());
    CHECK(!stack.canRedo());
    CHECK(stack.undoLabel() == "Transform");

    CHECK(stack.undo());
    CHECK(context.objects->localTransform(crate).position.x == before.position.x);
    CHECK(stack.canRedo());

    CHECK(stack.redo());
    CHECK(context.objects->localTransform(crate).position.x == 7.0f);
}

void testDeleteRestoresSubtree() {
    EditorContext context;
    UndoStack stack(context);

    // A crate with a child: components, physics and hierarchy must survive
    // the delete/undo round-trip.
    const auto crate = context.createCrate("Tower", {3.0f, 4.0f, 0.0f});
    const auto child = context.createEmpty("Flag");
    context.reparent(child, crate);

    auto command = sky::editor::makeDeleteCommand(context, crate);
    context.destroyObject(crate);
    stack.push(std::move(command));
    CHECK(!context.objects->exists(crate));
    CHECK(!context.objects->exists(child));

    CHECK(stack.undo());
    const auto restored = context.objects->findByName("Tower");
    CHECK(restored.size() == 1);
    CHECK(context.objects->localTransform(restored.front()).position.x == 3.0f);
    CHECK(context.components->componentsOf(restored.front()).size() == 3);
    CHECK(context.hasPhysicsBody(restored.front()));
    CHECK(context.objects->childrenOf(restored.front()).size() == 1);
    CHECK(context.objects->nameOf(
              context.objects->childrenOf(restored.front()).front()) == "Flag");

    // Redo deletes the restored copy again.
    CHECK(stack.redo());
    CHECK(context.objects->findByName("Tower").empty());
}

void testCreateRenameReparent() {
    EditorContext context;
    UndoStack stack(context);
    const auto rootCount = context.rootObjects().size();

    // Create.
    const auto cube = context.createCrate("Cube 1", {0.0f, 5.0f, 0.0f});
    stack.push(sky::editor::makeCreateCommand(cube, "Cube 1", {0.0f, 5.0f, 0.0f}, true));
    CHECK(context.rootObjects().size() == rootCount + 1);
    CHECK(stack.undo());
    CHECK(context.rootObjects().size() == rootCount);
    CHECK(stack.redo());
    CHECK(context.objects->findByName("Cube 1").size() == 1);

    // Rename.
    const auto renamed = context.objects->findByName("Cube 1").front();
    context.objects->renameObject(renamed, "Hero");
    stack.push(sky::editor::makeRenameCommand(renamed, "Cube 1", "Hero"));
    CHECK(stack.undo());
    CHECK(context.objects->nameOf(renamed) == "Cube 1");
    CHECK(stack.redo());
    CHECK(context.objects->nameOf(renamed) == "Hero");

    // Reparent under the terrain object, then undo back to root with the
    // exact local transform restored.
    const auto ground = context.objects->findByName("Terrain").front();
    const auto oldLocal = context.objects->localTransform(renamed);
    context.reparent(renamed, ground);
    stack.push(sky::editor::makeReparentCommand(
        renamed, sky::object::ObjectHandle::invalid(), ground, oldLocal));
    CHECK(context.objects->parentOf(renamed) == ground);
    CHECK(stack.undo());
    CHECK(!context.objects->parentOf(renamed).isValid());
    CHECK(context.objects->localTransform(renamed) == oldLocal);
}

void testHistoryDiscipline() {
    EditorContext context;
    UndoStack stack(context);
    const auto crate = context.objects->findByName("Crate A").front();
    const auto base = context.objects->localTransform(crate);

    // Three steps, undo two, then a new push must clear the redo branch.
    for (int i = 1; i <= 3; ++i) {
        auto after = base;
        after.position.x = static_cast<float>(i);
        context.objects->setLocalTransform(crate, after);
        auto before = base;
        before.position.x = static_cast<float>(i - 1);
        stack.push(sky::editor::makeTransformCommand(
            crate, i == 1 ? base : before, after));
    }
    CHECK(stack.undo());
    CHECK(stack.undo());
    CHECK(context.objects->localTransform(crate).position.x == 1.0f);
    CHECK(stack.canRedo());

    auto fork = base;
    fork.position.y = 9.0f;
    context.objects->setLocalTransform(crate, fork);
    stack.push(sky::editor::makeTransformCommand(crate, base, fork));
    CHECK(!stack.canRedo());

    // Duplicate + undo removes the copy.
    const auto copy = context.duplicateObject(crate);
    stack.push(sky::editor::makeDuplicateCommand(crate, copy));
    CHECK(context.objects->exists(copy));
    CHECK(stack.undo());
    CHECK(!context.objects->exists(copy));
}

void testToolCommandBus() {
    const auto bus = sky::editor::createToolCommandBus();

    // Unknown commands and missing delegates fail without side effects.
    CHECK(!bus->execute({"unknown.command", ""}));
    CHECK(!bus->undo());
    CHECK(!bus->redo());

    std::string lastPayload;
    CHECK(bus->registerHandler("scene.spawn", [&](const sky::editor::ToolCommand& cmd) {
        lastPayload = cmd.payload;
        return true;
    }));
    CHECK(!bus->registerHandler("scene.spawn", [](const auto&) { return true; }));
    CHECK(bus->registeredCommands() == std::vector<std::string>{"scene.spawn"});

    CHECK(bus->execute({"scene.spawn", "crate:5"}));
    CHECK(lastPayload == "crate:5");

    int undos = 0, redos = 0;
    bus->setUndoDelegate([&] { ++undos; return true; });
    bus->setRedoDelegate([&] { ++redos; return true; });
    CHECK(bus->undo());
    CHECK(bus->redo());
    CHECK(undos == 1);
    CHECK(redos == 1);
}

} // namespace

int main() {
    testTransformUndoRedo();
    testDeleteRestoresSubtree();
    testCreateRenameReparent();
    testHistoryDiscipline();
    testToolCommandBus();
    return sky::test::summary("undo_tests");
}
