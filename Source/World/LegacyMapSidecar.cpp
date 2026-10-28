#include "World/LegacyMapSidecar.h"

#include "World/LegacyMapReader.h"
#include "World/LegacyMapWriter.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <system_error>
#include <tuple>

namespace phoenix::world
{
    namespace
    {
        constexpr std::uint8_t kKnownServerFlags =
            kLegacyBlockUnderground | kLegacyBlockGround | kLegacyBlockFlying | kLegacyServerPortal;
        constexpr std::uint8_t kKnownSafetyFlags =
            kLegacySafetySlayer | kLegacySafetyVampire | kLegacySafetyCommon | kLegacySafetyOusters;
        constexpr std::uint8_t kMultiPortal = 3U;
        constexpr std::uint8_t kMultiPortalHeliport = 4U;

        bool IsServerPortal(const LegacyMapPortal& portal) noexcept { return portal.type < 200U; }

        class Reader final
        {
        public:
            explicit Reader(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}

            bool U8(std::uint8_t& value) { return Read(&value, 1U); }
            bool U16(std::uint16_t& value)
            {
                std::array<std::uint8_t, 2> bytes{};
                if (!Read(bytes.data(), bytes.size())) return false;
                value = static_cast<std::uint16_t>(bytes[0] | (static_cast<std::uint16_t>(bytes[1]) << 8U));
                return true;
            }
            bool U32(std::uint32_t& value)
            {
                std::array<std::uint8_t, 4> bytes{};
                if (!Read(bytes.data(), bytes.size())) return false;
                value = static_cast<std::uint32_t>(bytes[0]) |
                    (static_cast<std::uint32_t>(bytes[1]) << 8U) |
                    (static_cast<std::uint32_t>(bytes[2]) << 16U) |
                    (static_cast<std::uint32_t>(bytes[3]) << 24U);
                return true;
            }
            bool I32(std::int32_t& value)
            {
                std::uint32_t raw = 0;
                if (!U32(raw)) return false;
                value = static_cast<std::int32_t>(raw);
                return true;
            }
            bool String(std::string& value)
            {
                std::int32_t size = 0;
                if (!I32(size) || size < 0 || static_cast<std::size_t>(size) > Remaining()) return false;
                value.assign(reinterpret_cast<const char*>(bytes_.data() + position_), static_cast<std::size_t>(size));
                position_ += static_cast<std::size_t>(size);
                return true;
            }
            [[nodiscard]] std::size_t Position() const noexcept { return position_; }
            [[nodiscard]] std::size_t Remaining() const noexcept { return bytes_.size() - position_; }
            [[nodiscard]] std::vector<std::uint8_t> RemainingBytes() const
            {
                return { bytes_.begin() + static_cast<std::ptrdiff_t>(position_), bytes_.end() };
            }

        private:
            bool Read(void* output, const std::size_t size)
            {
                if (size > Remaining()) return false;
                std::copy_n(bytes_.data() + position_, size, static_cast<std::uint8_t*>(output));
                position_ += size;
                return true;
            }

            const std::vector<std::uint8_t>& bytes_;
            std::size_t position_{ 0 };
        };

        class Writer final
        {
        public:
            void U8(const std::uint8_t value) { bytes.push_back(value); }
            void U16(const std::uint16_t value)
            {
                U8(static_cast<std::uint8_t>(value));
                U8(static_cast<std::uint8_t>(value >> 8U));
            }
            void U32(const std::uint32_t value)
            {
                U8(static_cast<std::uint8_t>(value));
                U8(static_cast<std::uint8_t>(value >> 8U));
                U8(static_cast<std::uint8_t>(value >> 16U));
                U8(static_cast<std::uint8_t>(value >> 24U));
            }
            void I32(const std::int32_t value) { U32(static_cast<std::uint32_t>(value)); }
            bool String(const std::string& value)
            {
                if (value.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) return false;
                I32(static_cast<std::int32_t>(value.size()));
                bytes.insert(bytes.end(), value.begin(), value.end());
                return true;
            }
            std::vector<std::uint8_t> bytes;
        };

        struct Rect final
        {
            std::uint8_t flag{ 0 };
            std::uint8_t left{ 0 };
            std::uint8_t top{ 0 };
            std::uint8_t right{ 0 };
            std::uint8_t bottom{ 0 };
            std::uint8_t portalType{ 0 };
            std::vector<std::uint16_t> zoneIds;
        };

