#pragma once

#include "Assets/LegacyAssetTypes.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::validation
{
    enum class ValidationSeverity
    {
        Info,
        Warning,
        Error
    };

    enum class ValidationCategory
    {
        AssetRoot,
        CreatureChain,
        ArchiveIndex,
        FramePack,
        Shadow,
        SkillEffect,
        ItemVisual,
        AudioReference,
        UiMedia
    };

    struct ValidationIssue
    {
        ValidationSeverity severity{ ValidationSeverity::Error };
        ValidationCategory category{ ValidationCategory::AssetRoot };
        std::filesystem::path sourcePath;
        std::uint64_t sourceOffset{ 0 };
        std::wstring subject;
        std::wstring message;
    };

    class ValidationReport final
    {
    public:
        void AddIssue(ValidationIssue issue);

        [[nodiscard]] const std::vector<ValidationIssue>& Issues() const noexcept;
        [[nodiscard]] bool HasErrors() const noexcept;
        [[nodiscard]] std::size_t ErrorCount() const noexcept;
        [[nodiscard]] std::size_t WarningCount() const noexcept;

    private:
        std::vector<ValidationIssue> issues_;
    };

    [[nodiscard]] ValidationSeverity ToValidationSeverity(assets::LegacyReadSeverity severity) noexcept;
}
