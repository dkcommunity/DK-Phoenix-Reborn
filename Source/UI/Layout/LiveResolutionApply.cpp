#include "UI/Layout/LiveResolutionApply.h"

#include "UI/Layout/FrontendLayoutValidator.h"

#include <algorithm>

namespace phoenix::ui
{
    LiveResolutionApplyResult LiveResolutionApply::Apply(
        OptionsGraphicsModel& options,
        backends::directx11::DirectX11Renderer& renderer,
        const std::uint32_t width,
        const std::uint32_t height)
    {
        LiveResolutionApplyResult result;
        result.selection = options.SelectResolution(width, height);
        if (result.selection.resolutionPlan.resizeSwapChain)
        {
            result.resize = renderer.ResizeSwapChain(result.selection.display.width, result.selection.display.height);
        }

        result.layoutsValid = std::all_of(
            result.selection.layouts.begin(),
            result.selection.layouts.end(),
            [](const FrontendLayout& layout) {
                return FrontendLayoutValidator::Validate(layout).valid;
            });
        result.rendererStatePreserved = !result.selection.resolutionPlan.recreateRendererState &&
            (!result.resize.swapChainNeedsResize || result.resize.rendererStatePreserved);
        return result;
    }
}
