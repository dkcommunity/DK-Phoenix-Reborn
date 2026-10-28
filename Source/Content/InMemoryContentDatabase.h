#pragma once

#include "Content/ContentDatabase.h"

#include <map>

namespace phoenix::content
{
    class InMemoryContentDatabase final : public IContentDatabase
    {
    public:
        void AddCreatureVisual(CreatureVisualDefinition definition);
        void AddSpriteArchive(SpriteArchiveDefinition definition);

        [[nodiscard]] std::optional<CreatureVisualDefinition> FindCreatureVisual(core::BodyId bodyId) const override;
        [[nodiscard]] std::optional<AnimationDefinition> FindAnimation(
            core::BodyId bodyId,
            core::ActionId actionId,
            core::DirectionId directionId) const override;
        [[nodiscard]] std::optional<SpriteArchiveDefinition> FindSpriteArchive(core::ArchiveId archiveId) const override;
        [[nodiscard]] std::vector<core::BodyId> ListCreatureBodyIds() const override;

    private:
        std::map<core::BodyId, CreatureVisualDefinition> creatures_;
        std::map<core::ArchiveId, SpriteArchiveDefinition> archives_;
    };
}
