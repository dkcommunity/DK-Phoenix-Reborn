#include "Graphics/ResourceCache.h"

#include <utility>

namespace phoenix::graphics
{
    void DecodedSpriteCache::Store(core::SpriteId spriteId, ImageData image)
    {
        images_[spriteId] = std::move(image);
    }

    const ImageData* DecodedSpriteCache::Find(core::SpriteId spriteId) const noexcept
    {
        const auto found = images_.find(spriteId);
        return found == images_.end() ? nullptr : &found->second;
    }

    void DecodedSpriteCache::Clear() noexcept
    {
        images_.clear();
    }

    TextureDescriptor TextureCache::Register(core::SpriteId spriteId, std::uint32_t width, std::uint32_t height)
    {
        const auto existing = textures_.find(spriteId);
        if (existing != textures_.end())
        {
            return existing->second;
        }

        TextureDescriptor descriptor{ TextureHandle(nextHandle_++), width, height };
        textures_[spriteId] = descriptor;
        return descriptor;
    }

    std::optional<TextureDescriptor> TextureCache::Find(core::SpriteId spriteId) const noexcept
    {
        const auto found = textures_.find(spriteId);
        if (found == textures_.end())
        {
            return std::nullopt;
        }

        return found->second;
    }

    void TextureCache::Clear() noexcept
    {
        textures_.clear();
        nextHandle_ = 1;
    }
}
