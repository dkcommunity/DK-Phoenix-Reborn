#pragma once

#include "Settings/SettingsTypes.h"

#include <array>
#include <cstdint>

namespace phoenix::settings
{
    struct DisplayResolution
    {
        std::uint32_t width{ 1920 };
        std::uint32_t height{ 1080 };
    };

    class DisplayResolutionCatalog final
    {
    public:
        [[nodiscard]] static constexpr DisplayResolution Default() noexcept
        {
            return DisplayResolution{ 1920, 1080 };
        }

        [[nodiscard]] static constexpr std::array<DisplayResolution, 4> Supported() noexcept
        {
            return {
                DisplayResolution{ 1920, 1080 },
                DisplayResolution{ 1280, 1024 },
                DisplayResolution{ 1280, 960 },
                DisplayResolution{ 1024, 768 }
            };
        }

        [[nodiscard]] static constexpr bool IsSupported(const std::uint32_t width, const std::uint32_t height) noexcept
        {
            for (const auto resolution : Supported())
            {
                if (resolution.width == width && resolution.height == height)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] static constexpr DisplaySettings Normalize(DisplaySettings settings) noexcept
        {
            if (!IsSupported(settings.width, settings.height))
            {
                const auto fallback = Default();
                settings.width = fallback.width;
                settings.height = fallback.height;
            }

            return settings;
        }
    };
}
