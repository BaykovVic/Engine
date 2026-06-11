#pragma once

#include <memory>

#include "sky/platform/file_system.hpp"
#include "sky/serialization/serialization.hpp"

namespace sky::serialization {

/// Serialization backend storing blobs in Sky Engine's container format:
/// magic + schema id + schema version + payload, via the given file system.
std::unique_ptr<ISerializationBackend> createFileSerializationBackend(
    platform::IFileSystem& fileSystem);

} // namespace sky::serialization
