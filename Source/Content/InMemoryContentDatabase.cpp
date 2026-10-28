#include "Content/InMemoryContentDatabase.h"

#include <utility>

namespace phoenix::content
{
    void InMemoryContentDatabase::AddCreatureVisual(CreatureVisualDefinition definition)
    {
        creatures_[definition.bodyId] = std::move(definition);
    }

    void InMemoryContentDatabase::AddSpriteArchive(SpriteArchiveDefinition definition)
    {
        archives_[definition.archiveId] = std::move(definition);
    }

    std::optional<CreatureVisualDefinition> InMemoryContentDatabase::FindCreatureVisual(const core::BodyId bodyId) const
    {
        const auto it = creatures_.find(bodyId);
        if (it == creatures_.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    std::optional<AnimationDefinition> InMemoryContentDatabase::FindAnimation(
        const core::BodyId bodyId,
        const core::ActionId actionId,
        const core::DirectionId directionId) const
    {
        const auto creature = FindCreatureVisual(bodyId);
        if (!creature)
        {
            return std::nullopt;
        }

        for (const auto& animation : creature->animations)
        {
            if (animation.actionId == actionId && animation.directionId == directionId)
            {
                return animation;
            }
        }

        return std::nullopt;
    }

    std::optional<SpriteArchiveDefinition> InMemoryContentDatabase::FindSpriteArchive(const core::ArchiveId archiveId) const
    {
        const auto it = archives_.find(archiveId);
        if (it == archives_.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    std::vector<core::BodyId> InMemoryContentDatabase::ListCreatureBodyIds() const
    {
        std::vector<core::BodyId> ids;
        ids.reserve(creatures_.size());
        for (const auto& [id, _] : creatures_)
        {
            ids.push_back(id);
        }
        return ids;
    }
}
