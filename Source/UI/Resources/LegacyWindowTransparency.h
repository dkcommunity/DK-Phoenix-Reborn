#pragma once

#include <cstdint>

namespace phoenix::ui
{
    class LegacyWindowTransparency final
    {
    public:
        static constexpr std::uint8_t MaximumAlphaDepth = 31;
        static constexpr std::uint8_t DefaultAlphaDepth = 23;

        explicit constexpr LegacyWindowTransparency(const bool supported) noexcept
            : supported_(supported)
        {
        }

        [[nodiscard]] constexpr bool IsSupported() const noexcept { return supported_; }
        [[nodiscard]] constexpr bool IsEnabled() const noexcept { return supported_ && enabled_; }
        [[nodiscard]] constexpr std::uint8_t AlphaDepth() const noexcept { return alphaDepth_; }

        void SetEnabled(bool enabled) noexcept;
        void SetAlphaDepth(std::uint32_t alphaDepth) noexcept;
        void SetAlphaDepthFromTrack(std::int32_t position, std::int32_t width) noexcept;

        [[nodiscard]] float BackgroundOpacity() const noexcept;
        [[nodiscard]] float TrackRatio() const noexcept;

    private:
        std::uint8_t alphaDepth_{ DefaultAlphaDepth };
        bool supported_{ false };
        bool enabled_{ false };
    };
}
