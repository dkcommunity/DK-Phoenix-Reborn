#include "Content/AudioCatalog.h"

#include "Content/LegacyBinaryTableReader.h"

namespace phoenix::content
{
    namespace
    {
        [[nodiscard]] bool LoadSoundTable(const std::filesystem::path& path, std::vector<SoundCatalogEntry>& out, std::wstring& error)
        {
            const auto bytes = ReadBinaryFile(path);
            if (!bytes)
            {
                error = L"Sound.inf could not be opened.";
                return false;
            }

            LegacyBinaryTableReader reader(*bytes);
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > 100000)
            {
                error = L"Sound.inf has invalid table size.";
                return false;
            }

            out.reserve(static_cast<std::size_t>(count));
            for (std::int32_t i = 0; i < count; ++i)
            {
                SoundCatalogEntry entry;
                entry.soundId = static_cast<std::uint32_t>(i);
                if (!reader.String(entry.filename))
                {
                    error = L"Sound.inf record ended early.";
                    out.clear();
                    return false;
                }
                out.push_back(std::move(entry));
            }
            return true;
        }

        [[nodiscard]] bool LoadMusicTable(const std::filesystem::path& path, std::vector<MusicCatalogEntry>& out, std::wstring& error)
        {
            const auto bytes = ReadBinaryFile(path);
            if (!bytes)
            {
                error = L"music.inf could not be opened.";
                return false;
            }

            LegacyBinaryTableReader reader(*bytes);
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > 100000)
            {
                error = L"music.inf has invalid table size.";
                return false;
            }

            out.reserve(static_cast<std::size_t>(count));
            for (std::int32_t i = 0; i < count; ++i)
            {
                MusicCatalogEntry entry;
                entry.musicId = static_cast<std::uint32_t>(i);
                if (!reader.String(entry.filename) || !reader.String(entry.filenameWav))
                {
                    error = L"music.inf record ended early.";
                    out.clear();
                    return false;
                }
                out.push_back(std::move(entry));
            }
            return true;
        }
    }

    const SoundCatalogEntry* AudioCatalogResult::FindSound(const std::uint32_t soundId) const noexcept
    {
        return soundId < sounds.size() ? &sounds[soundId] : nullptr;
    }

    const MusicCatalogEntry* AudioCatalogResult::FindMusic(const std::uint32_t musicId) const noexcept
    {
        return musicId < music.size() ? &music[musicId] : nullptr;
    }

    AudioCatalogResult AudioCatalog::Load(
        const std::filesystem::path& soundPath,
        const std::filesystem::path& musicPath) const
    {
        AudioCatalogResult result;
        std::wstring soundError;
        std::wstring musicError;
        const auto soundOk = LoadSoundTable(soundPath, result.sounds, soundError);
        const auto musicOk = LoadMusicTable(musicPath, result.music, musicError);
        if (!soundOk && !musicOk)
        {
            result.error = soundError + L"; " + musicError;
        }
        return result;
    }
}
