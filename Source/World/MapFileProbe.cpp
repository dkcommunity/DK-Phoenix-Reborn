#include "World/MapFileProbe.h"

#include <fstream>
#include <system_error>

namespace phoenix::world
{
    namespace
    {
        std::uint32_t ReadU32LE(const unsigned char* data)
        {
            return static_cast<std::uint32_t>(data[0] | (data[1] << 8U) | (data[2] << 16U) | (data[3] << 24U));
        }
    }

    MapFileProbeResult MapFileProbe::Probe(const std::filesystem::path& path) const
    {
        MapFileProbeResult result;
        result.path = path;

        std::error_code ec;
        result.exists = std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
        if (!result.exists)
        {
            return result;
        }

        result.sizeBytes = std::filesystem::file_size(path, ec);
        if (ec)
        {
            result.sizeBytes = 0;
        }

        std::ifstream file(path, std::ios::binary);
        for (int index = 0; index < 8; ++index)
        {
            unsigned char raw[4]{};
            file.read(reinterpret_cast<char*>(raw), 4);
            if (file.gcount() != 4)
            {
                break;
            }
            result.firstDwords.push_back(ReadU32LE(raw));
        }

        return result;
    }
}
