#pragma once

#include "Core/StrongId.h"

#include <cstdint>

namespace phoenix::graphics
{
    struct TextureHandleTag;
    using TextureHandle = core::StrongId<TextureHandleTag, std::uint64_t>;

    struct TextureDescriptor
    {
        TextureHandle handle;
        std::uint32_t width{ 0 };
        std::uint32_t height{ 0 };
    };
}
