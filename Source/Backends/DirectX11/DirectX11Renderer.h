#pragma once

#include "Graphics/ImageData.h"
#include "Graphics/TextureTypes.h"
#include "Render/RenderTypes.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace phoenix::backends::directx11
{
    struct RendererConfig
    {
        std::uint32_t width{ 1920 };
        std::uint32_t height{ 1080 };
        bool vsync{ true };
        void* windowHandle{ nullptr };
    };

    struct ResizeResult
    {
        bool swapChainNeedsResize{ false };
        bool layoutNeedsUpdate{ false };
        bool rendererStatePreserved{ true };
    };

    struct UiSpritePipelineState
    {
        bool alphaBlendEnabled{ true };
        bool linearSamplerEnabled{ false };
        bool scissorEnabled{ true };
    };

    struct UiScissorRect
    {
        std::int32_t x{ 0 };
        std::int32_t y{ 0 };
        std::int32_t width{ 0 };
        std::int32_t height{ 0 };
    };

    enum class TexturedQuadBlendMode : std::uint8_t
    {
        Alpha,
        Screen
    };

    struct TexturedQuad
    {
        graphics::TextureDescriptor texture;
        render::ScreenPoint position;
        std::uint32_t width{ 0 };
        std::uint32_t height{ 0 };
        float opacity{ 1.0F };
        float u0{ 0.0F };
        float v0{ 0.0F };
        float u1{ 1.0F };
        float v1{ 1.0F };
        float colorScale{ 1.0F };
        TexturedQuadBlendMode blendMode{ TexturedQuadBlendMode::Alpha };
    };

    struct PresentResult
    {
        bool presented{ false };
        std::uint32_t frameIndex{ 0 };
    };

    class DirectX11Renderer final
    {
    public:
        struct NativeState;

        DirectX11Renderer();
        ~DirectX11Renderer();
        DirectX11Renderer(const DirectX11Renderer&) = delete;
        DirectX11Renderer& operator=(const DirectX11Renderer&) = delete;

        [[nodiscard]] bool Initialize(RendererConfig config);
        [[nodiscard]] ResizeResult ResizeSwapChain(std::uint32_t width, std::uint32_t height) noexcept;
        void Shutdown() noexcept;
        [[nodiscard]] bool IsInitialized() const noexcept;
        [[nodiscard]] bool HasNativeDevice() const noexcept;
        [[nodiscard]] bool HasSwapChain() const noexcept;
        [[nodiscard]] bool InitializeUiSpritePipeline() noexcept;
        [[nodiscard]] bool IsUiSpritePipelineInitialized() const noexcept;
        void ConfigureUiSpritePipeline(UiSpritePipelineState state) noexcept;
        [[nodiscard]] UiSpritePipelineState GetUiSpritePipelineState() const noexcept;
        void SetUiScissorRect(std::optional<UiScissorRect> rect) noexcept;
        void SetVSync(bool enabled) noexcept;
        [[nodiscard]] bool VSyncEnabled() const noexcept;
        [[nodiscard]] graphics::TextureDescriptor UploadTexture(const graphics::ImageData& image);
        void ReleaseTexture(graphics::TextureDescriptor texture) noexcept;
        void ClearFrame(float red, float green, float blue, float alpha) noexcept;
        [[nodiscard]] bool DrawTexturedQuad(const TexturedQuad& quad) noexcept;
        [[nodiscard]] std::uint32_t DrawTexturedQuads(const std::vector<TexturedQuad>& quads) noexcept;
        [[nodiscard]] PresentResult PresentFrame() noexcept;
        void Submit(const std::vector<render::RenderCommand>& commands);
        [[nodiscard]] std::uint32_t SubmittedCommandCount() const noexcept;
        [[nodiscard]] std::uint32_t UploadedTextureCount() const noexcept;
        [[nodiscard]] std::uint32_t DrawnQuadCount() const noexcept;
        [[nodiscard]] std::uint32_t FrameDrawCallCount() const noexcept;
        [[nodiscard]] std::uint32_t FrameQuadCount() const noexcept;
        [[nodiscard]] std::uint32_t PresentedFrameCount() const noexcept;

    private:
        RendererConfig config_;
        std::unique_ptr<NativeState> native_;
        bool initialized_{ false };
        bool uiSpritePipelineInitialized_{ false };
        UiSpritePipelineState uiSpritePipelineState_;
        std::optional<UiScissorRect> uiScissorRect_;
        std::uint32_t submittedCommandCount_{ 0 };
        std::uint32_t uploadedTextureCount_{ 0 };
        std::uint32_t drawnQuadCount_{ 0 };
        std::uint32_t frameDrawCallCount_{ 0 };
        std::uint32_t frameQuadCount_{ 0 };
        std::uint32_t presentedFrameCount_{ 0 };
    };
}
