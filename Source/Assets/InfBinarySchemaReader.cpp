#include "Assets/InfBinarySchemaReader.h"

#include "Core/BinaryReader.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>

namespace phoenix::assets
{
    namespace
    {
        std::vector<std::byte> ReadAllBytes(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return {};
            }

            const std::vector<char> chars{
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            };

            std::vector<std::byte> bytes;
            bytes.reserve(chars.size());
            for (const auto value : chars)
            {
                bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(value)));
            }

            return bytes;
        }

        std::vector<std::string> ExtractPrintableStrings(const std::vector<std::byte>& bytes)
        {
            std::vector<std::string> strings;
            std::string current;

            for (const auto byte : bytes)
            {
                const auto value = static_cast<unsigned char>(byte);
                if (value >= 32 && value <= 126)
                {
                    current.push_back(static_cast<char>(value));
                    continue;
                }

                if (current.size() >= 4)
                {
                    strings.push_back(current);
                    if (strings.size() >= 16)
                    {
                        break;
                    }
                }
                current.clear();
            }

            if (strings.size() < 16 && current.size() >= 4)
            {
                strings.push_back(current);
            }

            return strings;
        }
    }

    InfBinarySample InfBinarySchemaReader::Sample(const std::filesystem::path& path) const
    {
        InfBinarySample sample;
        sample.path = path;

        std::error_code ec;
        sample.exists = std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
        if (!sample.exists)
        {
            return sample;
        }

        sample.sizeBytes = std::filesystem::file_size(path, ec);
        if (ec)
        {
            sample.sizeBytes = 0;
        }

        const auto bytes = ReadAllBytes(path);
        core::BinaryReader reader(bytes.data(), bytes.size());
        if (const auto first = reader.ReadU32LE())
        {
            sample.firstDword = *first;
        }

        sample.printableStrings = ExtractPrintableStrings(bytes);
        sample.likelyBinary = std::any_of(bytes.begin(), bytes.end(), [](const std::byte byte) {
            return static_cast<unsigned char>(byte) == 0;
        });
        return sample;
    }
}
