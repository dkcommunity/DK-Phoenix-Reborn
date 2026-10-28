#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace phoenix::assets
{
    struct DiscoveredAsset
    {
        std::filesystem::path path;
        std::wstring extension;
        std::uint64_t sizeBytes{ 0 };
    };

    struct RuntimeAssetRootReport
    {
        std::vector<std::filesystem::path> scannedRoots;
        std::vector<DiscoveredAsset> files;
        std::map<std::wstring, std::uint32_t> countsByExtension;
        std::vector<std::wstring> missingRequiredExtensions;

        [[nodiscard]] bool HasRequiredVisualAssets() const noexcept;
    };

    class RuntimeAssetDiscovery final
    {
    public:
        [[nodiscard]] RuntimeAssetRootReport Scan(const std::vector<std::filesystem::path>& candidateRoots) const;

    private:
        static std::wstring NormalizeExtension(std::filesystem::path path);
    };
}
