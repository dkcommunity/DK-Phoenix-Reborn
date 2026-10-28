#include "World/ZoneInfoCatalog.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>
#include <string_view>
#include <utility>

namespace phoenix::world
{
    namespace
    {
        class LegacyBinaryReader final
        {
        public:
            explicit LegacyBinaryReader(std::vector<std::uint8_t> bytes)
                : bytes_(std::move(bytes))
            {
            }

            [[nodiscard]] std::size_t Offset() const noexcept
            {
                return offset_;
            }

            [[nodiscard]] bool U8(std::uint8_t& value) noexcept
            {
                if (offset_ + 1U > bytes_.size())
                {
                    return false;
                }
                value = bytes_[offset_++];
                return true;
            }

            [[nodiscard]] bool Bool(bool& value) noexcept
            {
                std::uint8_t raw = 0;
                if (!U8(raw))
                {
                    return false;
                }
                value = raw != 0;
                return true;
            }

            [[nodiscard]] bool U16(std::uint16_t& value) noexcept
            {
                if (offset_ + 2U > bytes_.size())
                {
                    return false;
                }
                value = static_cast<std::uint16_t>(bytes_[offset_]) |
                    static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes_[offset_ + 1U]) << 8U);
                offset_ += 2U;
                return true;
            }

            [[nodiscard]] bool I32(std::int32_t& value) noexcept
            {
                std::uint32_t raw = 0;
                if (!U32(raw))
                {
                    return false;
                }
                value = static_cast<std::int32_t>(raw);
                return true;
            }

            [[nodiscard]] bool U32(std::uint32_t& value) noexcept
            {
                if (offset_ + 4U > bytes_.size())
                {
                    return false;
                }
                value = static_cast<std::uint32_t>(bytes_[offset_]) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 1U]) << 8U) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 2U]) << 16U) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 3U]) << 24U);
                offset_ += 4U;
                return true;
            }

            [[nodiscard]] bool String(std::string& value) noexcept
            {
                std::int32_t length = 0;
                if (!I32(length) || length < 0 || length > 65535 || offset_ + static_cast<std::size_t>(length) > bytes_.size())
                {
                    return false;
                }

                value.assign(
                    reinterpret_cast<const char*>(bytes_.data() + offset_),
                    reinterpret_cast<const char*>(bytes_.data() + offset_ + static_cast<std::size_t>(length)));
                offset_ += static_cast<std::size_t>(length);
                return true;
            }

            [[nodiscard]] bool Skip(const std::size_t count) noexcept
            {
                if (offset_ + count > bytes_.size())
                {
                    return false;
                }
                offset_ += count;
                return true;
            }

        private:
            std::vector<std::uint8_t> bytes_;
            std::size_t offset_{ 0 };
        };

        std::vector<std::uint8_t> ReadAllBytes(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return {};
            }

            const std::vector<char> raw{
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            };
            std::vector<std::uint8_t> bytes;
            bytes.reserve(raw.size());
            for (const auto byte : raw)
            {
                bytes.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(byte)));
            }
            return bytes;
        }

        [[nodiscard]] bool ReadWorldMapInfo(LegacyBinaryReader& reader)
        {
            std::int32_t worldMapType = 0;
            if (!reader.I32(worldMapType))
            {
                return false;
            }

            constexpr std::int32_t kWorldMapMax = 1;
            if (worldMapType == kWorldMapMax)
            {
                return true;
            }

            std::int32_t paymentType = 0;
            std::string miscDescription;
            return reader.I32(paymentType) &&
                reader.Skip(16U) && // RECT
                reader.Skip(6U) &&  // encouraged level min/max for three races
                reader.String(miscDescription);
        }

        [[nodiscard]] bool ReadZoneInfo(LegacyBinaryReader& reader, ZoneInfo& info)
        {
            std::int32_t soundCount = 0;
            std::uint8_t mapShapeCount = 0;
            if (!reader.U16(info.id) ||
                !reader.String(info.name) ||
                !reader.U8(info.property) ||
                !reader.U16(info.musicId) ||
                !reader.String(info.filename) ||
                !reader.String(info.infoFilename) ||
                !reader.String(info.teenFilename) ||
                !reader.I32(soundCount) ||
                soundCount < 0 ||
                soundCount > 1000)
            {
                return false;
            }

            info.soundIds.reserve(static_cast<std::size_t>(soundCount));
            for (std::int32_t i = 0; i < soundCount; ++i)
            {
                std::uint16_t soundId = 0;
                if (!reader.U16(soundId))
                {
                    return false;
                }
                info.soundIds.push_back(soundId);
            }

            if (!reader.Bool(info.safety) ||
                !reader.U16(info.creatureColorSet) ||
                !reader.U16(info.minimapSpriteId) ||
                !reader.Bool(info.holyLand) ||
                !reader.Bool(info.chatMaskByRace) ||
                !reader.Bool(info.cannotAttackInSafe) ||
                !reader.Bool(info.cannotUseSpecialItem) ||
                !reader.Bool(info.competenceZone) ||
                !reader.U8(info.pkType) ||
                !ReadWorldMapInfo(reader) ||
                !reader.U8(mapShapeCount))
            {
                return false;
            }

            info.mapShapes.reserve(mapShapeCount);
            for (std::uint8_t i = 0; i < mapShapeCount; ++i)
            {
                std::string shape;
                if (!reader.String(shape))
                {
                    return false;
                }
                info.mapShapes.push_back(std::move(shape));
            }

            return true;
        }
    }

    ZoneInfoCatalogResult ZoneInfoCatalog::Load(const std::filesystem::path& path) const
    {
        ZoneInfoCatalogResult result;
        result.path = path;

        auto bytes = ReadAllBytes(path);
        if (bytes.empty())
        {
            result.error = "Zone.inf could not be opened or is empty.";
            return result;
        }

        LegacyBinaryReader reader(std::move(bytes));
        std::int32_t count = 0;
        if (!reader.I32(count) || count < 0 || count > 10000)
        {
            result.error = "Zone.inf has an invalid zone count.";
            return result;
        }

        result.zones.reserve(static_cast<std::size_t>(count));
        for (std::int32_t i = 0; i < count; ++i)
        {
            ZoneInfo info;
            if (!ReadZoneInfo(reader, info))
            {
                result.error = "Zone.inf parse failed at byte " + std::to_string(reader.Offset()) + ".";
                result.zones.clear();
                return result;
            }
            result.zones.push_back(std::move(info));
        }

        return result;
    }

    std::optional<ZoneInfo> ZoneInfoCatalog::Find(const ZoneInfoCatalogResult& catalog, const std::uint16_t zoneId)
    {
        const auto match = std::find_if(catalog.zones.begin(), catalog.zones.end(), [zoneId](const ZoneInfo& info) {
            return info.id == zoneId;
        });
        if (match == catalog.zones.end())
        {
            return std::nullopt;
        }
        return *match;
    }
}
