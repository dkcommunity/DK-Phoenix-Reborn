#pragma once

#include <filesystem>
#include <string>

namespace phoenix::prototype
{
    struct DataBackedPrototypePlan
    {
        std::wstring creatureBaseName;
        std::filesystem::path mapPath;
        std::filesystem::path soundPath;
        std::filesystem::path musicPath;
        std::filesystem::path uiTextDirectory;
        std::filesystem::path loginSpritePackPath;
        std::filesystem::path loginSpriteIndexPath;
        std::filesystem::path titleMenuSpritePackPath;
        std::filesystem::path titleMenuSpriteIndexPath;
        std::filesystem::path serverSelectSpritePackPath;
        std::filesystem::path serverSelectSpriteIndexPath;
        std::filesystem::path characterSelectSpriteDirectory;
        std::filesystem::path characterCreateSpriteDirectory;
    };

    class DataBackedPrototypePlanFactory final
    {
    public:
        [[nodiscard]] static DataBackedPrototypePlan CreateDefault(const std::filesystem::path& gameDataRoot);
    };
}
