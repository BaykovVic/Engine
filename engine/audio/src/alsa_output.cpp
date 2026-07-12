#include "sky/audio/audio.hpp"

#ifdef SKY_HAS_ALSA

#include <alsa/asoundlib.h>

#include <atomic>
#include <cstdlib>
#include <thread>
#include <vector>

namespace sky::audio {
namespace {

/// Pulls the mixer and writes interleaved float stereo to an ALSA PCM. The
/// device paces the loop through blocking writes; underruns are recovered
/// with snd_pcm_prepare and never surface as errors.
class AlsaOutputImpl final : public IAudioOutput {
public:
    explicit AlsaOutputImpl(snd_pcm_t* pcm) : pcm_(pcm) {}

    ~AlsaOutputImpl() override {
        stop();
        snd_pcm_close(pcm_);
    }

    bool start(IAudioMixer& mixer) override {
        if (running_.exchange(true)) {
            return false;
        }
        thread_ = std::thread([this, &mixer] {
            constexpr std::uint32_t kChunkFrames = kMixSampleRate / 100; // 10 ms
            std::vector<float> scratch(std::size_t(kChunkFrames) * 2);
            while (running_.load()) {
                mixer.mix(scratch.data(), kChunkFrames);
                const auto written =
                    snd_pcm_writei(pcm_, scratch.data(), kChunkFrames);
                if (written < 0) {
                    snd_pcm_prepare(pcm_); // underrun: recover and continue
                }
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
    snd_pcm_t* pcm_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

void silentAlsaErrorHandler(const char*, int, const char*, int, const char*,
                            ...) {}

} // namespace

std::unique_ptr<IAudioOutput> createAlsaAudioOutput() {
    // Probing devices on cardless machines spams stderr through ALSA's
    // default error handler; failures here are expected and handled.
    snd_lib_error_set_handler(&silentAlsaErrorHandler);
    // SKY_AUDIO_DEVICE overrides; otherwise the system default, then ALSA's
    // built-in "null" sink — audio semantics keep working (voices advance
    // and finish) on machines without sound hardware.
    const char* override_ = std::getenv("SKY_AUDIO_DEVICE");
    const char* candidates[] = {override_, "default", "null"};
    for (const char* device : candidates) {
        if (device == nullptr || device[0] == '\0') {
            continue;
        }
        snd_pcm_t* pcm = nullptr;
        if (snd_pcm_open(&pcm, device, SND_PCM_STREAM_PLAYBACK, 0) < 0) {
            continue;
        }
        if (snd_pcm_set_params(pcm, SND_PCM_FORMAT_FLOAT_LE,
                               SND_PCM_ACCESS_RW_INTERLEAVED, 2, kMixSampleRate,
                               /*soft_resample=*/1, /*latency_us=*/50000) < 0) {
            snd_pcm_close(pcm);
            continue;
        }
        return std::make_unique<AlsaOutputImpl>(pcm);
    }
    return nullptr;
}

} // namespace sky::audio

#else

namespace sky::audio {

std::unique_ptr<IAudioOutput> createAlsaAudioOutput() {
    return nullptr; // built without ALSA; callers fall back to the null output
}

} // namespace sky::audio

#endif
