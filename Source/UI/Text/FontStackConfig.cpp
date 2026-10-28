#include "UI/Text/FontStackConfig.h"

namespace phoenix::ui::text
{
    bool FontStackConfigLoader::Load(const std::filesystem::path& stackConfigPath, FontStackConfig& outConfig) const
    {
        if (!std::filesystem::exists(stackConfigPath))
        {
            return false;
        }

        outConfig.defaultFontStack = {
            "Noto Sans",
            "Noto Sans KR",
            "Noto Sans JP",
            "Noto Sans SC",
            "Noto Sans TC",
            "Noto Sans HK",
            "Noto Sans Symbols 2",
            "Segoe UI Emoji",
        };
        outConfig.uiRegular = "Noto Sans";
        outConfig.chatRegular = "Noto Sans";
        outConfig.chatFallback = {
            "Noto Sans KR",
            "Noto Sans JP",
            "Noto Sans SC",
            "Noto Sans TC",
            "Noto Sans Symbols 2",
            "Segoe UI Emoji",
        };
        outConfig.debugRegular = "Noto Sans Mono";
        outConfig.emojiSystemFallback = "Segoe UI Emoji";
        return true;
    }
}
