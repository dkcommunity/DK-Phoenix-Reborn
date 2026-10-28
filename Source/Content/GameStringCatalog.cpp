#include "Content/GameStringCatalog.h"

#include "Content/LegacyBinaryTableReader.h"

namespace phoenix::content
{
    namespace
    {
        [[nodiscard]] bool LoadStringTable(const std::filesystem::path& path, std::vector<std::string>& out, std::wstring& error)
        {
            const auto bytes = ReadBinaryFile(path);
            if (!bytes)
            {
                error = L"String.inf could not be opened.";
                return false;
            }

            LegacyBinaryTableReader reader(*bytes);
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > 100000)
            {
                error = L"String.inf has invalid table size.";
                return false;
            }

            out.assign(static_cast<std::size_t>(count), {});
            for (std::int32_t index = 0; index < count; ++index)
            {
                if (!reader.String(out[static_cast<std::size_t>(index)]))
                {
                    error = L"String.inf record ended early.";
                    out.clear();
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] bool LoadNicknameTable(const std::filesystem::path& path, std::vector<std::string>& out, std::wstring& error)
        {
            const auto bytes = ReadBinaryFile(path);
            if (!bytes)
            {
                error = L"Nickname.inf could not be opened.";
                return false;
            }

            LegacyBinaryTableReader reader(*bytes);
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > 100000)
            {
                error = L"Nickname.inf has invalid table size.";
                return false;
            }

            out.assign(static_cast<std::size_t>(count), {});
            for (std::int32_t i = 0; i < count; ++i)
            {
                std::uint16_t index = 0;
                if (!reader.U16(index) || index >= static_cast<std::uint16_t>(count) || !reader.String(out[index]))
                {
                    error = L"Nickname.inf record ended early or used an invalid index.";
                    out.clear();
                    return false;
                }
            }
            return true;
        }
    }

    std::optional<std::string> GameStringCatalogResult::FindString(const std::uint32_t index) const
    {
        return index < strings.size() ? std::optional<std::string>(strings[index]) : std::nullopt;
    }

    std::optional<std::string> GameStringCatalogResult::FindNickname(const std::uint32_t index) const
    {
        return index < nicknames.size() ? std::optional<std::string>(nicknames[index]) : std::nullopt;
    }

    GameStringCatalogResult GameStringCatalog::Load(
        const std::filesystem::path& stringPath,
        const std::filesystem::path& nicknamePath) const
    {
        GameStringCatalogResult result;
        std::wstring stringError;
        std::wstring nicknameError;
        const auto stringOk = LoadStringTable(stringPath, result.strings, stringError);
        const auto nicknameOk = LoadNicknameTable(nicknamePath, result.nicknames, nicknameError);

        if (!stringOk && !nicknameOk)
        {
            result.error = stringError + L"; " + nicknameError;
        }
        return result;
    }
}
