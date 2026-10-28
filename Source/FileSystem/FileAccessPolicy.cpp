#include "FileSystem/FileAccessPolicy.h"

#include "FileSystem/ProjectPaths.h"

#include <algorithm>
#include <cwctype>

namespace phoenix::filesystem
{
    namespace
    {
        std::wstring NormalizePathForCompare(const std::filesystem::path& input)
        {
            std::error_code error;
            auto path = std::filesystem::absolute(input, error);
            if (error)
            {
                path = input;
            }

            auto text = path.lexically_normal().wstring();
            for (auto& ch : text)
            {
                if (ch == L'/')
                {
                    ch = L'\\';
                }
                else
                {
                    ch = static_cast<wchar_t>(std::towlower(ch));
                }
            }

            while (text.size() > 1 && text.back() == L'\\')
            {
                text.pop_back();
            }

            return text;
        }

        FileAccessDiagnostic Deny(
            FileAccessOperation operation,
            const std::filesystem::path& path,
            const std::filesystem::path& matchedRoot,
            std::wstring reason)
        {
            return FileAccessDiagnostic{ false, operation, path, matchedRoot, std::move(reason) };
        }

        FileAccessDiagnostic Allow(
            FileAccessOperation operation,
            const std::filesystem::path& path,
            const std::filesystem::path& matchedRoot,
            std::wstring reason)
        {
            return FileAccessDiagnostic{ true, operation, path, matchedRoot, std::move(reason) };
        }
    }

    std::vector<LegacyMount> LegacyMounts()
    {
        return {
            LegacyMount{ L"GameData", GameDataRoot(), true },
            LegacyMount{ L"Designed_Common", LegacyDesignedCommonRoot(), true },
            LegacyMount{ L"Designed_Chs", LegacyDesignedChsRoot(), true },
            LegacyMount{ L"ServerSRC", LegacyServerRoot(), true },
        };
    }

    std::vector<std::filesystem::path> ApprovedWritableRoots()
    {
        return {
            BuildRoot(),
            GeneratedRoot(),
            LogsRoot(),
            ProjectRoot() / L"Docs",
        };
    }

    std::vector<std::filesystem::path> ForbiddenWritableRoots()
    {
        return {
            GameDataRoot(),
            LegacyDesignedCommonRoot(),
            LegacyDesignedChsRoot(),
            LegacyServerRoot(),
        };
    }

    bool IsPathUnderRoot(const std::filesystem::path& path, const std::filesystem::path& root)
    {
        const auto normalizedPath = NormalizePathForCompare(path);
        const auto normalizedRoot = NormalizePathForCompare(root);

        if (normalizedPath == normalizedRoot)
        {
            return true;
        }

        if (normalizedPath.size() <= normalizedRoot.size())
        {
            return false;
        }

        return normalizedPath.rfind(normalizedRoot + L"\\", 0) == 0;
    }

    FileAccessDiagnostic ValidateFileAccess(FileAccessOperation operation, const std::filesystem::path& path)
    {
        for (const auto& root : ForbiddenWritableRoots())
        {
            if (IsPathUnderRoot(path, root))
            {
                if (operation == FileAccessOperation::Read)
                {
                    return Allow(operation, path, root, L"Read access to legacy reference root is allowed.");
                }

                return Deny(operation, path, root, L"Write access to legacy reference root is forbidden.");
            }
        }

        if (operation == FileAccessOperation::Read)
        {
            return Allow(operation, path, {}, L"Read access is allowed outside forbidden write operations.");
        }

        for (const auto& root : ApprovedWritableRoots())
        {
            if (IsPathUnderRoot(path, root))
            {
                return Allow(operation, path, root, L"Write access is inside an approved new-project root.");
            }
        }

        return Deny(operation, path, {}, L"Write access is outside approved new-project writable roots.");
    }

    FileAccessDiagnostic EnsureDirectoryExists(const std::filesystem::path& directory)
    {
        auto diagnostic = ValidateFileAccess(FileAccessOperation::CreateDirectory, directory);
        if (!diagnostic.allowed)
        {
            return diagnostic;
        }

        std::error_code error;
        std::filesystem::create_directories(directory, error);
        if (error)
        {
            const auto message = error.message();
            return Deny(
                FileAccessOperation::CreateDirectory,
                directory,
                diagnostic.matchedRoot,
                L"Failed to create directory: " + std::wstring(message.begin(), message.end()));
        }

        return diagnostic;
    }
}
