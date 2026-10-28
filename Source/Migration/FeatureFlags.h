#pragma once

#include <set>

namespace phoenix::migration
{
    enum class ClientFeature
    {
        LegacyReader,
        ContentDatabase,
        RenderQueue,
        SpriteDecode,
        AudioBackend,
        LoginFrontend,
        UiFramework,
        MediaSystem,
        WorldRendering,
        PlayerVisuals,
        SkillEffects,
        ItemVisuals
    };

    class FeatureFlagSet final
    {
    public:
        void Enable(ClientFeature feature);
        void Disable(ClientFeature feature);
        [[nodiscard]] bool IsEnabled(ClientFeature feature) const;

    private:
        std::set<ClientFeature> enabled_;
    };
}
