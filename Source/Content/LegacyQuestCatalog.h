#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>

namespace phoenix::content
{
    struct LegacyQuestText
    {
        std::string title;
        std::string description;
        std::map<std::pair<std::uint8_t, std::uint16_t>, std::string> missions;
    };

    class LegacyQuestCatalog final
    {
    public:
        [[nodiscard]] bool Load(const std::filesystem::path& dataRoot);
        [[nodiscard]] const LegacyQuestText* Find(std::uint32_t questId) const noexcept;
        [[nodiscard]] bool Loaded() const noexcept { return loaded_; }

    private:
        [[nodiscard]] bool LoadXml(const std::filesystem::path& path);

        std::map<std::uint32_t, LegacyQuestText> quests_;
        bool loaded_{ false };
    };
}
