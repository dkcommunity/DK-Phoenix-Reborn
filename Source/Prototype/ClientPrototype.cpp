#include "Prototype/ClientPrototype.h"

namespace phoenix::prototype
{
    PrototypeRunResult ClientPrototype::RunOneCreature(
        const content::IContentDatabase& content,
        const core::BodyId bodyId,
        const core::ActionId actionId,
        const core::DirectionId directionId)
    {
        PrototypeRunResult result;
        const auto animation = content.FindAnimation(bodyId, actionId, directionId);
        result.contentAvailable = animation.has_value();

        backends::directx11::DirectX11Renderer renderer;
        result.rendererInitialized = renderer.Initialize(backends::directx11::RendererConfig{});

        render::RenderQueue queue;
        if (animation)
        {
            std::uint32_t sequence = 0;
            for (const auto& frame : animation->frames)
            {
                queue.Submit(render::RenderCommand{
                    render::RenderSortKey{ render::RenderLayer::Creature, 0, frame.originY, sequence++ },
                    frame.spriteId,
                    render::ScreenPoint{ 0, 0 },
                    render::ScreenPoint{ frame.originX, frame.originY }
                });
            }
        }

        queue.Sort();
        renderer.Submit(queue.Commands());
        result.submittedCommands = renderer.SubmittedCommandCount();
        return result;
    }
}
