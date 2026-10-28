#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::content
{
    struct SoundCatalogEntry
    {
        std::uint32_t soundId{ 0 };
        std::string filename;
    };

    struct MusicCatalogEntry
    {
        std::uint32_t musicId{ 0 };
        std::string filename;
        std::string filenameWav;
    };

    struct AudioCatalogResult
    {
        std::vector<SoundCatalogEntry> sounds;
        std::vector<MusicCatalogEntry> music;
        std::wstring error;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return error.empty() && (!sounds.empty() || !music.empty());
        }

        [[nodiscard]] const SoundCatalogEntry* FindSound(std::uint32_t soundId) const noexcept;
        [[nodiscard]] const MusicCatalogEntry* FindMusic(std::uint32_t musicId) const noexcept;
    };

    class AudioCatalog final
    {
    public:
        [[nodiscard]] AudioCatalogResult Load(
            const std::filesystem::path& soundPath,
            const std::filesystem::path& musicPath) const;
    };
}
