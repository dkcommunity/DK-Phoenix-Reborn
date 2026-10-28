#include "Content/CreatureInfoCatalog.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace phoenix::content
{
    namespace
    {
        class CreatureTableReader final
        {
        public:
            explicit CreatureTableReader(std::vector<std::uint8_t> bytes)
                : bytes_(std::move(bytes))
            {
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

            [[nodiscard]] bool String(std::string& value)
            {
                std::int32_t length = 0;
                if (!I32(length) || length < 0 || length > 65535)
                {
                    return false;
                }
                if (offset_ + static_cast<std::size_t>(length) > bytes_.size())
                {
                    return false;
                }
                value.assign(
                    reinterpret_cast<const char*>(bytes_.data() + offset_),
                    reinterpret_cast<const char*>(bytes_.data() + offset_ + static_cast<std::size_t>(length)));
                offset_ += static_cast<std::size_t>(length);
                return true;
            }

            [[nodiscard]] bool Skip(const std::size_t size) noexcept
            {
                if (offset_ + size > bytes_.size())
                {
                    return false;
                }
                offset_ += size;
                return true;
            }

            [[nodiscard]] bool Bytes(const std::size_t size, std::vector<std::uint8_t>& value)
            {
                if (offset_ + size > bytes_.size())
                {
                    return false;
                }
                value.assign(bytes_.begin() + static_cast<std::ptrdiff_t>(offset_),
                    bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + size));
                offset_ += size;
                return true;
            }

        private:
            std::vector<std::uint8_t> bytes_;
            std::size_t offset_{ 0 };
        };

        [[nodiscard]] std::vector<std::uint8_t> ReadAllBytes(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return {};
            }

            return std::vector<std::uint8_t>(
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>());
        }

        [[nodiscard]] std::uint16_t ReadU16Le(const std::vector<std::uint8_t>& bytes, const std::size_t offset) noexcept
        {
            if (offset + 1U >= bytes.size())
            {
                return 0;
            }

            return static_cast<std::uint16_t>(bytes[offset]) |
                static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1U]) << 8U);
        }

        [[nodiscard]] CreatureWearInfo ParseCreatureWearInfo(const std::vector<std::uint8_t>& bytes) noexcept
        {
            CreatureWearInfo info;
            if (bytes.size() < 23U)
            {
                return info;
            }

            info.skinColor = ReadU16Le(bytes, 0U);
            info.hairColor = ReadU16Le(bytes, 2U);
            info.jacketColor = ReadU16Le(bytes, 4U);
            info.pantsColor = ReadU16Le(bytes, 6U);
            info.helmetColor = ReadU16Le(bytes, 8U);
            info.weaponColor = ReadU16Le(bytes, 10U);
            info.shieldColor = ReadU16Le(bytes, 12U);
            info.motorcycleColor = ReadU16Le(bytes, 14U);
            info.hair = bytes[16U];
            info.jacket = bytes[17U];
            info.pants = bytes[18U];
            info.helmet = bytes[19U];
            info.weapon = bytes[20U];
            info.shield = bytes[21U];
            info.motorcycle = bytes[22U];
            return info;
        }

        [[nodiscard]] int CreatureActionMax(const std::uint8_t tribe, const std::vector<std::uint32_t>& spriteTypes) noexcept
        {
            switch (tribe)
            {
            case 0:
            case 3:
                return 40;
            case 4:
            case 5:
                return 22;
            case 1:
                return (!spriteTypes.empty() && spriteTypes.front() == 204U) ? 22 : 17;
            default:
                return 17;
            }
        }

        [[nodiscard]] bool ReadCreatureInfo(CreatureTableReader& reader, const std::uint16_t creatureType, CreatureInfo& info)
        {
            info.creatureType = creatureType;
            std::int32_t spriteTypeCount = 0;
            std::uint8_t male = 0;
            std::uint8_t tribe = 0;
            std::uint8_t flying = 0;

            if (!reader.String(info.name) ||
                !reader.I32(spriteTypeCount) ||
                spriteTypeCount < 0 ||
                spriteTypeCount > 1000)
            {
                return false;
            }

            info.spriteTypes.reserve(static_cast<std::size_t>(spriteTypeCount));
            for (std::int32_t index = 0; index < spriteTypeCount; ++index)
            {
                std::uint32_t spriteType = 0;
                if (!reader.U32(spriteType))
                {
                    return false;
                }
                info.spriteTypes.push_back(spriteType);
            }

            if (!reader.U8(male) ||
                !reader.U8(tribe) ||
                !reader.U8(info.moveTimes) ||
                !reader.U8(info.moveRatio) ||
                !reader.U8(info.moveTimesMotor) ||
                !reader.I32(info.height) ||
                !reader.I32(info.width) ||
                !reader.I32(info.deadHeight) ||
                !reader.U16(info.reservedAfterDeadHeight) ||
                !reader.I32(info.colorSet) ||
                !reader.U8(flying) ||
                !reader.I32(info.reservedAfterFlying) ||
                !reader.U32(info.reservedBeforeHpBar) ||
                !reader.I32(info.hpBarWidth) ||
                !reader.U16(info.changeColorSet) ||
                !reader.U16(info.shadowCount) ||
                !reader.I32(info.reservedBeforeLevel) ||
                !reader.I32(info.level))
            {
                return false;
            }

            info.male = male != 0U;
            info.tribe = tribe;
            info.flying = flying != 0U;
            const auto actionMax = CreatureActionMax(tribe, info.spriteTypes);
            info.actionSoundIds.clear();
            info.actionSoundIds.reserve(actionMax);
            for (std::size_t actionIndex = 0; actionIndex < actionMax; ++actionIndex)
            {
                std::uint16_t actionSoundId = 0;
                if (!reader.U16(actionSoundId))
                {
                    return false;
                }
                info.actionSoundIds.push_back(actionSoundId);
            }

            info.actionFrameCounts.clear();
            info.actionFrameCounts.reserve(actionMax);
            for (std::size_t actionIndex = 0; actionIndex < actionMax; ++actionIndex)
            {
                std::int32_t frameCount = 0;
                if (!reader.I32(frameCount))
                {
                    return false;
                }
                info.actionFrameCounts.push_back(static_cast<std::uint32_t>(std::max(frameCount, 0)));
            }

            std::uint8_t hasWearInfo = 0;
            if (!reader.U8(hasWearInfo))
            {
                return false;
            }
            info.hasWearInfo = hasWearInfo != 0U;
            info.wearInfoRaw.clear();
            if (info.hasWearInfo && !reader.Bytes(23U, info.wearInfoRaw))
            {
                return false;
            }
            info.wearInfo = info.hasWearInfo ? ParseCreatureWearInfo(info.wearInfoRaw) : CreatureWearInfo{};

            std::uint8_t fade = 0;
            std::uint8_t fadeShadow = 0;
            if (!reader.U8(fade) ||
                !reader.U8(fadeShadow) ||
                !reader.I32(info.effectSpriteType) ||
                !reader.U32(info.reservedTail))
            {
                return false;
            }

            info.fade = fade != 0U;
            info.fadeShadow = fadeShadow != 0U;
            return true;
        }

        [[nodiscard]] bool ReadCreatureSpriteInfo(
            CreatureTableReader& reader,
            const std::uint16_t spriteType,
            CreatureSpriteInfo& info)
        {
            info.spriteType = spriteType;
            return reader.U16(info.frameId) &&
                reader.I32(info.spriteFilePosition) &&
                reader.I32(info.spriteShadowFilePosition) &&
                reader.U16(info.firstSpriteId) &&
                reader.U16(info.lastSpriteId) &&
                reader.U16(info.firstShadowSpriteId) &&
                reader.U16(info.lastShadowSpriteId) &&
                reader.U8(info.flags);
        }

        [[nodiscard]] bool ReadCreatureSpriteMapper(
            CreatureTableReader& reader,
            std::vector<std::vector<std::uint16_t>>& creatureTypesBySpriteType)
        {
            std::int32_t spriteTypeCount = 0;
            if (!reader.I32(spriteTypeCount) || spriteTypeCount <= 0 || spriteTypeCount > 100000)
            {
                return false;
            }

            creatureTypesBySpriteType.clear();
            creatureTypesBySpriteType.resize(static_cast<std::size_t>(spriteTypeCount));
            for (std::int32_t spriteType = 0; spriteType < spriteTypeCount; ++spriteType)
            {
                std::int32_t creatureCount = 0;
                if (!reader.I32(creatureCount) || creatureCount < 0 || creatureCount > 100000)
                {
                    return false;
                }

                auto& creatureTypes = creatureTypesBySpriteType[static_cast<std::size_t>(spriteType)];
                creatureTypes.reserve(static_cast<std::size_t>(creatureCount));
                for (std::int32_t index = 0; index < creatureCount; ++index)
                {
                    std::uint16_t creatureType = 0;
                    if (!reader.U16(creatureType))
                    {
                        return false;
                    }
                    creatureTypes.push_back(creatureType);
                }
            }

            return true;
        }
    }

    const CreatureInfo* CreatureInfoCatalogResult::FindCreature(const std::uint16_t creatureType) const noexcept
    {
        return creatureType < creatures.size() ? &creatures[creatureType] : nullptr;
    }

    const CreatureSpriteInfo* CreatureInfoCatalogResult::FindSprite(const std::uint32_t spriteType) const noexcept
    {
        return spriteType < sprites.size() ? &sprites[static_cast<std::size_t>(spriteType)] : nullptr;
    }

    const std::vector<std::uint16_t>* CreatureInfoCatalogResult::FindCreatureTypesForSprite(
        const std::uint32_t spriteType) const noexcept
    {
        return spriteType < creatureTypesBySpriteType.size()
            ? &creatureTypesBySpriteType[static_cast<std::size_t>(spriteType)]
            : nullptr;
    }

    std::optional<std::uint16_t> CreatureInfoCatalogResult::ResolveCreatureTypeFromSprite(
        const std::uint32_t spriteType) const noexcept
    {
        const auto* creatureTypes = FindCreatureTypesForSprite(spriteType);
        if (creatureTypes == nullptr || creatureTypes->empty())
        {
            return std::nullopt;
        }
        return creatureTypes->front();
    }

    std::optional<std::uint16_t> CreatureInfoCatalogResult::ResolvePrimaryFrameId(const std::uint16_t creatureType) const noexcept
    {
        const auto* creature = FindCreature(creatureType);
        if (creature == nullptr || creature->spriteTypes.empty())
        {
            return std::nullopt;
        }

        const auto* sprite = FindSprite(creature->spriteTypes.front());
        if (sprite == nullptr || sprite->frameId == 0xFFFFU)
        {
            return std::nullopt;
        }

        return sprite->frameId;
    }

    std::vector<std::uint16_t> CreatureInfoCatalogResult::ResolveFrameIds(const std::uint16_t creatureType) const
    {
        std::vector<std::uint16_t> frameIds;
        const auto* creature = FindCreature(creatureType);
        if (creature == nullptr || creature->spriteTypes.empty())
        {
            return frameIds;
        }

        frameIds.reserve(creature->spriteTypes.size());
        for (const auto spriteType : creature->spriteTypes)
        {
            const auto* sprite = FindSprite(spriteType);
            if (sprite != nullptr && sprite->frameId != 0xFFFFU)
            {
                frameIds.push_back(sprite->frameId);
            }
        }
        return frameIds;
    }

    std::uint32_t CreatureInfoCatalogResult::ResolveActionFrameCount(
        const std::uint16_t creatureType,
        const std::uint32_t action) const noexcept
    {
        const auto* creature = FindCreature(creatureType);
        if (creature == nullptr || action >= creature->actionFrameCounts.size())
        {
            return 0U;
        }
        return creature->actionFrameCounts[static_cast<std::size_t>(action)];
    }

    std::uint16_t CreatureInfoCatalogResult::ResolveShadowCount(const std::uint16_t creatureType) const noexcept
    {
        const auto* creature = FindCreature(creatureType);
        return creature != nullptr ? creature->shadowCount : 0U;
    }

    CreatureInfoCatalogResult CreatureInfoCatalog::Load(const std::filesystem::path& infoRoot) const
    {
        return Load(
            infoRoot / L"creature.inf",
            infoRoot / L"creaturesprite.inf",
            infoRoot / L"creaturespritemapper.inf");
    }

    CreatureInfoCatalogResult CreatureInfoCatalog::Load(
        const std::filesystem::path& creaturePath,
        const std::filesystem::path& creatureSpritePath,
        const std::filesystem::path& creatureSpriteMapperPath) const
    {
        CreatureInfoCatalogResult result;

        const auto creatureBytes = ReadAllBytes(creaturePath);
        if (creatureBytes.empty())
        {
            result.error = L"creature.inf could not be opened or is empty: " + creaturePath.wstring();
            return result;
        }

        CreatureTableReader creatureReader(creatureBytes);
        std::int32_t creatureCount = 0;
        if (!creatureReader.I32(creatureCount) || creatureCount <= 0 || creatureCount > 100000)
        {
            result.error = L"creature.inf has invalid table size.";
            return result;
        }

        result.creatures.reserve(static_cast<std::size_t>(creatureCount));
        for (std::int32_t creatureType = 0; creatureType < creatureCount; ++creatureType)
        {
            CreatureInfo info;
            if (!ReadCreatureInfo(creatureReader, static_cast<std::uint16_t>(creatureType), info))
            {
                result.error = L"creature.inf record parse failed at creature type " + std::to_wstring(creatureType) + L".";
                result.creatures.clear();
                return result;
            }
            result.creatures.push_back(std::move(info));
        }

        const auto spriteBytes = ReadAllBytes(creatureSpritePath);
        if (spriteBytes.empty())
        {
            result.error = L"creaturesprite.inf could not be opened or is empty: " + creatureSpritePath.wstring();
            result.creatures.clear();
            return result;
        }

        CreatureTableReader spriteReader(spriteBytes);
        std::int32_t spriteCount = 0;
        if (!spriteReader.I32(spriteCount) || spriteCount <= 0 || spriteCount > 100000)
        {
            result.error = L"creaturesprite.inf has invalid table size.";
            result.creatures.clear();
            return result;
        }

        result.sprites.reserve(static_cast<std::size_t>(spriteCount));
        for (std::int32_t spriteType = 0; spriteType < spriteCount; ++spriteType)
        {
            CreatureSpriteInfo info;
            if (!ReadCreatureSpriteInfo(spriteReader, static_cast<std::uint16_t>(spriteType), info))
            {
                result.error = L"creaturesprite.inf record parse failed at sprite type " + std::to_wstring(spriteType) + L".";
                result.creatures.clear();
                result.sprites.clear();
                return result;
            }
            result.sprites.push_back(info);
        }

        const auto mapperBytes = ReadAllBytes(creatureSpriteMapperPath);
        if (mapperBytes.empty())
        {
            result.error = L"creaturespritemapper.inf could not be opened or is empty: " + creatureSpriteMapperPath.wstring();
            result.creatures.clear();
            result.sprites.clear();
            return result;
        }

        CreatureTableReader mapperReader(mapperBytes);
        if (!ReadCreatureSpriteMapper(mapperReader, result.creatureTypesBySpriteType))
        {
            result.error = L"creaturespritemapper.inf record parse failed.";
            result.creatures.clear();
            result.sprites.clear();
            result.creatureTypesBySpriteType.clear();
            return result;
        }

        for (const auto& creature : result.creatures)
        {
            if (creature.name.empty())
            {
                result.issues.push_back(CreatureInfoCatalogResult::Issue{
                    creature.creatureType,
                    creature.name,
                    0xFFFFFFFFU,
                    0xFFFFFFFFU,
                    L"name",
                    L"Creature name is empty."
                });
            }
            if (creature.spriteTypes.empty())
            {
                result.issues.push_back(CreatureInfoCatalogResult::Issue{
                    creature.creatureType,
                    creature.name,
                    0xFFFFFFFFU,
                    0xFFFFFFFFU,
                    L"spriteTypes",
                    L"Creature has no sprite references."
                });
            }
            for (const auto spriteType : creature.spriteTypes)
            {
                if (spriteType >= result.sprites.size())
                {
                    result.issues.push_back(CreatureInfoCatalogResult::Issue{
                        creature.creatureType,
                        creature.name,
                        0xFFFFFFFFU,
                        0xFFFFFFFFU,
                        L"spriteType",
                        L"Sprite reference is outside creaturesprite.inf: " + std::to_wstring(spriteType)
                    });
                }
            }
            if (creature.actionSoundIds.size() != creature.actionFrameCounts.size())
            {
                result.issues.push_back(CreatureInfoCatalogResult::Issue{
                    creature.creatureType,
                    creature.name,
                    0xFFFFFFFFU,
                    0xFFFFFFFFU,
                    L"actionTable",
                    L"Action sound/count table lengths differ."
                });
            }
        }

        for (std::size_t spriteType = 0U; spriteType < result.creatureTypesBySpriteType.size(); ++spriteType)
        {
            for (const auto creatureType : result.creatureTypesBySpriteType[spriteType])
            {
                if (creatureType >= result.creatures.size())
                {
                    result.issues.push_back(CreatureInfoCatalogResult::Issue{
                        creatureType,
                        {},
                        0xFFFFFFFFU,
                        0xFFFFFFFFU,
                        L"spriteMapper",
                        L"Mapper references a creature outside creature.inf from sprite " + std::to_wstring(spriteType)
                    });
                }
            }
        }

        return result;
    }
}
