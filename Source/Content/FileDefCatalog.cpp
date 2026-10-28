#include "Content/FileDefCatalog.h"

#include <algorithm>
#include <cwctype>
#include <fstream>
#include <sstream>

namespace phoenix::content
{
    namespace
    {
        [[nodiscard]] std::wstring Trim(std::wstring value)
        {
            const auto notSpace = [](const wchar_t ch) {
                return !std::iswspace(ch);
            };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
            value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
            return value;
        }

        [[nodiscard]] std::wstring NormalizeKey(std::wstring key)
        {
            key = Trim(std::move(key));
            std::transform(key.begin(), key.end(), key.begin(), [](const wchar_t ch) {
                return static_cast<wchar_t>(std::towupper(ch));
            });
            return key;
        }

        [[nodiscard]] std::wstring NormalizePathText(std::wstring text)
        {
            text = Trim(std::move(text));
            for (auto& ch : text)
            {
                if (ch == L'/')
                {
                    ch = L'\\';
                }
            }

            std::wstring collapsed;
            collapsed.reserve(text.size());
            bool previousSlash = false;
            for (const auto ch : text)
            {
                const auto slash = ch == L'\\';
                if (slash && previousSlash)
                {
                    continue;
                }
                collapsed.push_back(ch);
                previousSlash = slash;
            }
            return collapsed;
        }

        [[nodiscard]] std::filesystem::path ResolveRelativeToDataRoot(
            const std::filesystem::path& dataRoot,
            std::wstring text)
        {
            text = NormalizePathText(std::move(text));
            std::filesystem::path path(text);
            if (path.is_absolute())
            {
                return path.lexically_normal();
            }

            auto wide = path.wstring();
            if (wide.rfind(L"Data\\", 0) == 0 || wide.rfind(L"data\\", 0) == 0)
            {
                wide = wide.substr(5);
                path = std::filesystem::path(wide);
            }
            return (dataRoot / path).lexically_normal();
        }
    }

    std::optional<std::filesystem::path> FileDefCatalogResult::Find(const std::wstring_view key) const
    {
        const auto normalized = NormalizeKey(std::wstring(key));
        const auto it = entries.find(normalized);
        if (it == entries.end())
        {
            return std::nullopt;
        }
        return it->second;
    }

    std::filesystem::path FileDefCatalogResult::Resolve(
        const std::wstring_view key,
        const std::wstring_view fallbackRelativePath) const
    {
        if (const auto resolved = Find(key))
        {
            return *resolved;
        }
        return ResolveRelativeToDataRoot(dataRoot, std::wstring(fallbackRelativePath));
    }

    FileDefCatalogResult FileDefCatalog::Load(const std::filesystem::path& dataRoot) const
    {
        FileDefCatalogResult result;
        result.dataRoot = dataRoot;

        const auto fileDefPath = dataRoot / L"Info" / L"filedef.inf";
        std::wifstream input(fileDefPath);
        if (!input)
        {
            result.error = L"filedef.inf could not be opened: " + fileDefPath.wstring();
            return result;
        }

        std::wstring line;
        std::size_t lineNumber = 0;
        while (std::getline(input, line))
        {
            ++lineNumber;
            line = Trim(std::move(line));
            if (line.empty() || line.front() == L'#')
            {
                continue;
            }

            const auto separator = line.find(L':');
            if (separator == std::wstring::npos)
            {
                continue;
            }

            auto key = NormalizeKey(line.substr(0, separator));
            auto value = NormalizePathText(line.substr(separator + 1U));
            if (key.empty() || value.empty())
            {
                continue;
            }

            result.entries[key] = ResolveRelativeToDataRoot(dataRoot, std::move(value));
        }

        if (result.entries.empty())
        {
            result.error = L"filedef.inf contained no usable entries.";
        }
        return result;
    }
}
