#pragma once

#include <filesystem>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace phoenix::ui::text
{
    struct FontFileRecord
    {
        std::filesystem::path path;
        std::string sourceUrl;
        std::string sha256;
        std::uint64_t sizeBytes{ 0 };
    };

    struct FontFamilyRecord
    {
        std::string id;
        std::string displayName;
        std::string role;
        bool systemFont{ false };
        std::vector<FontFileRecord> localFiles;
    };

    class FontAssetManifest final
    {
    public:
        bool Load(const std::filesystem::path& manifestPath);

        [[nodiscard]] bool IsLoaded() const noexcept;
        [[nodiscard]] const std::filesystem::path& SourcePath() const noexcept;
        [[nodiscard]] const std::vector<FontFamilyRecord>& Families() const noexcept;
        [[nodiscard]] std::optional<FontFamilyRecord> FindFamily(std::string_view familyId) const;

    private:
        std::filesystem::path sourcePath_;
        bool loaded_{ false };
        std::vector<FontFamilyRecord> families_;
    };
}
