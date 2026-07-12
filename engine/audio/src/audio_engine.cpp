#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "sky/audio/audio.hpp"

namespace sky::audio {
namespace {

// --- WAV decoding -----------------------------------------------------------

std::uint32_t readU32(const std::byte* p) {
    std::uint32_t v = 0;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

std::uint16_t readU16(const std::byte* p) {
    std::uint16_t v = 0;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

struct WavData {
    std::uint16_t format = 0; // 1 = PCM int, 3 = IEEE float
    std::uint16_t channels = 0;
    std::uint32_t sampleRate = 0;
    std::uint16_t bitsPerSample = 0;
    const std::byte* payload = nullptr;
    std::uint32_t payloadBytes = 0;
};

bool parseWav(const std::vector<std::byte>& bytes, WavData& out) {
    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
        std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
        return false;
    }
    // Chunk walk: only fmt and data matter; everything else is skipped.
    std::size_t offset = 12;
    bool haveFmt = false;
    while (offset + 8 <= bytes.size()) {
        const auto* header = bytes.data() + offset;
        const auto chunkSize = readU32(header + 4);
        const auto* body = header + 8;
        if (offset + 8 + chunkSize > bytes.size()) {
            return false;
        }
        if (std::memcmp(header, "fmt ", 4) == 0 && chunkSize >= 16) {
            out.format = readU16(body);
            out.channels = readU16(body + 2);
            out.sampleRate = readU32(body + 4);
            out.bitsPerSample = readU16(body + 14);
            haveFmt = true;
        } else if (std::memcmp(header, "data", 4) == 0) {
            out.payload = body;
            out.payloadBytes = chunkSize;
        }
        offset += 8 + chunkSize + (chunkSize & 1u); // chunks are word-aligned
    }
    return haveFmt && out.payload != nullptr && out.channels >= 1 &&
           out.channels <= 2 && out.sampleRate > 0;
}

/// Decodes the supported sample formats to float and folds channels to
/// stereo (mono duplicates into both ears).
bool decodeToStereoFloat(const WavData& wav, std::vector<float>& stereo) {
    const std::uint32_t bytesPerSample = wav.bitsPerSample / 8u;
    if (bytesPerSample == 0 || wav.payloadBytes < bytesPerSample) {
        return false;
    }
    const std::uint32_t sampleCount = wav.payloadBytes / bytesPerSample;
    const std::uint32_t frames = sampleCount / wav.channels;
    const bool pcm16 = wav.format == 1 && wav.bitsPerSample == 16;
    const bool float32 = wav.format == 3 && wav.bitsPerSample == 32;
    if (frames == 0 || (!pcm16 && !float32)) {
        return false;
    }
    stereo.resize(std::size_t(frames) * 2);
    for (std::uint32_t frame = 0; frame < frames; ++frame) {
        float lane[2] = {0.0f, 0.0f};
        for (std::uint16_t ch = 0; ch < wav.channels; ++ch) {
            const auto* sample =
                wav.payload + (std::size_t(frame) * wav.channels + ch) * bytesPerSample;
            if (pcm16) {
                std::int16_t v = 0;
                std::memcpy(&v, sample, sizeof(v));
                lane[ch] = static_cast<float>(v) / 32768.0f;
            } else {
                std::memcpy(&lane[ch], sample, sizeof(float));
            }
        }
        if (wav.channels == 1) {
            lane[1] = lane[0];
        }
        stereo[std::size_t(frame) * 2 + 0] = lane[0];
        stereo[std::size_t(frame) * 2 + 1] = lane[1];
    }
    return true;
}

/// Linear resampling to the mix rate. Positions map endpoints to endpoints,
/// so short clips keep their first and last samples exactly.
std::vector<float> resampleStereo(const std::vector<float>& input,
                                  std::uint32_t inputRate) {
    if (inputRate == kMixSampleRate || input.size() < 2) {
        return input;
    }
    const auto inFrames = static_cast<std::uint32_t>(input.size() / 2);
    const auto outFrames = static_cast<std::uint32_t>(
        static_cast<std::uint64_t>(inFrames) * kMixSampleRate / inputRate);
    if (outFrames == 0 || inFrames < 2) {
        return input;
    }
    std::vector<float> output(std::size_t(outFrames) * 2);
    const double step = double(inFrames - 1) / double(outFrames - 1 ? outFrames - 1 : 1);
    for (std::uint32_t frame = 0; frame < outFrames; ++frame) {
        const double pos = step * frame;
        const auto base = std::min<std::uint32_t>(
            static_cast<std::uint32_t>(pos), inFrames - 2);
        const auto t = static_cast<float>(pos - base);
        for (int ch = 0; ch < 2; ++ch) {
            const float a = input[std::size_t(base) * 2 + ch];
            const float b = input[std::size_t(base + 1) * 2 + ch];
            output[std::size_t(frame) * 2 + ch] = a + (b - a) * t;
        }
    }
    return output;
}

// --- Clip library ------------------------------------------------------------

class ClipLibraryImpl final : public IAudioClipLibrary {
public:
    ClipHandle loadWav(const std::vector<std::byte>& bytes) override {
        WavData wav;
        std::vector<float> stereo;
        if (!parseWav(bytes, wav) || !decodeToStereoFloat(wav, stereo)) {
            return ClipHandle::invalid();
        }
        AudioClip clip;
        clip.samples = resampleStereo(stereo, wav.sampleRate);
        clip.frameCount = static_cast<std::uint32_t>(clip.samples.size() / 2);
        const ClipHandle handle{nextId_++};
        clips_.emplace(handle.value, std::move(clip));
        return handle;
    }

    void unload(ClipHandle clip) override { clips_.erase(clip.value); }

    const AudioClip* clip(ClipHandle handle) const override {
        const auto it = clips_.find(handle.value);
        return it != clips_.end() ? &it->second : nullptr;
    }

private:
    std::unordered_map<std::uint64_t, AudioClip> clips_;
    std::uint64_t nextId_ = 1;
};

// --- Mixer --------------------------------------------------------------------

class MixerImpl final : public IAudioMixer {
public:
    explicit MixerImpl(const IAudioClipLibrary& clips) : clips_(clips) {}

    VoiceHandle play(ClipHandle clip, const VoiceParams& params) override {
        if (clips_.clip(clip) == nullptr) {
            return VoiceHandle::invalid();
        }
        const std::scoped_lock lock(mutex_);
        const VoiceHandle handle{nextId_++};
        voices_.push_back({handle, clip, params, 0});
        return handle;
    }

    void stop(VoiceHandle voice) override {
        const std::scoped_lock lock(mutex_);
        std::erase_if(voices_, [&](const Voice& v) { return v.handle == voice; });
    }

    void stopAll() override {
        const std::scoped_lock lock(mutex_);
        voices_.clear();
    }

    void setVolume(VoiceHandle voice, float volume) override {
        const std::scoped_lock lock(mutex_);
        for (auto& v : voices_) {
            if (v.handle == voice) {
                v.params.volume = volume;
            }
        }
    }

    bool isPlaying(VoiceHandle voice) const override {
        const std::scoped_lock lock(mutex_);
        return std::any_of(voices_.begin(), voices_.end(),
                           [&](const Voice& v) { return v.handle == voice; });
    }

    std::size_t activeVoices() const override {
        const std::scoped_lock lock(mutex_);
        return voices_.size();
    }

    void mix(float* interleavedStereo, std::uint32_t frames) override {
        std::fill_n(interleavedStereo, std::size_t(frames) * 2, 0.0f);
        const std::scoped_lock lock(mutex_);
        for (auto& voice : voices_) {
            renderVoice(voice, interleavedStereo, frames);
        }
        // Finished non-loop voices vanish here — after their final samples.
        std::erase_if(voices_, [&](const Voice& v) {
            const auto* clip = clips_.clip(v.clip);
            return clip == nullptr ||
                   (!v.params.loop && v.cursor >= clip->frameCount);
        });
        mixedFrames_ += frames;
    }

    std::uint64_t mixedFrames() const override { return mixedFrames_.load(); }

private:
    struct Voice {
        VoiceHandle handle;
        ClipHandle clip;
        VoiceParams params;
        std::uint32_t cursor = 0; // next clip frame to render
    };

    void renderVoice(Voice& voice, float* out, std::uint32_t frames) {
        const auto* clip = clips_.clip(voice.clip);
        if (clip == nullptr || clip->frameCount == 0) {
            return;
        }
        for (std::uint32_t frame = 0; frame < frames; ++frame) {
            if (voice.cursor >= clip->frameCount) {
                if (!voice.params.loop) {
                    return;
                }
                voice.cursor = 0;
            }
            const auto base = std::size_t(voice.cursor) * 2;
            out[std::size_t(frame) * 2 + 0] +=
                clip->samples[base + 0] * voice.params.volume;
            out[std::size_t(frame) * 2 + 1] +=
                clip->samples[base + 1] * voice.params.volume;
            ++voice.cursor;
        }
    }

    const IAudioClipLibrary& clips_;
    mutable std::mutex mutex_;
    std::vector<Voice> voices_;
    std::uint64_t nextId_ = 1;
    std::atomic<std::uint64_t> mixedFrames_{0};
};

// --- Null output ---------------------------------------------------------------

class NullOutputImpl final : public IAudioOutput {
public:
    ~NullOutputImpl() override { stop(); }

    bool start(IAudioMixer& mixer) override {
        if (running_.exchange(true)) {
            return false;
        }
        thread_ = std::thread([this, &mixer] {
            // 10 ms chunks against the wall clock: real-time pacing without
            // a device, so voices advance and finish like they would play.
            constexpr std::uint32_t kChunkFrames = kMixSampleRate / 100;
            std::vector<float> scratch(std::size_t(kChunkFrames) * 2);
            while (running_.load()) {
                mixer.mix(scratch.data(), kChunkFrames);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        return true;
    }

    void stop() override {
        if (running_.exchange(false) && thread_.joinable()) {
            thread_.join();
        }
    }

private:
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace

std::unique_ptr<IAudioClipLibrary> createAudioClipLibrary() {
    return std::make_unique<ClipLibraryImpl>();
}

std::unique_ptr<IAudioMixer> createAudioMixer(const IAudioClipLibrary& clips) {
    return std::make_unique<MixerImpl>(clips);
}

std::unique_ptr<IAudioOutput> createNullAudioOutput() {
    return std::make_unique<NullOutputImpl>();
}

} // namespace sky::audio
