#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace phoenix::tools::map_editor
{
    enum class SpriteResourceType : std::uint8_t
    {
        Tile,
        Object,
        Animation,
        Effect
    };

    struct FixedSpriteSource
    {
        std::filesystem::path archive;
        std::filesystem::path index;
        SpriteResourceType type{ SpriteResourceType::Tile };
    };

    struct SpriteGridLayout
    {
        int columns{ 1 };
        int cellWidth{ 1 };
        int cellHeight{ 1 };
    };

    struct VisibleItemRange
    {
        std::size_t first{ 0 };
        std::size_t last{ 0 };
    };

    struct SplitterLayout
    {
        int firstExtent{ 0 };
        int splitterPosition{ 0 };
        int secondExtent{ 0 };
    };

    class SpriteSourceSelectionState final
    {
    public:
        [[nodiscard]] bool Select(const FixedSpriteSource& source);
        void Reset() noexcept;
    private:
        FixedSpriteSource selected_;
        bool selectedValid_{ false };
    };

    [[nodiscard]] FixedSpriteSource ResolveFixedSpriteSource(
        const std::filesystem::path& imageRoot, SpriteResourceType type);
    [[nodiscard]] SpriteGridLayout CalculateSpriteGridLayout(
        int width, int itemHeight, int preferredColumns = 5, int minimumCellWidth = 72) noexcept;
    [[nodiscard]] VisibleItemRange CalculateVisibleItemRange(
        std::size_t itemCount, const SpriteGridLayout& layout, int scrollY,
        int viewportHeight, int overscanRows = 1) noexcept;
    [[nodiscard]] SplitterLayout CalculateSplitterLayout(
        int totalExtent, int requestedFirstExtent, int minimumFirstExtent,
        int minimumSecondExtent, int splitterThickness) noexcept;
    [[nodiscard]] int CalculateToolbarRowCount(
        int clientWidth, int singleRowRequiredWidth) noexcept;
}
