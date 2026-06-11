#pragma once

#include "sky/mapgen/map_generation.hpp"

namespace sky::mapgen {

/// Stage names understood by the default pipeline.
inline constexpr const char* kStageHeightfield = "heightfield";
inline constexpr const char* kStagePlacement = "placement";

/// Default procedural pipeline: layered value-noise heightfield plus
/// deterministic object scattering. The same profile (seed, size, stages)
/// always produces identical output.
std::unique_ptr<IGenerationPipeline> createGenerationPipeline();

} // namespace sky::mapgen
