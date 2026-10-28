#pragma once

#include <cstddef>
#include <cstdint>

namespace phoenix::world
{
    struct WorldAssetCacheBudget
    {
        std::size_t maxTileTextures{ 2048 };
        std::size_t maxObjectTextures{ 2048 };
        std::size_t maxCreatureTextures{ 2048 };
        std::size_t maxDecodedSprites{ 8192 };
    };

    struct WorldAssetCacheStats
    {
        std::size_t tileTextureCount{ 0 };
        std::size_t objectTextureCount{ 0 };
        std::size_t creatureTextureCount{ 0 };
        std::size_t itemTextureCount{ 0 };
        std::size_t effectTextureCount{ 0 };
        std::size_t decodedSpriteCount{ 0 };
        std::size_t pendingDecodeCount{ 0 };
        std::uint64_t cacheHits{ 0 };
        std::uint64_t cacheMisses{ 0 };
        std::uint64_t textureUploads{ 0 };
        std::uint64_t deferredTextureUploads{ 0 };
        std::uint32_t frameTextureUploadAttempts{ 0 };
        std::uint32_t lastFrameMs{ 0 };
        std::uint32_t worstFrameMs{ 0 };

        void ResetCounters() noexcept
        {
            cacheHits = 0;
            cacheMisses = 0;
            textureUploads = 0;
            deferredTextureUploads = 0;
            frameTextureUploadAttempts = 0;
            lastFrameMs = 0;
            worstFrameMs = 0;
        }
    };

    [[nodiscard]] WorldAssetCacheBudget SelectWorldAssetCacheBudget(bool highQuality) noexcept;
}
