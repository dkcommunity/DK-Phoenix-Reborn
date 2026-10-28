#pragma once

#include "Backends/DirectX11/DirectX11Renderer.h"
#include "Content/ContentDatabase.h"
#include "Render/RenderQueue.h"

namespace phoenix::prototype
{
    struct PrototypeRunResult
    {
        bool contentAvailable{ false };
        bool rendererInitialized{ false };
        std::uint32_t submittedCommands{ 0 };
    };

    class ClientPrototype final
    {
    public:
        [[nodiscard]] PrototypeRunResult RunOneCreature(
            const content::IContentDatabase& content,
            core::BodyId bodyId,
            core::ActionId actionId,
            core::DirectionId directionId);
    };
}
