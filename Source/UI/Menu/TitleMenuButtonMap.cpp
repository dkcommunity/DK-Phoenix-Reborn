#include "UI/Menu/TitleMenuButtonMap.h"

namespace phoenix::ui
{
    std::vector<TitleMenuButtonFrames> TitleMenuButtonMap::CreateDefault()
    {
        return {
            TitleMenuButtonFrames{ TitleMenuButtonRole::Login, core::SpriteId{ 0 }, core::SpriteId{ 1 }, L"CONNECT" },
            TitleMenuButtonFrames{ TitleMenuButtonRole::Option, core::SpriteId{ 2 }, core::SpriteId{ 3 }, L"OPTION" },
            TitleMenuButtonFrames{ TitleMenuButtonRole::Credits, core::SpriteId{ 4 }, core::SpriteId{ 5 }, L"CREDIT" },
            TitleMenuButtonFrames{ TitleMenuButtonRole::Exit, core::SpriteId{ 6 }, core::SpriteId{ 7 }, L"EXIT" },
            TitleMenuButtonFrames{ TitleMenuButtonRole::NewAccount, core::SpriteId{ 8 }, core::SpriteId{ 9 }, L"NEWID" }
        };
    }

    const TitleMenuButtonFrames* TitleMenuButtonMap::Find(
        const std::vector<TitleMenuButtonFrames>& map,
        const TitleMenuButtonRole role) noexcept
    {
        for (const auto& entry : map)
        {
            if (entry.role == role)
            {
                return &entry;
            }
        }

        return nullptr;
    }
}
