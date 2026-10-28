#pragma once

#include "Core/StrongId.h"
#include "Graphics/ImageData.h"
#include "Graphics/TextureTypes.h"

#include <map>
#include <optional>

namespace phoenix::graphics
{
    class DecodedSpriteCache final
    {
    public:
        void Store(core::SpriteId spriteId, ImageData image);
        [[nodiscard]] const ImageData* Find(core::SpriteId spriteId) const noexcept;
        void Clear() noexcept;

    private:
        std::map<core::SpriteId, ImageData> images_;
    };

    class TextureCache final
    {
    public:
        TextureDescriptor Register(core::SpriteId spriteId, std::uint32_t width, std::uint32_t height);
        [[nodiscard]] std::optional<TextureDescriptor> Find(core::SpriteId spriteId) const noexcept;
        void Clear() noexcept;

    private:
        std::map<core::SpriteId, TextureDescriptor> textures_;
        std::uint64_t nextHandle_{ 1 };
    };
}
