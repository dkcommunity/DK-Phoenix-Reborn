#pragma once

#include "Animation/AnimationTypes.h"

#include <cstdint>
#include <optional>

namespace phoenix::animation
{
    class AnimationController final
    {
    public:
        void Play(AnimationClip clip);
        void Stop() noexcept;
        void Update(std::uint32_t elapsedMs) noexcept;

        [[nodiscard]] std::optional<AnimationSample> CurrentSample() const noexcept;
        [[nodiscard]] bool IsPlaying() const noexcept;

    private:
        [[nodiscard]] std::uint32_t CurrentFrameDuration() const noexcept;

        std::optional<AnimationClip> clip_;
        std::uint32_t currentFrame_{ 0 };
        std::uint32_t elapsedInFrameMs_{ 0 };
        bool finished_{ false };
    };
}
