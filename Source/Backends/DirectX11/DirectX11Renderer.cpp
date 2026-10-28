#include "Backends/DirectX11/DirectX11Renderer.h"

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <limits>
#include <string>
#include <vector>

namespace phoenix::backends::directx11
{
    namespace
    {
        struct UiVertex
        {
            float x;
            float y;
            float u;
            float v;
            float opacity;
            float colorScale;
        };

        struct UiConstants
        {
            float viewportWidth;
            float viewportHeight;
            float padding[2];
        };

        constexpr char kUiShaderSource[] =
            "cbuffer UiConstants : register(b0) { float2 viewportSize; float2 padding; };"
            "struct VSInput { float2 pos : POSITION; float2 uv : TEXCOORD0; float opacity : TEXCOORD1; float colorScale : TEXCOORD2; };"
            "struct PSInput { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float opacity : TEXCOORD1; float colorScale : TEXCOORD2; };"
            "PSInput VSMain(VSInput input) {"
            "  PSInput output;"
            "  float2 ndc = float2((input.pos.x / viewportSize.x) * 2.0f - 1.0f, 1.0f - (input.pos.y / viewportSize.y) * 2.0f);"
            "  output.pos = float4(ndc, 0.0f, 1.0f);"
            "  output.uv = input.uv;"
            "  output.opacity = input.opacity;"
            "  output.colorScale = input.colorScale;"
            "  return output;"
            "}"
            "Texture2D uiTexture : register(t0);"
            "SamplerState uiSampler : register(s0);"
            "float4 PSMain(PSInput input) : SV_TARGET {"
            "  float4 color = uiTexture.Sample(uiSampler, input.uv);"
            "  color.rgb *= input.colorScale;"
            "  color.a *= input.opacity;"
            "  return color;"
            "}";

        template <typename T>
        void SafeRelease(Microsoft::WRL::ComPtr<T>& value) noexcept
        {
            value.Reset();
        }
    }

    struct DirectX11Renderer::NativeState
    {
        Microsoft::WRL::ComPtr<ID3D11Device> device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> uiVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> uiPixelShader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> uiInputLayout;
        Microsoft::WRL::ComPtr<ID3D11Buffer> uiVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> uiIndexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> uiConstantBuffer;
        Microsoft::WRL::ComPtr<ID3D11BlendState> alphaBlendState;
        Microsoft::WRL::ComPtr<ID3D11BlendState> screenBlendState;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> pointSamplerState;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> linearSamplerState;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> scissorRasterizerState;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> defaultRasterizerState;
        std::map<std::uint64_t, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> shaderResources;
        std::vector<const TexturedQuad*> drawableScratch;
        HWND window{ nullptr };
        bool renderTargetBound{ false };
        bool uiBuffersReady{ false };
        std::uint32_t uiQuadCapacity{ 0 };
    };

    DirectX11Renderer::DirectX11Renderer()
        : native_(std::make_unique<NativeState>())
    {
    }

    DirectX11Renderer::~DirectX11Renderer() = default;

    namespace
    {
        bool CreateRenderTarget(DirectX11Renderer::NativeState& native)
        {
            if (!native.swapChain || !native.device)
            {
                return false;
            }

            Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
            if (FAILED(native.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()))))
            {
                return false;
            }

            native.renderTargetView.Reset();
            if (FAILED(native.device->CreateRenderTargetView(backBuffer.Get(), nullptr, native.renderTargetView.GetAddressOf())))
            {
                return false;
            }

