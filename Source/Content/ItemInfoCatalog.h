#pragma once

#include <cstdint>
#include <filesystem>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::content
{
    struct ItemInfo
    {
        std::uint8_t itemClass{ 0 };
        std::uint16_t itemType{ 0 };
        std::string englishName;
        std::string displayName;
        std::string description;
        std::uint16_t tileFrameId{ 0xFFFFU };
        std::uint16_t inventoryFrameId{ 0xFFFFU };
        std::uint16_t gearFrameId{ 0xFFFFU };
        std::uint16_t dropFrameId{ 0xFFFFU };
        std::uint16_t addonMaleFrameId{ 0xFFFFU };
        std::uint16_t addonFemaleFrameId{ 0xFFFFU };
        std::uint16_t useFrameId{ 0xFFFFU };
        std::uint16_t descriptionFrameId{ 0U };
        std::uint16_t useSoundId{ 0xFFFFU };
        std::uint16_t tileSoundId{ 0xFFFFU };
        std::uint16_t inventorySoundId{ 0xFFFFU };
        std::uint16_t gearSoundId{ 0xFFFFU };
        std::uint8_t gridWidth{ 1 };
        std::uint8_t gridHeight{ 1 };
        std::uint32_t price{ 0 };
        std::uint16_t weight{ 0 };
        std::array<std::int32_t, 7> values{};
        std::uint8_t requireStr{ 0 };
        std::uint8_t requireDex{ 0 };
        std::uint8_t requireInt{ 0 };
        std::uint16_t requireSum{ 0 };
        std::uint8_t requireLevel{ 0 };
        std::uint8_t requireAdvancementLevel{ 0 };
        bool maleOnly{ false };
        bool femaleOnly{ false };
        std::uint32_t useActionInfo{ 0 };
        std::int32_t silverMax{ 0 };
        std::int32_t toHit{ 0 };
        std::uint32_t maxNumber{ 1 };
        std::int32_t criticalHit{ 0 };
        std::vector<std::uint16_t> defaultOptions;
        std::int32_t itemStyle{ 0 };
        std::int32_t elementalType{ -1 };
        std::uint16_t elemental{ 0 };
        std::uint8_t race{ 0 };
        std::uint32_t itemMoveControl{ 0 };
        std::uint32_t itemCanAdvance{ 0 };
        std::uint32_t dropItemNameTag{ 0 };
        std::int32_t normalItemGrade{ 0 };
        std::int32_t requiredTranscendenceLevel{ 0 };
    };

    struct ItemInfoCatalogResult
    {
        std::map<std::uint64_t, ItemInfo> items;
        std::size_t classCount{ 0 };
        std::size_t itemCount{ 0 };
        std::wstring error;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return error.empty() && itemCount > 0;
        }

        [[nodiscard]] const ItemInfo* Find(std::uint8_t itemClass, std::uint16_t itemType) const noexcept;
    };

    class ItemInfoCatalog final
    {
    public:
        [[nodiscard]] ItemInfoCatalogResult Load(const std::filesystem::path& path) const;

        [[nodiscard]] static std::uint64_t Key(std::uint8_t itemClass, std::uint16_t itemType) noexcept;
    };
}
