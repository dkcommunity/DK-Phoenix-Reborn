#include "Assets/AssetDiscovery.h"

#include <algorithm>
#include <array>
#include <cwctype>
#include <system_error>

namespace phoenix::assets
{
    namespace
    {
        constexpr std::array RequiredVisualExtensions{
            L".inf",
            L".spk",
            L".ispk",
            L".sspk",
            L".cfpk"
        };
    }

    bool RuntimeAssetRootReport::HasRequiredVisualAssets() const noexcept
    {
        return missingRequiredExtensions.empty();
    }

    RuntimeAssetRootReport RuntimeAssetDiscovery::Scan(const std::vector<std::filesystem::path>& candidateRoots) const
    {
        RuntimeAssetRootReport report;

        for (const auto& root : candidateRoots)
        {
            report.scannedRoots.push_back(root);

            std::error_code ec;
            if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec))
            {
                continue;
            }

            std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied, ec);
            const std::filesystem::recursive_directory_iterator end;

            while (!ec && it != end)
            {
                const auto path = it->path();
                if (it->is_regular_file(ec))
                {
                    auto extension = NormalizeExtension(path);
                    auto size = std::filesystem::file_size(path, ec);
                    if (ec)
                    {
                        size = 0;
                        ec.clear();
                    }

                    report.countsByExtension[extension] += 1;
                    report.files.push_back(DiscoveredAsset{ path, std::move(extension), size });
                }

                it.increment(ec);
            }
        }

        for (const auto* extension : RequiredVisualExtensions)
        {
            if (report.countsByExtension.find(extension) == report.countsByExtension.end())
            {
                report.missingRequiredExtensions.push_back(extension);
            }
        }

        return report;
    }

    std::wstring RuntimeAssetDiscovery::NormalizeExtension(std::filesystem::path path)
    {
        auto extension = path.extension().wstring();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](const wchar_t value) {
            return static_cast<wchar_t>(::towlower(value));
        });
        return extension;
    }
}