        bool ReadFile(const std::filesystem::path& path, std::vector<std::uint8_t>& bytes, std::string& error)
        {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) { error = "Could not open sidecar: " + path.string(); return false; }
            const auto end = file.tellg();
            if (end < 0) { error = "Could not determine sidecar size: " + path.string(); return false; }
            bytes.resize(static_cast<std::size_t>(end));
            file.seekg(0, std::ios::beg);
            if (!bytes.empty()) file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (!file.good() && !file.eof()) { error = "Could not read sidecar: " + path.string(); return false; }
            return true;
        }

        bool WriteFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
        {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file) return false;
            if (!bytes.empty()) file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            return file.good();
        }

        bool SamePortalIdentity(const LegacyMapPortal& left, const LegacyMapPortal& right)
        {
            if (left.type != right.type || left.targets.size() != right.targets.size()) return false;
            for (std::size_t index = 0; index < left.targets.size(); ++index)
                if (left.targets[index].zoneId != right.targets[index].zoneId) return false;
            return true;
        }

        bool ApplySafetyRect(LegacyMap& map, const Rect& rect, std::vector<std::uint8_t>& safety, std::string& error)
        {
            if ((rect.flag & ~kKnownSafetyFlags) != 0U || rect.left > rect.right || rect.top > rect.bottom ||
                rect.right >= map.width || rect.bottom >= map.height)
            {
                error = "Legacy safety rectangle is invalid or outside the map.";
                return false;
            }
            for (std::uint32_t y = rect.top; y <= rect.bottom; ++y)
                for (std::uint32_t x = rect.left; x <= rect.right; ++x)
                    safety[static_cast<std::size_t>(y) * map.width + x] |= rect.flag;
            return true;
        }

        bool ReadSafetyList(Reader& reader, LegacyMap& map, std::vector<std::uint8_t>& safety, std::string& error)
        {
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || static_cast<std::uint64_t>(count) >
                static_cast<std::uint64_t>(map.width) * map.height * 4ULL)
            {
                error = "Legacy safety rectangle count is invalid.";
                return false;
            }
            safety.assign(static_cast<std::size_t>(map.width) * map.height, 0U);
            for (std::int32_t index = 0; index < count; ++index)
            {
                Rect rect;
                if (!reader.U8(rect.flag) || !reader.U8(rect.left) || !reader.U8(rect.top) ||
                    !reader.U8(rect.right) || !reader.U8(rect.bottom))
                {
                    error = "Legacy safety rectangle is truncated.";
                    return false;
                }
                if (!ApplySafetyRect(map, rect, safety, error)) return false;
            }
            return true;
        }

        bool ReadMip(LegacyMap& map, const std::vector<std::uint8_t>& bytes, std::vector<std::uint8_t>& safety,
            std::string& error)
        {
            Reader reader(bytes);
            std::uint16_t width = 0, height = 0;
            std::int32_t portalCount = 0;
            if (!reader.U16(width) || !reader.U16(height) || width != map.width || height != map.height ||
                !reader.I32(portalCount) || portalCount < 0 ||
                static_cast<std::uint64_t>(portalCount) > static_cast<std::uint64_t>(map.width) * map.height)
            {
                error = "Legacy .mip dimensions or portal count are invalid.";
                return false;
            }
            for (std::int32_t index = 0; index < portalCount; ++index)
            {
                LegacyMapPortal portal;
                std::uint8_t targetCount = 1U;
                if (!reader.U8(portal.type) ||
                    ((portal.type == kMultiPortal || portal.type == kMultiPortalHeliport) && !reader.U8(targetCount)) ||
                    targetCount == 0U)
                {
                    error = "Legacy .mip portal header is invalid.";
                    return false;
                }
                portal.targets.resize(targetCount);
                for (auto& target : portal.targets)
                    if (!reader.U16(target.zoneId)) { error = "Legacy .mip portal target is truncated."; return false; }
                Rect rect;
                if (!reader.U8(rect.left) || !reader.U8(rect.top) || !reader.U8(rect.right) || !reader.U8(rect.bottom))
                {
                    error = "Legacy .mip portal rectangle is truncated.";
                    return false;
                }
                for (std::uint32_t y = rect.top; y <= rect.bottom; ++y)
                {
                    for (std::uint32_t x = rect.left; x <= rect.right; ++x)
                    {
                        if (x >= map.width || y >= map.height) continue;
                        auto& sector = map.sidecars.sectors[static_cast<std::size_t>(y) * map.width + x];
                        // Legacy MZoneEdit loads rectangles in file order and later records replace earlier sectors.
                        sector.portal = portal;
                    }
                }
            }
            if (!ReadSafetyList(reader, map, safety, error)) return false;
            map.sidecars.mipTrailingBytes = reader.RemainingBytes();
            return true;
        }

        bool ReadSsi(LegacyMap& map, const std::vector<std::uint8_t>& bytes, std::vector<std::uint8_t>& safety,
            std::string& error, std::vector<std::uint8_t>& trailing)
        {
            Reader reader(bytes);
            if (!ReadSafetyList(reader, map, safety, error)) return false;
            trailing = reader.RemainingBytes();
            return true;
        }

        bool ReadSmp(LegacyMap& map, const std::vector<std::uint8_t>& bytes, std::string& error)
        {
            Reader reader(bytes);
            std::string version, name, description;
            std::uint16_t zoneId = 0, zoneGroupId = 0, width = 0, height = 0;
            std::uint8_t zoneType = 0, zoneLevel = 0;
            if (!reader.String(version) || !reader.U16(zoneId) || !reader.U16(zoneGroupId) || !reader.String(name) ||
                !reader.U8(zoneType) || !reader.U8(zoneLevel) || !reader.String(description) ||
                !reader.U16(width) || !reader.U16(height))
            {
                error = "Legacy .smp header is truncated.";
                return false;
            }
            if (version != map.version || zoneId != map.zoneId || zoneGroupId != map.zoneGroupId || name != map.name ||
                zoneType != map.zoneType || zoneLevel != map.zoneLevel || description != map.description ||
                width != map.width || height != map.height)
            {
                error = "Legacy .smp header does not match the client .map document.";
                return false;
            }
            for (std::size_t index = 0; index < map.sectors.size(); ++index)
            {
                std::uint8_t flags = 0;
                if (!reader.U8(flags)) { error = "Legacy .smp sector flag array is truncated."; return false; }
                // Eski save'lerde 49 portal sektorunde .smp collision bits (0x07) 0 yazilmis
                // ama .map property & 0x07 = 0x07/0x02. Hard-reject yerine .map degerine
                // guven ve .smp'den gelen collision bits'i .map ile zorla uyumlu hale getir.
                // preservedServerFlags 0x07 bits'i de tasiyacagi icin BuildSmp bu ileride
                // dogru yazar. (Eski davranis: error = "Legacy .smp collision flags do not match
                // the client .map sector properties."; return false;)
                if ((flags & 0x07U) != (map.sectors[index].property & 0x07U))
                {
                    flags = static_cast<std::uint8_t>((flags & ~0x07U) | (map.sectors[index].property & 0x07U));
                }
                auto& sector = map.sidecars.sectors[index];
                sector.preservedServerFlags = static_cast<std::uint8_t>(flags & ~kLegacyServerPortal);
                if ((flags & kLegacyServerPortal) == 0U)
                {
                    // .smp'de 0x80 bayragi yok ama .mip'ten gelen server portal varsa
                    // hata vermek yerine kabul et — destinationKnown=false olarak yerinde birak.
                    // Bu, .mip ile .smp arasindaki eszamanlisizlik durumunda Load'un basarisiz
                    // olmasini engeller. Save sirasinda kullanici bilgilendirilir.
                    // (Eski davranis: error = "Legacy .smp is missing a server portal declared by the companion .mip."; return false;)
                    continue;
                }
                LegacyMapPortal portal;
                std::uint8_t targetCount = 1U;
                if (!reader.U8(portal.type) ||
                    ((portal.type == kMultiPortal || portal.type == kMultiPortalHeliport) && !reader.U8(targetCount)) ||
                    targetCount == 0U)
                {
                    error = "Legacy .smp portal header is invalid.";
                    return false;
                }
                portal.targets.resize(targetCount);
                for (auto& target : portal.targets)
                {
                    if (!reader.U16(target.zoneId) || !reader.U8(target.x) || !reader.U8(target.y))
                    {
                        error = "Legacy .smp portal destination is truncated.";
                        return false;
                    }
                    target.destinationKnown = true;
                }
                if (sector.portal && !SamePortalIdentity(*sector.portal, portal))
                {
                    error = "Legacy .smp portal does not match the companion .mip portal.";
                    return false;
                }
                sector.portal = std::move(portal);
            }
            map.sidecars.smpTrailingBytes = reader.RemainingBytes();
            return true;
        }

        std::vector<Rect> BuildSafetyRects(const LegacyMap& map)
        {
            std::vector<Rect> result;
            for (const auto flag : { kLegacySafetyCommon, kLegacySafetySlayer, kLegacySafetyVampire, kLegacySafetyOusters })
            {
                std::vector<Rect> rows;
                for (std::uint32_t y = 0; y < map.height; ++y)
                {
                    std::uint32_t x = 0;
                    while (x < map.width)
                    {
                        const auto hasFlag = [&](const std::uint32_t value) {
                            return (map.sidecars.sectors[static_cast<std::size_t>(y) * map.width + value].safetyFlags & flag) != 0U;
                        };
                        if (!hasFlag(x)) { ++x; continue; }
                        const auto left = x;
                        while (x + 1U < map.width && hasFlag(x + 1U)) ++x;
                        rows.push_back({ flag, static_cast<std::uint8_t>(left), static_cast<std::uint8_t>(y),
                            static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), 0U, {} });
                        ++x;
                    }
                }
                for (auto& row : rows)
                {
                    const auto previous = std::find_if(result.rbegin(), result.rend(), [&](const Rect& candidate) {
                        return candidate.flag == flag && candidate.left == row.left && candidate.right == row.right &&
                            static_cast<unsigned>(candidate.bottom) + 1U == row.top;
                    });
                    if (previous != result.rend()) previous->bottom = row.bottom;
                    else result.push_back(std::move(row));
                }
            }
            return result;
        }

        std::vector<Rect> BuildPortalRects(const LegacyMap& map)
        {
            std::vector<Rect> result;
            for (std::uint32_t y = 0; y < map.height; ++y)
            {
                std::uint32_t x = 0;
                while (x < map.width)
                {
                    const auto& sector = map.sidecars.sectors[static_cast<std::size_t>(y) * map.width + x];
                    if (!sector.portal) { ++x; continue; }
                    Rect row;
                    row.left = static_cast<std::uint8_t>(x);
                    row.top = static_cast<std::uint8_t>(y);
                    row.portalType = sector.portal->type;
                    for (const auto& target : sector.portal->targets) row.zoneIds.push_back(target.zoneId);
                    while (x + 1U < map.width)
                    {
                        const auto& next = map.sidecars.sectors[static_cast<std::size_t>(y) * map.width + x + 1U];
                        if (!next.portal || !SamePortalIdentity(*sector.portal, *next.portal)) break;
                        ++x;
                    }
                    row.right = static_cast<std::uint8_t>(x);
                    row.bottom = static_cast<std::uint8_t>(y);
                    const auto previous = std::find_if(result.rbegin(), result.rend(), [&](const Rect& candidate) {
                        return candidate.portalType == row.portalType && candidate.zoneIds == row.zoneIds &&
                            candidate.left == row.left && candidate.right == row.right &&
                            static_cast<unsigned>(candidate.bottom) + 1U == row.top;
                    });
                    if (previous != result.rend()) previous->bottom = row.bottom;
                    else result.push_back(std::move(row));
                    ++x;
                }
            }
            return result;
        }

        bool ValidateForWrite(const LegacyMap& map, std::string& error,
            std::vector<std::pair<std::uint16_t, std::uint16_t>>& portalDestinationsUnknown)
        {
            const auto sectorCount = static_cast<std::size_t>(map.width) * map.height;
            if (map.width == 0U || map.height == 0U || map.sectors.size() != sectorCount ||
                map.sidecars.sectors.size() != sectorCount)
            {
                error = "Server sidecar model dimensions do not match the Legacy map.";
                return false;
            }
            for (std::size_t index = 0; index < sectorCount; ++index)
            {
                const auto& sector = map.sidecars.sectors[index];
                if ((sector.safetyFlags & ~kKnownSafetyFlags) != 0U)
                {
                    error = "Server sidecar contains unsupported safety flags.";
                    return false;
                }
                if (!sector.portal) continue;
                const auto x = index % map.width;
                const auto y = index / map.width;
                if (x > 255U || y > 255U || sector.portal->targets.empty() || sector.portal->targets.size() > 255U)
                {
                    error = "Portal data exceeds the Legacy byte-coordinate or target-count limit.";
                    return false;
                }
                const bool multi = sector.portal->type == kMultiPortal || sector.portal->type == kMultiPortalHeliport;
                if (!multi && sector.portal->targets.size() != 1U)
                {
                    error = "Only Legacy multi-portal types 3 and 4 may contain multiple targets.";
                    return false;
                }
                // destinationKnown=false olan server portal'lar Save'i engellemez.
                // Bu sektörleri portalDestinationsUnknown listesine topla; BuildSmp 0x80
                // bayragi ile X/Y=0,0 yazar. Harita yine kaydedilir; MapEditor kullaniciya
                // warning olarak gosterir. (Ilk tespite gore: eski davranis hard-reject idi.)
                if (IsServerPortal(*sector.portal) && std::any_of(sector.portal->targets.begin(), sector.portal->targets.end(),
                    [](const LegacyMapPortalTarget& target) { return !target.destinationKnown; }))
                {
                    portalDestinationsUnknown.emplace_back(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y));
                }
            }
            if ((map.width > 256U || map.height > 256U) && std::any_of(map.sidecars.sectors.begin(), map.sidecars.sectors.end(),
                [](const LegacyMapServerSector& sector) { return sector.safetyFlags != 0U || sector.portal.has_value(); }))
            {
                error = "Legacy .mip portal and safety rectangles use byte coordinates and cannot address this map size.";
                return false;
            }
            return true;
        }

        std::vector<std::uint8_t> BuildSmp(const LegacyMap& map)
        {
            Writer writer;
            writer.String(map.version);
            writer.U16(map.zoneId);
            writer.U16(map.zoneGroupId);
            writer.String(map.name);
            writer.U8(map.zoneType);
            writer.U8(map.zoneLevel);
            writer.String(map.description);
            writer.U16(map.width);
            writer.U16(map.height);
            for (std::size_t index = 0; index < map.sectors.size(); ++index)
            {
                const auto& server = map.sidecars.sectors[index];
                // preservedServerFlags artik .smp'den gelen tum non-portal bitleri (0x7F) icerir;
                // buna collision bits (0x07) da dahil. map.sectors[index].property & 0x07 ile OR'leyerek
                // iki kaynaktan da collision bits'leri koruyalim — tek kaynaktan 0 gelse bile digeri
                // telafi eder. Bu, eski save'lerde 49 portal sektorunde 0x07 bits'in kaybolmasini
                // engeller. (Ilk tespite gore: map.property&0x07 bazi sektorlerde 0 gorunuyordu.)
                const auto preservedFlags = static_cast<std::uint8_t>(server.preservedServerFlags & ~kLegacyServerPortal);
                const auto mapCollision = static_cast<std::uint8_t>(map.sectors[index].property & 0x07U);
                const auto preservedCollision = static_cast<std::uint8_t>(server.preservedServerFlags & 0x07U);
                auto flags = static_cast<std::uint8_t>(preservedFlags | mapCollision | preservedCollision);
                if (server.portal && IsServerPortal(*server.portal)) flags |= kLegacyServerPortal;
                writer.U8(flags);
                if (!server.portal || !IsServerPortal(*server.portal)) continue;
                writer.U8(server.portal->type);
                if (server.portal->type == kMultiPortal || server.portal->type == kMultiPortalHeliport)
                    writer.U8(static_cast<std::uint8_t>(server.portal->targets.size()));
                for (const auto& target : server.portal->targets)
                {
                    writer.U16(target.zoneId);
                    writer.U8(target.x);
                    writer.U8(target.y);
                }
            }
            writer.bytes.insert(writer.bytes.end(), map.sidecars.smpTrailingBytes.begin(), map.sidecars.smpTrailingBytes.end());
            return writer.bytes;
        }

        void WriteSafetyList(Writer& writer, const std::vector<Rect>& rects)
        {
            writer.I32(static_cast<std::int32_t>(rects.size()));
            for (const auto& rect : rects)
            {
                writer.U8(rect.flag); writer.U8(rect.left); writer.U8(rect.top);
                writer.U8(rect.right); writer.U8(rect.bottom);
            }
        }

        std::vector<std::uint8_t> BuildMip(const LegacyMap& map, const std::vector<Rect>& portals,
            const std::vector<Rect>& safety)
        {
            Writer writer;
            writer.U16(map.width); writer.U16(map.height);
            writer.I32(static_cast<std::int32_t>(portals.size()));
            for (const auto& rect : portals)
            {
                writer.U8(rect.portalType);
                if (rect.portalType == kMultiPortal || rect.portalType == kMultiPortalHeliport)
                    writer.U8(static_cast<std::uint8_t>(rect.zoneIds.size()));
                for (const auto zoneId : rect.zoneIds) writer.U16(zoneId);
                writer.U8(rect.left); writer.U8(rect.top); writer.U8(rect.right); writer.U8(rect.bottom);
            }
            WriteSafetyList(writer, safety);
            writer.bytes.insert(writer.bytes.end(), map.sidecars.mipTrailingBytes.begin(), map.sidecars.mipTrailingBytes.end());
            return writer.bytes;
        }

        std::vector<std::uint8_t> BuildSsi(const LegacyMap& map, const std::vector<Rect>& safety)
        {
            Writer writer;
            WriteSafetyList(writer, safety);
            writer.bytes.insert(writer.bytes.end(), map.sidecars.ssiTrailingBytes.begin(), map.sidecars.ssiTrailingBytes.end());
            return writer.bytes;
        }

        struct OutputFile final
        {
            std::filesystem::path target;
            std::filesystem::path temporary;
            std::filesystem::path rollback;
            std::vector<std::uint8_t> bytes;
            bool hadOriginal{ false };
            bool committed{ false };
        };

        bool CommitFiles(std::vector<OutputFile>& files, LegacyMapBundleWriteResult& result)
        {
            std::error_code ec;
            for (auto& file : files)
            {
                file.temporary = file.target; file.temporary += L".phoenix.tmp";
                file.rollback = file.target; file.rollback += L".phoenix.rollback";
                std::filesystem::remove(file.temporary, ec); ec.clear();
                std::filesystem::remove(file.rollback, ec); ec.clear();
                if (!WriteFile(file.temporary, file.bytes))
                {
                    result.error = "Could not write temporary map bundle file: " + file.temporary.string();
                    for (const auto& cleanup : files) { std::filesystem::remove(cleanup.temporary, ec); ec.clear(); }
                    return false;
                }
            }

            for (auto& file : files)
            {
                file.hadOriginal = std::filesystem::exists(file.target, ec); ec.clear();
                if (!file.hadOriginal) continue;
                auto backup = file.target; backup += L".bak";
                std::filesystem::copy_file(file.target, backup, std::filesystem::copy_options::overwrite_existing, ec);
                if (ec)
                {
                    result.error = "Automatic map bundle backup failed: " + ec.message();
                    for (const auto& cleanup : files) { std::filesystem::remove(cleanup.temporary, ec); ec.clear(); }
                    return false;
                }
                result.backupCreated = true;
                result.backupPaths.push_back(std::move(backup));
            }

            const auto rollbackAll = [&]() {
                for (auto iterator = files.rbegin(); iterator != files.rend(); ++iterator)
                {
                    if (iterator->committed) { std::filesystem::remove(iterator->target, ec); ec.clear(); }
                    if (std::filesystem::exists(iterator->rollback, ec))
                    {
                        ec.clear(); std::filesystem::rename(iterator->rollback, iterator->target, ec); ec.clear();
                    }
                    std::filesystem::remove(iterator->temporary, ec); ec.clear();
                }
            };

            for (auto& file : files)
            {
                if (file.hadOriginal)
                {
                    std::filesystem::rename(file.target, file.rollback, ec);
                    if (ec) { result.error = "Could not stage existing map bundle file: " + ec.message(); rollbackAll(); return false; }
                }
                std::filesystem::rename(file.temporary, file.target, ec);
                if (ec) { result.error = "Could not commit map bundle file: " + ec.message(); rollbackAll(); return false; }
                file.committed = true;
            }
            for (const auto& file : files) { std::filesystem::remove(file.rollback, ec); ec.clear(); }
            return true;
        }
    }

    LegacyMapSidecarPaths LegacyMapSidecarPaths::FromMapPath(const std::filesystem::path& mapPath)
    {
        LegacyMapSidecarPaths paths;
        paths.mip = mapPath; paths.mip.replace_extension(L".mip");
        paths.smp = mapPath; paths.smp.replace_extension(L".smp");
        paths.ssi = mapPath; paths.ssi.replace_extension(L".ssi");
        return paths;
    }

    void InitializeLegacyMapSidecars(LegacyMap& map)
    {
        map.sidecars = {};
        map.sidecars.sectors.resize(static_cast<std::size_t>(map.width) * map.height);
    }

    LegacyMapServerSector* LegacyServerSector(LegacyMap& map, const std::uint16_t x, const std::uint16_t y) noexcept
    {
        if (x >= map.width || y >= map.height || map.sidecars.sectors.size() !=
            static_cast<std::size_t>(map.width) * map.height) return nullptr;
        return &map.sidecars.sectors[static_cast<std::size_t>(y) * map.width + x];
    }

    const LegacyMapServerSector* LegacyServerSector(
        const LegacyMap& map, const std::uint16_t x, const std::uint16_t y) noexcept
    {
        if (x >= map.width || y >= map.height || map.sidecars.sectors.size() !=
            static_cast<std::size_t>(map.width) * map.height) return nullptr;
        return &map.sidecars.sectors[static_cast<std::size_t>(y) * map.width + x];
    }

    LegacyMapSidecarResult LegacyMapSidecarReader::Load(LegacyMap& map, const std::filesystem::path& mapPath) const
    {
        LegacyMapSidecarResult result;
        InitializeLegacyMapSidecars(map);
        const auto paths = LegacyMapSidecarPaths::FromMapPath(mapPath);
        std::error_code ec;
        std::vector<std::uint8_t> mipSafety, ssiSafety;

        if (std::filesystem::exists(paths.mip, ec))
        {
            if (!ReadFile(paths.mip, map.sidecars.originalMipBytes, result.error) ||
                !ReadMip(map, map.sidecars.originalMipBytes, mipSafety, result.error)) return result;
            map.sidecars.mipLoaded = true;
        }
        ec.clear();
        if (std::filesystem::exists(paths.ssi, ec))
        {
            if (!ReadFile(paths.ssi, map.sidecars.originalSsiBytes, result.error) ||
                !ReadSsi(map, map.sidecars.originalSsiBytes, ssiSafety, result.error, map.sidecars.ssiTrailingBytes)) return result;
            map.sidecars.ssiLoaded = true;
        }
        if (map.sidecars.mipLoaded && map.sidecars.ssiLoaded && mipSafety != ssiSafety)
        {
            result.error = "Legacy .mip and .ssi safety regions do not match.";
            return result;
        }
        const auto& safety = map.sidecars.ssiLoaded ? ssiSafety : mipSafety;
        if (!safety.empty())
            for (std::size_t index = 0; index < safety.size(); ++index) map.sidecars.sectors[index].safetyFlags = safety[index];

        ec.clear();
        if (std::filesystem::exists(paths.smp, ec))
        {
            if (!ReadFile(paths.smp, map.sidecars.originalSmpBytes, result.error) ||
                !ReadSmp(map, map.sidecars.originalSmpBytes, result.error)) return result;
            map.sidecars.smpLoaded = true;
        }
        map.sidecars.modified = false;
        result.succeeded = true;
        return result;
    }

    LegacyMapBundleWriteResult LegacyMapBundleWriter::Save(
        const LegacyMap& map, const std::filesystem::path& mapPath) const
    {
        LegacyMapBundleWriteResult result;
        if (mapPath.empty()) { result.error = "Map output path is empty."; return result; }
        if (!ValidateForWrite(map, result.error, result.portalDestinationsUnknown)) return result;
        const auto mapBytes = LegacyMapWriter{}.Serialize(map);
        if (!mapBytes.succeeded) { result.error = mapBytes.error; return result; }

        const auto portals = BuildPortalRects(map);
        const auto safety = BuildSafetyRects(map);
        const auto paths = LegacyMapSidecarPaths::FromMapPath(mapPath);
        const bool preserve = !map.sidecars.modified;
        std::vector<OutputFile> files;
        files.push_back({ mapPath, {}, {}, mapBytes.bytes });
        files.push_back({ paths.smp, {}, {}, preserve && map.sidecars.smpLoaded ? map.sidecars.originalSmpBytes : BuildSmp(map) });
        files.push_back({ paths.ssi, {}, {}, preserve && map.sidecars.ssiLoaded ? map.sidecars.originalSsiBytes : BuildSsi(map, safety) });
        files.push_back({ paths.mip, {}, {}, preserve && map.sidecars.mipLoaded ? map.sidecars.originalMipBytes : BuildMip(map, portals, safety) });
        if (!CommitFiles(files, result)) return result;
        result.succeeded = true;
        result.originalMapBytesPreserved = mapBytes.originalBytesPreserved;
        return result;
    }
}
