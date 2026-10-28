#pragma once

#include "Content/FileDefCatalog.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace phoenix::content
{
    enum class InfSchemaFormat
    {
        BinaryTable,
        TextKeyValue,
        SpriteHeader,
        UpdaterOnly,
        LegacyOnly
    };

    enum class InfSchemaRequirement
    {
        Required,
        Optional,
        UpdaterOnly,
        SpriteHeader
    };

    struct InfSchemaEntry
    {
        std::wstring logicalName;
        std::wstring fileDefKey;
        std::wstring fallbackRelativePath;
        InfSchemaFormat format{ InfSchemaFormat::BinaryTable };
        InfSchemaRequirement requirement{ InfSchemaRequirement::Required };
        std::string legacyParser;
        std::string runtimeConsumer;
    };

    struct InfSchemaResolvedEntry
    {
        InfSchemaEntry schema;
        std::filesystem::path path;
        bool exists{ false };
    };

    struct InfSchemaRegistryResult
    {
        std::vector<InfSchemaResolvedEntry> entries;
        std::size_t requiredMissing{ 0 };
        std::size_t updaterOnlyCount{ 0 };
        std::wstring error;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return error.empty() && requiredMissing == 0;
        }
    };

    class InfSchemaRegistry final
    {
    public:
        [[nodiscard]] static std::vector<InfSchemaEntry> BuildDefault();

        [[nodiscard]] InfSchemaRegistryResult Resolve(
            const std::filesystem::path& dataRoot,
            const FileDefCatalogResult* fileDefCatalog) const;
    };
}
