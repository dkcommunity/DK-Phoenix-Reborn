#include "World/WorldAssetCache.h"

namespace phoenix::world
{
    WorldAssetCacheBudget SelectWorldAssetCacheBudget(const bool highQuality) noexcept
    {
        if (highQuality)
        {
            return WorldAssetCacheBudget{
                16384,
                16384,
                8192,
                32768
            };
        }

        return WorldAssetCacheBudget{
            16384,
            16384,
            8192,
            32768
        };
    }
}
