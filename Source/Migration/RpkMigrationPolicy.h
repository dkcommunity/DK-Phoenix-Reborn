#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::migration
{
    struct RpkPasswordCandidate
    {
        std::string value;
        std::string note;
    };

    struct RpkMigrationTarget
    {
        std::filesystem::path sourceRpk;
        std::filesystem::path extractedDirectory;
        std::vector<std::string> expectedFiles;
    };

    class RpkMigrationPolicy final
    {
    public:
        [[nodiscard]] static std::vector<RpkPasswordCandidate> KnownPasswords();
        [[nodiscard]] static RpkMigrationTarget TitleTextTarget(const std::filesystem::path& gameDataRoot);
        [[nodiscard]] static bool ShouldPreferExtractedText(const std::filesystem::path& extractedDirectory);
    };
}
