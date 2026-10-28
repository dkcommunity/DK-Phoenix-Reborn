#include "MapEditorRenderCache.h"

#include <algorithm>
#include <limits>

namespace phoenix::tools::map_editor
{
    const world::WorldDrawList& MapEditorDrawListCache::Ensure(
        const world::LegacyMap& map, const std::uint64_t contentRevision)
    {
        if (ValidFor(contentRevision)) return list_;

        if (map.width == 0U || map.height == 0U)
        {
            list_ = {};
            mapWidth_ = 0U;
            tileLookup_.clear();
            revision_ = contentRevision;
            valid_ = true;
            ++rebuildCount_;
            return list_;
        }

        EditorWorldDrawListBuilder builder;
        list_ = builder.BuildFullMap(map, tileSpriteIndex_, physicalTileSpriteCount_);
        mapWidth_ = map.width;
        tileLookup_.assign(static_cast<std::size_t>(map.width) * map.height, MissingIndex);
        for (std::size_t index = 0; index < list_.tiles.size(); ++index)
        {
            const auto& tile = list_.tiles[index];
            tileLookup_[static_cast<std::size_t>(tile.y) * map.width + tile.x] = index;
        }
        revision_ = contentRevision;
        valid_ = true;
        ++rebuildCount_;
        return list_;
    }

    void MapEditorDrawListCache::Invalidate() noexcept { valid_ = false; }
    void MapEditorDrawListCache::SetTileSpriteIndex(
        const world::LegacyTileSpriteIndex* index, const std::uint32_t physicalSpriteCount) noexcept
    {
        if (tileSpriteIndex_ == index && physicalTileSpriteCount_ == physicalSpriteCount) return;
        tileSpriteIndex_ = index;
        physicalTileSpriteCount_ = physicalSpriteCount;
        Invalidate();
    }
    bool MapEditorDrawListCache::ValidFor(const std::uint64_t revision) const noexcept
    {
        return valid_ && revision_ == revision;
    }
    std::uint64_t MapEditorDrawListCache::RebuildCount() const noexcept { return rebuildCount_; }
    std::size_t MapEditorDrawListCache::TileIndex(const std::uint16_t x, const std::uint16_t y) const noexcept
    {
        if (x >= mapWidth_) return MissingIndex;
        const auto index = static_cast<std::size_t>(y) * mapWidth_ + x;
        return index < tileLookup_.size() ? tileLookup_[index] : MissingIndex;
    }

    bool RevisionCacheState::NeedsRebuild(const std::uint64_t revision) const noexcept
    {
        return !valid_ || revision_ != revision;
    }
    void RevisionCacheState::MarkRebuilt(const std::uint64_t revision) noexcept
    {
        revision_ = revision;
        valid_ = true;
        ++rebuildCount_;
    }
    void RevisionCacheState::Invalidate() noexcept { valid_ = false; }
    std::uint64_t RevisionCacheState::RebuildCount() const noexcept { return rebuildCount_; }
}
