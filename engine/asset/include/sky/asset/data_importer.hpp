#pragma once

#include <memory>

#include "sky/asset/asset_system.hpp"

namespace sky::asset {

/// Importer for data assets: `.skydata` (typed field bags, the
/// ScriptableObject analog) and `.skymat` (materials). Content parsing
/// lives with the component module; the asset pipeline only needs the
/// type, so identity (GUID sidecars) and references work uniformly.
std::unique_ptr<IAssetImporter> createDataImporter();

} // namespace sky::asset
