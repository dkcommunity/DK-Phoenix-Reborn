#pragma once

#include "UI/Text/LoginTextLayout.h"
#include "UI/Text/LoginTextVisualModel.h"
#include "Graphics/ImageData.h"

#include <cstdint>
#include <vector>

namespace phoenix::ui::text
{
    struct DirectWriteTextRendererConfig
    {
        void* windowHandle{ nullptr };
        std::uint32_t width{ 1920 };
        std::uint32_t height{ 1080 };
    };

    struct DirectWriteTextDrawResult
    {
        bool drawn{ false };
        std::uint32_t textRunCount{ 0 };
        bool focusDrawn{ false };
        bool caretDrawn{ false };
    };

    class DirectWriteTextRenderer final
    {
    public:
        DirectWriteTextRenderer();
        ~DirectWriteTextRenderer();
        DirectWriteTextRenderer(const DirectWriteTextRenderer&) = delete;
        DirectWriteTextRenderer& operator=(const DirectWriteTextRenderer&) = delete;

        [[nodiscard]] bool Initialize(DirectWriteTextRendererConfig config);
        [[nodiscard]] bool Resize(std::uint32_t width, std::uint32_t height);
        [[nodiscard]] DirectWriteTextDrawResult DrawTextRuns(const std::vector<LoginTextRun>& runs);
        [[nodiscard]] DirectWriteTextDrawResult DrawLoginText(
            const std::vector<LoginTextRun>& runs,
            const LoginTextVisualState& visual);
        [[nodiscard]] graphics::ImageData RasterizeText(
            const std::wstring& text,
            std::uint32_t width,
            std::uint32_t height,
            std::uint32_t color);
        void Shutdown() noexcept;

    private:
        struct NativeState;

        DirectWriteTextRendererConfig config_;
        NativeState* native_{ nullptr };
    };
}
