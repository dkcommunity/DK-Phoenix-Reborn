#pragma once

#include "Audio/AudioTypes.h"

#include <map>
#include <optional>

namespace phoenix::audio
{
    class SoundBank final
    {
    public:
        void Add(SoundDescriptor descriptor);
        [[nodiscard]] std::optional<SoundDescriptor> Find(core::AudioId audioId) const;
        void Clear() noexcept;

    private:
        std::map<core::AudioId, SoundDescriptor> sounds_;
    };
}
