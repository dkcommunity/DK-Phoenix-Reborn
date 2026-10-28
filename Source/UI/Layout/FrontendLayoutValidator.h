#pragma once

#include "UI/Layout/FrontendLayoutCatalog.h"

#include <cstdint>

namespace phoenix::ui
{
    struct FrontendLayoutValidation
    {
        bool valid{ false };
        std::uint32_t elementCount{ 0 };
        std::uint32_t invalidRectCount{ 0 };
        std::uint32_t outOfBoundsCount{ 0 };
    };

    class FrontendLayoutValidator final
    {
    public:
        [[nodiscard]] static FrontendLayoutValidation Validate(const FrontendLayout& layout) noexcept;
    };
}
