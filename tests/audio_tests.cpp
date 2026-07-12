#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>
#include <vector>

#include "sky/audio/audio.hpp"
#include "sky_test.hpp"

namespace {

void appendU32(std::vector<std::byte>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(std::byte((v >> (8 * i)) & 0xFF));
    }
}

void appendU16(std::vector<std::byte>& out, std::uint16_t v) {
    out.push_back(std::byte(v & 0xFF));
    out.push_back(std::byte((v >> 8) & 0xFF));
}

void appendTag(std::vector<std::byte>& out, const char* tag) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(std::byte(tag[i]));
    }
}

/// Builds a minimal RIFF/WAVE buffer around raw sample bytes.
std::vector<std::byte> makeWav(std::uint16_t format, std::uint16_t channels,
                               std::uint32_t sampleRate,
                               std::uint16_t bitsPerSample,
                               const std::vector<std::byte>& payload) {
    std::vector<std::byte> out;
    appendTag(out, "RIFF");
    appendU32(out, 36 + std::uint32_t(payload.size()));
    appendTag(out, "WAVE");
    appendTag(out, "fmt ");
    appendU32(out, 16);
    appendU16(out, format);
    appendU16(out, channels);
    appendU32(out, sampleRate);
    const std::uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
    appendU32(out, byteRate);
    appendU16(out, std::uint16_t(channels * bitsPerSample / 8));
    appendU16(out, bitsPerSample);
    appendTag(out, "data");
    appendU32(out, std::uint32_t(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

std::vector<std::byte> pcm16Payload(const std::vector<std::int16_t>& samples) {
    std::vector<std::byte> payload(samples.size() * 2);
    std::memcpy(payload.data(), samples.data(), payload.size());
    return payload;
}

void testWavDecode() {
    const auto library = sky::audio::createAudioClipLibrary();

    // 16-bit stereo at the mix rate: values land as v/32768, no resampling.
    const auto stereo = library->loadWav(makeWav(
        1, 2, sky::audio::kMixSampleRate, 16,
        pcm16Payload({16384, -16384, 32767, -32768})));
    CHECK(stereo.isValid());
    const auto* clip = library->clip(stereo);
    CHECK(clip != nullptr);
    if (clip != nullptr) {
        CHECK(clip->frameCount == 2);
        CHECK(clip->samples[0] == 0.5f);
        CHECK(clip->samples[1] == -0.5f);
        CHECK(clip->samples[2] == 32767.0f / 32768.0f);
        CHECK(clip->samples[3] == -1.0f);
    }

    // Mono duplicates into both ears.
    const auto mono = library->loadWav(
        makeWav(1, 1, sky::audio::kMixSampleRate, 16, pcm16Payload({16384})));
    const auto* monoClip = library->clip(mono);
    CHECK(monoClip != nullptr);
    if (monoClip != nullptr) {
        CHECK(monoClip->frameCount == 1);
        CHECK(monoClip->samples[0] == 0.5f);
        CHECK(monoClip->samples[1] == 0.5f);
    }

    // IEEE float 32 passes through bit-exact.
    std::vector<std::byte> floats(2 * sizeof(float));
    const float values[2] = {0.25f, -0.75f};
    std::memcpy(floats.data(), values, sizeof(values));
    const auto f32 = library->loadWav(
        makeWav(3, 2, sky::audio::kMixSampleRate, 32, floats));
    const auto* f32Clip = library->clip(f32);
    CHECK(f32Clip != nullptr);
    if (f32Clip != nullptr) {
        CHECK(f32Clip->samples[0] == 0.25f);
        CHECK(f32Clip->samples[1] == -0.75f);
    }

    // Half the mix rate roughly doubles the frame count (linear resample).
    const auto low = library->loadWav(
        makeWav(1, 1, sky::audio::kMixSampleRate / 2, 16,
                pcm16Payload({0, 16384, 32767, 16384})));
    const auto* lowClip = library->clip(low);
    CHECK(lowClip != nullptr);
    if (lowClip != nullptr) {
        CHECK(lowClip->frameCount == 8);
        CHECK(lowClip->samples[0] == 0.0f); // endpoints are preserved exactly
    }

    // Garbage and unsupported layouts are rejected, not crashed on.
    CHECK(!library->loadWav({std::byte{0xDE}, std::byte{0xAD}}).isValid());
    CHECK(!library
               ->loadWav(makeWav(1, 2, sky::audio::kMixSampleRate, 8,
                                 pcm16Payload({0, 0})))
               .isValid());

    library->unload(stereo);
    CHECK(library->clip(stereo) == nullptr);
}

void testMixerVoices() {
    const auto library = sky::audio::createAudioClipLibrary();
    const auto mixer = sky::audio::createAudioMixer(*library);
    // A 3-frame ramp: L = 0.1, 0.2, 0.3 (R mirrors L via mono duplication).
    const auto clip = library->loadWav(
        makeWav(1, 1, sky::audio::kMixSampleRate, 16,
                pcm16Payload({3277, 6554, 9830})));
    CHECK(clip.isValid());
    const auto* data = library->clip(clip);
    CHECK(data != nullptr && data->frameCount == 3);

    float out[8] = {0};
    // Volume scales every sample; the voice ends inside the buffer and the
    // tail stays silent; the finished voice is swept.
    const auto voice = mixer->play(clip, {0.5f, false});
    CHECK(voice.isValid());
    CHECK(mixer->isPlaying(voice));
    mixer->mix(out, 4);
    CHECK(out[0] == data->samples[0] * 0.5f);
    CHECK(out[1] == data->samples[1] * 0.5f);
    CHECK(out[4] == data->samples[4] * 0.5f);
    CHECK(out[6] == 0.0f);
    CHECK(out[7] == 0.0f);
    CHECK(!mixer->isPlaying(voice));
    CHECK(mixer->activeVoices() == 0);
    CHECK(mixer->mixedFrames() == 4);

    // Two voices sum sample by sample.
    mixer->play(clip, {1.0f, false});
    mixer->play(clip, {1.0f, false});
    mixer->mix(out, 2);
    CHECK(out[0] == data->samples[0] * 2.0f);
    CHECK(out[2] == data->samples[2] * 2.0f);
    mixer->stopAll();

    // A looping voice wraps mid-buffer and never ends on its own.
    const auto loop = mixer->play(clip, {1.0f, true});
    mixer->mix(out, 4);
    CHECK(out[6] == data->samples[0]); // frame 3 wrapped to clip frame 0
    CHECK(mixer->isPlaying(loop));
    mixer->stop(loop);
    CHECK(mixer->activeVoices() == 0);
    mixer->mix(out, 2);
    CHECK(out[0] == 0.0f); // silence after stop

    // Unknown clips refuse to play.
    CHECK(!mixer->play(sky::audio::ClipHandle{999}, {}).isValid());
}

void testOutputsDrainMixer() {
    const auto library = sky::audio::createAudioClipLibrary();
    const auto mixer = sky::audio::createAudioMixer(*library);

    // The null output paces the mixer against the wall clock.
    {
        const auto output = sky::audio::createNullAudioOutput();
        CHECK(output != nullptr);
        CHECK(output->start(*mixer));
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (mixer->mixedFrames() == 0 &&
               std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        output->stop();
        CHECK(mixer->mixedFrames() > 0);
    }

    // ALSA (structural): when a device opens — the "null" PCM in containers —
    // the same drain contract holds. Absence is a clean nullptr, not a crash.
    const auto before = mixer->mixedFrames();
    if (const auto alsa = sky::audio::createAlsaAudioOutput(); alsa != nullptr) {
        CHECK(alsa->start(*mixer));
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (mixer->mixedFrames() == before &&
               std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        alsa->stop();
        CHECK(mixer->mixedFrames() > before);
    } else {
        std::printf("audio_tests: ALSA device unavailable, structural output "
                    "check ran on the null output only\n");
    }
}

} // namespace

int main() {
    testWavDecode();
    testMixerVoices();
    testOutputsDrainMixer();
    return sky::test::summary("audio_tests");
}
