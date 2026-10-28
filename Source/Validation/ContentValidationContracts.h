#pragma once

#include "Validation/ValidationReport.h"

#include <filesystem>

namespace phoenix::validation
{
    struct ValidationContext
    {
        std::filesystem::path assetRoot;
        std::filesystem::path generatedReportRoot;
    };

    class IContentValidator
    {
    public:
        virtual ~IContentValidator() = default;

        virtual void Validate(const ValidationContext& context, ValidationReport& report) const = 0;
    };
}
