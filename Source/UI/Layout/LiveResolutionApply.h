#pragma once

#include "Backends/DirectX11/DirectX11Renderer.h"
#include "UI/Options/OptionsGraphicsModel.h"

namespace phoenix::ui
{
    struct LiveResolutionApplyResult
    {
        GraphicsResolutionSelection selection;
        backends::directx11::ResizeResult resize;
        bool layoutsValid{ false };
        bool rendererStatePreserved{ false };
    };

    class LiveResolutionApply final
    {
    public:
        [[nodiscard]] static LiveResolutionApplyResult Apply(
            OptionsGraphicsModel& options,
            backends::directx11::DirectX11Renderer& renderer,
            std::uint32_t width,
            std::uint32_t height);
    };
}
