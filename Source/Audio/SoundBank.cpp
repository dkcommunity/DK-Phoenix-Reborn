#include "Audio/SoundBank.h"

#include <utility>

namespace phoenix::audio
{
    void SoundBank::Add(SoundDescriptor descriptor)
    {
        sounds_[descriptor.audioId] = std::move(descriptor);
    }

    std::optional<SoundDescriptor> SoundBank::Find(core::AudioId audioId) const
    {
        const auto found = sounds_.find(audioId);
        if (found == sounds_.end())
        {
            return std::nullopt;
        }

        return found->second;
    }

    void SoundBank::Clear() noexcept
    {
        sounds_.clear();
    }
}
