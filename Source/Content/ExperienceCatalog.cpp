#include "Content/ExperienceCatalog.h"

#include "Content/FileDefCatalog.h"

#include <array>
#include <fstream>

namespace phoenix::content
{
    namespace
    {
        [[nodiscard]] bool ReadU32(std::ifstream& input, std::uint32_t& value)
        {
            std::array<std::uint8_t, 4> bytes{};
            if (!input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
            {
                return false;
            }
            value = static_cast<std::uint32_t>(bytes[0]) |
                (static_cast<std::uint32_t>(bytes[1]) << 8U) |
                (static_cast<std::uint32_t>(bytes[2]) << 16U) |
                (static_cast<std::uint32_t>(bytes[3]) << 24U);
            return true;
        }

        [[nodiscard]] bool LoadTable(
            const std::filesystem::path& path,
            std::map<std::int32_t, ExperienceEntry>& table,
            std::wstring& error)
        {
            std::ifstream input(path, std::ios::binary);
            if (!input)
            {
                error = L"Experience table could not be opened: " + path.wstring();
                return false;
            }

            std::uint32_t count = 0;
            if (!ReadU32(input, count) || count > 100000U)
            {
                error = L"Experience table has an invalid count: " + path.wstring();
                return false;
            }

            for (std::uint32_t index = 0; index < count; ++index)
            {
                std::uint32_t rawLevel = 0;
                std::uint32_t rawGoal = 0;
                std::uint32_t accumulated = 0;
                if (!ReadU32(input, rawLevel) || !ReadU32(input, rawGoal) || !ReadU32(input, accumulated))
                {
                    error = L"Experience table ended inside a record: " + path.wstring();
                    return false;
                }
                table.emplace(
                    static_cast<std::int32_t>(rawLevel),
                    ExperienceEntry{ static_cast<std::int32_t>(rawGoal), accumulated });
            }
            return true;
        }
    }

    bool ExperienceCatalogResult::Succeeded() const noexcept
    {
        return error.empty() && !strength.empty() && !dexterity.empty() &&
            !intelligence.empty() && !vampire.empty() && !ousters.empty() && !advancement.empty();
    }

    std::optional<ExperienceEntry> ExperienceCatalogResult::Find(
        const std::map<std::int32_t, ExperienceEntry>& table,
        const std::int32_t level)
    {
        const auto found = table.find(level);
        if (found == table.end())
        {
            return std::nullopt;
        }
        return found->second;
    }

    ExperienceCatalogResult ExperienceCatalog::Load(const std::filesystem::path& dataRoot) const
    {
        ExperienceCatalogResult result;
        const auto fileDef = FileDefCatalog{}.Load(dataRoot);
        const auto resolve = [&fileDef, &dataRoot](const std::wstring_view key, const std::wstring_view fallback) {
            if (fileDef.Succeeded())
            {
                return fileDef.Resolve(key, fallback);
            }
            return (dataRoot / std::filesystem::path(fallback)).lexically_normal();
        };

        const struct TableSource
        {
            std::wstring_view key;
            std::wstring_view fallback;
            std::map<std::int32_t, ExperienceEntry>* target;
        } sources[] = {
            { L"FILE_INFO_STR_EXP", L"Info\\STRExp.inf", &result.strength },
            { L"FILE_INFO_DEX_EXP", L"Info\\DEXExp.inf", &result.dexterity },
            { L"FILE_INFO_INT_EXP", L"Info\\INTExp.inf", &result.intelligence },
            { L"FILE_INFO_VAMPIRE_EXP", L"Info\\VampireExp.inf", &result.vampire },
            { L"FILE_INFO_OUSTERS_EXP", L"Info\\OustersExp.inf", &result.ousters },
            { L"FILE_ADVANCE_EXP_INFO", L"Info\\AdvancementClassExp.inf", &result.advancement }
        };

        for (const auto& source : sources)
        {
            if (!LoadTable(resolve(source.key, source.fallback), *source.target, result.error))
            {
                return result;
            }
        }
        return result;
    }
}
