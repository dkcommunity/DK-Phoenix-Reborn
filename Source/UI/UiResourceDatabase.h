#pragma once

#include "UI/UiTypes.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace phoenix::ui
{
    class UiResourceDatabase final
    {
    public:
        void Add(UiResourceRef resource)
        {
            resources_.push_back(std::move(resource));
        }

        [[nodiscard]] std::optional<UiResourceRef> Find(const std::wstring& logicalName) const
        {
            for (const auto& resource : resources_)
            {
                if (resource.logicalName == logicalName)
                {
                    return resource;
                }
            }

            return std::nullopt;
        }

    private:
        std::vector<UiResourceRef> resources_;
    };
}
