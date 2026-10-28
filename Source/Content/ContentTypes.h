#pragma once

#include "Assets/LegacyAssetTypes.h"
#include "Core/StrongId.h"

#include <cstdint>
#include <string>
#include <vector>

namespace phoenix::content
{
    struct FrameDefinition
    {
        core::SpriteId spriteId;
        std::int32_t originX{ 0 };
        std::int32_t originY{ 0 };
    };

    struct AnimationDefinition
    {
        core::BodyId bodyId;
        core::ActionId actionId;
        core::DirectionId directionId;
        std::vector<FrameDefinition> frames;
    };

    struct CreatureVisualDefinition
    {
        core::BodyId bodyId;
        std::wstring name;
        std::vector<AnimationDefinition> animations;
    };

    struct SpriteArchiveDefinition
    {
        core::ArchiveId archiveId;
        assets::LegacyAssetFormat sourceFormat{ assets::LegacyAssetFormat::Unknown };
        std::uint32_t spriteCount{ 0 };
    };
}
