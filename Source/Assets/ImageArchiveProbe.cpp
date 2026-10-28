#include "Assets/ImageArchiveProbe.h"

#include <fstream>
#include <system_error>

namespace phoenix::assets
{
    namespace
    {
        std::uint64_t FileSizeOrZero(const std::filesystem::path& path)
        {
            std::error_code ec;
            if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec))
            {
                return 0;
            }

            const auto size = std::filesystem::file_size(path, ec);
            return ec ? 0 : size;
        }

        bool ReadIndexHeader(const std::filesystem::path& path, std::uint16_t& count)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return false;
            }

            unsigned char raw[2]{};
            file.read(reinterpret_cast<char*>(raw), 2);
            if (file.gcount() != 2)
            {
                return false;
            }

            count = static_cast<std::uint16_t>(raw[0] | (raw[1] << 8U));
            const auto expected = static_cast<std::uint64_t>(2U + (count * 4U));
            return FileSizeOrZero(path) == expected;
        }
    }

    ImageArchiveProbeResult ImageArchiveProbe::ProbeCreatureChain(
        const std::filesystem::path& imageRoot,
        const std::wstring& baseName) const
    {
        ImageArchiveProbeResult result;
        result.framePackPath = imageRoot / (baseName + L".cfpk");
        result.frameIndexPath = imageRoot / (baseName + L".cfpki");
        result.spriteRootPath = imageRoot / (baseName + L".ispk");
        result.spriteIndexPath = imageRoot / (baseName + L".ispki");
        result.shadowRootPath = imageRoot / (baseName + L"shadow.sspk");
        result.shadowIndexPath = imageRoot / (baseName + L"shadow.sspki");

        result.framePackBytes = FileSizeOrZero(result.framePackPath);
        result.frameIndexValid = ReadIndexHeader(result.frameIndexPath, result.frameIndexCount);

        std::error_code ec;
        result.hasSpritePayload = std::filesystem::exists(result.spriteRootPath, ec) &&
            std::filesystem::exists(result.spriteIndexPath, ec);
        result.hasShadowPayload = std::filesystem::exists(result.shadowRootPath, ec) &&
            std::filesystem::exists(result.shadowIndexPath, ec);

        return result;
    }
}
