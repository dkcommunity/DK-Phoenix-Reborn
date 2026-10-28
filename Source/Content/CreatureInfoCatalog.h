#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::content
{
    enum CreatureSpriteFlag : std::uint8_t
    {
        CreatureSpriteSlayer = 0x01,
        CreatureSpriteVampire = 0x02,
        CreatureSpriteOusters = 0x04,
        CreatureSpritePlayer = 0x10,
        CreatureSpriteNpc = 0x20,
        CreatureSpriteMonster = 0x40
    };

    struct CreatureSpriteInfo
    {
        std::uint16_t spriteType{ 0 };
        std::uint16_t frameId{ 0xFFFFU };
        std::int32_t spriteFilePosition{ 0 };
        std::int32_t spriteShadowFilePosition{ 0 };
        std::uint16_t firstSpriteId{ 0 };
        std::uint16_t lastSpriteId{ 0 };
        std::uint16_t firstShadowSpriteId{ 0 };
        std::uint16_t lastShadowSpriteId{ 0 };
        std::uint8_t flags{ 0 };

        [[nodiscard]] bool IsNpcSprite() const noexcept
        {
            return (flags & CreatureSpriteNpc) != 0U;
        }

        [[nodiscard]] bool IsMonsterSprite() const noexcept
        {
            return (flags & CreatureSpriteMonster) != 0U;
        }
    };

    struct CreatureWearInfo
    {
        std::uint16_t skinColor{ 0 };
        std::uint16_t hairColor{ 0 };
        std::uint16_t jacketColor{ 0 };
        std::uint16_t pantsColor{ 0 };
        std::uint16_t helmetColor{ 0 };
        std::uint16_t weaponColor{ 0 };
        std::uint16_t shieldColor{ 0 };
        std::uint16_t motorcycleColor{ 0 };
        std::uint8_t hair{ 0 };
        std::uint8_t jacket{ 0 };
        std::uint8_t pants{ 0 };
        std::uint8_t helmet{ 0 };
        std::uint8_t weapon{ 0 };
        std::uint8_t shield{ 0 };
        std::uint8_t motorcycle{ 0 };
    };

    struct CreatureInfo
    {
        std::uint16_t creatureType{ 0 };
        std::string name;
        std::vector<std::uint32_t> spriteTypes;
        std::uint8_t tribe{ 0 };
        bool male{ true };
        std::uint8_t moveTimes{ 0 };
        std::uint8_t moveRatio{ 0 };
        std::uint8_t moveTimesMotor{ 0 };
        std::int32_t height{ 0 };
        std::int32_t width{ 0 };
        std::int32_t deadHeight{ 0 };
        std::uint16_t reservedAfterDeadHeight{ 0 };
        std::int32_t colorSet{ 0 };
        bool flying{ false };
        std::int32_t reservedAfterFlying{ 0 };
        std::uint32_t reservedBeforeHpBar{ 0 };
        std::int32_t hpBarWidth{ 0 };
        std::uint16_t changeColorSet{ 0xFFFFU };
        std::uint16_t shadowCount{ 0 };
        std::int32_t reservedBeforeLevel{ 0 };
        std::int32_t level{ 0 };
        std::int32_t effectSpriteType{ 0 };
        bool fade{ false };
        bool fadeShadow{ false };
        bool hasWearInfo{ false };
        CreatureWearInfo wearInfo;
        std::uint32_t reservedTail{ 0 };
        std::vector<std::uint16_t> actionSoundIds;
        std::vector<std::uint32_t> actionFrameCounts;
        std::vector<std::uint8_t> wearInfoRaw;
    };

    struct CreatureInfoCatalogResult
    {
        struct Issue
        {
            std::uint16_t creatureType{ 0U };
            std::string creatureName;
            std::uint32_t action{ 0xFFFFFFFFU };
            std::uint32_t direction{ 0xFFFFFFFFU };
            std::wstring field;
            std::wstring detail;
        };

        std::vector<CreatureInfo> creatures;
        std::vector<CreatureSpriteInfo> sprites;
        std::vector<std::vector<std::uint16_t>> creatureTypesBySpriteType;
        std::vector<Issue> issues;
        std::wstring error;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return error.empty() && !creatures.empty() && !sprites.empty() && !creatureTypesBySpriteType.empty();
        }

        [[nodiscard]] const CreatureInfo* FindCreature(std::uint16_t creatureType) const noexcept;
        [[nodiscard]] const CreatureSpriteInfo* FindSprite(std::uint32_t spriteType) const noexcept;
        [[nodiscard]] const std::vector<std::uint16_t>* FindCreatureTypesForSprite(std::uint32_t spriteType) const noexcept;
        [[nodiscard]] std::optional<std::uint16_t> ResolveCreatureTypeFromSprite(std::uint32_t spriteType) const noexcept;
        [[nodiscard]] std::optional<std::uint16_t> ResolvePrimaryFrameId(std::uint16_t creatureType) const noexcept;
        [[nodiscard]] std::vector<std::uint16_t> ResolveFrameIds(std::uint16_t creatureType) const;
        [[nodiscard]] std::uint32_t ResolveActionFrameCount(std::uint16_t creatureType, std::uint32_t action) const noexcept;
        [[nodiscard]] std::uint16_t ResolveShadowCount(std::uint16_t creatureType) const noexcept;
    };

    class CreatureInfoCatalog final
    {
    public:
        [[nodiscard]] CreatureInfoCatalogResult Load(const std::filesystem::path& infoRoot) const;
        [[nodiscard]] CreatureInfoCatalogResult Load(
            const std::filesystem::path& creaturePath,
            const std::filesystem::path& creatureSpritePath,
            const std::filesystem::path& creatureSpriteMapperPath) const;
    };
}
