#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "sky/core/handle.hpp"

namespace sky::audio {

struct ClipTag {};
struct VoiceTag {};
using ClipHandle = core::Handle<ClipTag>;
using VoiceHandle = core::Handle<VoiceTag>;

/// Every clip is stored mixer-ready: interleaved float stereo at the fixed
/// engine mix rate. Conversion (bit depth, channel count, sample rate)
/// happens once at load, so the mix loop only ever adds and scales.
constexpr std::uint32_t kMixSampleRate = 48000;

struct AudioClip {
    std::uint32_t frameCount = 0;
    /// Interleaved L/R, size == 2 * frameCount.
    std::vector<float> samples;
};

/// Audio module contract: decoded clip storage. Owns the PCM data; the mixer
/// reads clips through this service and never copies them per voice.
class IAudioClipLibrary {
public:
    virtual ~IAudioClipLibrary() = default;

    /// Decodes a RIFF/WAVE buffer (PCM 16-bit or IEEE float 32-bit, mono or
    /// stereo, any sample rate — linearly resampled to kMixSampleRate).
    /// Returns an invalid handle on malformed or unsupported input.
    virtual ClipHandle loadWav(const std::vector<std::byte>& bytes) = 0;
    virtual void unload(ClipHandle clip) = 0;
    /// nullptr for unknown handles. The pointer stays valid until unload.
    [[nodiscard]] virtual const AudioClip* clip(ClipHandle handle) const = 0;
};

struct VoiceParams {
    float volume = 1.0f;
    bool loop = false;
};

/// Audio module contract: the software mixer. play/stop are safe to call
/// from the main thread while an output device pulls mix() on its own
/// thread; a finished (non-loop) voice disappears on its final mix.
class IAudioMixer {
public:
    virtual ~IAudioMixer() = default;

    virtual VoiceHandle play(ClipHandle clip, const VoiceParams& params) = 0;
    virtual void stop(VoiceHandle voice) = 0;
    virtual void stopAll() = 0;
    virtual void setVolume(VoiceHandle voice, float volume) = 0;
    [[nodiscard]] virtual bool isPlaying(VoiceHandle voice) const = 0;
    [[nodiscard]] virtual std::size_t activeVoices() const = 0;

    /// Renders the next `frames` frames of interleaved stereo float into
    /// `interleavedStereo` (2 * frames values), overwriting it, and advances
    /// every voice. The engine's only source of audio truth — tests call it
    /// directly, output devices call it from their fill loop.
    virtual void mix(float* interleavedStereo, std::uint32_t frames) = 0;

    /// Total frames rendered since creation (structural check that an
    /// output device is actually draining the mixer).
    [[nodiscard]] virtual std::uint64_t mixedFrames() const = 0;
};

/// Audio module contract: a device pulling the mixer in real time on its
/// own thread. start() may be called once; stop() joins the thread.
class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;

    virtual bool start(IAudioMixer& mixer) = 0;
    virtual void stop() = 0;
};

std::unique_ptr<IAudioClipLibrary> createAudioClipLibrary();
std::unique_ptr<IAudioMixer> createAudioMixer(const IAudioClipLibrary& clips);

/// Paces the mixer against the wall clock and discards the result. Keeps
/// play-mode audio semantics (voices advance and finish) on machines and CI
/// containers without a sound device.
std::unique_ptr<IAudioOutput> createNullAudioOutput();

/// ALSA playback ("default" device, or SKY_AUDIO_DEVICE when set). Returns
/// nullptr in builds without ALSA or when no device opens; callers fall
/// back to createNullAudioOutput().
std::unique_ptr<IAudioOutput> createAlsaAudioOutput();

} // namespace sky::audio
