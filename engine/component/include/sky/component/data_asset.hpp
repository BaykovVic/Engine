#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>

#include "sky/component/component_world.hpp"
#include "sky/serialization/serialization.hpp"

namespace sky::component {

/// A data asset: a typed bag of authored field values living as a project
/// file (`.skydata`; materials reuse the format as `.skymat`) — the
/// ScriptableObject analog. `parentGuid` optionally points at a base asset
/// whose fields this one overrides (Unigine-Properties-style inheritance);
/// resolving the parent by GUID is the caller's concern.
struct DataAssetDesc {
    std::string typeId;
    std::uint64_t parentGuid = 0;
    std::map<std::string, FieldValue> fields;
};

/// Writes the asset as a versioned `sky.data` document (SKYB container).
bool saveDataAsset(serialization::ISerializationBackend& storage,
                   const std::filesystem::path& path, const DataAssetDesc& desc);

/// Reads a `sky.data` document; nullopt on a missing/foreign/corrupt file.
std::optional<DataAssetDesc> loadDataAsset(
    serialization::ISerializationBackend& storage,
    const std::filesystem::path& path);

/// Parent-chain merge: the base fields overlaid with the overrides (an
/// override wins on a name collision; base-only fields shine through).
std::map<std::string, FieldValue> mergedFields(
    const std::map<std::string, FieldValue>& base,
    const std::map<std::string, FieldValue>& overrides);

} // namespace sky::component