            native.context->OMSetRenderTargets(1, native.renderTargetView.GetAddressOf(), nullptr);
            native.renderTargetBound = true;
            return true;
        }

        bool CreateOffscreenRenderTarget(DirectX11Renderer::NativeState& native, const std::uint32_t width, const std::uint32_t height)
        {
            if (!native.device)
            {
                return false;
            }

            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = width;
            desc.Height = height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET;

            Microsoft::WRL::ComPtr<ID3D11Texture2D> target;
            if (FAILED(native.device->CreateTexture2D(&desc, nullptr, target.GetAddressOf())) ||
                FAILED(native.device->CreateRenderTargetView(target.Get(), nullptr, native.renderTargetView.GetAddressOf())))
            {
                return false;
            }

            native.context->OMSetRenderTargets(1, native.renderTargetView.GetAddressOf(), nullptr);
            native.renderTargetBound = true;
            return true;
        }

        void SetViewport(DirectX11Renderer::NativeState& native, const std::uint32_t width, const std::uint32_t height)
        {
            if (!native.context)
            {
                return;
            }

            D3D11_VIEWPORT viewport{};
            viewport.Width = static_cast<float>(width);
            viewport.Height = static_cast<float>(height);
            viewport.MinDepth = 0.0F;
            viewport.MaxDepth = 1.0F;
            native.context->RSSetViewports(1, &viewport);
        }

        bool EnsureUiQuadBuffers(DirectX11Renderer::NativeState& native, const std::uint32_t quadCount)
        {
            if (!native.device || quadCount == 0)
            {
                return false;
            }
            if (native.uiBuffersReady && native.uiQuadCapacity >= quadCount)
            {
                return true;
            }

            constexpr std::uint32_t kMaxIndexedQuads = std::numeric_limits<std::uint16_t>::max() / 4U;
            const auto capacity = std::min<std::uint32_t>(
                kMaxIndexedQuads,
                std::max<std::uint32_t>(quadCount, native.uiQuadCapacity == 0 ? 256U : native.uiQuadCapacity * 2U));
            if (capacity < quadCount)
            {
                return false;
            }

            D3D11_BUFFER_DESC vertexBufferDesc{};
            vertexBufferDesc.ByteWidth = sizeof(UiVertex) * 4U * capacity;
            vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
            vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
            if (FAILED(native.device->CreateBuffer(&vertexBufferDesc, nullptr, vertexBuffer.GetAddressOf())))
            {
                return false;
            }

            std::vector<std::uint16_t> indices;
            indices.reserve(static_cast<std::size_t>(capacity) * 6U);
            for (std::uint32_t quad = 0; quad < capacity; ++quad)
            {
                const auto base = static_cast<std::uint16_t>(quad * 4U);
                indices.push_back(base);
                indices.push_back(static_cast<std::uint16_t>(base + 1U));
                indices.push_back(static_cast<std::uint16_t>(base + 2U));
                indices.push_back(static_cast<std::uint16_t>(base + 2U));
                indices.push_back(static_cast<std::uint16_t>(base + 1U));
                indices.push_back(static_cast<std::uint16_t>(base + 3U));
            }

            D3D11_BUFFER_DESC indexBufferDesc{};
            indexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(std::uint16_t) * indices.size());
            indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            D3D11_SUBRESOURCE_DATA indexData{};
            indexData.pSysMem = indices.data();
            Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
            if (FAILED(native.device->CreateBuffer(&indexBufferDesc, &indexData, indexBuffer.GetAddressOf())))
            {
                return false;
            }

            native.uiVertexBuffer = std::move(vertexBuffer);
            native.uiIndexBuffer = std::move(indexBuffer);
            native.uiQuadCapacity = capacity;
            native.uiBuffersReady = true;
            return true;
        }

        bool CreateNativeDevice(DirectX11Renderer::NativeState& native, const RendererConfig& config)
        {
            native.window = static_cast<HWND>(config.windowHandle);
            constexpr std::array<D3D_FEATURE_LEVEL, 3> levels{
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1
            };

            UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
            flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

            D3D_FEATURE_LEVEL selectedLevel{};
            if (native.window != nullptr)
            {
                DXGI_SWAP_CHAIN_DESC desc{};
                desc.BufferCount = 2;
                desc.BufferDesc.Width = config.width;
                desc.BufferDesc.Height = config.height;
                desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                desc.OutputWindow = native.window;
                desc.SampleDesc.Count = 1;
                desc.Windowed = TRUE;
                desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

                auto hr = D3D11CreateDeviceAndSwapChain(
                    nullptr,
                    D3D_DRIVER_TYPE_HARDWARE,
                    nullptr,
                    flags,
                    levels.data(),
                    static_cast<UINT>(levels.size()),
                    D3D11_SDK_VERSION,
                    &desc,
                    native.swapChain.GetAddressOf(),
                    native.device.GetAddressOf(),
                    &selectedLevel,
                    native.context.GetAddressOf());
#if defined(_DEBUG)
                if (FAILED(hr))
                {
                    flags &= ~D3D11_CREATE_DEVICE_DEBUG;
                    hr = D3D11CreateDeviceAndSwapChain(
                        nullptr,
                        D3D_DRIVER_TYPE_HARDWARE,
                        nullptr,
                        flags,
                        levels.data(),
                        static_cast<UINT>(levels.size()),
                        D3D11_SDK_VERSION,
                        &desc,
                        native.swapChain.GetAddressOf(),
                        native.device.GetAddressOf(),
                        &selectedLevel,
                        native.context.GetAddressOf());
                }
#endif
                if (FAILED(hr))
                {
                    return false;
                }

                if (!CreateRenderTarget(native))
                {
                    return false;
                }
            }
            else
            {
                auto hr = D3D11CreateDevice(
                    nullptr,
                    D3D_DRIVER_TYPE_HARDWARE,
                    nullptr,
                    flags,
                    levels.data(),
                    static_cast<UINT>(levels.size()),
                    D3D11_SDK_VERSION,
                    native.device.GetAddressOf(),
                    &selectedLevel,
                    native.context.GetAddressOf());
#if defined(_DEBUG)
                if (FAILED(hr))
                {
                    flags &= ~D3D11_CREATE_DEVICE_DEBUG;
                    hr = D3D11CreateDevice(
                        nullptr,
                        D3D_DRIVER_TYPE_HARDWARE,
                        nullptr,
                        flags,
                        levels.data(),
                        static_cast<UINT>(levels.size()),
                        D3D11_SDK_VERSION,
                        native.device.GetAddressOf(),
                        &selectedLevel,
                        native.context.GetAddressOf());
                }
#endif
                if (FAILED(hr))
                {
                    return false;
                }
                if (!CreateOffscreenRenderTarget(native, config.width, config.height))
                {
                    return false;
                }
            }

            SetViewport(native, config.width, config.height);
            return true;
        }

        bool CompileShader(const char* entry, const char* profile, Microsoft::WRL::ComPtr<ID3DBlob>& blob)
        {
            Microsoft::WRL::ComPtr<ID3DBlob> errors;
            return SUCCEEDED(D3DCompile(
                kUiShaderSource,
                std::strlen(kUiShaderSource),
                nullptr,
                nullptr,
                nullptr,
                entry,
                profile,
                D3DCOMPILE_ENABLE_STRICTNESS,
                0,
                blob.GetAddressOf(),
                errors.GetAddressOf()));
        }
    }

    bool DirectX11Renderer::Initialize(const RendererConfig config)
    {
        config_ = config;
        initialized_ = config.width > 0 && config.height > 0;
        if (initialized_)
        {
            if (!CreateNativeDevice(*native_, config))
            {
                initialized_ = false;
            }
        }
        return initialized_;
    }

    ResizeResult DirectX11Renderer::ResizeSwapChain(const std::uint32_t width, const std::uint32_t height) noexcept
    {
        ResizeResult result;
        if (!initialized_ || width == 0 || height == 0)
        {
            result.rendererStatePreserved = initialized_;
            return result;
        }

        result.swapChainNeedsResize = config_.width != width || config_.height != height;
        result.layoutNeedsUpdate = result.swapChainNeedsResize;
        config_.width = width;
        config_.height = height;
        if (result.swapChainNeedsResize && native_->swapChain)
        {
            native_->context->OMSetRenderTargets(0, nullptr, nullptr);
            native_->renderTargetView.Reset();
            if (FAILED(native_->swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0)))
            {
                result.rendererStatePreserved = false;
                return result;
            }

            result.rendererStatePreserved = CreateRenderTarget(*native_);
        }
        else if (result.swapChainNeedsResize && HasNativeDevice())
        {
            native_->context->OMSetRenderTargets(0, nullptr, nullptr);
            native_->renderTargetView.Reset();
            result.rendererStatePreserved = CreateOffscreenRenderTarget(*native_, width, height);
        }
        SetViewport(*native_, width, height);
        result.rendererStatePreserved = initialized_ && uiSpritePipelineInitialized_;
        return result;
    }

    void DirectX11Renderer::Shutdown() noexcept
    {
        initialized_ = false;
        uiSpritePipelineInitialized_ = false;
        native_->shaderResources.clear();
        native_->renderTargetView.Reset();
        native_->swapChain.Reset();
        native_->context.Reset();
        native_->device.Reset();
        submittedCommandCount_ = 0;
        uploadedTextureCount_ = 0;
        drawnQuadCount_ = 0;
        frameDrawCallCount_ = 0;
        frameQuadCount_ = 0;
        presentedFrameCount_ = 0;
        uiScissorRect_.reset();
    }

    bool DirectX11Renderer::IsInitialized() const noexcept
    {
        return initialized_;
    }

    bool DirectX11Renderer::HasNativeDevice() const noexcept
    {
        return native_ && native_->device != nullptr && native_->context != nullptr;
    }

    bool DirectX11Renderer::HasSwapChain() const noexcept
    {
        return native_ && native_->swapChain != nullptr;
    }

    bool DirectX11Renderer::InitializeUiSpritePipeline() noexcept
    {
        if (!initialized_ || !HasNativeDevice())
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }

        Microsoft::WRL::ComPtr<ID3DBlob> vertexBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> pixelBlob;
        if (!CompileShader("VSMain", "vs_4_0", vertexBlob) || !CompileShader("PSMain", "ps_4_0", pixelBlob))
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }

        if (FAILED(native_->device->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, native_->uiVertexShader.GetAddressOf())) ||
            FAILED(native_->device->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, native_->uiPixelShader.GetAddressOf())))
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }

        constexpr D3D11_INPUT_ELEMENT_DESC inputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        if (FAILED(native_->device->CreateInputLayout(inputElements, 4, vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), native_->uiInputLayout.GetAddressOf())))
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }

        if (!EnsureUiQuadBuffers(*native_, 256U))
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }

        D3D11_BUFFER_DESC constantBufferDesc{};
        constantBufferDesc.ByteWidth = sizeof(UiConstants);
        constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        if (FAILED(native_->device->CreateBuffer(&constantBufferDesc, nullptr, native_->uiConstantBuffer.GetAddressOf())))
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }

        D3D11_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(native_->device->CreateBlendState(&blendDesc, native_->alphaBlendState.GetAddressOf())))
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        if (FAILED(native_->device->CreateBlendState(&blendDesc, native_->screenBlendState.GetAddressOf())))
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }

        D3D11_SAMPLER_DESC samplerDesc{};
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        if (FAILED(native_->device->CreateSamplerState(&samplerDesc, native_->pointSamplerState.GetAddressOf())))
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        if (FAILED(native_->device->CreateSamplerState(&samplerDesc, native_->linearSamplerState.GetAddressOf())))
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }

        D3D11_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        rasterizerDesc.DepthClipEnable = TRUE;
        rasterizerDesc.ScissorEnable = TRUE;
        if (FAILED(native_->device->CreateRasterizerState(&rasterizerDesc, native_->scissorRasterizerState.GetAddressOf())))
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }
        rasterizerDesc.ScissorEnable = FALSE;
        if (FAILED(native_->device->CreateRasterizerState(&rasterizerDesc, native_->defaultRasterizerState.GetAddressOf())))
        {
            uiSpritePipelineInitialized_ = false;
            return false;
        }

        native_->uiBuffersReady = true;
        uiSpritePipelineInitialized_ = true;
        return uiSpritePipelineInitialized_;
    }

    bool DirectX11Renderer::IsUiSpritePipelineInitialized() const noexcept
    {
        return uiSpritePipelineInitialized_;
    }

    void DirectX11Renderer::ConfigureUiSpritePipeline(const UiSpritePipelineState state) noexcept
    {
        uiSpritePipelineState_ = state;
    }

    UiSpritePipelineState DirectX11Renderer::GetUiSpritePipelineState() const noexcept
    {
        return uiSpritePipelineState_;
    }

    void DirectX11Renderer::SetUiScissorRect(std::optional<UiScissorRect> rect) noexcept
    {
        uiScissorRect_ = std::move(rect);
    }

    void DirectX11Renderer::SetVSync(const bool enabled) noexcept
    {
        config_.vsync = enabled;
    }

    bool DirectX11Renderer::VSyncEnabled() const noexcept
    {
        return config_.vsync;
    }

    graphics::TextureDescriptor DirectX11Renderer::UploadTexture(const graphics::ImageData& image)
    {
        if (!initialized_ || !uiSpritePipelineInitialized_ || image.Empty())
        {
            return {};
        }

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        if (HasNativeDevice())
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = image.width;
            desc.Height = image.height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = image.format == graphics::PixelFormat::Bgra8 ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = image.pixels.data();
            data.SysMemPitch = image.width * 4U;
            if (FAILED(native_->device->CreateTexture2D(&desc, &data, texture.GetAddressOf())) ||
                FAILED(native_->device->CreateShaderResourceView(texture.Get(), nullptr, srv.GetAddressOf())))
            {
                return {};
            }
        }

        ++uploadedTextureCount_;
        native_->shaderResources[uploadedTextureCount_] = srv;
        return graphics::TextureDescriptor{
            graphics::TextureHandle{ uploadedTextureCount_ },
            image.width,
            image.height
        };
    }

    void DirectX11Renderer::ReleaseTexture(const graphics::TextureDescriptor texture) noexcept
    {
        if (!texture.handle.IsValid() || !native_)
        {
            return;
        }
        native_->shaderResources.erase(texture.handle.ValueOf());
    }

    void DirectX11Renderer::ClearFrame(const float red, const float green, const float blue, const float alpha) noexcept
    {
        frameDrawCallCount_ = 0;
        frameQuadCount_ = 0;
        if (!native_ || !native_->context || !native_->renderTargetView)
        {
            return;
        }

        const float color[4] = { red, green, blue, alpha };
        native_->context->ClearRenderTargetView(native_->renderTargetView.Get(), color);
    }

    bool DirectX11Renderer::DrawTexturedQuad(const TexturedQuad& quad) noexcept
    {
        if (!initialized_ || !uiSpritePipelineInitialized_ || !quad.texture.handle.IsValid() || quad.width == 0 || quad.height == 0)
        {
            return false;
        }

        if (HasNativeDevice() && native_->uiBuffersReady)
        {
            const auto srv = native_->shaderResources.find(quad.texture.handle.ValueOf());
            if (srv == native_->shaderResources.end())
            {
                return false;
            }

            if (native_->renderTargetView)
            {
                native_->context->OMSetRenderTargets(1, native_->renderTargetView.GetAddressOf(), nullptr);
            }

            const float x = static_cast<float>(quad.position.x);
            const float y = static_cast<float>(quad.position.y);
            const float w = static_cast<float>(quad.width);
            const float h = static_cast<float>(quad.height);
            const UiVertex vertices[] = {
                { x, y, quad.u0, quad.v0, quad.opacity, quad.colorScale },
                { x + w, y, quad.u1, quad.v0, quad.opacity, quad.colorScale },
                { x, y + h, quad.u0, quad.v1, quad.opacity, quad.colorScale },
                { x + w, y + h, quad.u1, quad.v1, quad.opacity, quad.colorScale }
            };

            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (FAILED(native_->context->Map(native_->uiVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                return false;
            }
            std::memcpy(mapped.pData, vertices, sizeof(vertices));
            native_->context->Unmap(native_->uiVertexBuffer.Get(), 0);

            const UiConstants constants{
                static_cast<float>(config_.width),
                static_cast<float>(config_.height),
                { 0.0F, 0.0F }
            };
            native_->context->UpdateSubresource(native_->uiConstantBuffer.Get(), 0, nullptr, &constants, 0, 0);

            constexpr UINT stride = sizeof(UiVertex);
            constexpr UINT offset = 0;
            native_->context->IASetInputLayout(native_->uiInputLayout.Get());
            native_->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            native_->context->IASetVertexBuffers(0, 1, native_->uiVertexBuffer.GetAddressOf(), &stride, &offset);
            native_->context->IASetIndexBuffer(native_->uiIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
            native_->context->VSSetShader(native_->uiVertexShader.Get(), nullptr, 0);
            native_->context->VSSetConstantBuffers(0, 1, native_->uiConstantBuffer.GetAddressOf());
            native_->context->PSSetShader(native_->uiPixelShader.Get(), nullptr, 0);
            native_->context->PSSetShaderResources(0, 1, srv->second.GetAddressOf());
            ID3D11SamplerState* sampler = uiSpritePipelineState_.linearSamplerEnabled ? native_->linearSamplerState.Get() : native_->pointSamplerState.Get();
            native_->context->PSSetSamplers(0, 1, &sampler);

            if (quad.blendMode == TexturedQuadBlendMode::Screen)
            {
                constexpr float blendFactor[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
                native_->context->OMSetBlendState(native_->screenBlendState.Get(), blendFactor, 0xFFFFFFFFU);
            }
            else if (uiSpritePipelineState_.alphaBlendEnabled)
            {
                constexpr float blendFactor[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
                native_->context->OMSetBlendState(native_->alphaBlendState.Get(), blendFactor, 0xFFFFFFFFU);
            }
            else
            {
                native_->context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFU);
            }
            native_->context->RSSetState(uiSpritePipelineState_.scissorEnabled ? native_->scissorRasterizerState.Get() : native_->defaultRasterizerState.Get());
            if (uiSpritePipelineState_.scissorEnabled)
            {
                const auto& requested = uiScissorRect_;
                D3D11_RECT scissor{ requested ? std::max<LONG>(0, requested->x) : 0,
                    requested ? std::max<LONG>(0, requested->y) : 0,
                    requested ? std::min<LONG>(static_cast<LONG>(config_.width), requested->x + requested->width) : static_cast<LONG>(config_.width),
                    requested ? std::min<LONG>(static_cast<LONG>(config_.height), requested->y + requested->height) : static_cast<LONG>(config_.height) };
                native_->context->RSSetScissorRects(1, &scissor);
            }

            native_->context->DrawIndexed(6, 0, 0);
        }

        ++drawnQuadCount_;
        ++frameDrawCallCount_;
        ++frameQuadCount_;
        return true;
    }

    std::uint32_t DirectX11Renderer::DrawTexturedQuads(const std::vector<TexturedQuad>& quads) noexcept
    {
        if (!initialized_ || !uiSpritePipelineInitialized_ || quads.empty())
        {
            return 0;
        }

        if (!HasNativeDevice() || !native_->uiBuffersReady)
        {
            std::uint32_t drawn = 0;
            for (const auto& quad : quads)
            {
                if (quad.texture.handle.IsValid() && quad.width != 0 && quad.height != 0)
                {
                    ++drawn;
                }
            }
            drawnQuadCount_ += drawn;
            frameDrawCallCount_ += drawn;
            frameQuadCount_ += drawn;
            return drawn;
        }

        auto& drawable = native_->drawableScratch;
        drawable.clear();
        if (drawable.capacity() < quads.size()) drawable.reserve(quads.size());
        for (const auto& quad : quads)
        {
            if (!quad.texture.handle.IsValid() || quad.width == 0 || quad.height == 0)
            {
                continue;
            }
            if (native_->shaderResources.find(quad.texture.handle.ValueOf()) != native_->shaderResources.end())
            {
                drawable.push_back(&quad);
            }
        }

        constexpr std::size_t kMaxIndexedQuads = std::numeric_limits<std::uint16_t>::max() / 4U;
        std::uint32_t drawn = 0;
        std::size_t firstQuad = 0;
        while (firstQuad < drawable.size())
        {
            const auto chunkQuadCount = std::min(kMaxIndexedQuads, drawable.size() - firstQuad);
            if (!EnsureUiQuadBuffers(*native_, static_cast<std::uint32_t>(chunkQuadCount)))
            {
                break;
            }

            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (FAILED(native_->context->Map(native_->uiVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                break;
            }

            auto* vertices = static_cast<UiVertex*>(mapped.pData);
            for (std::size_t chunkIndex = 0; chunkIndex < chunkQuadCount; ++chunkIndex)
            {
                const auto& quad = *drawable[firstQuad + chunkIndex];
                const auto vertexIndex = chunkIndex * 4U;
                const float x = static_cast<float>(quad.position.x);
                const float y = static_cast<float>(quad.position.y);
                const float w = static_cast<float>(quad.width);
                const float h = static_cast<float>(quad.height);
                vertices[vertexIndex + 0U] = { x, y, quad.u0, quad.v0, quad.opacity, quad.colorScale };
                vertices[vertexIndex + 1U] = { x + w, y, quad.u1, quad.v0, quad.opacity, quad.colorScale };
                vertices[vertexIndex + 2U] = { x, y + h, quad.u0, quad.v1, quad.opacity, quad.colorScale };
                vertices[vertexIndex + 3U] = { x + w, y + h, quad.u1, quad.v1, quad.opacity, quad.colorScale };
            }
            native_->context->Unmap(native_->uiVertexBuffer.Get(), 0);

            if (native_->renderTargetView)
            {
                native_->context->OMSetRenderTargets(1, native_->renderTargetView.GetAddressOf(), nullptr);
            }

            const UiConstants constants{
                static_cast<float>(config_.width),
                static_cast<float>(config_.height),
                { 0.0F, 0.0F }
            };
            native_->context->UpdateSubresource(native_->uiConstantBuffer.Get(), 0, nullptr, &constants, 0, 0);

            constexpr UINT stride = sizeof(UiVertex);
            constexpr UINT offset = 0;
            native_->context->IASetInputLayout(native_->uiInputLayout.Get());
            native_->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            native_->context->IASetVertexBuffers(0, 1, native_->uiVertexBuffer.GetAddressOf(), &stride, &offset);
            native_->context->IASetIndexBuffer(native_->uiIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
            native_->context->VSSetShader(native_->uiVertexShader.Get(), nullptr, 0);
            native_->context->VSSetConstantBuffers(0, 1, native_->uiConstantBuffer.GetAddressOf());
            native_->context->PSSetShader(native_->uiPixelShader.Get(), nullptr, 0);
            ID3D11SamplerState* sampler = uiSpritePipelineState_.linearSamplerEnabled
                ? native_->linearSamplerState.Get()
                : native_->pointSamplerState.Get();
            native_->context->PSSetSamplers(0, 1, &sampler);

            native_->context->RSSetState(uiSpritePipelineState_.scissorEnabled
                ? native_->scissorRasterizerState.Get()
                : native_->defaultRasterizerState.Get());
            if (uiSpritePipelineState_.scissorEnabled)
            {
                const auto& requested = uiScissorRect_;
                D3D11_RECT scissor{ requested ? std::max<LONG>(0, requested->x) : 0,
                    requested ? std::max<LONG>(0, requested->y) : 0,
                    requested ? std::min<LONG>(static_cast<LONG>(config_.width), requested->x + requested->width) : static_cast<LONG>(config_.width),
                    requested ? std::min<LONG>(static_cast<LONG>(config_.height), requested->y + requested->height) : static_cast<LONG>(config_.height) };
                native_->context->RSSetScissorRects(1, &scissor);
            }

            std::size_t batchBegin = 0;
            while (batchBegin < chunkQuadCount)
            {
                const auto textureHandle = drawable[firstQuad + batchBegin]->texture.handle.ValueOf();
                const auto blendMode = drawable[firstQuad + batchBegin]->blendMode;
                std::size_t batchEnd = batchBegin + 1U;
                while (batchEnd < chunkQuadCount &&
                       drawable[firstQuad + batchEnd]->texture.handle.ValueOf() == textureHandle &&
                       drawable[firstQuad + batchEnd]->blendMode == blendMode)
                {
                    ++batchEnd;
                }

                const auto srv = native_->shaderResources.find(textureHandle);
                if (srv != native_->shaderResources.end())
                {
                    constexpr float blendFactor[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
                    if (blendMode == TexturedQuadBlendMode::Screen)
                        native_->context->OMSetBlendState(native_->screenBlendState.Get(), blendFactor, 0xFFFFFFFFU);
                    else if (uiSpritePipelineState_.alphaBlendEnabled)
                        native_->context->OMSetBlendState(native_->alphaBlendState.Get(), blendFactor, 0xFFFFFFFFU);
                    else
                        native_->context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFU);
                    native_->context->PSSetShaderResources(0, 1, srv->second.GetAddressOf());
                    const auto batchQuadCount = static_cast<UINT>(batchEnd - batchBegin);
                    native_->context->DrawIndexed(batchQuadCount * 6U, static_cast<UINT>(batchBegin * 6U), 0);
                    drawn += batchQuadCount;
                    ++frameDrawCallCount_;
                }
                batchBegin = batchEnd;
            }

            firstQuad += chunkQuadCount;
        }

        drawnQuadCount_ += drawn;
        frameQuadCount_ += drawn;
        return drawn;
    }

    PresentResult DirectX11Renderer::PresentFrame() noexcept
    {
        if (!initialized_ || drawnQuadCount_ == 0)
        {
            return {};
        }

        if (native_->swapChain)
        {
            if (FAILED(native_->swapChain->Present(config_.vsync ? 1U : 0U, 0)))
            {
                return {};
            }
        }

        ++presentedFrameCount_;
        return PresentResult{ true, presentedFrameCount_ };
    }

    void DirectX11Renderer::Submit(const std::vector<render::RenderCommand>& commands)
    {
        if (initialized_)
        {
            submittedCommandCount_ = static_cast<std::uint32_t>(commands.size());
        }
    }

    std::uint32_t DirectX11Renderer::SubmittedCommandCount() const noexcept
    {
        return submittedCommandCount_;
    }

    std::uint32_t DirectX11Renderer::UploadedTextureCount() const noexcept
    {
        return uploadedTextureCount_;
    }

    std::uint32_t DirectX11Renderer::DrawnQuadCount() const noexcept
    {
        return drawnQuadCount_;
    }

    std::uint32_t DirectX11Renderer::FrameDrawCallCount() const noexcept
    {
        return frameDrawCallCount_;
    }

    std::uint32_t DirectX11Renderer::FrameQuadCount() const noexcept
    {
        return frameQuadCount_;
    }

    std::uint32_t DirectX11Renderer::PresentedFrameCount() const noexcept
    {
        return presentedFrameCount_;
    }
}
