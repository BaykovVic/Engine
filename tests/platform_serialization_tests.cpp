#include <cstdlib>
#include <filesystem>

#include "sky/platform/platform_services.hpp"
#include "sky/serialization/backends.hpp"
#include "sky/serialization/byte_stream.hpp"
#include "sky_test.hpp"

namespace {

std::filesystem::path tempDir() {
    return std::filesystem::temp_directory_path() / "sky_engine_tests" / "platform_serialization";
}

void testFileSystem() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto root = tempDir() / "fs";
    const auto file = root / "nested" / "data.bin";

    const std::vector<std::byte> payload{std::byte{1}, std::byte{2}, std::byte{3}};
    CHECK(fileSystem->writeAll(file, payload));
    CHECK(fileSystem->exists(file));
    CHECK(fileSystem->isDirectory(root / "nested"));
    CHECK(fileSystem->readAll(file) == payload);
    CHECK(fileSystem->list(root / "nested").size() == 1);
    CHECK(fileSystem->remove(root));
    CHECK(!fileSystem->exists(file));
}

void testByteStream() {
    sky::serialization::ByteWriter writer;
    writer.writeU32(42);
    writer.writeString("sky");
    writer.writeF32(1.5f);

    sky::serialization::ByteReader reader(writer.buffer());
    CHECK(reader.readU32() == 42u);
    CHECK(reader.readString() == "sky");
    CHECK(reader.readF32() == 1.5f);
    // Reading past the end fails instead of returning garbage.
    CHECK(!reader.readU64().has_value());
}

void testSerializationRoundTrip() {
    const auto fileSystem = sky::platform::createStdFileSystem();
    const auto backend = sky::serialization::createFileSerializationBackend(*fileSystem);
    const auto path = tempDir() / "blob.skyb";

    sky::serialization::SerializedBlob blob{
        "sky.test", {1, 2}, {std::byte{0xAB}, std::byte{0xCD}}};
    CHECK(backend->write(path, blob));

    const auto restored = backend->read(path);
    CHECK(restored.has_value());
    CHECK(restored->schemaId == "sky.test");
    CHECK((restored->version == sky::serialization::SchemaVersion{1, 2}));
    CHECK(restored->payload == blob.payload);

    // A corrupted file must be rejected, not misparsed.
    fileSystem->writeAll(path, {std::byte{0x00}, std::byte{0x01}});
    CHECK(!backend->read(path).has_value());

    fileSystem->remove(tempDir());
}

void testTimerAndThreading() {
    const auto timer = sky::platform::createChronoTimerService();
    const auto first = timer->frameTimestamp();
    const auto second = timer->frameTimestamp();
    CHECK(second >= first);

    const auto threading = sky::platform::createStdThreading();
    CHECK(threading->hardwareConcurrency() >= 1);
    CHECK(threading->currentThreadId() != 0);
}

void testHeadlessWindowSystem() {
    const auto windows = sky::platform::createHeadlessWindowSystem();
    const auto window = windows->createWindow({"Sky", 800, 600, true});
    CHECK(window.isValid());
    windows->resize(window, 1024, 768);
    windows->setTitle(window, "Sky Engine");
    CHECK(windows->pumpEvents());
    windows->destroyWindow(window);
}

} // namespace

int main() {
    testFileSystem();
    testByteStream();
    testSerializationRoundTrip();
    testTimerAndThreading();
    testHeadlessWindowSystem();
    return sky::test::summary("platform_serialization_tests");
}
