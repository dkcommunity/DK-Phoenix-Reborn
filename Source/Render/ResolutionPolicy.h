#pragma once

#include <cstdint>

namespace phoenix::render
{
    struct ResolutionChangeRequest
    {
        std::uint32_t width{ 0 };
        std::uint32_t height{ 0 };
    };

    struct ResolutionChangePlan
    {
        bool resizeSwapChain{ false };
        bool updateLayout{ false };
        bool recreateRendererState{ false };
    };

    class ResolutionPolicy final
    {
    public:
        [[nodiscard]] static constexpr bool IsSupported(const std::uint32_t width, const std::uint32_t height) noexcept
        {
            return (width == 1920 && height == 1080) || (width == 1024 && height == 768);
        }

        [[nodiscard]] static constexpr ResolutionChangeRequest Normalize(ResolutionChangeRequest request) noexcept
        {
            if (!IsSupported(request.width, request.height))
            {
                return ResolutionChangeRequest{ 1920, 1080 };
            }

            return request;
        }

        [[nodiscard]] static ResolutionChangePlan Plan(
            std::uint32_t currentWidth,
            std::uint32_t currentHeight,
            ResolutionChangeRequest request) noexcept
        {
            request = Normalize(request);
            const auto changed = request.width != 0 &&
                request.height != 0 &&
                (request.width != currentWidth || request.height != currentHeight);

            return ResolutionChangePlan{
                changed,
                changed,
                false
            };
        }
    };
}
