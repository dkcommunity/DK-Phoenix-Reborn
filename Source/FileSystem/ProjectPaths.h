#pragma once

#include <filesystem>

namespace phoenix::filesystem
{
    std::filesystem::path ProjectRoot();
    std::filesystem::path LogsRoot();
    std::filesystem::path BuildRoot();
    std::filesystem::path GeneratedRoot();
    std::filesystem::path GameDataRoot();
    std::filesystem::path GameDataTextRoot();
    std::filesystem::path LegacyDesignedCommonRoot();
    std::filesystem::path LegacyDesignedChsRoot();
    std::filesystem::path LegacyServerRoot();

    bool IsForbiddenLegacyRoot(const std::filesystem::path& path);
}
