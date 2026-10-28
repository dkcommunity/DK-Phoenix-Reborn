#pragma once

#include "Prototype/DataBackedPrototypePlan.h"

#include <cstdint>
#include <vector>

namespace phoenix::prototype
{
    struct LoginTitleSpriteSummary
    {
        std::uint16_t spriteIndex{ 0 };
        std::uint16_t width{ 0 };
        std::uint16_t height{ 0 };
        std::uint32_t byteSize{ 0 };
        bool decoded{ false };
    };

    struct LoginTitlePrototypeResult
    {
        std::uint16_t indexedSpriteCount{ 0 };
        std::uint32_t decodedSpriteCount{ 0 };
        std::uint32_t uploadedTextureCount{ 0 };
        std::uint32_t submittedCommandCount{ 0 };
        std::uint32_t titleMenuSpriteCount{ 0 };
        std::uint32_t titleMenuDecodedSpriteCount{ 0 };
        std::uint32_t drawnQuadCount{ 0 };
        std::uint32_t presentedFrameCount{ 0 };
        std::uint32_t textRunCount{ 0 };
        bool visibleBackgroundSubmitted{ false };
        bool visibleButtonSubmitted{ false };
        bool layoutValid{ false };
        bool framePresented{ false };
        std::vector<LoginTitleSpriteSummary> sprites;
    };

    class LoginTitlePrototype final
    {
    public:
        [[nodiscard]] LoginTitlePrototypeResult Run(const DataBackedPrototypePlan& plan, std::uint16_t maxSprites) const;
        [[nodiscard]] LoginTitlePrototypeResult RunForViewport(
            const DataBackedPrototypePlan& plan,
            std::uint16_t maxSprites,
            std::uint32_t width,
            std::uint32_t height) const;
    };
}
