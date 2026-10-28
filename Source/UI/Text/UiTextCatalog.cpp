#include "UI/Text/UiTextCatalog.h"

#include <fstream>
#include <iterator>

namespace phoenix::ui::text
{
    bool UiTextCatalog::LoadDirectory(const std::filesystem::path& directory)
    {
        entries_.clear();

        std::error_code ec;
        if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec))
        {
            return false;
        }

        for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
        {
            if (ec)
            {
                break;
            }

            if (!entry.is_regular_file(ec) || entry.path().extension() != L".txt")
            {
                continue;
            }

            std::ifstream file(entry.path(), std::ios::binary);
            if (!file)
            {
                continue;
            }

            const std::string text{
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            };

            entries_[entry.path().stem().string()] = text;
        }

        return !entries_.empty();
    }

    std::optional<std::string> UiTextCatalog::Find(const std::string_view key) const
    {
        const auto it = entries_.find(std::string(key));
        if (it == entries_.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    std::size_t UiTextCatalog::Count() const noexcept
    {
        return entries_.size();
    }
}
