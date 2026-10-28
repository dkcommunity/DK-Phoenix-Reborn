#pragma once

#include "Core/EditorWorldDrawListBuilder.h"
#include "World/LegacyMapReader.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace phoenix::tools::map_editor
{
    [[nodiscard]] inline std::vector<std::size_t> BuildLegacyEditorObjectLayerOrder(
        const world::WorldDrawList& list)
    {
        std::vector<std::size_t> ordered;
        ordered.reserve(list.staticObjects.size());
        for (std::size_t layerIndex = 0; layerIndex < list.layers.size(); ++layerIndex)
        {
            const auto& layer = list.layers[layerIndex];
            if (layer.kind == world::WorldLayerDrawKind::StaticObject && layer.index < list.staticObjects.size())
                ordered.push_back(layerIndex);
        }
        return ordered;
    }

    class MapEditorDrawListCache final
    {
    public:
        [[nodiscard]] const world::WorldDrawList& Ensure(
            const world::LegacyMap& map, std::uint64_t contentRevision);
        void SetTileSpriteIndex(const world::LegacyTileSpriteIndex* index, std::uint32_t physicalSpriteCount) noexcept;
        void Invalidate() noexcept;
        [[nodiscard]] bool ValidFor(std::uint64_t contentRevision) const noexcept;
        [[nodiscard]] std::uint64_t RebuildCount() const noexcept;
        [[nodiscard]] std::size_t TileIndex(std::uint16_t x, std::uint16_t y) const noexcept;

        static constexpr std::size_t MissingIndex = std::numeric_limits<std::size_t>::max();

    private:
        world::WorldDrawList list_;
        std::uint64_t revision_{ 0 };
        std::uint64_t rebuildCount_{ 0 };
        bool valid_{ false };
        std::uint16_t mapWidth_{ 0 };
        std::vector<std::size_t> tileLookup_;
        const world::LegacyTileSpriteIndex* tileSpriteIndex_{ nullptr };
        std::uint32_t physicalTileSpriteCount_{ 0x10000U };
    };

    class RevisionCacheState final
    {
    public:
        [[nodiscard]] bool NeedsRebuild(std::uint64_t revision) const noexcept;
        void MarkRebuilt(std::uint64_t revision) noexcept;
        void Invalidate() noexcept;
        [[nodiscard]] std::uint64_t RebuildCount() const noexcept;

    private:
        std::uint64_t revision_{ 0 };
        std::uint64_t rebuildCount_{ 0 };
        bool valid_{ false };
    };
}
