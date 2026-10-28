#include "FileSystem/ProjectPaths.h"

#include <array>
#include <string>

namespace phoenix::filesystem
{
    namespace
    {
        std::wstring NormalizeForPrefixCheck(const std::filesystem::path& path)
        {
            auto text = path.lexically_normal().wstring();
            for (auto& ch : text)
            {
                if (ch == L'/')
                {
                    ch = L'\\';
                }
            }

            if (!text.empty() && text.back() != L'\\')
            {
                text.push_back(L'\\');
            }

            return text;
        }
    }

    std::filesystem::path ProjectRoot()
    {
        return std::filesystem::path(PHOENIX_PROJECT_ROOT);
    }

    std::filesystem::path LogsRoot()
    {
        return ProjectRoot() / L"Logs";
    }

    std::filesystem::path BuildRoot()
    {
        return ProjectRoot() / L"Build";
    }

    std::filesystem::path GeneratedRoot()
    {
        return ProjectRoot() / L"Generated";
    }

    std::filesystem::path GameDataRoot()
    {
        return ProjectRoot() / L"GameData" / L"data";
    }

    std::filesystem::path GameDataTextRoot()
    {
        return GameDataRoot() / L"txt";
    }

    std::filesystem::path LegacyDesignedCommonRoot()
    {
        return std::filesystem::path(L"F:\\Project Phoenix\\Designed_Common");
    }

    std::filesystem::path LegacyDesignedChsRoot()
    {
        return std::filesystem::path(L"F:\\Project Phoenix\\Designed_Chs");
    }

    std::filesystem::path LegacyServerRoot()
    {
        return std::filesystem::path(L"F:\\Project Phoenix\\ServerSRC");
    }

    bool IsForbiddenLegacyRoot(const std::filesystem::path& path)
    {
        const auto normalized = NormalizeForPrefixCheck(path);
        const std::array legacyRoots{
            NormalizeForPrefixCheck(LegacyDesignedCommonRoot()),
            NormalizeForPrefixCheck(LegacyDesignedChsRoot()),
            NormalizeForPrefixCheck(LegacyServerRoot()),
        };

        for (const auto& legacyRoot : legacyRoots)
        {
            if (normalized.rfind(legacyRoot, 0) == 0)
            {
                return true;
            }
        }

        return false;
    }
}
