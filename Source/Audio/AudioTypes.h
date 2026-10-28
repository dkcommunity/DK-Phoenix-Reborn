#pragma once

#include "Core/StrongId.h"

#include <filesystem>
#include <string>

namespace phoenix::audio
{
    enum class AudioBus
    {
        Master,
        Music,
        Effects,
        Ui,
        Ambient
    };

    struct SoundDescriptor
    {
        core::AudioId audioId;
        std::filesystem::path sourcePath;
        AudioBus bus{ AudioBus::Effects };
        bool loop{ false };
    };

    struct AudioPlaybackRequest
    {
        core::AudioId audioId;
        AudioBus bus{ AudioBus::Effects };
        float volume{ 1.0F };
        bool loop{ false };
    };
}
