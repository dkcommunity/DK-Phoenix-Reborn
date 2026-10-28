#include "Backends/XAudio2/XAudio2Backend.h"

#include <filesystem>

namespace phoenix::backends::xaudio2
{
    bool XAudio2Backend::Initialize()
    {
        initialized_ = true;
        return true;
    }

    void XAudio2Backend::Shutdown() noexcept
    {
        initialized_ = false;
        loaded_.clear();
        volumes_.clear();
    }

    bool XAudio2Backend::Load(const core::AudioId audioId, const std::filesystem::path& sourcePath)
    {
        if (!initialized_)
        {
            return false;
        }

        std::error_code ec;
        if (!std::filesystem::exists(sourcePath, ec))
        {
            return false;
        }

        loaded_[audioId] = sourcePath;
        return true;
    }

    bool XAudio2Backend::Play(const audio::AudioPlaybackRequest& request)
    {
        return initialized_ && loaded_.find(request.audioId) != loaded_.end();
    }

    void XAudio2Backend::Stop(core::AudioId) noexcept
    {
    }

    void XAudio2Backend::SetBusVolume(const audio::AudioBus bus, const float volume) noexcept
    {
        volumes_[bus] = volume;
    }

    bool XAudio2Backend::IsInitialized() const noexcept
    {
        return initialized_;
    }
}
