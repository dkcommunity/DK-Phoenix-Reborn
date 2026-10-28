#include "Prototype/LoginTitlePrototype.h"

#include "Assets/LegacyAssetTypes.h"
#include "Assets/SpriteArchiveReader.h"
#include "Backends/DirectX11/DirectX11Renderer.h"
#include "Render/RenderQueue.h"
#include "UI/Login/LoginInputModel.h"
#include "UI/Layout/LoginTitleLayout.h"
#include "UI/Layout/FrontendLayoutCatalog.h"
#include "UI/Layout/FrontendLayoutValidator.h"
#include "UI/Text/LoginTextLayout.h"

#include <algorithm>
#include <unordered_map>

namespace phoenix::prototype
{
    LoginTitlePrototypeResult LoginTitlePrototype::Run(const DataBackedPrototypePlan& plan, const std::uint16_t maxSprites) const
    {
        return RunForViewport(plan, maxSprites, 1920U, 1080U);
    }

    LoginTitlePrototypeResult LoginTitlePrototype::RunForViewport(
        const DataBackedPrototypePlan& plan,
        const std::uint16_t maxSprites,
        const std::uint32_t width,
        const std::uint32_t height) const
    {
        LoginTitlePrototypeResult result;

        assets::SpriteArchiveReader reader;
        auto atlas = reader.InspectSpriteAtlas(
            plan.loginSpritePackPath,
            plan.loginSpriteIndexPath,
            assets::LegacyAssetFormat::SpritePack);
        if (!atlas.value)
        {
            return result;
        }

        result.indexedSpriteCount = atlas.value->spriteCount;

        backends::directx11::DirectX11Renderer renderer;
        if (!renderer.Initialize(backends::directx11::RendererConfig{ width, height, false }))
        {
            return result;
        }
        if (!renderer.InitializeUiSpritePipeline())
        {
            return result;
        }
        renderer.ConfigureUiSpritePipeline(backends::directx11::UiSpritePipelineState{ true, true, true });

        const auto layout = ui::LoginTitleLayoutCatalog::ScaleToViewport(
            ui::LoginTitleLayoutCatalog::Create(ui::LoginTitleResolution::R1280x960),
            ui::ScreenViewport{ width, height });
        result.layoutValid = ui::FrontendLayoutValidator::Validate(
            ui::FrontendLayoutCatalog::Create(
                ui::FrontendLayoutScreen::Login,
                ui::FrontendLayoutCatalog::SelectResolution(width, height))).valid;
        auto titleMenuAtlas = reader.InspectSpriteAtlas(
            plan.titleMenuSpritePackPath,
            plan.titleMenuSpriteIndexPath,
            assets::LegacyAssetFormat::SpritePack);
        if (titleMenuAtlas.value)
        {
            result.titleMenuSpriteCount = titleMenuAtlas.value->spriteCount;
        }

        std::unordered_map<std::uint32_t, graphics::TextureDescriptor> renewalTextures;
        std::unordered_map<std::uint32_t, graphics::TextureDescriptor> titleMenuTextures;
        render::RenderQueue queue;
        const auto spriteLimit = std::min<std::uint16_t>(maxSprites, atlas.value->spriteCount);
        for (std::uint16_t spriteIndex = 0; spriteIndex < spriteLimit; ++spriteIndex)
        {
            const auto& metadata = atlas.value->sprites[spriteIndex];
            LoginTitleSpriteSummary summary;
            summary.spriteIndex = static_cast<std::uint16_t>(metadata.spriteIndex);
            summary.width = metadata.width;
            summary.height = metadata.height;
            summary.byteSize = metadata.byteSize;

            auto decoded = reader.ReadSprite565(
                plan.loginSpritePackPath,
                plan.loginSpriteIndexPath,
                spriteIndex,
                assets::LegacyAssetFormat::SpritePack);
            if (decoded.value && !decoded.value->image.Empty())
            {
                const auto texture = renderer.UploadTexture(decoded.value->image);
                if (texture.handle.IsValid())
                {
                    renewalTextures.emplace(spriteIndex, texture);
                    summary.decoded = true;
                    ++result.decodedSpriteCount;
                }
            }

            result.sprites.push_back(summary);
        }
        if (titleMenuAtlas.value)
        {
            for (std::uint16_t spriteIndex = 0; spriteIndex < titleMenuAtlas.value->spriteCount; ++spriteIndex)
            {
                auto decoded = reader.ReadSprite565(
                    plan.titleMenuSpritePackPath,
                    plan.titleMenuSpriteIndexPath,
                    spriteIndex,
                    assets::LegacyAssetFormat::SpritePack);
                if (decoded.value && !decoded.value->image.Empty())
                {
                    const auto texture = renderer.UploadTexture(decoded.value->image);
                    if (texture.handle.IsValid())
                    {
                        titleMenuTextures.emplace(spriteIndex, texture);
                        ++result.titleMenuDecodedSpriteCount;
                    }
                }
            }
        }

        std::uint32_t sequence = 0;
        for (const auto& binding : layout.sprites)
        {
            queue.Submit(render::RenderCommand{
                render::RenderSortKey{ render::RenderLayer::Ui, 0, binding.rect.y, sequence++ },
                binding.spriteId,
                render::ScreenPoint{ binding.rect.x, binding.rect.y },
                render::ScreenPoint{ 0, 0 }
            });

            const auto spriteIndex = static_cast<std::uint32_t>(binding.spriteId.ValueOf());
            const auto& textureMap = binding.source == ui::LoginTitleSpriteSource::TitleMenuDefault
                ? titleMenuTextures
                : renewalTextures;
            const auto texture = textureMap.find(spriteIndex);
            if (texture != textureMap.end())
            {
                if (renderer.DrawTexturedQuad(backends::directx11::TexturedQuad{
                    texture->second,
                    render::ScreenPoint{ binding.rect.x, binding.rect.y },
                    static_cast<std::uint32_t>(std::max(0, binding.rect.width)),
                    static_cast<std::uint32_t>(std::max(0, binding.rect.height)) }))
                {
                    result.visibleBackgroundSubmitted = result.visibleBackgroundSubmitted || binding.role == ui::LoginTitleSpriteRole::Background;
                    result.visibleButtonSubmitted = result.visibleButtonSubmitted ||
                        binding.role == ui::LoginTitleSpriteRole::LoginButton ||
                        binding.role == ui::LoginTitleSpriteRole::NewAccountButton ||
                        binding.role == ui::LoginTitleSpriteRole::OptionButton ||
                        binding.role == ui::LoginTitleSpriteRole::CreditsButton ||
                        binding.role == ui::LoginTitleSpriteRole::ExitButton;
                }
            }
        }

        ui::LoginInputModel input;
        input.SetLayout(layout);
        input.AddChar('u');
        input.TabFocus();
        input.AddChar('p');
        result.textRunCount = static_cast<std::uint32_t>(ui::text::LoginTextLayout::Build(input).size());

        queue.Sort();
        renderer.Submit(queue.Commands());
        result.uploadedTextureCount = renderer.UploadedTextureCount();
        result.submittedCommandCount = renderer.SubmittedCommandCount();
        result.drawnQuadCount = renderer.DrawnQuadCount();
        const auto present = renderer.PresentFrame();
        result.framePresented = present.presented;
        result.presentedFrameCount = renderer.PresentedFrameCount();
        renderer.Shutdown();

        return result;
    }
}
