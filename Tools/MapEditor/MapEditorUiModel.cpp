#include "MapEditorUiModel.h"

#include <algorithm>

namespace phoenix::tools::map_editor
{
    bool SpriteSourceSelectionState::Select(const FixedSpriteSource& source)
    {
        if (selectedValid_ && selected_.archive == source.archive && selected_.index == source.index &&
            selected_.type == source.type) return false;
        selected_ = source;
        selectedValid_ = true;
        return true;
    }

    void SpriteSourceSelectionState::Reset() noexcept { selectedValid_ = false; }

    FixedSpriteSource ResolveFixedSpriteSource(
        const std::filesystem::path& imageRoot, const SpriteResourceType type)
    {
        FixedSpriteSource result;
        result.type = type;
        if (type == SpriteResourceType::Tile)
        {
            result.archive = imageRoot / L"tile.spk";
            result.index = imageRoot / L"tile.spki";
        }
        else if (type == SpriteResourceType::Object)
        {
            result.archive = imageRoot / L"imageobject.spk";
            result.index = imageRoot / L"imageobject.spki";
        }
        else if (type == SpriteResourceType::Animation)
        {
            result.archive = imageRoot / L"animationobject.afpk";
        }
        else
        {
            result.archive = imageRoot / L"effect.efpk";
            result.index = imageRoot / L"effect.aspki";
        }
        return result;
    }

    SpriteGridLayout CalculateSpriteGridLayout(const int width, const int itemHeight,
        const int preferredColumns, const int minimumCellWidth) noexcept
    {
        const auto safeWidth = std::max(1, width);
        const auto safeMinimum = std::max(1, minimumCellWidth);
        const auto columns = std::clamp(safeWidth / safeMinimum, 1, std::max(1, preferredColumns));
        return { columns, std::max(1, safeWidth / columns), std::max(1, itemHeight) };
    }

    VisibleItemRange CalculateVisibleItemRange(const std::size_t itemCount,
        const SpriteGridLayout& layout, const int scrollY, const int viewportHeight,
        const int overscanRows) noexcept
    {
        if (itemCount == 0U) return {};
        const auto columns = static_cast<std::size_t>(std::max(1, layout.columns));
        const auto cellHeight = std::max(1, layout.cellHeight);
        const auto firstRow = std::max(0, scrollY / cellHeight - std::max(0, overscanRows));
        const auto finalPixel = std::max(0, scrollY) + std::max(0, viewportHeight);
        const auto lastRow = finalPixel / cellHeight + std::max(0, overscanRows) + 1;
        const auto first = std::min(itemCount, static_cast<std::size_t>(firstRow) * columns);
        const auto last = std::min(itemCount, static_cast<std::size_t>(lastRow) * columns);
        return { first, std::max(first, last) };
    }

    SplitterLayout CalculateSplitterLayout(const int totalExtent, const int requestedFirstExtent,
        const int minimumFirstExtent, const int minimumSecondExtent, const int splitterThickness) noexcept
    {
        const auto total = std::max(0, totalExtent);
        const auto thickness = std::clamp(splitterThickness, 0, total);
        const auto available = total - thickness;
        const auto firstMinimum = std::clamp(minimumFirstExtent, 0, available);
        const auto secondMinimum = std::clamp(minimumSecondExtent, 0, available - firstMinimum);
        const auto maximumFirst = available - secondMinimum;
        const auto first = std::clamp(requestedFirstExtent, firstMinimum, maximumFirst);
        return { first, first, available - first };
    }

    int CalculateToolbarRowCount(const int clientWidth, const int singleRowRequiredWidth) noexcept
    {
        return clientWidth >= std::max(1, singleRowRequiredWidth) ? 1 : 2;
    }
}
