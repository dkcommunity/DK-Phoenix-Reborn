#include "Validation/ValidationReport.h"

#include <algorithm>
#include <utility>

namespace phoenix::validation
{
    void ValidationReport::AddIssue(ValidationIssue issue)
    {
        issues_.push_back(std::move(issue));
    }

    const std::vector<ValidationIssue>& ValidationReport::Issues() const noexcept
    {
        return issues_;
    }

    bool ValidationReport::HasErrors() const noexcept
    {
        return ErrorCount() > 0;
    }

    std::size_t ValidationReport::ErrorCount() const noexcept
    {
        return static_cast<std::size_t>(std::count_if(
            issues_.begin(),
            issues_.end(),
            [](const ValidationIssue& issue)
            {
                return issue.severity == ValidationSeverity::Error;
            }));
    }

    std::size_t ValidationReport::WarningCount() const noexcept
    {
        return static_cast<std::size_t>(std::count_if(
            issues_.begin(),
            issues_.end(),
            [](const ValidationIssue& issue)
            {
                return issue.severity == ValidationSeverity::Warning;
            }));
    }

    ValidationSeverity ToValidationSeverity(assets::LegacyReadSeverity severity) noexcept
    {
        switch (severity)
        {
        case assets::LegacyReadSeverity::Info:
            return ValidationSeverity::Info;
        case assets::LegacyReadSeverity::Warning:
            return ValidationSeverity::Warning;
        case assets::LegacyReadSeverity::Error:
        default:
            return ValidationSeverity::Error;
        }
    }
}
