#include "World/LegacyTileSpriteIndex.h"

#include <fstream>
#include <iterator>
#include <limits>

namespace phoenix::world
{
    bool LegacyTileSpriteIndex::Load(const std::filesystem::path& path, std::string& error)
    {
        Clear();
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            error = "Legacy TILE.IDX could not be opened: " + path.string();
            return false;
        }
        const std::vector<std::uint8_t> bytes{
            std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
        if (bytes.size() < 8U)
        {
            error = "Legacy TILE.IDX is truncated.";
            return false;
        }
        const auto readU32 = [&bytes](const std::size_t offset) {
            return static_cast<std::uint32_t>(bytes[offset]) |
                (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
                (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
                (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
        };
        const auto maximumLogicalId = readU32(0U);
        const auto count = static_cast<std::uint64_t>(maximumLogicalId) + 1U;
        const auto expected = 4U + count * 4U;
        if (expected != bytes.size() || count > std::numeric_limits<std::size_t>::max())
        {
            error = "Legacy TILE.IDX size does not match its maximum logical sprite ID.";
            return false;
        }
        logicalToPhysical_.reserve(static_cast<std::size_t>(count));
        for (std::size_t index = 0; index < static_cast<std::size_t>(count); ++index)
            logicalToPhysical_.push_back(readU32(4U + index * 4U));
        return true;
    }

    void LegacyTileSpriteIndex::Clear() noexcept { logicalToPhysical_.clear(); }
    bool LegacyTileSpriteIndex::Loaded() const noexcept { return !logicalToPhysical_.empty(); }

    std::optional<std::uint32_t> LegacyTileSpriteIndex::Resolve(
        const std::uint32_t logicalSpriteId, const std::uint32_t physicalSpriteCount) const noexcept
    {
        if (logicalSpriteId == 0xFFFFFFFFU || logicalSpriteId == 0xFFFFU) return std::nullopt;
        if (logicalSpriteId < physicalSpriteCount) return logicalSpriteId;
        if (logicalSpriteId >= logicalToPhysical_.size()) return std::nullopt;
        const auto physical = logicalToPhysical_[logicalSpriteId];
        if (physical == 0xFFFFFFFFU || physical >= physicalSpriteCount) return std::nullopt;
        return physical;
    }
}
