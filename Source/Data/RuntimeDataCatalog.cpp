#include "Data/RuntimeDataCatalog.h"

#include <algorithm>
#include <cwctype>
#include <system_error>

namespace phoenix::data
{
    namespace
    {
        std::wstring NormalizeExtension(std::filesystem::path path)
        {
            auto extension = path.extension().wstring();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](const wchar_t value) {
                return static_cast<wchar_t>(::towlower(value));
            });
            return extension;
        }
    }

    bool RuntimeDataInventory::HasExtension(const std::wstring& extension) const
    {
        return std::any_of(extensions.begin(), extensions.end(), [&](const DataExtensionSummary& summary) {
            return summary.extension == extension && summary.fileCount > 0;
        });
    }

    RuntimeDataInventory RuntimeDataCatalog::Scan(const std::filesystem::path& root) const
    {
        RuntimeDataInventory inventory;
        inventory.root = root;

        std::error_code ec;
        if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec))
        {
            return inventory;
        }

        std::map<std::wstring, DataFolderSummary> folders;
        std::map<std::wstring, DataExtensionSummary> extensions;

        for (const auto& entry : std::filesystem::directory_iterator(root, ec))
        {
            if (!entry.is_directory(ec))
            {
                continue;
            }

            const auto folderName = entry.path().filename().wstring();
            auto& folder = folders[folderName];
            folder.name = folderName;

            std::filesystem::recursive_directory_iterator it(entry.path(), std::filesystem::directory_options::skip_permission_denied, ec);
            const std::filesystem::recursive_directory_iterator end;
            while (!ec && it != end)
            {
                if (it->is_regular_file(ec))
                {
                    auto size = it->file_size(ec);
                    if (ec)
                    {
                        size = 0;
                        ec.clear();
                    }

                    folder.fileCount += 1;
                    folder.totalBytes += size;

                    const auto extensionName = NormalizeExtension(it->path());
                    auto& extension = extensions[extensionName];
                    extension.extension = extensionName;
                    extension.fileCount += 1;
                    extension.totalBytes += size;
                }

                it.increment(ec);
            }
        }

        for (const auto& [_, folder] : folders)
        {
            inventory.folders.push_back(folder);
        }

        for (const auto& [_, extension] : extensions)
        {
            inventory.extensions.push_back(extension);
        }

        return inventory;
    }
}
