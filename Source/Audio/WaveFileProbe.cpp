#include "Audio/WaveFileProbe.h"

#include <array>
#include <fstream>
#include <system_error>

namespace phoenix::audio
{
    namespace
    {
        std::uint16_t ReadU16LE(const unsigned char* data)
        {
            return static_cast<std::uint16_t>(data[0] | (data[1] << 8U));
        }

        std::uint32_t ReadU32LE(const unsigned char* data)
        {
            return static_cast<std::uint32_t>(data[0] | (data[1] << 8U) | (data[2] << 16U) | (data[3] << 24U));
        }
    }

    WaveFileProbeResult WaveFileProbe::Probe(const std::filesystem::path& path) const
    {
        WaveFileProbeResult result;
        result.path = path;

        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
        {
            return result;
        }

        result.sizeBytes = std::filesystem::file_size(path, ec);
        if (ec)
        {
            result.sizeBytes = 0;
        }

        std::ifstream file(path, std::ios::binary);
        std::array<unsigned char, 44> header{};
        file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
        if (file.gcount() < static_cast<std::streamsize>(header.size()))
        {
            return result;
        }

        result.validRiffWave =
            header[0] == 'R' && header[1] == 'I' && header[2] == 'F' && header[3] == 'F' &&
            header[8] == 'W' && header[9] == 'A' && header[10] == 'V' && header[11] == 'E';

        if (result.validRiffWave)
        {
            result.channels = ReadU16LE(header.data() + 22);
            result.sampleRate = ReadU32LE(header.data() + 24);
            result.bitsPerSample = ReadU16LE(header.data() + 34);
        }

        return result;
    }
}
