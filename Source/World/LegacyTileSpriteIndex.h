#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::world
{
    class LegacyTileSpriteIndex final
    {
    public:
        [[nodiscard]] bool Load(const std::filesystem::path& path, std::string& error);
        void Clear() noexcept;
        [[nodiscard]] bool Loaded() const noexcept;
        [[nodiscard]] std::optional<std::uint32_t> Resolve(
            std::uint32_t logicalSpriteId, std::uint32_t physicalSpriteCount) const noexcept;

    private:
        std::vector<std::uint32_t> logicalToPhysical_;
    };
}
