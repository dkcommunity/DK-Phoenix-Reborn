#include "UI/Text/DirectWriteTextRenderer.h"

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstring>

namespace phoenix::ui::text
{
    struct DirectWriteTextRenderer::NativeState
    {
        Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory;
        Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory;
        Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
        Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> renderTarget;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> focusBrush;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> caretBrush;
        Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
        HWND window{ nullptr };
    };

    namespace
    {
        D2D1_RECT_F ToD2DRect(const UiRect rect) noexcept
        {
            return D2D1::RectF(
                static_cast<float>(rect.x),
                static_cast<float>(rect.y),
                static_cast<float>(rect.x + rect.width),
                static_cast<float>(rect.y + rect.height));
        }
    }

    DirectWriteTextRenderer::DirectWriteTextRenderer()
        : native_(new NativeState())
    {
    }

    DirectWriteTextRenderer::~DirectWriteTextRenderer()
    {
        Shutdown();
        // native_ is already deleted and nullified in Shutdown()
    }

    bool DirectWriteTextRenderer::Initialize(const DirectWriteTextRendererConfig config)
    {
        config_ = config;
        native_->window = static_cast<HWND>(config.windowHandle);
        if (native_->window == nullptr || config.width == 0 || config.height == 0)
        {
            return false;
        }

        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, native_->d2dFactory.GetAddressOf())))
        {
            return false;
        }
        if (FAILED(DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(native_->dwriteFactory.GetAddressOf()))))
        {
            return false;
        }
        if (FAILED(CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(native_->wicFactory.GetAddressOf()))))
        {
            return false;
        }

        const auto renderTargetProperties = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
        const auto hwndProperties = D2D1::HwndRenderTargetProperties(
            native_->window,
            D2D1::SizeU(config.width, config.height));
        if (FAILED(native_->d2dFactory->CreateHwndRenderTarget(
            renderTargetProperties,
            hwndProperties,
            native_->renderTarget.GetAddressOf())))
        {
            return false;
        }

        if (FAILED(native_->renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), native_->textBrush.GetAddressOf())) ||
            FAILED(native_->renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.35F, 0.65F, 1.0F, 0.75F), native_->focusBrush.GetAddressOf())) ||
            FAILED(native_->renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), native_->caretBrush.GetAddressOf())))
        {
            return false;
        }

        if (FAILED(native_->dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            14.0F,
            L"",
            native_->textFormat.GetAddressOf())))
        {
            return false;
        }

        native_->textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        return true;
    }

    bool DirectWriteTextRenderer::Resize(const std::uint32_t width, const std::uint32_t height)
    {
        config_.width = width;
        config_.height = height;
        if (!native_->renderTarget)
        {
            return false;
        }

        return SUCCEEDED(native_->renderTarget->Resize(D2D1::SizeU(width, height)));
    }

    DirectWriteTextDrawResult DirectWriteTextRenderer::DrawLoginText(
        const std::vector<LoginTextRun>& runs,
        const LoginTextVisualState& visual)
    {
        DirectWriteTextDrawResult result;
        if (!native_->renderTarget || !native_->textFormat || !native_->textBrush)
        {
            return result;
        }

        (void)visual;
        native_->renderTarget->BeginDraw();

        for (const auto& run : runs)
        {
            native_->textBrush->SetColor(D2D1::ColorF(
                static_cast<float>(GetRValue(run.color)) / 255.0F,
                static_cast<float>(GetGValue(run.color)) / 255.0F,
                static_cast<float>(GetBValue(run.color)) / 255.0F));
            native_->renderTarget->DrawTextW(
                run.text.c_str(),
                static_cast<UINT32>(run.text.size()),
                native_->textFormat.Get(),
                ToD2DRect(run.rect),
                native_->textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
            ++result.textRunCount;
        }

        result.drawn = SUCCEEDED(native_->renderTarget->EndDraw());
        return result;
    }

    DirectWriteTextDrawResult DirectWriteTextRenderer::DrawTextRuns(const std::vector<LoginTextRun>& runs)
    {
        DirectWriteTextDrawResult result;
        if (!native_->renderTarget || !native_->textFormat || !native_->textBrush)
        {
            return result;
        }

        native_->renderTarget->BeginDraw();
        for (const auto& run : runs)
        {
            native_->textBrush->SetColor(D2D1::ColorF(
                static_cast<float>(GetRValue(run.color)) / 255.0F,
                static_cast<float>(GetGValue(run.color)) / 255.0F,
                static_cast<float>(GetBValue(run.color)) / 255.0F));
            native_->renderTarget->DrawTextW(
                run.text.c_str(),
                static_cast<UINT32>(run.text.size()),
                native_->textFormat.Get(),
                ToD2DRect(run.rect),
                native_->textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
            ++result.textRunCount;
        }

        result.drawn = SUCCEEDED(native_->renderTarget->EndDraw());
        return result;
    }

    graphics::ImageData DirectWriteTextRenderer::RasterizeText(
        const std::wstring& text,
        const std::uint32_t width,
        const std::uint32_t height,
        const std::uint32_t color)
    {
        graphics::ImageData image;
        image.width = std::max(1U, width);
        image.height = std::max(1U, height);
        image.format = graphics::PixelFormat::Rgba8;
        image.pixels.assign(static_cast<std::size_t>(image.width) * image.height * 4U, 0U);
        if (text.empty() || !native_->wicFactory || !native_->d2dFactory || !native_->dwriteFactory)
        {
            return image;
        }

        Microsoft::WRL::ComPtr<IWICBitmap> bitmap;
        if (FAILED(native_->wicFactory->CreateBitmap(
            image.width,
            image.height,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapCacheOnLoad,
            bitmap.GetAddressOf())))
        {
            return image;
        }

        Microsoft::WRL::ComPtr<ID2D1RenderTarget> renderTarget;
        const auto properties = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        if (FAILED(native_->d2dFactory->CreateWicBitmapRenderTarget(
            bitmap.Get(), properties, renderTarget.GetAddressOf())))
        {
            return image;
        }

        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
        if (FAILED(renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(
                static_cast<float>(GetRValue(color)) / 255.0F,
                static_cast<float>(GetGValue(color)) / 255.0F,
                static_cast<float>(GetBValue(color)) / 255.0F,
                1.0F),
            brush.GetAddressOf())))
        {
            return image;
        }

        Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
        if (FAILED(native_->dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            static_cast<float>(std::clamp<std::uint32_t>(image.height > 4U ? image.height - 4U : image.height, 8U, 18U)),
            L"",
            format.GetAddressOf())))
        {
            return image;
        }
        format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        renderTarget->BeginDraw();
        renderTarget->Clear(D2D1::ColorF(0.0F, 0.0F, 0.0F, 0.0F));
        // ClearType assumes an opaque destination. The chat texture is
        // transparent, so grayscale antialiasing is required to produce the
        // coverage alpha consumed by the DX11 straight-alpha blend state.
        renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        renderTarget->DrawTextW(
            text.c_str(),
            static_cast<UINT32>(text.size()),
            format.Get(),
            D2D1::RectF(2.0F, 0.0F, static_cast<float>(image.width), static_cast<float>(image.height)),
            brush.Get(),
            D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
        if (FAILED(renderTarget->EndDraw()))
        {
            return image;
        }

        WICRect lockRect{ 0, 0, static_cast<INT>(image.width), static_cast<INT>(image.height) };
        Microsoft::WRL::ComPtr<IWICBitmapLock> lock;
        if (FAILED(bitmap->Lock(&lockRect, WICBitmapLockRead, lock.GetAddressOf())))
        {
            return image;
        }
        UINT stride = 0;
        UINT byteCount = 0;
        BYTE* source = nullptr;
        if (FAILED(lock->GetStride(&stride)) || FAILED(lock->GetDataPointer(&byteCount, &source)) || source == nullptr)
        {
            return image;
        }

        for (std::uint32_t y = 0; y < image.height; ++y)
        {
            const auto* row = source + static_cast<std::size_t>(y) * stride;
            for (std::uint32_t x = 0; x < image.width; ++x)
            {
                const auto src = row + static_cast<std::size_t>(x) * 4U;
                const auto dst = (static_cast<std::size_t>(y) * image.width + x) * 4U;
                const auto alpha = static_cast<std::uint32_t>(src[3U]);
                const auto unpremultiply = [alpha](const BYTE component) {
                    return alpha == 0U
                        ? static_cast<std::uint8_t>(0U)
                        : static_cast<std::uint8_t>(std::min<std::uint32_t>(255U,
                            (static_cast<std::uint32_t>(component) * 255U + alpha / 2U) / alpha));
                };
                image.pixels[dst + 0U] = unpremultiply(src[2U]);
                image.pixels[dst + 1U] = unpremultiply(src[1U]);
                image.pixels[dst + 2U] = unpremultiply(src[0U]);
                image.pixels[dst + 3U] = static_cast<std::uint8_t>(alpha);
            }
        }
        return image;
    }

    void DirectWriteTextRenderer::Shutdown() noexcept
    {
        if (native_ == nullptr)
        {
            return;
        }

        native_->textFormat.Reset();
        native_->caretBrush.Reset();
        native_->focusBrush.Reset();
        native_->textBrush.Reset();
        native_->renderTarget.Reset();
        native_->wicFactory.Reset();
        native_->dwriteFactory.Reset();
        native_->d2dFactory.Reset();
        native_->window = nullptr;
        
        // Prevent double-shutdown by marking as shut down
        delete native_;
        native_ = nullptr;
    }
}
