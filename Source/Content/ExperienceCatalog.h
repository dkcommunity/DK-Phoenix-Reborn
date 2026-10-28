#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace phoenix::content
{
    struct ExperienceEntry
    {
        std::int32_t goalExperience{ 0 };
        std::uint32_t accumulatedExperience{ 0 };
    };

    struct ExperienceCatalogResult
    {
        std::map<std::int32_t, ExperienceEntry> strength;
        std::map<std::int32_t, ExperienceEntry> dexterity;
        std::map<std::int32_t, ExperienceEntry> intelligence;
        std::map<std::int32_t, ExperienceEntry> vampire;
        std::map<std::int32_t, ExperienceEntry> ousters;
        std::map<std::int32_t, ExperienceEntry> advancement;
        std::wstring error;

        [[nodiscard]] bool Succeeded() const noexcept;
        [[nodiscard]] static std::optional<ExperienceEntry> Find(
            const std::map<std::int32_t, ExperienceEntry>& table,
            std::int32_t level);
    };

    class ExperienceCatalog final
    {
    public:
        [[nodiscard]] ExperienceCatalogResult Load(const std::filesystem::path& dataRoot) const;
    };
}
