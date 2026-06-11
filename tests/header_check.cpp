// Includes every public contract header of Sky Engine and instantiates the
// core value types, so any breakage of the contract surface fails the build.

#include "sky/platform/file_system.hpp"
#include "sky/platform/input_source.hpp"
#include "sky/platform/threading.hpp"
#include "sky/platform/timer_service.hpp"
#include "sky/platform/window_system.hpp"

#include "sky/core/config_service.hpp"
#include "sky/core/diagnostics.hpp"
#include "sky/core/event_bus.hpp"
#include "sky/core/handle.hpp"
#include "sky/core/job_scheduler.hpp"
#include "sky/core/logger.hpp"
#include "sky/core/math.hpp"

#include "sky/asset/asset_system.hpp"
#include "sky/component/component_model.hpp"
#include "sky/ecs/ecs.hpp"
#include "sky/mapgen/map_generation.hpp"
#include "sky/object/object_model.hpp"
#include "sky/package/package_system.hpp"
#include "sky/physics/physics.hpp"
#include "sky/project/project_model.hpp"
#include "sky/rendering/rendering.hpp"
#include "sky/scene/scene_system.hpp"
#include "sky/scripting/script_host.hpp"
#include "sky/scripting/scripting_boundary.hpp"
#include "sky/serialization/serialization.hpp"
#include "sky/terrain/terrain.hpp"

#ifdef SKY_HAS_OPENGL_BACKEND
#include "sky/rendering_opengl/opengl_backend.hpp"
#endif
#ifdef SKY_HAS_VULKAN_BACKEND
#include "sky/rendering_vulkan/vulkan_backend.hpp"
#endif

#include "sky/editor/shell/editor_shell.hpp"
#include "sky/editor/tools/editor_tools.hpp"
#include "sky/editor/viewport/viewport_bridge.hpp"

#include <cstdio>

int main() {
    const sky::object::ObjectHandle object{42};
    const sky::scene::SceneHandle scene{7};
    const sky::asset::AssetId asset{1};
    const sky::ecs::EntityId entity{3};
    const sky::scripting::NativeHandle script{9};

    const bool ok = object.isValid() && scene.isValid() && asset.isValid() &&
                    entity.isValid() && script.isValid() &&
                    !sky::object::ObjectHandle::invalid().isValid();

    std::puts(ok ? "sky engine contract surface: OK" : "sky engine contract surface: FAILED");
    return ok ? 0 : 1;
}
