#pragma once

#include "Audio/AudioBackend.h"

#include <map>

namespace phoenix::backends::xaudio2
{
    class XAudio2Backend final : public audio::IAudioBackend
    {
    public:
        bool Initialize() override;
        void Shutdown() noexcept override;
        bool Load(core::AudioId audioId, const std::filesystem::path& sourcePath) override;
        bool Play(const audio::AudioPlaybackRequest& request) override;
        void Stop(core::AudioId audioId) noexcept override;
        void SetBusVolume(audio::AudioBus bus, float volume) noexcept override;

        [[nodiscard]] bool IsInitialized() const noexcept;

    private:
        bool initialized_{ false };
        std::map<core::AudioId, std::filesystem::path> loaded_;
        std::map<audio::AudioBus, float> volumes_;
    };
}
