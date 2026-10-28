#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace phoenix::ui::text
{
    class UiTextCatalog final
    {
    public:
        [[nodiscard]] bool LoadDirectory(const std::filesystem::path& directory);
        [[nodiscard]] std::optional<std::string> Find(std::string_view key) const;
        [[nodiscard]] std::size_t Count() const noexcept;

    private:
        std::map<std::string, std::string> entries_;
    };
}
