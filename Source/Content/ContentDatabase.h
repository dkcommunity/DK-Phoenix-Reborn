#pragma once

#include "Content/ContentTypes.h"

#include <optional>
#include <vector>

namespace phoenix::content
{
    class IContentDatabase
    {
    public:
        virtual ~IContentDatabase() = default;

        [[nodiscard]] virtual std::optional<CreatureVisualDefinition> FindCreatureVisual(
            core::BodyId bodyId) const = 0;

        [[nodiscard]] virtual std::optional<AnimationDefinition> FindAnimation(
            core::BodyId bodyId,
            core::ActionId actionId,
            core::DirectionId directionId) const = 0;

        [[nodiscard]] virtual std::optional<SpriteArchiveDefinition> FindSpriteArchive(
            core::ArchiveId archiveId) const = 0;

        [[nodiscard]] virtual std::vector<core::BodyId> ListCreatureBodyIds() const = 0;
    };
}
