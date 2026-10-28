#include "Migration/FeatureFlags.h"

namespace phoenix::migration
{
    void FeatureFlagSet::Enable(const ClientFeature feature)
    {
        enabled_.insert(feature);
    }

    void FeatureFlagSet::Disable(const ClientFeature feature)
    {
        enabled_.erase(feature);
    }

    bool FeatureFlagSet::IsEnabled(const ClientFeature feature) const
    {
        return enabled_.find(feature) != enabled_.end();
    }
}
