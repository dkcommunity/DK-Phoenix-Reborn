#pragma once

#include <cstdint>
#include <filesystem>

namespace phoenix::audio
{
    struct WaveFileProbeResult
    {
        std::filesystem::path path;
        std::uint64_t sizeBytes{ 0 };
        std::uint16_t channels{ 0 };
        std::uint32_t sampleRate{ 0 };
        std::uint16_t bitsPerSample{ 0 };
        bool validRiffWave{ false };
    };

    class WaveFileProbe final
    {
    public:
        [[nodiscard]] WaveFileProbeResult Probe(const std::filesystem::path& path) const;
    };
}
