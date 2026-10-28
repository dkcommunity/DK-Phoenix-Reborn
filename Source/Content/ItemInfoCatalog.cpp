#include "Content/ItemInfoCatalog.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace phoenix::content
{
    namespace
    {
        class ItemTableReader final
        {
        public:
            explicit ItemTableReader(std::vector<std::uint8_t> bytes)
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

        private:
            std::vector<std::uint8_t> bytes_;
            std::size_t offset_{ 0 };
        };

        [[nodiscard]] bool ReadItemInfo(ItemTableReader& reader, const std::uint8_t itemClass, const std::uint16_t itemType, ItemInfo& item)
        {
            item.itemClass = itemClass;
            item.itemType = itemType;
            if (!reader.String(item.englishName) ||
                !reader.String(item.displayName) ||
                !reader.String(item.description) ||
                !reader.U16(item.tileFrameId) ||
                !reader.U16(item.inventoryFrameId) ||
                !reader.U16(item.gearFrameId) ||
                !reader.U16(item.dropFrameId) ||
                !reader.U16(item.addonMaleFrameId) ||
                !reader.U16(item.addonFemaleFrameId) ||
                !reader.U16(item.useFrameId) ||
                !reader.U16(item.useSoundId) ||
                !reader.U16(item.tileSoundId) ||
                !reader.U16(item.inventorySoundId) ||
                !reader.U16(item.gearSoundId) ||
                !reader.U8(item.gridWidth) ||
                !reader.U8(item.gridHeight) ||
                !reader.U32(item.price) ||
                !reader.U16(item.weight))
            {
                return false;
            }

            for (auto& value : item.values)
            {
                if (!reader.I32(value))
                {
                    return false;
                }
            }

            std::uint8_t maleOnly = 0;
            std::uint8_t femaleOnly = 0;
            if (!reader.U8(item.requireStr) ||
                !reader.U8(item.requireDex) ||
                !reader.U8(item.requireInt) ||
                !reader.U16(item.requireSum) ||
                !reader.U8(item.requireLevel) ||
                !reader.U8(item.requireAdvancementLevel) ||
                !reader.U8(maleOnly) ||
                !reader.U8(femaleOnly) ||
                !reader.U32(item.useActionInfo) ||
                !reader.I32(item.silverMax) ||
                !reader.I32(item.toHit) ||
                !reader.U32(item.maxNumber) ||
                !reader.I32(item.criticalHit))
            {
                return false;
            }
            item.maleOnly = maleOnly != 0;
            item.femaleOnly = femaleOnly != 0;

            std::uint8_t defaultOptionCount = 0;
            if (!reader.U8(defaultOptionCount))
            {
                return false;
            }
            item.defaultOptions.clear();
            item.defaultOptions.reserve(defaultOptionCount);
            for (std::uint8_t i = 0; i < defaultOptionCount; ++i)
            {
                std::uint16_t option = 0;
                if (!reader.U16(option))
                {
                    return false;
                }
                item.defaultOptions.push_back(option);
            }

            if (!reader.I32(item.itemStyle) ||
                !reader.I32(item.elementalType) ||
                !reader.U16(item.elemental) ||
                !reader.U8(item.race) ||
                !reader.U16(item.descriptionFrameId) ||
                !reader.U32(item.itemMoveControl) ||
                !reader.U32(item.itemCanAdvance) ||
                !reader.U32(item.dropItemNameTag) ||
                !reader.I32(item.normalItemGrade) ||
                !reader.I32(item.requiredTranscendenceLevel))
            {
                return false;
            }

            item.gridWidth = std::max<std::uint8_t>(1U, item.gridWidth);
            item.gridHeight = std::max<std::uint8_t>(1U, item.gridHeight);
            item.maxNumber = std::max<std::uint32_t>(1U, item.maxNumber);
            return true;
        }
    }

    const ItemInfo* ItemInfoCatalogResult::Find(const std::uint8_t itemClass, const std::uint16_t itemType) const noexcept
    {
        const auto it = items.find(ItemInfoCatalog::Key(itemClass, itemType));
        return it == items.end() ? nullptr : &it->second;
    }

    std::uint64_t ItemInfoCatalog::Key(const std::uint8_t itemClass, const std::uint16_t itemType) noexcept
    {
        return (static_cast<std::uint64_t>(itemClass) << 32U) | itemType;
    }

    ItemInfoCatalogResult ItemInfoCatalog::Load(const std::filesystem::path& path) const
    {
        ItemInfoCatalogResult result;
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            result.error = L"item.inf could not be opened.";
            return result;
        }

        std::vector<std::uint8_t> bytes(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        if (bytes.empty())
        {
            result.error = L"item.inf is empty.";
            return result;
        }

        ItemTableReader reader(std::move(bytes));
        std::int32_t classCount = 0;
        if (!reader.I32(classCount) || classCount <= 0 || classCount > 255)
        {
            result.error = L"item.inf has invalid class count.";
            return result;
        }

        result.classCount = static_cast<std::size_t>(classCount);
        for (std::int32_t itemClass = 0; itemClass < classCount; ++itemClass)
        {
            std::int32_t typeCount = 0;
            if (!reader.I32(typeCount) || typeCount < 0 || typeCount > 100000)
            {
                result.error = L"item.inf has invalid type count.";
                result.items.clear();
                result.itemCount = 0;
                return result;
            }

            for (std::int32_t itemType = 0; itemType < typeCount; ++itemType)
            {
                ItemInfo item;
                if (!ReadItemInfo(
                        reader,
                        static_cast<std::uint8_t>(itemClass),
                        static_cast<std::uint16_t>(itemType),
                        item))
                {
                    result.error = L"item.inf item record parse failed.";
                    result.items.clear();
                    result.itemCount = 0;
                    return result;
                }
                result.items[Key(item.itemClass, item.itemType)] = std::move(item);
                ++result.itemCount;
            }
        }

        return result;
    }
}
