#pragma once

#include "Content/ContentTypes.h"
#include "Core/StrongId.h"

#include <cstdint>
#include <vector>

namespace phoenix::animation
{
    struct AnimationFrame
    {
        content::FrameDefinition visual;
        std::uint32_t durationMs{ 0 };
        core::AudioId soundTrigger;
    };

    struct AnimationClip
    {
        core::BodyId bodyId;
        core::ActionId actionId;
        core::DirectionId directionId;
        bool loop{ false };
        std::vector<AnimationFrame> frames;
    };

    struct AnimationSample
    {
        content::FrameDefinition visual;
        core::FrameIndex frameIndex;
        bool clipFinished{ false };
    };
}
