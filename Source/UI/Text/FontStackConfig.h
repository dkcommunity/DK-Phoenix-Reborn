#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::ui::text
{
    struct FontStackConfig
    {
        std::vector<std::string> defaultFontStack;
        std::string uiRegular;
        std::string chatRegular;
        std::vector<std::string> chatFallback;
        std::string debugRegular;
        std::string emojiSystemFallback;
    };

    class FontStackConfigLoader final
    {
    public:
        [[nodiscard]] bool Load(const std::filesystem::path& stackConfigPath, FontStackConfig& outConfig) const;
    };
}
