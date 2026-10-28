#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::filesystem
{
    enum class FileAccessOperation
    {
        Read,
        Write,
        CreateDirectory,
    };

    struct FileAccessDiagnostic final
    {
        bool allowed{ false };
        FileAccessOperation operation{ FileAccessOperation::Read };
        std::filesystem::path requestedPath;
        std::filesystem::path matchedRoot;
        std::wstring reason;
    };

    struct LegacyMount final
    {
        std::wstring name;
        std::filesystem::path root;
        bool readOnly{ true };
    };

    [[nodiscard]] std::vector<LegacyMount> LegacyMounts();
    [[nodiscard]] std::vector<std::filesystem::path> ApprovedWritableRoots();
    [[nodiscard]] std::vector<std::filesystem::path> ForbiddenWritableRoots();

    [[nodiscard]] bool IsPathUnderRoot(const std::filesystem::path& path, const std::filesystem::path& root);
    [[nodiscard]] FileAccessDiagnostic ValidateFileAccess(
        FileAccessOperation operation,
        const std::filesystem::path& path);

    [[nodiscard]] FileAccessDiagnostic EnsureDirectoryExists(const std::filesystem::path& directory);
}
