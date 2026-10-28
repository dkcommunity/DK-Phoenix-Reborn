#include "Content/ItemOptionCatalog.h"

#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace phoenix::content
{
    namespace
    {
        class ItemOptionReader final
        {
        public:
            explicit ItemOptionReader(std::vector<std::uint8_t> bytes)
                : bytes_(std::move(bytes))
            {
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

        private:
            std::vector<std::uint8_t> bytes_;
            std::size_t offset_{ 0 };
        };

        [[nodiscard]] bool ReadOptionInfo(ItemOptionReader& reader, const std::uint16_t optionId, ItemOptionInfo& info)
        {
            info.optionId = optionId;
            return reader.String(info.englishName) &&
                reader.String(info.displayName) &&
                reader.I32(info.part) &&
                reader.I32(info.plusPoint) &&
                reader.I32(info.priceMultiplier) &&
                reader.I32(info.requireStr) &&
                reader.I32(info.requireDex) &&
                reader.I32(info.requireInt) &&
                reader.I32(info.requireSum) &&
                reader.I32(info.requireLevel) &&
                reader.I32(info.colorSet) &&
                reader.I32(info.upgradeOptionType) &&
                reader.I32(info.previousOptionType);
        }
    }

    const ItemOptionInfo* ItemOptionCatalogResult::Find(const std::uint16_t optionId) const noexcept
    {
        const auto it = options.find(optionId);
        return it == options.end() ? nullptr : &it->second;
    }

    ItemOptionCatalogResult ItemOptionCatalog::Load(const std::filesystem::path& path) const
    {
        ItemOptionCatalogResult result;
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            result.error = L"ItemOption.inf could not be opened.";
            return result;
        }

        std::vector<std::uint8_t> bytes(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        if (bytes.empty())
        {
            result.error = L"ItemOption.inf is empty.";
            return result;
        }

        ItemOptionReader reader(std::move(bytes));
        std::int32_t partCount = 0;
        if (!reader.I32(partCount) || partCount < 0 || partCount > 512)
        {
            result.error = L"ItemOption.inf has invalid part count.";
            return result;
        }

        result.partNames.reserve(static_cast<std::size_t>(partCount));
        for (std::int32_t i = 0; i < partCount; ++i)
        {
            ItemOptionPartName part;
            if (!reader.String(part.englishName) || !reader.String(part.displayName))
            {
                result.error = L"ItemOption.inf part name parse failed.";
                result.partNames.clear();
                return result;
            }
            result.partNames.push_back(std::move(part));
        }

        std::int32_t optionCount = 0;
        if (!reader.I32(optionCount) || optionCount < 0 || optionCount > 100000)
        {
            result.error = L"ItemOption.inf has invalid option count.";
            result.partNames.clear();
            return result;
        }

        for (std::int32_t optionId = 0; optionId < optionCount; ++optionId)
        {
            ItemOptionInfo option;
            if (!ReadOptionInfo(reader, static_cast<std::uint16_t>(optionId), option))
            {
                result.error = L"ItemOption.inf option record parse failed.";
                result.options.clear();
                return result;
            }
            result.options[option.optionId] = std::move(option);
        }

        return result;
    }
}
