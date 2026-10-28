#include "UI/Layout/FrontendLayoutValidator.h"

namespace phoenix::ui
{
    FrontendLayoutValidation FrontendLayoutValidator::Validate(const FrontendLayout& layout) noexcept
    {
        FrontendLayoutValidation result;
        result.elementCount = static_cast<std::uint32_t>(layout.elements.size());
        for (const auto& element : layout.elements)
        {
            if (element.rect.width <= 0 || element.rect.height <= 0)
            {
                ++result.invalidRectCount;
                continue;
            }

            if (element.rect.x < 0 ||
                element.rect.y < 0 ||
                element.rect.x + element.rect.width > static_cast<std::int32_t>(layout.width) ||
                element.rect.y + element.rect.height > static_cast<std::int32_t>(layout.height))
            {
                ++result.outOfBoundsCount;
            }
        }

        result.valid = result.elementCount > 0 && result.invalidRectCount == 0 && result.outOfBoundsCount == 0;
        return result;
    }
}
