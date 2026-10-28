#pragma once

#include "Audio/AudioTypes.h"

#include <filesystem>

namespace phoenix::audio
{
    class IAudioBackend
    {
    public:
        virtual ~IAudioBackend() = default;

        virtual bool Initialize() = 0;
        virtual void Shutdown() noexcept = 0;
        virtual bool Load(core::AudioId audioId, const std::filesystem::path& sourcePath) = 0;
        virtual bool Play(const AudioPlaybackRequest& request) = 0;
        virtual void Stop(core::AudioId audioId) noexcept = 0;
        virtual void SetBusVolume(AudioBus bus, float volume) noexcept = 0;
    };
}
