#include "Protocol/LegacyProtocol.h"

#include "LegacyPacket/Bridge/LegacyPacketBridge.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <utility>

namespace phoenix::protocol
{
    namespace
    {
        class PacketWriter final
        {
        public:
            void U8(const std::uint8_t value)
            {
                bytes_.push_back(value);
            }

            void U16(const std::uint16_t value)
            {
                bytes_.push_back(static_cast<std::uint8_t>(value & 0xFFU));
                bytes_.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
            }

            void U32(const std::uint32_t value)
            {
                bytes_.push_back(static_cast<std::uint8_t>(value & 0xFFU));
                bytes_.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
                bytes_.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
                bytes_.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
            }

            void Bytes(const std::uint8_t* data, const std::size_t size)
            {
                bytes_.insert(bytes_.end(), data, data + size);
            }

            void String8(const std::string& text, const std::size_t maxLength)
            {
                const auto size = static_cast<std::uint8_t>(std::min<std::size_t>(text.size(), maxLength));
                U8(size);
                bytes_.insert(bytes_.end(), text.begin(), text.begin() + size);
            }

            [[nodiscard]] const std::vector<std::uint8_t>& Bytes() const noexcept
            {
                return bytes_;
            }

        private:
            std::vector<std::uint8_t> bytes_;
        };

        class PacketReader final
        {
        public:
            explicit PacketReader(const std::vector<std::uint8_t>& bytes)
                : bytes_(bytes)
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

            [[nodiscard]] bool String8(std::string& text, const std::uint8_t maxLength) noexcept
            {
                std::uint8_t size = 0;
                if (!U8(size) || size == 0 || size > maxLength || offset_ + size > bytes_.size())
                {
                    return false;
                }
                text.assign(reinterpret_cast<const char*>(bytes_.data() + offset_), reinterpret_cast<const char*>(bytes_.data() + offset_ + size));
                offset_ += size;
                return true;
            }

            [[nodiscard]] bool String8AllowEmpty(std::string& text, const std::uint8_t maxLength) noexcept
            {
                std::uint8_t size = 0;
                if (!U8(size) || size > maxLength || offset_ + size > bytes_.size())
                {
                    return false;
                }
                text.assign(reinterpret_cast<const char*>(bytes_.data() + offset_), reinterpret_cast<const char*>(bytes_.data() + offset_ + size));
                offset_ += size;
                return true;
            }

            [[nodiscard]] bool String16(std::string& text, const std::uint16_t maxLength) noexcept
            {
                std::uint16_t size = 0;
                if (!U16(size) || size == 0 || size > maxLength || offset_ + size > bytes_.size())
                {
                    return false;
                }
                text.assign(reinterpret_cast<const char*>(bytes_.data() + offset_), reinterpret_cast<const char*>(bytes_.data() + offset_ + size));
                offset_ += size;
                return true;
            }

            [[nodiscard]] bool String16AllowEmpty(std::string& text, const std::uint16_t maxLength) noexcept
            {
                std::uint16_t size = 0;
                if (!U16(size) || size > maxLength || offset_ + size > bytes_.size())
                {
                    return false;
                }
                text.assign(reinterpret_cast<const char*>(bytes_.data() + offset_), reinterpret_cast<const char*>(bytes_.data() + offset_ + size));
                offset_ += size;
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

            [[nodiscard]] std::size_t Offset() const noexcept
            {
                return offset_;
            }

            [[nodiscard]] bool AtEnd() const noexcept
            {
                return offset_ == bytes_.size();
            }

            [[nodiscard]] bool Seek(const std::size_t offset) noexcept
            {
                if (offset > bytes_.size())
                {
                    return false;
                }
                offset_ = offset;
                return true;
            }

            [[nodiscard]] std::size_t Remaining() const noexcept
            {
                return bytes_.size() - offset_;
            }

            [[nodiscard]] std::vector<std::uint8_t> RawFrom(const std::size_t start) const
            {
                if (start > offset_ || offset_ > bytes_.size())
                {
                    return {};
                }
                return std::vector<std::uint8_t>(bytes_.begin() + static_cast<std::ptrdiff_t>(start), bytes_.begin() + static_cast<std::ptrdiff_t>(offset_));
            }

        private:
            const std::vector<std::uint8_t>& bytes_;
            std::size_t offset_{ 0 };
        };

        enum class LegacyEncryptedFieldType : std::uint8_t
        {
            U8,
            U16,
            U32
        };

        struct LegacyEncryptedField final
        {
            LegacyEncryptedFieldType type;
        };

        [[nodiscard]] bool ReadI16(PacketReader& reader, std::int16_t& value) noexcept
        {
            std::uint16_t raw = 0;
            if (!reader.U16(raw))
            {
                return false;
            }
            value = static_cast<std::int16_t>(raw);
            return true;
        }

        [[nodiscard]] bool ReadI32(PacketReader& reader, std::int32_t& value) noexcept
        {
            std::uint32_t raw = 0;
            if (!reader.U32(raw))
            {
                return false;
            }
            value = static_cast<std::int32_t>(raw);
            return true;
        }

        [[nodiscard]] bool ReadShopItemCore(PacketReader& reader, ShopItemInfo& item)
        {
            std::uint8_t optionCount = 0;
            if (!reader.U32(item.objectId) ||
                !reader.U8(item.itemClass) ||
                !reader.U16(item.itemType) ||
                !reader.U8(optionCount))
            {
                return false;
            }

            item.options.clear();
            item.options.reserve(optionCount);
            for (std::uint8_t index = 0; index < optionCount; ++index)
            {
                std::uint16_t optionType = 0;
                if (!reader.U16(optionType))
                {
                    return false;
                }
                item.options.push_back(optionType);
            }

            if (!reader.U32(item.durability) ||
                !reader.U16(item.silver) ||
                !reader.U32(item.grade) ||
                !reader.U8(item.enchantLevel))
            {
                return false;
            }

            item.present = true;
            return true;
        }

        [[nodiscard]] bool ReadShopRackItem(PacketReader& reader, ShopItemInfo& item)
        {
            std::uint8_t rackIndex = 0;
            if (!reader.U8(rackIndex))
            {
                return false;
            }
            item.rackIndex = rackIndex;
            return ReadShopItemCore(reader, item);
        }

        [[nodiscard]] bool ReadMysteriousShopRackItem(PacketReader& reader, ShopItemInfo& item)
        {
            if (!reader.U8(item.rackIndex) ||
                !reader.U8(item.itemClass) ||
                !reader.U16(item.itemType))
            {
                return false;
            }
            item.present = true;
            item.mysterious = true;
            return true;
        }

        [[nodiscard]] bool ReadModifyInfo(PacketReader& reader, ModifyInfoRecord& info)
        {
            const auto start = reader.Offset();
            std::uint8_t shortCount = 0;
            if (!reader.U8(shortCount))
            {
                return false;
            }

            info.shorts.clear();
            info.shorts.reserve(shortCount);
            for (std::uint8_t index = 0; index < shortCount; ++index)
            {
                ModifyShortData data;
                if (!reader.U8(data.type) || !reader.U16(data.value))
                {
                    return false;
                }
                info.shorts.push_back(data);
            }

            std::uint8_t longCount = 0;
            if (!reader.U8(longCount))
            {
                return false;
            }

            info.longs.clear();
            info.longs.reserve(longCount);
            for (std::uint8_t index = 0; index < longCount; ++index)
            {
                ModifyLongData data;
                if (!reader.U8(data.type) || !reader.U32(data.value))
                {
                    return false;
                }
                info.longs.push_back(data);
            }

            info.raw = reader.RawFrom(start);
            return true;
        }

        [[nodiscard]] std::size_t LegacyEncryptedFieldSize(const LegacyEncryptedFieldType type) noexcept
        {
            switch (type)
            {
            case LegacyEncryptedFieldType::U16:
                return 2U;
            case LegacyEncryptedFieldType::U32:
                return 4U;
            default:
                return 1U;
            }
        }

        [[nodiscard]] std::uint32_t ReadLegacyEncryptedValue(
            const std::vector<std::uint8_t>& bytes,
            std::size_t& offset,
            const LegacyEncryptedFieldType type) noexcept
        {
            switch (type)
            {
            case LegacyEncryptedFieldType::U16:
            {
                const auto value = static_cast<std::uint16_t>(bytes[offset]) |
                    static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1U]) << 8U);
                offset += 2U;
                return value;
            }
            case LegacyEncryptedFieldType::U32:
            {
                const auto value = static_cast<std::uint32_t>(bytes[offset]) |
                    (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
                    (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
                    (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
                offset += 4U;
                return value;
            }
            default:
                return bytes[offset++];
            }
        }

        void WriteLegacyEncryptedValue(
            std::vector<std::uint8_t>& bytes,
            const LegacyEncryptedFieldType type,
            const std::uint32_t value)
        {
            switch (type)
            {
            case LegacyEncryptedFieldType::U16:
                bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
                bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
                break;
            case LegacyEncryptedFieldType::U32:
                bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
                bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
                bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
                bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
                break;
            default:
                bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
                break;
            }
        }

        [[nodiscard]] std::uint32_t ApplyLegacyEncryptedValueCode(
            const std::uint32_t value,
            const LegacyEncryptedFieldType type,
            const std::uint8_t code) noexcept
        {
            switch (type)
            {
            case LegacyEncryptedFieldType::U16:
                return static_cast<std::uint16_t>(static_cast<std::uint16_t>(value) ^ static_cast<std::uint16_t>(code));
            case LegacyEncryptedFieldType::U32:
                return value ^ static_cast<std::uint32_t>(code);
            default:
                return static_cast<std::uint8_t>(static_cast<std::uint8_t>(value) ^ code);
            }
        }

        [[nodiscard]] std::array<std::size_t, 5> LegacyShuffleOrder(
            const std::size_t count,
            const std::uint8_t code) noexcept
        {
            std::array<std::size_t, 5> order{ 0, 1, 2, 3, 4 };
            switch (count)
            {
            case 2:
                if (code % 2U == 1U)
                {
                    order = { 1, 0, 2, 3, 4 };
                }
                break;
            case 3:
                switch (code % 3U)
                {
                case 1:
                    order = { 1, 2, 0, 3, 4 };
                    break;
                case 2:
                    order = { 2, 0, 1, 3, 4 };
                    break;
                default:
                    break;
                }
                break;
            case 4:
                switch (code % 4U)
                {
                case 1:
                    order = { 1, 2, 3, 0, 4 };
                    break;
                case 2:
                    order = { 2, 3, 0, 1, 4 };
                    break;
                case 3:
                    order = { 3, 0, 2, 1, 4 };
                    break;
                default:
                    break;
                }
                break;
            case 5:
                switch (code % 5U)
                {
                case 1:
                    order = { 1, 2, 3, 4, 0 };
                    break;
                case 2:
                    order = { 2, 3, 4, 0, 1 };
                    break;
                case 3:
                    order = { 3, 4, 1, 0, 2 };
                    break;
                case 4:
                    order = { 4, 2, 3, 0, 1 };
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
            return order;
        }

        [[nodiscard]] bool TransformLegacyEncryptedFields(
            std::vector<std::uint8_t>& body,
            const std::initializer_list<LegacyEncryptedField> fields,
            const std::uint8_t code,
            const bool incoming)
        {
            if (code == 0U)
            {
                return true;
            }

            const auto fieldCount = fields.size();
            if (fieldCount < 2U || fieldCount > 5U)
            {
                return false;
            }

            std::array<LegacyEncryptedField, 5> fieldArray{};
            std::size_t expectedSize = 0;
            std::size_t index = 0;
            for (const auto field : fields)
            {
                fieldArray[index++] = field;
                expectedSize += LegacyEncryptedFieldSize(field.type);
            }
            if (body.size() != expectedSize)
            {
                return false;
            }

            std::array<std::uint32_t, 5> values{};
            std::size_t offset = 0;
            const auto order = LegacyShuffleOrder(fieldCount, code);

            if (incoming)
            {
                for (std::size_t i = 0; i < fieldCount; ++i)
                {
                    const auto logicalIndex = order[i];
                    const auto type = fieldArray[logicalIndex].type;
                    values[logicalIndex] = ApplyLegacyEncryptedValueCode(
                        ReadLegacyEncryptedValue(body, offset, type),
                        type,
                        code);
                }
            }
            else
            {
                for (std::size_t i = 0; i < fieldCount; ++i)
                {
                    const auto type = fieldArray[i].type;
                    values[i] = ReadLegacyEncryptedValue(body, offset, type);
                }
            }

            std::vector<std::uint8_t> transformed;
            transformed.reserve(body.size());
            if (incoming)
            {
                for (std::size_t i = 0; i < fieldCount; ++i)
                {
                    WriteLegacyEncryptedValue(transformed, fieldArray[i].type, values[i]);
                }
            }
            else
            {
                for (std::size_t i = 0; i < fieldCount; ++i)
                {
                    const auto logicalIndex = order[i];
                    const auto type = fieldArray[logicalIndex].type;
                    WriteLegacyEncryptedValue(transformed, type, ApplyLegacyEncryptedValueCode(values[logicalIndex], type, code));
                }
            }

            body = std::move(transformed);
            return true;
        }

        constexpr std::size_t kSlayerInfoFixedAfterSlot =
            4U +     // Alignment_t
            2U * 3U + // STR/DEX/INT
            1U +     // Rank
            4U * 3U + // STR/DEX/INT exp
            4U * 2U + // HP current/max
            4U * 2U + // MP current/max
            4U +     // Fame
            1U * 6U + // Skill domain levels
            4U +     // Outlook DWORD
            2U * 7U + // Colors
            2U;      // AdvancementLevel

        constexpr std::size_t kVampireInfoFixedAfterSlot =
            4U +      // Alignment_t
            1U +      // Sex
            2U * 2U + // Bat/Skin color
            1U + 2U + // Coat type/color
            2U * 3U + // STR/DEX/INT
            4U * 2U + // HP current/max
            2U +      // Level
            1U +      // Rank
            4U +      // Exp
            4U +      // Fame
            2U +      // Bonus
            2U +      // AdvancementLevel
            2U;       // ArmColor

        constexpr std::size_t kOustersInfoFixedAfterSlot =
            4U +      // Alignment_t
            1U +      // Sex
            2U * 4U + // Coat/Hair/Arm/Boot colors
            1U +      // Shape
            2U * 3U + // STR/DEX/INT
            4U * 2U + // HP current/max
            4U * 2U + // MP current/max
            2U +      // Level
            1U +      // Rank
            4U +      // Exp
            4U +      // Fame
            2U +      // Bonus
            2U +      // SkillBonus
            2U;       // AdvancementLevel

        [[nodiscard]] std::wstring LoginErrorMessage(const std::uint8_t errorId)
        {
            switch (errorId)
            {
            case 0:
                return L"Invalid ID or password.";
            case 1:
                return L"Account is already connected.";
            case 13:
                return L"This account is not allowed to play.";
            case 15:
                return L"Login server returned a generic error.";
            case 16:
                return L"Login denied for this IP address.";
            case 20:
                return L"Login key expired.";
            case 21:
                return L"Login key was not found.";
            case 27:
                return L"Server is under inspection.";
            default:
                return L"Login failed. Error ID: " + std::to_wstring(errorId);
            }
        }

        [[nodiscard]] bool ReadAttributeTriplet(PacketReader& reader, AttributeTriplet& value)
        {
            return reader.U16(value.current) && reader.U16(value.maximum) && reader.U16(value.basic);
        }

        [[nodiscard]] bool ReadSubItemInfo(PacketReader& reader, SubItemRecord& item)
        {
            return reader.U32(item.objectId) &&
                reader.U8(item.itemClass) &&
                reader.U16(item.itemType) &&
                reader.U8(item.itemNum) &&
                reader.U8(item.slotId);
        }

        [[nodiscard]] bool ReadOptionList(PacketReader& reader, std::vector<std::uint16_t>& options)
        {
            std::uint8_t count = 0;
            if (!reader.U8(count))
            {
                return false;
            }
            options.clear();
            options.reserve(count);
            for (std::uint8_t i = 0; i < count; ++i)
            {
                std::uint16_t option = 0;
                if (!reader.U16(option))
                {
                    return false;
                }
                options.push_back(option);
            }
            return true;
        }

        [[nodiscard]] bool ReadPCItemInfo(PacketReader& reader, RawItemRecord& item)
        {
            const auto start = reader.Offset();
            if (!reader.U32(item.objectId) ||
                !reader.U8(item.itemClass) ||
                !reader.U16(item.itemType))
            {
                return false;
            }

            if (!ReadOptionList(reader, item.options))
            {
                return false;
            }

            if (!ReadOptionList(reader, item.g2Options))
            {
                return false;
            }

            std::uint32_t durability = 0;
            std::uint16_t silver = 0;
            std::uint32_t grade = 0;
            std::uint8_t enchantLevel = 0;
            std::uint8_t itemNum = 0;
            std::uint16_t mainColor = 0;
            std::uint8_t subItemCount = 0;
            if (!reader.U32(durability) ||
                !reader.U16(silver) ||
                !reader.U32(grade) ||
                !reader.U8(enchantLevel) ||
                !reader.U8(itemNum) ||
                !reader.U16(mainColor) ||
                !reader.U8(subItemCount))
            {
                return false;
            }
            item.durability = durability;
            item.silver = silver;
            item.grade = grade;
            item.enchantLevel = enchantLevel;
            item.itemNum = itemNum;
            item.mainColor = mainColor;

            item.subItems.clear();
            item.subItems.reserve(subItemCount);
            for (std::uint8_t i = 0; i < subItemCount; ++i)
            {
                SubItemRecord subItem;
                if (!ReadSubItemInfo(reader, subItem))
                {
                    return false;
                }
                item.subItems.push_back(subItem);
            }

            // ServerSRC/types/VersionType.h enables VERSION_THIRD_ENCHANT_1.
            if (!ReadOptionList(reader, item.thirdOptions) ||
                !reader.U8(item.thirdEnchantType))
            {
                return false;
            }

            if (!reader.U8(item.addedInfo1) ||
                !reader.U8(item.addedInfo2) ||
                !reader.U8(item.addedInfo3) ||
                !ReadOptionList(reader, item.mixOptions))
            {
                return false;
            }

            item.raw = reader.RawFrom(start);
            return true;
        }

        [[nodiscard]] bool ReadStashSubItems(PacketReader& reader, RawItemRecord& item)
        {
            std::uint8_t subItemCount = 0;
            if (!reader.U8(subItemCount))
            {
                return false;
            }
            item.subItems.clear();
            item.subItems.reserve(subItemCount);
            for (std::uint8_t i = 0; i < subItemCount; ++i)
            {
                SubItemRecord subItem;
                if (!ReadSubItemInfo(reader, subItem))
                {
                    return false;
                }
                item.subItems.push_back(subItem);
            }
            return true;
        }

        [[nodiscard]] bool ReadStashItemInfo(PacketReader& reader, RawItemRecord& item, std::uint8_t& cashItem)
        {
            const auto start = reader.Offset();
            if (!reader.U32(item.objectId) ||
                !reader.U8(item.itemClass) ||
                !reader.U16(item.itemType) ||
                !ReadOptionList(reader, item.options) ||
                !reader.U32(item.durability) ||
                !reader.U8(item.itemNum) ||
                !reader.U16(item.silver) ||
                !reader.U32(item.grade) ||
                !reader.U8(item.enchantLevel) ||
                !ReadOptionList(reader, item.thirdOptions) ||
                !reader.U8(item.thirdEnchantType))
            {
                return false;
            }

            cashItem = 0;
            if (!ReadStashSubItems(reader, item))
            {
                return false;
            }
            item.raw = reader.RawFrom(start);
            return true;
        }

        [[nodiscard]] bool ReadInventoryInfo(PacketReader& reader, std::vector<RawItemRecord>& items)
        {
            std::uint8_t width = 0;
            std::uint8_t height = 0;
            std::uint8_t count = 0;
            if (!reader.U8(width) || !reader.U8(height) || !reader.U8(count))
            {
                return false;
            }

            items.reserve(items.size() + count);
            for (std::uint8_t i = 0; i < count; ++i)
            {
                RawItemRecord item;
                const auto start = reader.Offset();
                if (!ReadPCItemInfo(reader, item) || !reader.U8(item.slotOrX) || !reader.U8(item.y))
                {
                    return false;
                }
                item.raw = reader.RawFrom(start);
                items.push_back(std::move(item));
            }
            return true;
        }

        [[nodiscard]] bool ReadGearInfo(
            PacketReader& reader,
            std::vector<RawItemRecord>& items,
            std::uint16_t& gearSlotId,
            bool& active)
        {
            std::uint8_t count = 0;
            if (!reader.U8(count))
            {
                return false;
            }

            items.reserve(items.size() + count);
            for (std::uint8_t i = 0; i < count; ++i)
            {
                RawItemRecord item;
                const auto start = reader.Offset();
                if (!ReadPCItemInfo(reader, item) || !reader.U8(item.slotOrX) || !reader.Bool(item.gearActive))
                {
                    return false;
                }
                item.raw = reader.RawFrom(start);
                items.push_back(std::move(item));
            }

            return reader.U16(gearSlotId) && reader.Bool(active);
        }

        [[nodiscard]] bool ReadExtraInfo(PacketReader& reader, std::vector<RawItemRecord>& items)
        {
            std::uint8_t count = 0;
            if (!reader.U8(count))
            {
                return false;
            }

            items.reserve(items.size() + count);
            for (std::uint8_t i = 0; i < count; ++i)
            {
                RawItemRecord item;
                if (!ReadPCItemInfo(reader, item))
                {
                    return false;
                }
                items.push_back(std::move(item));
            }
            return true;
        }

        [[nodiscard]] bool ReadEffectInfo(PacketReader& reader, std::vector<EffectRecord>& effects)
        {
            std::uint8_t count = 0;
            if (!reader.U8(count))
            {
                return false;
            }
            effects.reserve(count);
            for (std::uint8_t i = 0; i < count; ++i)
            {
                EffectRecord effect;
                if (!reader.U16(effect.effectId) || !reader.U32(effect.remainingTime))
                {
                    return false;
                }
                effects.push_back(effect);
            }
            return true;
        }

        [[nodiscard]] bool ReadRideMotorcycleInfo(PacketReader& reader, std::vector<RawItemRecord>& items)
        {
            std::uint32_t objectId = 0;
            std::uint16_t itemType = 0;
            std::uint8_t optionCount = 0;
            std::uint8_t count = 0;
            if (!reader.U32(objectId) ||
                !reader.U16(itemType) ||
                !reader.U8(optionCount) ||
                !reader.Skip(static_cast<std::size_t>(optionCount) * 2U) ||
                !reader.U8(count))
            {
                return false;
            }

            items.reserve(count);
            for (std::uint8_t i = 0; i < count; ++i)
            {
                RawItemRecord item;
                const auto start = reader.Offset();
                if (!ReadPCItemInfo(reader, item) || !reader.U8(item.slotOrX) || !reader.U8(item.y))
                {
                    return false;
                }
                item.raw = reader.RawFrom(start);
                items.push_back(std::move(item));
            }
            return true;
        }

        [[nodiscard]] bool ReadGameTime(PacketReader& reader, GameTimeInfo& time)
        {
            return reader.U16(time.year) &&
                reader.U8(time.month) &&
                reader.U8(time.day) &&
                reader.U8(time.hour) &&
                reader.U8(time.minute) &&
                reader.U8(time.second);
        }

        [[nodiscard]] bool ReadNickname(PacketReader& reader, NicknameRecord& nickname)
        {
            if (!reader.U16(nickname.id) || !reader.U8(nickname.type))
            {
                return false;
            }

            switch (nickname.type)
            {
            case 0:
                return true;
            case 1:
            case 2:
            case 3:
            case 6:
                return reader.U16(nickname.index);
            case 4:
            case 5:
                return reader.String8AllowEmpty(nickname.text, 22);
            default:
                return false;
            }
        }

        [[nodiscard]] bool ReadPetInfo(PacketReader& reader, PetInfoRecord& pet)
        {
            const auto start = reader.Offset();
            std::uint8_t petType = 0;
            if (!reader.U8(petType))
            {
                return false;
            }
            pet = {};
            pet.petType = petType;
            if (petType == 0)
            {
                pet.raw = reader.RawFrom(start);
                return true;
            }

            std::uint8_t canGamble = 0;
            std::uint8_t canCutHead = 0;
            std::uint8_t canAttack = 0;
            std::uint8_t summonInfo = 0;
            if (!reader.U16(pet.creatureType) ||
                !reader.U8(pet.level) ||
                !reader.U32(pet.experience) ||
                !reader.U32(pet.hp) ||
                !reader.U8(pet.attr) ||
                !reader.U8(pet.attrLevel) ||
                !reader.U16(pet.option) ||
                !reader.U16(pet.option2) ||
                !reader.U16(pet.option3) ||
                !reader.U16(pet.foodType) ||
                !reader.U8(canGamble) ||
                !reader.U8(canCutHead) ||
                !reader.U8(canAttack) ||
                !reader.U8(summonInfo) ||
                !reader.String8AllowEmpty(pet.nickname, 22) ||
                !reader.U32(pet.itemObjectId) ||
                !reader.I32(pet.magicRemainSec) ||
                !reader.I32(pet.expertPetChangerRemainSec) ||
                !reader.U16(pet.enchantSkillType))
            {
                return false;
            }
            pet.present = true;
            pet.canGamble = canGamble != 0;
            pet.canCutHead = canCutHead != 0;
            pet.canAttack = canAttack != 0;
            pet.summonInfo = summonInfo != 0;

            if (!ReadOptionList(reader, pet.options))
            {
                return false;
            }

            if (!reader.U8(pet.functionPetfoodType))
            {
                return false;
            }

            if (!ReadOptionList(reader, pet.mixOptions))
            {
                return false;
            }
            pet.raw = reader.RawFrom(start);
            return true;
        }

        [[nodiscard]] bool ReadSkillSummary(PacketReader& reader, SkillInfoSummary& info)
        {
            if (!reader.U8(info.pcType) || !reader.U8(info.groupCount))
            {
                return false;
            }

            info.groups.clear();
            info.groups.reserve(info.groupCount);
            info.skillCount = 0;
            for (std::uint8_t groupIndex = 0; groupIndex < info.groupCount; ++groupIndex)
            {
                SkillInfoSummary::Group group;
                std::uint8_t listCount = 0;
                if (!reader.Bool(group.learnNewSkill))
                {
                    return false;
                }

                if (info.pcType == static_cast<std::uint8_t>(PcType::Slayer))
                {
                    if (!reader.U8(group.domainType) || !reader.U8(listCount))
                    {
                        return false;
                    }
                    group.entries.reserve(listCount);
                    for (std::uint8_t i = 0; i < listCount; ++i)
                    {
                        SkillInfoSummary::Entry entry;
                        if (!reader.U16(entry.skillType) ||
                            !reader.U32(entry.experience) ||
                            !reader.U16(entry.experienceLevel) ||
                            !reader.U32(entry.interval) ||
                            !reader.U32(entry.castingTime) ||
                            !reader.Bool(entry.enabled))
                        {
                            return false;
                        }
                        group.entries.push_back(entry);
                    }
                }
                else if (info.pcType == static_cast<std::uint8_t>(PcType::Vampire))
                {
                    if (!reader.U8(listCount))
                    {
                        return false;
                    }
                    group.entries.reserve(listCount);
                    for (std::uint8_t i = 0; i < listCount; ++i)
                    {
                        SkillInfoSummary::Entry entry;
                        if (!reader.U16(entry.skillType) ||
                            !reader.U32(entry.interval) ||
                            !reader.U32(entry.castingTime))
                        {
                            return false;
                        }
                        group.entries.push_back(entry);
                    }
                }
                else if (info.pcType == static_cast<std::uint8_t>(PcType::Ousters))
                {
                    if (!reader.U8(listCount))
                    {
                        return false;
                    }
                    group.entries.reserve(listCount);
                    for (std::uint8_t i = 0; i < listCount; ++i)
                    {
                        SkillInfoSummary::Entry entry;
                        if (!reader.U16(entry.skillType) ||
                            !reader.U16(entry.experienceLevel) ||
                            !reader.U32(entry.interval) ||
                            !reader.U32(entry.castingTime))
                        {
                            return false;
                        }
                        group.entries.push_back(entry);
                    }
                }
                else
                {
                    return false;
                }

                info.skillCount += group.entries.size();
                info.groups.push_back(std::move(group));
            }
            return true;
        }

        [[nodiscard]] bool ReadStoreOutlook(PacketReader& reader)
        {
            std::uint8_t open = 0;
            if (!reader.U8(open))
            {
                return false;
            }
            if (open == 0)
            {
                return true;
            }

            std::string sign;
            std::uint16_t paintColor = 0;
            return reader.String8AllowEmpty(sign, 80) && reader.U16(paintColor);
        }

        [[nodiscard]] bool ReadPlayerAddTrailer(PacketReader& reader, WorldObjectInfo& info)
        {
            return ReadEffectInfo(reader, info.effects) &&
                ReadPetInfo(reader, info.petInfo) &&
                ReadNickname(reader, info.nickname) &&
                ReadStoreOutlook(reader);
        }

        [[nodiscard]] bool ReadSlayerWorldObject(
            PacketReader& reader,
            WorldObjectInfo& info,
            const bool readContributePoint = true)
        {
            info.kind = WorldObjectKind::Player;
            info.pcType = PcType::Slayer;

            std::uint8_t x = 0;
            std::uint8_t y = 0;
            if (!reader.U32(info.objectId) ||
                !reader.String8(info.name, 20) ||
                !reader.U8(x) ||
                !reader.U8(y) ||
                !reader.U8(info.direction))
            {
                return false;
            }
            info.x = x;
            info.y = y;

            for (int i = 0; i < 7; ++i)
            {
                if (!reader.U8(info.slayerWearTypes[static_cast<std::size_t>(i)]))
                {
                    return false;
                }
            }
            if (!reader.U32(info.outlook))
            {
                return false;
            }
            for (int i = 0; i < 10; ++i)
            {
                if (!reader.U16(info.slayerColors[static_cast<std::size_t>(i)]))
                {
                    return false;
                }
            }
            info.sex = (info.outlook & 0x1U) != 0U ? 1U : 0U;
            info.hairStyle = static_cast<std::uint8_t>((info.outlook >> 1U) & 0x3U);
            info.hairColor = info.slayerColors[0];
            info.skinColor = info.slayerColors[1];
            info.mainColor = info.slayerColors[3];
            info.subColor = info.slayerColors[4];

            if (!(reader.U16(info.masterEffectColor) &&
                reader.U8(info.born) &&
                reader.U32(info.currentHp) &&
                reader.U32(info.maxHp) &&
                reader.U8(info.attackSpeed) &&
                reader.I32(info.alignment) &&
                reader.U8(info.competence) &&
                reader.U16(info.guildId) &&
                reader.U32(info.unionId) &&
                reader.U8(info.rank) &&
                reader.U8(info.advancementLevel)))
            {
                return false;
            }
            if (readContributePoint && !reader.I32(info.contributePoint))
            {
                return false;
            }
            if (!readContributePoint)
            {
                info.contributePoint = 0;
            }
            return reader.U16(info.motorItemType);
        }

        [[nodiscard]] bool ReadVampireWorldObject(
            PacketReader& reader,
            WorldObjectInfo& info,
            const bool readContributePoint = true)
        {
            info.kind = WorldObjectKind::Player;
            info.pcType = PcType::Vampire;

            std::uint8_t x = 0;
            std::uint8_t y = 0;
            if (!reader.U32(info.objectId) ||
                !reader.String8(info.name, 20) ||
                !reader.U8(x) ||
                !reader.U8(y) ||
                !reader.U8(info.direction))
            {
                return false;
            }
            info.x = x;
            info.y = y;

            std::uint8_t shapeByte = 0;
            if (!reader.U8(info.sex) ||
                !reader.U8(shapeByte) ||
                !reader.U8(info.batType) ||
                !reader.U16(info.wingItemType))
            {
                return false;
            }
            info.coatType = static_cast<std::uint8_t>(shapeByte & 0x0FU);
            info.armType = static_cast<std::uint8_t>((shapeByte >> 4U) & 0x03U);
            for (int i = 0; i < 8; ++i)
            {
                if (!reader.U16(info.vampireColors[static_cast<std::size_t>(i)]))
                {
                    return false;
                }
            }
            info.skinColor = info.vampireColors[2];
            info.mainColor = info.vampireColors[3];
            info.subColor = info.vampireColors[4];

            if (!(reader.U16(info.masterEffectColor) &&
                reader.U8(info.born) &&
                reader.U32(info.currentHp) &&
                reader.U32(info.maxHp) &&
                reader.U8(info.attackSpeed) &&
                reader.I32(info.alignment) &&
                reader.U8(info.shape) &&
                reader.U8(info.competence) &&
                reader.U16(info.guildId) &&
                reader.U32(info.unionId) &&
                reader.U8(info.rank) &&
                reader.U8(info.advancementLevel)))
            {
                return false;
            }
            if (readContributePoint)
            {
                return reader.I32(info.contributePoint);
            }
            info.contributePoint = 0;
            return true;
        }

        [[nodiscard]] bool ReadOustersWorldObject(
            PacketReader& reader,
            WorldObjectInfo& info,
            const bool readContributePoint = true)
        {
            info.kind = WorldObjectKind::Player;
            info.pcType = PcType::Ousters;

            std::uint8_t x = 0;
            std::uint8_t y = 0;
            if (!reader.U32(info.objectId) ||
                !reader.String8(info.name, 20) ||
                !reader.U8(x) ||
                !reader.U8(y) ||
                !reader.U8(info.direction))
            {
                return false;
            }
            info.x = x;
            info.y = y;

            std::uint8_t shapeByte = 0;
            if (!reader.U8(info.sex) ||
                !reader.U8(shapeByte) ||
                !reader.U8(info.wingSylphType) ||
                !reader.U16(info.wingItemType))
            {
                return false;
            }
            info.coatType = static_cast<std::uint8_t>(shapeByte & 0x0FU);
            info.armType = static_cast<std::uint8_t>((shapeByte >> 4U) & 0x07U);
            info.sylphType = static_cast<std::uint8_t>((shapeByte >> 7U) & 0x01U);
            for (int i = 0; i < 6; ++i)
            {
                if (!reader.U16(info.oustersColors[static_cast<std::size_t>(i)]))
                {
                    return false;
                }
            }
            info.hairColor = info.oustersColors[1];
            info.mainColor = info.oustersColors[0];
            info.subColor = info.oustersColors[2];

            if (!(reader.U16(info.masterEffectColor) &&
                reader.U8(info.born) &&
                reader.U32(info.currentHp) &&
                reader.U32(info.maxHp) &&
                reader.U8(info.attackSpeed) &&
                reader.I32(info.alignment) &&
                reader.U8(info.competence) &&
                reader.U16(info.guildId) &&
                reader.U32(info.unionId) &&
                reader.U8(info.rank) &&
                reader.U8(info.advancementLevel)))
            {
                return false;
            }
            if (readContributePoint)
            {
                return reader.I32(info.contributePoint);
            }
            info.contributePoint = 0;
            return true;
        }

        [[nodiscard]] bool ReadNpcInfo(PacketReader& reader, NpcRuntimeInfo& info)
        {
            std::uint8_t nameLength = 0;
            if (!reader.U8(nameLength))
            {
                return false;
            }
            if (nameLength == 0)
            {
                return true;
            }
            if (nameLength > 30U || reader.Remaining() < static_cast<std::size_t>(nameLength) + 6U)
            {
                return false;
            }
            if (!reader.Skip(0U))
            {
                return false;
            }
            info.name.resize(nameLength);
            for (std::uint8_t i = 0; i < nameLength; ++i)
            {
                std::uint8_t ch = 0;
                if (!reader.U8(ch))
                {
                    return false;
                }
                info.name[i] = static_cast<char>(ch);
            }
            return reader.U16(info.npcId) && reader.U16(info.x) && reader.U16(info.y);
        }

        [[nodiscard]] bool ReadBloodBibleSignInfo(PacketReader& reader, UpdateInfo& info)
        {
            std::uint8_t count = 0;
            if (!reader.U32(info.bloodBibleOpenNum) || !reader.U8(count))
            {
                return false;
            }
            info.bloodBibleSigns.reserve(count);
            for (std::uint8_t i = 0; i < count; ++i)
            {
                std::uint16_t sign = 0;
                if (!reader.U16(sign))
                {
                    return false;
                }
                info.bloodBibleSigns.push_back(sign);
            }
            return true;
        }

        [[nodiscard]] bool ReadSlayerUpdateInfo(PacketReader& reader, UpdateInfo& info)
        {
            info.normalizedPcType = PcType::Slayer;
            info.level = 1;
            std::uint8_t hairStyle = 0;
            std::uint16_t hairColor = 0;
            std::uint16_t skinColor = 0;
            std::uint16_t masterEffectColor = 0;
            std::uint8_t born = 0;
            std::int32_t alignment = 0;
            std::uint8_t rank = 0;
            std::uint32_t rankExp = 0;
            std::array<std::uint32_t, 3> attributeExp{};
            std::uint32_t fame = 0;
            std::uint32_t gold = 0;
            std::uint8_t sight = 0;
            std::uint8_t competence = 0;
            std::uint16_t guildId = 0;
            std::string guildName;
            std::uint8_t guildRank = 0;
            std::uint32_t unionId = 0;
            std::uint16_t advancementLevel = 0;
            std::uint32_t advancementGoal = 0;
            std::uint16_t attrBonus = 0;
            std::uint16_t scratch16 = 0;
            std::uint32_t scratch32 = 0;
            std::uint8_t attackSpeed = 0;

            if (!reader.U32(info.objectId) ||
                !reader.String8(info.name, 20) ||
                !reader.U8(info.sex) ||
                !reader.U8(hairStyle) ||
                !reader.U16(hairColor) ||
                !reader.U16(skinColor) ||
                !reader.U16(masterEffectColor) ||
                !reader.U8(born) ||
                !reader.I32(alignment) ||
                !ReadAttributeTriplet(reader, info.str) ||
                !ReadAttributeTriplet(reader, info.dex) ||
                !ReadAttributeTriplet(reader, info.intelligence) ||
                !reader.U8(rank) ||
                !reader.U32(rankExp))
            {
                return false;
            }
            info.hairStyle = hairStyle;
            info.hairColor = hairColor;
            info.skinColor = skinColor;
            info.masterEffectColor = masterEffectColor;
            for (auto& exp : attributeExp)
            {
                if (!reader.U32(exp))
                {
                    return false;
                }
            }
            if (!reader.U32(info.hp) ||
                !reader.U32(info.maxHp) ||
                !reader.U32(info.mp) ||
                !reader.U32(info.maxMp) ||
                !reader.U32(fame) ||
                !reader.U32(gold))
            {
                return false;
            }
            info.gold = gold;
            info.goldKnown = true;
            for (std::size_t i = 0; i < info.domainLevels.size(); ++i)
            {
                if (!reader.U8(info.domainLevels[i]) || !reader.U32(info.domainExperience[i]))
                {
                    return false;
                }
            }
            if (!reader.U8(sight))
            {
                return false;
            }
            for (int i = 0; i < 4; ++i)
            {
                if (!reader.U16(scratch16))
                {
                    return false;
                }
            }
            if (!reader.U8(competence) ||
                !reader.U16(guildId) ||
                !reader.String8AllowEmpty(guildName, 30) ||
                !reader.U8(guildRank) ||
                !reader.U32(unionId) ||
                !reader.U16(advancementLevel) ||
                !reader.U32(advancementGoal) ||
                !reader.U16(attrBonus))
            {
                return false;
            }
            info.born = born;
            info.alignment = alignment;
            info.rank = rank;
            info.rankExperienceRemaining = rankExp;
            info.strExperienceRemaining = attributeExp[0];
            info.dexExperienceRemaining = attributeExp[1];
            info.intExperienceRemaining = attributeExp[2];
            info.advancementGoalExperience = advancementGoal;
            info.competence = competence;
            info.guildId = guildId;
            info.guildName = std::move(guildName);
            info.guildRank = guildRank;
            info.unionId = unionId;
            info.advancementLevel = advancementLevel;
            info.bonus = attrBonus;
            for (int i = 0; i < 6; ++i)
            {
                if (!reader.U16(scratch16))
                {
                    return false;
                }
            }
            // ServerSRC/types/VersionType.h enables __CONTRIBUTION_SYSTEM__.
            if (!reader.U32(scratch32) ||
                !reader.U8(attackSpeed) ||
                !reader.U32(scratch32) ||
                !reader.U32(scratch32))
            {
                return false;
            }
            info.attackSpeed = attackSpeed;
            return true;
        }

        [[nodiscard]] bool ReadVampireUpdateInfo(PacketReader& reader, UpdateInfo& info)
        {
            info.normalizedPcType = PcType::Vampire;
            std::uint8_t batType = 0;
            std::uint16_t scratch16 = 0;
            std::uint32_t scratch32 = 0;
            std::uint8_t scratch8 = 0;
            std::int32_t alignment = 0;
            std::string guildName;
            std::uint8_t born = 0;
            std::uint8_t rank = 0;
            std::uint8_t competence = 0;
            std::uint16_t guildId = 0;
            std::uint8_t guildRank = 0;
            std::uint32_t unionId = 0;
            std::uint16_t advancementLevel = 0;
            std::uint32_t rankExp = 0;
            std::uint32_t experienceRemaining = 0;
            std::uint32_t gold = 0;
            std::uint32_t fame = 0;
            std::uint32_t advancementGoal = 0;
            std::uint8_t attackSpeed = 0;

            if (!reader.U32(info.objectId) ||
                !reader.String8(info.name, 20) ||
                !reader.U16(info.level) ||
                !reader.U8(info.sex) ||
                !reader.U8(batType))
            {
                return false;
            }
            if (batType == 2)
            {
                if (!reader.Skip(2U + 2U + 2U))
                {
                    return false;
                }
            }
            else if (batType == 1)
            {
                if (!reader.Skip(2U + 2U))
                {
                    return false;
                }
            }
            if (!reader.Skip(2U + 2U) ||
                !reader.U8(born) ||
                !reader.I32(alignment) ||
                !ReadAttributeTriplet(reader, info.str) ||
                !ReadAttributeTriplet(reader, info.dex) ||
                !ReadAttributeTriplet(reader, info.intelligence) ||
                !reader.U32(info.hp) ||
                !reader.U32(info.maxHp) ||
                !reader.U8(rank) ||
                !reader.U32(rankExp) ||
                !reader.U32(experienceRemaining) ||
                !reader.U32(gold) ||
                !reader.U32(fame) ||
                !reader.U8(scratch8) ||
                !reader.U16(info.bonus))
            {
                return false;
            }
            for (int i = 0; i < 8; ++i)
            {
                if (!reader.U16(scratch16))
                {
                    return false;
                }
            }
            if (!reader.U16(scratch16) ||
                !reader.U8(competence) ||
                !reader.U16(guildId) ||
                !reader.String8AllowEmpty(guildName, 30) ||
                !reader.U8(guildRank) ||
                !reader.U32(unionId) ||
                !reader.U16(advancementLevel) ||
                !reader.U32(advancementGoal))
            {
                return false;
            }
            for (int i = 0; i < 3; ++i)
            {
                if (!reader.U16(scratch16))
                {
                    return false;
                }
            }
            if (!reader.U32(scratch32) ||
                !reader.U8(attackSpeed) ||
                !reader.U32(scratch32) ||
                !reader.U32(scratch32))
            {
                return false;
            }
            info.born = born;
            info.alignment = alignment;
            info.rank = rank;
            info.rankExperienceRemaining = rankExp;
            info.experienceRemaining = experienceRemaining;
            info.advancementGoalExperience = advancementGoal;
            info.gold = gold;
            info.goldKnown = true;
            info.competence = competence;
            info.guildId = guildId;
            info.guildName = std::move(guildName);
            info.guildRank = guildRank;
            info.unionId = unionId;
            info.advancementLevel = advancementLevel;
            info.attackSpeed = attackSpeed;
            return true;
        }

        [[nodiscard]] bool ReadOustersUpdateInfo(PacketReader& reader, UpdateInfo& info)
        {
            info.normalizedPcType = PcType::Ousters;
            std::uint8_t sylphType = 0;
            std::uint16_t scratch16 = 0;
            std::uint32_t scratch32 = 0;
            std::uint8_t scratch8 = 0;
            std::int32_t alignment = 0;
            std::string guildName;
            std::uint8_t born = 0;
            std::uint8_t rank = 0;
            std::uint8_t competence = 0;
            std::uint16_t guildId = 0;
            std::uint8_t guildRank = 0;
            std::uint32_t unionId = 0;
            std::uint16_t advancementLevel = 0;
            std::uint32_t rankExp = 0;
            std::uint32_t experienceRemaining = 0;
            std::uint32_t gold = 0;
            std::uint32_t fame = 0;
            std::uint32_t advancementGoal = 0;
            std::uint8_t attackSpeed = 0;

            if (!reader.U32(info.objectId) ||
                !reader.String8(info.name, 20) ||
                !reader.U16(info.level) ||
                !reader.U8(info.sex) ||
                !reader.U8(sylphType) ||
                !reader.U16(scratch16))
            {
                return false;
            }
            if (sylphType == 2)
            {
                if (!reader.Skip(2U + 2U + 2U))
                {
                    return false;
                }
            }
            if (!reader.Skip(2U) ||
                !reader.U8(born) ||
                !reader.I32(alignment) ||
                !ReadAttributeTriplet(reader, info.str) ||
                !ReadAttributeTriplet(reader, info.dex) ||
                !ReadAttributeTriplet(reader, info.intelligence) ||
                !reader.U32(info.hp) ||
                !reader.U32(info.maxHp) ||
                !reader.U32(info.mp) ||
                !reader.U32(info.maxMp) ||
                !reader.U8(rank) ||
                !reader.U32(rankExp) ||
                !reader.U32(experienceRemaining) ||
                !reader.U32(gold) ||
                !reader.U32(fame) ||
                !reader.U8(scratch8) ||
                !reader.U16(info.bonus) ||
                !reader.U16(info.skillBonus) ||
                !reader.U16(scratch16) ||
                !reader.U8(competence) ||
                !reader.U16(guildId) ||
                !reader.String8AllowEmpty(guildName, 30) ||
                !reader.U8(guildRank) ||
                !reader.U32(unionId) ||
                !reader.U16(advancementLevel) ||
                !reader.U32(advancementGoal))
            {
                return false;
            }
            for (int i = 0; i < 3; ++i)
            {
                if (!reader.U16(scratch16))
                {
                    return false;
                }
            }
            if (!reader.U32(scratch32) ||
                !reader.U8(attackSpeed) ||
                !reader.U32(scratch32) ||
                !reader.U32(scratch32))
            {
                return false;
            }
            info.born = born;
            info.alignment = alignment;
            info.rank = rank;
            info.rankExperienceRemaining = rankExp;
            info.experienceRemaining = experienceRemaining;
            info.advancementGoalExperience = advancementGoal;
            info.gold = gold;
            info.goldKnown = true;
            info.competence = competence;
            info.guildId = guildId;
            info.guildName = std::move(guildName);
            info.guildRank = guildRank;
            info.unionId = unionId;
            info.advancementLevel = advancementLevel;
            info.attackSpeed = attackSpeed;
            return true;
        }

        [[nodiscard]] bool DecodeUpdateInfoBody(PacketReader& reader, UpdateInfo& info)
        {
            std::uint8_t rawType = 0;
            if (!reader.U8(rawType))
            {
                return false;
            }
            info.pcType = static_cast<char>(rawType);
            switch (info.pcType)
            {
            case 'S':
                if (!ReadSlayerUpdateInfo(reader, info))
                {
                    return false;
                }
                break;
            case 'V':
                if (!ReadVampireUpdateInfo(reader, info))
                {
                    return false;
                }
                break;
            case 'O':
                if (!ReadOustersUpdateInfo(reader, info))
                {
                    return false;
                }
                break;
            default:
                return false;
            }

            if (!ReadInventoryInfo(reader, info.inventoryItems) ||
                !ReadGearInfo(reader, info.gearItems, info.currentGearSlotId, info.currentGearSlotActive) ||
                !ReadExtraInfo(reader, info.extraItems) ||
                !ReadEffectInfo(reader, info.effects) ||
                !reader.Bool(info.hasMotorcycle))
            {
                return false;
            }
            if (info.hasMotorcycle && !ReadRideMotorcycleInfo(reader, info.motorcycleItems))
            {
                return false;
            }

            std::uint8_t zoneX = 0;
            std::uint8_t zoneY = 0;
            if (!reader.U16(info.zoneId) ||
                !reader.U8(zoneX) ||
                !reader.U8(zoneY) ||
                !ReadGameTime(reader, info.time) ||
                !reader.U8(info.weather) ||
                !reader.U8(info.weatherLevel) ||
                !reader.U8(info.darkLevel) ||
                !reader.U8(info.lightLevel))
            {
                return false;
            }
            info.zoneX = zoneX;
            info.zoneY = zoneY;
            info.zoneKnown = true;

            std::uint8_t npcTypeCount = 0;
            if (!reader.U8(npcTypeCount))
            {
                return false;
            }
            info.npcTypes.reserve(npcTypeCount);
            for (std::uint8_t i = 0; i < npcTypeCount; ++i)
            {
                std::uint16_t type = 0;
                if (!reader.U16(type))
                {
                    return false;
                }
                info.npcTypes.push_back(type);
            }

            std::uint8_t monsterTypeCount = 0;
            if (!reader.U8(monsterTypeCount))
            {
                return false;
            }
            info.monsterTypes.reserve(monsterTypeCount);
            for (std::uint8_t i = 0; i < monsterTypeCount; ++i)
            {
                std::uint16_t type = 0;
                if (!reader.U16(type))
                {
                    return false;
                }
                info.monsterTypes.push_back(type);
            }

            std::uint8_t npcInfoCount = 0;
            if (!reader.U8(npcInfoCount))
            {
                return false;
            }
            info.npcInfos.reserve(npcInfoCount);
            for (std::uint8_t i = 0; i < npcInfoCount; ++i)
            {
                NpcRuntimeInfo npc;
                if (!ReadNpcInfo(reader, npc))
                {
                    return false;
                }
                info.npcInfos.push_back(std::move(npc));
            }

            return reader.U8(info.serverStat) &&
                reader.U8(info.premium) &&
                reader.U32(info.smsCharge) &&
                ReadNickname(reader, info.nickname) &&
                reader.U8(info.nonPk) &&
                reader.U32(info.guildUnionId) &&
                reader.U8(info.guildUnionUserType) &&
                ReadBloodBibleSignInfo(reader, info) &&
                reader.I32(info.powerjjang);
        }

        [[nodiscard]] bool DecodeSlayerInfo(PacketReader& reader, CharacterList& result)
        {
            CharacterInfo info;
            info.occupied = true;
            info.pcType = PcType::Slayer;
            if (!reader.String8(info.name, 20))
            {
                return false;
            }

            std::uint8_t slot = 0;
            if (!reader.U8(slot) || !reader.Skip(kSlayerInfoFixedAfterSlot))
            {
                return false;
            }

            info.slot = slot < result.slots.size() ? slot : 0;
            info.level = 1;
            result.slots[info.slot] = info;
            return true;
        }

        [[nodiscard]] bool DecodeVampireInfo(PacketReader& reader, CharacterList& result)
        {
            CharacterInfo info;
            info.occupied = true;
            info.pcType = PcType::Vampire;
            if (!reader.String8(info.name, 20))
            {
                return false;
            }

            std::uint8_t slot = 0;
            if (!reader.U8(slot))
            {
                return false;
            }

            if (!reader.Skip(kVampireInfoFixedAfterSlot))
            {
                return false;
            }

            info.slot = slot < result.slots.size() ? slot : 0;
            info.level = 1;
            result.slots[info.slot] = info;
            return true;
        }

        [[nodiscard]] bool DecodeOustersInfo(PacketReader& reader, CharacterList& result)
        {
            CharacterInfo info;
            info.occupied = true;
            info.pcType = PcType::Ousters;
            if (!reader.String8(info.name, 20))
            {
                return false;
            }

            std::uint8_t slot = 0;
            if (!reader.U8(slot) || !reader.Skip(kOustersInfoFixedAfterSlot))
            {
                return false;
            }

            info.slot = slot < result.slots.size() ? slot : 0;
            info.level = 1;
            result.slots[info.slot] = info;
            return true;
        }
    }

    const char* PacketName(const PacketId id) noexcept
    {
        switch (id)
        {
        case PacketId::CGAttack:
            return "CGAttack";
        case PacketId::CGAddGearToMouse:
            return "CGAddGearToMouse";
        case PacketId::CGAddInventoryToMouse:
            return "CGAddInventoryToMouse";
        case PacketId::CGAddMouseToGear:
            return "CGAddMouseToGear";
        case PacketId::CGAddMouseToInventory:
            return "CGAddMouseToInventory";
        case PacketId::CGAddMouseToZone:
            return "CGAddMouseToZone";
        case PacketId::CGAddZoneToMouse:
            return "CGAddZoneToMouse";
        case PacketId::CGAddZoneToInventory:
            return "CGAddZoneToInventory";
        case PacketId::CGConnect:
            return "CGConnect";
        case PacketId::CGLearnSkill:
            return "CGLearnSkill";
        case PacketId::CGLogout:
            return "CGLogout";
        case PacketId::CGResurrect:
            return "CGResurrect";
        case PacketId::CGMove:
            return "CGMove";
        case PacketId::CGReady:
            return "CGReady";
        case PacketId::CGSay:
            return "CGSay";
        case PacketId::CGGlobalChat:
            return "CGGlobalChat";
        case PacketId::CGWhisper:
            return "CGWhisper";
        case PacketId::CGPartySay:
            return "CGPartySay";
        case PacketId::CGGuildChat:
            return "CGGuildChat";
        case PacketId::CGRangerSay:
            return "CGRangerSay";
        case PacketId::CGPhoneSay:
            return "CGPhoneSay";
        case PacketId::GCPhoneSay:
            return "GCPhoneSay";
        case PacketId::RCSay:
            return "RCSay";
        case PacketId::CRWhisper:
            return "CRWhisper";
        case PacketId::CGSkillToInventory:
            return "CGSkillToInventory";
        case PacketId::CGCastingSkill:
            return "CGCastingSkill";
        case PacketId::CGUseItemFromInventory:
            return "CGUseItemFromInventory";
        case PacketId::CGUseItemFromGear:
            return "CGUseItemFromGear";
        case PacketId::CGUseItemFromGQuestInventory:
            return "CGUseItemFromGQuestInventory";
        case PacketId::CGGQuestAccept:
            return "CGGQuestAccept";
        case PacketId::CGGQuestCancel:
            return "CGGQuestCancel";
        case PacketId::CGTradePrepare:
            return "CGTradePrepare";
        case PacketId::CGTradeAddItem:
            return "CGTradeAddItem";
        case PacketId::CGTradeRemoveItem:
            return "CGTradeRemoveItem";
        case PacketId::CGTradeMoney:
            return "CGTradeMoney";
        case PacketId::CGTradeFinish:
            return "CGTradeFinish";
        case PacketId::CGStashList:
            return "CGStashList";
        case PacketId::CGAddItemToItem:
            return "CGAddItemToItem";
        case PacketId::CGUseMessageItemFromInventory:
            return "CGUseMessageItemFromInventory";
        case PacketId::CGNPCTalk:
            return "CGNPCTalk";
        case PacketId::CGNPCAskAnswer:
            return "CGNPCAskAnswer";
        case PacketId::CGSelectRankBonus:
            return "CGSelectRankBonus";
        case PacketId::CGWithdrawPet:
            return "CGWithdrawPet";
        case PacketId::CGDepositPet:
            return "CGDepositPet";
        case PacketId::CGSelectGuild:
            return "CGSelectGuild";
        case PacketId::CGRequestGuildMemberList:
            return "CGRequestGuildMemberList";
        case PacketId::CGRequestGuildList:
            return "CGRequestGuildList";
        case PacketId::CGRequestStoreInfo:
            return "CGRequestStoreInfo";
        case PacketId::CGRequestWebMarket:
            return "CGRequestWebMarket";
        case PacketId::CGRequestTmallList:
            return "CGRequestTmallList";
        case PacketId::CGShopRequestList:
            return "CGShopRequestList";
        case PacketId::CGShopRequestBuy:
            return "CGShopRequestBuy";
        case PacketId::CGShopRequestSell:
            return "CGShopRequestSell";
        case PacketId::CGVerifyTime:
            return "CGVerifyTime";
        case PacketId::CGRequestPing:
            return "CGRequestPing";
        case PacketId::CLLogin:
            return "CLLogin";
        case PacketId::CLRegisterPlayer:
            return "CLRegisterPlayer";
        case PacketId::CLSelectPC:
            return "CLSelectPC";
        case PacketId::GCGlobalChat:
            return "GCGlobalChat";
        case PacketId::GCWhisper:
            return "GCWhisper";
        case PacketId::GCWhisperFailed:
            return "GCWhisperFailed";
        case PacketId::GCPartyInvite:
            return "GCPartyInvite";
        case PacketId::GCPartyLeave:
            return "GCPartyLeave";
        case PacketId::GCPartyJoined:
            return "GCPartyJoined";
        case PacketId::GCPartyError:
            return "GCPartyError";
        case PacketId::GCSystemMessage:
            return "GCSystemMessage";
        case PacketId::GCReconnectLogin:
            return "GCReconnectLogin";
        case PacketId::GCUseOK:
            return "GCUseOK";
        case PacketId::GCCannotAdd:
            return "GCCannotAdd";
        case PacketId::GCNPCAsk:
            return "GCNPCAsk";
        case PacketId::GCNPCSay:
            return "GCNPCSay";
        case PacketId::GCShopVersion:
            return "GCShopVersion";
        case PacketId::GCShopList:
            return "GCShopList";
        case PacketId::GCShopBuyOK:
            return "GCShopBuyOK";
        case PacketId::GCShopBuyFail:
            return "GCShopBuyFail";
        case PacketId::GCShopSold:
            return "GCShopSold";
        case PacketId::GCShopSellFail:
            return "GCShopSellFail";
        case PacketId::GCShopSellOK:
            return "GCShopSellOK";
        case PacketId::GCShopBought:
            return "GCShopBought";
        case PacketId::GCShopListMysterious:
            return "GCShopListMysterious";
        case PacketId::GCShopMarketCondition:
            return "GCShopMarketCondition";
        case PacketId::GCTradePrepare:
            return "GCTradePrepare";
        case PacketId::GCTradeAddItem:
            return "GCTradeAddItem";
        case PacketId::GCTradeRemoveItem:
            return "GCTradeRemoveItem";
        case PacketId::GCTradeMoney:
            return "GCTradeMoney";
        case PacketId::GCTradeFinish:
            return "GCTradeFinish";
        case PacketId::GCTradeError:
            return "GCTradeError";
        case PacketId::GCTradeVerify:
            return "GCTradeVerify";
        case PacketId::CGSkillToObject:
            return "CGSkillToObject";
        case PacketId::CGSkillToSelf:
            return "CGSkillToSelf";
        case PacketId::CGSkillToTile:
            return "CGSkillToTile";
        case PacketId::GCStashList:
            return "GCStashList";
        case PacketId::GCStashSell:
            return "GCStashSell";
        case PacketId::GCAddNewItemToZone:
            return "GCAddNewItemToZone";
        case PacketId::GCAddMonster:
            return "GCAddMonster";
        case PacketId::GCAddMonsterCorpse:
            return "GCAddMonsterCorpse";
        case PacketId::GCAddNPC:
            return "GCAddNPC";
        case PacketId::GCAddSlayer:
            return "GCAddSlayer";
        case PacketId::GCAddSlayerCorpse:
            return "GCAddSlayerCorpse";
        case PacketId::GCAddVampire:
            return "GCAddVampire";
        case PacketId::GCAddVampireCorpse:
            return "GCAddVampireCorpse";
        case PacketId::GCAttack:
            return "GCAttack";
        case PacketId::GCChangeDarkLight:
            return "GCChangeDarkLight";
        case PacketId::GCChangeWeather:
            return "GCChangeWeather";
        case PacketId::GCCreatureDied:
            return "GCCreatureDied";
        case PacketId::GCDeleteObject:
            return "GCDeleteObject";
        case PacketId::GCDeleteAndPickUpOK:
            return "GCDeleteAndPickUpOK";
        case PacketId::GCGetDamage:
            return "GCGetDamage";
        case PacketId::GCLearnSkillFailed:
            return "GCLearnSkillFailed";
        case PacketId::GCLearnSkillOK:
            return "GCLearnSkillOK";
        case PacketId::GCLearnSkillReady:
            return "GCLearnSkillReady";
        case PacketId::GCModifyInformation:
            return "GCModifyInformation";
        case PacketId::CGRequestRankGemData:
            return "CGRequestRankGemData";
        case PacketId::GCSendRankGemData:
            return "GCSendRankGemData";
        case PacketId::CGUseRankGemFromInventory:
            return "CGUseRankGemFromInventory";
        case PacketId::CGRemoveRankGem:
            return "CGRemoveRankGem";
        case PacketId::CLReconnectLogin:
            return "CLReconnectLogin";
        case PacketId::GCBloodDrainOK3:
            return "GCBloodDrainOK3";
        case PacketId::GCMove:
            return "GCMove";
        case PacketId::GCMoveError:
            return "GCMoveError";
        case PacketId::GCMoveOK:
            return "GCMoveOK";
        case PacketId::GCKnocksTargetBackOK1:
            return "GCKnocksTargetBackOK1";
        case PacketId::GCKnocksTargetBackOK2:
            return "GCKnocksTargetBackOK2";
        case PacketId::GCKnocksTargetBackOK4:
            return "GCKnocksTargetBackOK4";
        case PacketId::GCKnocksTargetBackOK5:
            return "GCKnocksTargetBackOK5";
        case PacketId::GCUnburrowOK:
            return "GCUnburrowOK";
        case PacketId::GCKnockBack:
            return "GCKnockBack";
        case PacketId::GCFastMove:
            return "GCFastMove";
        case PacketId::GCUntransformOK:
            return "GCUntransformOK";
        case PacketId::GCEnterVampirePortal:
            return "GCEnterVampirePortal";
        case PacketId::GCFakeMove:
            return "GCFakeMove";
        case PacketId::GCSay:
            return "GCSay";
        case PacketId::GCSetPosition:
            return "GCSetPosition";
        case PacketId::GCUpdateInfo:
            return "GCUpdateInfo";
        case PacketId::LCLoginError:
            return "LCLoginError";
        case PacketId::LCLoginOK:
            return "LCLoginOK";
        case PacketId::LCPCList:
            return "LCPCList";
        case PacketId::LCReconnect:
            return "LCReconnect";
        case PacketId::LCRegisterPlayerError:
            return "LCRegisterPlayerError";
        case PacketId::LCRegisterPlayerOK:
            return "LCRegisterPlayerOK";
        case PacketId::GCStatusCurrentHP:
            return "GCStatusCurrentHP";
        case PacketId::GCAddGearToInventory:
            return "GCAddGearToInventory";
        case PacketId::GCDropItemToZone:
            return "GCDropItemToZone";
        case PacketId::GCCreateItem:
            return "GCCreateItem";
        case PacketId::GCNPCResponse:
            return "GCNPCResponse";
        case PacketId::GCNPCAskDynamic:
            return "GCNPCAskDynamic";
        case PacketId::GCNPCSayDynamic:
            return "GCNPCSayDynamic";
        case PacketId::GCRemoveEffect:
            return "GCRemoveEffect";
        case PacketId::GCAddVampireFromBurrowing:
            return "GCAddVampireFromBurrowing";
        case PacketId::GCAddMonsterFromBurrowing:
            return "GCAddMonsterFromBurrowing";
        case PacketId::GCAddBurrowingCreature:
            return "GCAddBurrowingCreature";
        case PacketId::GCAddEffect:
            return "GCAddEffect";
        case PacketId::GCSkillInfo:
            return "GCSkillInfo";
        case PacketId::GCAddEffectToTile:
            return "GCAddEffectToTile";
        case PacketId::GCRealWearingInfo:
            return "GCRealWearingInfo";
        case PacketId::GCRemoveFromGear:
            return "GCRemoveFromGear";
        case PacketId::GCDeleteEffectFromTile:
            return "GCDeleteEffectFromTile";
        case PacketId::GCDeleteInventoryItem:
            return "GCDeleteInventoryItem";
        case PacketId::GCAddWolf:
            return "GCAddWolf";
        case PacketId::GCAddBat:
            return "GCAddBat";
        case PacketId::GCAddMonsterFromTransformation:
            return "GCAddMonsterFromTransformation";
        case PacketId::GCAddVampireFromTransformation:
            return "GCAddVampireFromTransformation";
        case PacketId::GCAddItemToItemVerify:
            return "GCAddItemToItemVerify";
        case PacketId::GCGuildChat:
            return "GCGuildChat";
        case PacketId::CGSkillToNamed:
            return "CGSkillToNamed";
        case PacketId::GCSelectRankBonusOK:
            return "GCSelectRankBonusOK";
        case PacketId::GCSelectRankBonusFailed:
            return "GCSelectRankBonusFailed";
        case PacketId::GCRankBonusInfo:
            return "GCRankBonusInfo";
        case PacketId::GCAddOusters:
            return "GCAddOusters";
        case PacketId::GCPartySay:
            return "GCPartySay";
        case PacketId::GCNPCInfo:
            return "GCNPCInfo";
        case PacketId::GCNPCAskVariable:
            return "GCNPCAskVariable";
        case PacketId::GCPetInfo:
            return "GCPetInfo";
        case PacketId::GCGQuestStatusInfo:
            return "GCGQuestStatusInfo";
        case PacketId::GCGQuestStatusModify:
            return "GCGQuestStatusModify";
        case PacketId::GCGQuestInventory:
            return "GCGQuestInventory";
        case PacketId::CGChangeGearSlot:
            return "CGChangeGearSlot";
        case PacketId::GCChangeGearSlot:
            return "GCChangeGearSlot";
        case PacketId::GCUpdateWeekItemList:
            return "GCUpdateWeekItemList";
        case PacketId::GCFriendList:
            return "GCFriendList";
        case PacketId::GCFriendWaitList:
            return "GCFriendWaitList";
        case PacketId::LCServerList:
            return "LCServerList";
        case PacketId::CLGetWorldList:
            return "CLGetWorldList";
        case PacketId::LCWorldList:
            return "LCWorldList";
        case PacketId::CLSelectWorld:
            return "CLSelectWorld";
        case PacketId::CLSelectServer:
            return "CLSelectServer";
        default:
            return "UnknownPacket";
        }
    }

    std::uint32_t LegacyReportedPacketSize(const PacketId id, const std::vector<std::uint8_t>& body) noexcept
    {
        const auto actual = static_cast<std::uint32_t>(std::min<std::size_t>(
            body.size(),
            static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())));
        switch (id)
        {
        case PacketId::CLLogin:
            // Designed_Common CLLogin::write emits MAC + loginMode + realIpaddr,
            // but its getPacketSize() omits the six MAC bytes from the header.
            return actual >= 6U ? actual - 6U : actual;
        default:
            return actual;
        }
    }

    std::uint32_t LegacyIncomingWireBodySize(const PacketId id, const std::uint32_t headerBodySize) noexcept
    {
        switch (id)
        {
        case PacketId::GCSkillInfo:
            // Legacy GCSkillInfo writes PCType + list count, while getPacketSize()
            // reports only the PCType base byte.
            return headerBodySize == std::numeric_limits<std::uint32_t>::max()
                ? headerBodySize
                : headerBodySize + 1U;
        default:
            return headerBodySize;
        }
    }

    std::optional<std::uint32_t> ProbeLegacyIncomingRequiredBodySize(
        const PacketId id,
        const std::vector<std::uint8_t>& body) noexcept
    {
        if (id != PacketId::GCSkillInfo)
        {
            return static_cast<std::uint32_t>(body.size());
        }
        if (body.size() < 2U)
        {
            return 2U;
        }
        if (body[0] != static_cast<std::uint8_t>(PcType::Ousters))
        {
            return static_cast<std::uint32_t>(body.size());
        }

        // Legacy SubOustersSkillInfo::getSize() omits its two-byte ExpLevel,
        // even though write() emits it. Walk group counts to recover the real
        // body extent without consuming bytes from the next packet header.
        constexpr std::size_t kOustersGroupHeaderSize = 2U;
        constexpr std::size_t kOustersSkillEntrySize = 12U;
        std::size_t offset = 2U;
        const auto groupCount = body[1];
        for (std::uint8_t group = 0; group < groupCount; ++group)
        {
            const auto groupHeaderEnd = offset + kOustersGroupHeaderSize;
            if (groupHeaderEnd > std::numeric_limits<std::uint32_t>::max())
            {
                return std::nullopt;
            }
            if (body.size() < groupHeaderEnd)
            {
                return static_cast<std::uint32_t>(groupHeaderEnd);
            }

            const auto skillCount = body[offset + 1U];
            const auto groupEnd = groupHeaderEnd + static_cast<std::size_t>(skillCount) * kOustersSkillEntrySize;
            if (groupEnd > std::numeric_limits<std::uint32_t>::max())
            {
                return std::nullopt;
            }
            if (body.size() < groupEnd)
            {
                return static_cast<std::uint32_t>(groupEnd);
            }
            offset = groupEnd;
        }
        return static_cast<std::uint32_t>(offset);
    }

    std::vector<std::uint8_t> BuildPacket(const PacketId id, const std::vector<std::uint8_t>& body)
    {
        PacketWriter writer;
        writer.U16(static_cast<std::uint16_t>(id));
        writer.U32(LegacyReportedPacketSize(id, body));
        auto packet = writer.Bytes();
        packet.insert(packet.end(), body.begin(), body.end());
        return packet;
    }

    std::vector<std::uint8_t> EncodeCLLogin(
        const std::string& username,
        const std::string& password,
        const std::array<std::uint8_t, 6>& macAddress,
        const std::uint8_t loginMode,
        const std::uint32_t realIpAddress)
    {
        if (auto legacyPacket = legacy_packet::EncodeCLLogin(username, password, macAddress, loginMode, realIpAddress))
        {
            return *legacyPacket;
        }

        PacketWriter body;
        body.String8(username, 30);
        body.String8(password, 32);
        body.Bytes(macAddress.data(), macAddress.size());
        body.U8(loginMode);
        body.U32(realIpAddress);
        return BuildPacket(PacketId::CLLogin, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCLGetWorldList()
    {
        return BuildPacket(PacketId::CLGetWorldList, {});
    }

    std::vector<std::uint8_t> EncodeCLRegisterPlayer(const std::string& username, const std::string& password)
    {
        PacketWriter body;
        body.String8(username, 8);
        body.String8(password, 8);
        return BuildPacket(PacketId::CLRegisterPlayer, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCLSelectWorld(const std::uint8_t worldId)
    {
        PacketWriter body;
        body.U8(worldId);
        return BuildPacket(PacketId::CLSelectWorld, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCLSelectServer(const std::uint8_t serverGroupId)
    {
        PacketWriter body;
        body.U8(serverGroupId);
        return BuildPacket(PacketId::CLSelectServer, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCLSelectPC(const std::string& pcName, const PcType pcType)
    {
        if (auto legacyPacket = legacy_packet::EncodeCLSelectPC(pcName, pcType))
        {
            return *legacyPacket;
        }

        PacketWriter body;
        body.String8(pcName, 20);
        body.U8(static_cast<std::uint8_t>(pcType));
        return BuildPacket(PacketId::CLSelectPC, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCLReconnectLogin(
        const std::uint32_t authKey,
        const std::uint8_t loginMode)
    {
        PacketWriter body;
        body.U32(authKey);
        body.U8(loginMode);
        return BuildPacket(PacketId::CLReconnectLogin, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGConnect(
        const std::uint32_t authKey,
        const PcType pcType,
        const std::string& pcName,
        const std::array<std::uint8_t, 6>& macAddress)
    {
        if (auto legacyPacket = legacy_packet::EncodeCGConnect(authKey, pcType, pcName, macAddress))
        {
            return *legacyPacket;
        }

        PacketWriter body;
        body.U32(authKey);
        body.U8(static_cast<std::uint8_t>(pcType));
        body.String8(pcName, 20);
        body.Bytes(macAddress.data(), macAddress.size());
        return BuildPacket(PacketId::CGConnect, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGMove(const std::uint8_t x, const std::uint8_t y, const std::uint8_t direction)
    {
        // TcpClient owns the Legacy outgoing encryption transform. CGMove::write
        // cannot be used with the bridge's plain output stream in encrypted mode.
        PacketWriter body;
        body.U8(x);
        body.U8(y);
        body.U8(direction);
        return BuildPacket(PacketId::CGMove, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGReady()
    {
        if (auto legacyPacket = legacy_packet::EncodeCGReady())
        {
            return *legacyPacket;
        }

        return BuildPacket(PacketId::CGReady, {});
    }

    std::vector<std::uint8_t> EncodeCGUseBonusPoint(const std::uint8_t which)
    {
        if (auto legacyPacket = legacy_packet::EncodeCGUseBonusPoint(which))
        {
            return *legacyPacket;
        }
        return {};
    }

    std::vector<std::uint8_t> EncodeCGSay(const std::uint32_t color, const std::string& message)
    {
        if (auto legacyPacket = legacy_packet::EncodeCGSay(color, message))
        {
            return *legacyPacket;
        }
        return {};
    }

    std::vector<std::uint8_t> EncodeCGGlobalChat(const std::uint32_t color, const std::string& message)
    {
        if (auto legacyPacket = legacy_packet::EncodeCGGlobalChat(color, message))
        {
            return *legacyPacket;
        }
        return {};
    }

    std::vector<std::uint8_t> EncodeCGWhisper(
        const std::string& name,
        const std::uint32_t color,
        const std::string& message)
    {
        if (auto legacyPacket = legacy_packet::EncodeCGWhisper(name, color, message))
        {
            return *legacyPacket;
        }
        return {};
    }

    std::vector<std::uint8_t> EncodeCGPartySay(const std::uint32_t color, const std::string& message)
    {
        if (auto legacyPacket = legacy_packet::EncodeCGPartySay(color, message))
        {
            return *legacyPacket;
        }
        return {};
    }

    std::vector<std::uint8_t> EncodeCGGuildChat(
        const std::uint8_t type,
        const std::uint32_t color,
        const std::string& message)
    {
        if (auto legacyPacket = legacy_packet::EncodeCGGuildChat(type, color, message))
        {
            return *legacyPacket;
        }
        return {};
    }

    std::vector<std::uint8_t> EncodeCGRangerSay(const std::string& message)
    {
        if (auto legacyPacket = legacy_packet::EncodeCGRangerSay(message))
        {
            return *legacyPacket;
        }
        return {};
    }

    std::vector<std::uint8_t> EncodeCGSelectRankBonus(const std::uint32_t rankBonusType)
    {
        PacketWriter body;
        body.U32(rankBonusType);
        return BuildPacket(PacketId::CGSelectRankBonus, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGWithdrawPet()
    {
        return BuildPacket(PacketId::CGWithdrawPet, {});
    }

    std::vector<std::uint8_t> EncodeCGDepositPet()
    {
        return BuildPacket(PacketId::CGDepositPet, {});
    }

    std::vector<std::uint8_t> EncodeCGSelectGuild(const std::uint32_t guildId)
    {
        PacketWriter writer;
        writer.U32(guildId);
        return BuildPacket(PacketId::CGSelectGuild, writer.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGRequestGuildMemberList(const std::uint32_t guildId)
    {
        PacketWriter writer;
        writer.U32(guildId);
        return BuildPacket(PacketId::CGRequestGuildMemberList, writer.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGRequestGuildList(const std::uint8_t guildType)
    {
        PacketWriter writer;
        writer.U8(guildType);
        return BuildPacket(PacketId::CGRequestGuildList, writer.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGRequestStoreInfo(const std::uint32_t ownerObjectId)
    {
        PacketWriter writer;
        writer.U32(ownerObjectId);
        return BuildPacket(PacketId::CGRequestStoreInfo, writer.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGRequestWebMarket(const std::uint8_t requestCode)
    {
        PacketWriter writer;
        writer.U8(requestCode);
        return BuildPacket(PacketId::CGRequestWebMarket, writer.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGRequestTmallList(const std::uint8_t kind)
    {
        PacketWriter writer;
        writer.U8(kind);
        return BuildPacket(PacketId::CGRequestTmallList, writer.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGAddZoneToInventory(
        const std::uint32_t objectId,
        const std::uint8_t zoneX,
        const std::uint8_t zoneY,
        const std::uint8_t inventoryX,
        const std::uint8_t inventoryY)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U8(zoneX);
        body.U8(zoneY);
        body.U8(inventoryX);
        body.U8(inventoryY);
        return BuildPacket(PacketId::CGAddZoneToInventory, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGAddZoneToMouse(
        const std::uint32_t objectId,
        const std::uint8_t zoneX,
        const std::uint8_t zoneY)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U8(zoneX);
        body.U8(zoneY);
        return BuildPacket(PacketId::CGAddZoneToMouse, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGAddInventoryToMouse(
        const std::uint32_t objectId,
        const std::uint32_t inventoryItemObjectId,
        const std::uint8_t inventoryX,
        const std::uint8_t inventoryY)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U32(inventoryItemObjectId);
        body.U8(inventoryX);
        body.U8(inventoryY);
        return BuildPacket(PacketId::CGAddInventoryToMouse, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGAddMouseToInventory(
        const std::uint32_t objectId,
        const std::uint32_t inventoryItemObjectId,
        const std::uint8_t inventoryX,
        const std::uint8_t inventoryY)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U32(inventoryItemObjectId);
        body.U8(inventoryX);
        body.U8(inventoryY);
        return BuildPacket(PacketId::CGAddMouseToInventory, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGAddMouseToZone(const std::uint32_t objectId)
    {
        PacketWriter body;
        body.U32(objectId);
        return BuildPacket(PacketId::CGAddMouseToZone, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGAddGearToMouse(
        const std::uint32_t objectId,
        const std::uint8_t slotId)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U8(slotId);
        return BuildPacket(PacketId::CGAddGearToMouse, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGAddMouseToGear(
        const std::uint16_t gearSlotId,
        const std::uint32_t objectId,
        const std::uint8_t slotId)
    {
        PacketWriter body;
        body.U16(gearSlotId);
        body.U32(objectId);
        body.U8(slotId);
        return BuildPacket(PacketId::CGAddMouseToGear, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGChangeGearSlot(const std::uint16_t gearSlotId)
    {
        PacketWriter body;
        body.U16(gearSlotId);
        return BuildPacket(PacketId::CGChangeGearSlot, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGAddItemToItem(
        const std::uint32_t objectId,
        const std::uint8_t inventoryX,
        const std::uint8_t inventoryY)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U8(inventoryX);
        body.U8(inventoryY);
        return BuildPacket(PacketId::CGAddItemToItem, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGCastingSkill(const std::uint16_t skillType)
    {
        PacketWriter body;
        body.U16(skillType);
        return BuildPacket(PacketId::CGCastingSkill, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGLogout()
    {
        return BuildPacket(PacketId::CGLogout, {});
    }

    std::vector<std::uint8_t> EncodeCGResurrect()
    {
        return BuildPacket(PacketId::CGResurrect, {});
    }

    std::vector<std::uint8_t> EncodeCGLearnSkill(
        const std::uint16_t skillType,
        const std::uint8_t domainType)
    {
        PacketWriter body;
        body.U16(skillType);
        body.U8(domainType);
        return BuildPacket(PacketId::CGLearnSkill, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGRequestRankGemData()
    {
        return BuildPacket(PacketId::CGRequestRankGemData, {});
    }

    std::vector<std::uint8_t> EncodeCGUseRankGemFromInventory(
        const std::uint32_t objectId,
        const std::uint32_t slotId)
    {
        PacketWriter writer;
        writer.U32(objectId);
        writer.U32(slotId);
        return BuildPacket(PacketId::CGUseRankGemFromInventory, writer.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGRemoveRankGem(const std::uint32_t slotId)
    {
        PacketWriter writer;
        writer.U32(slotId);
        return BuildPacket(PacketId::CGRemoveRankGem, writer.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGSkillToSelf(
        const std::uint16_t skillType,
        const std::uint16_t effectId)
    {
        PacketWriter body;
        body.U16(skillType);
        body.U16(effectId);
        return BuildPacket(PacketId::CGSkillToSelf, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGSkillToObject(
        const std::uint16_t skillType,
        const std::uint16_t effectId,
        const std::uint32_t targetObjectId,
        const std::uint8_t actionStep)
    {
        PacketWriter body;
        body.U16(skillType);
        body.U16(effectId);
        body.U32(targetObjectId);
        body.U8(actionStep);
        return BuildPacket(PacketId::CGSkillToObject, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGSkillToTile(
        const std::uint16_t skillType,
        const std::uint16_t effectId,
        const std::uint8_t x,
        const std::uint8_t y)
    {
        PacketWriter body;
        body.U16(skillType);
        body.U16(effectId);
        body.U8(x);
        body.U8(y);
        return BuildPacket(PacketId::CGSkillToTile, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGSkillToInventory(
        const std::uint16_t skillType,
        const std::uint32_t objectId,
        const std::uint32_t inventoryItemObjectId,
        const std::uint8_t x,
        const std::uint8_t y,
        const std::uint8_t targetX,
        const std::uint8_t targetY)
    {
        PacketWriter body;
        body.U16(skillType);
        body.U32(objectId);
        body.U8(x);
        body.U8(y);
        body.U8(targetX);
        body.U8(targetY);
        body.U32(inventoryItemObjectId);
        return BuildPacket(PacketId::CGSkillToInventory, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGSkillToNamed(
        const std::uint16_t skillType,
        const std::uint16_t effectId,
        const std::string& targetName)
    {
        PacketWriter body;
        body.U16(skillType);
        body.U16(effectId);
        body.String8(targetName, 20);
        return BuildPacket(PacketId::CGSkillToNamed, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGAttack(
        const std::uint32_t objectId,
        const std::uint8_t x,
        const std::uint8_t y,
        const std::uint8_t direction,
        const std::uint8_t actionStep)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U8(x);
        body.U8(y);
        body.U8(direction);
        body.U8(actionStep);
        return BuildPacket(PacketId::CGAttack, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGNPCTalk(const std::uint32_t objectId)
    {
        PacketWriter body;
        body.U32(objectId);
        return BuildPacket(PacketId::CGNPCTalk, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGNPCAskAnswer(
        const std::uint32_t objectId,
        const std::uint32_t scriptId,
        const std::uint8_t answerId)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U32(scriptId);
        body.U8(answerId);
        return BuildPacket(PacketId::CGNPCAskAnswer, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGShopRequestList(
        const std::uint32_t objectId,
        const std::uint8_t rackType)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U8(rackType);
        return BuildPacket(PacketId::CGShopRequestList, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGShopRequestBuy(
        const std::uint32_t objectId,
        const std::uint8_t rackType,
        const std::uint8_t rackIndex,
        const std::uint8_t itemNum,
        const std::uint8_t inventoryX,
        const std::uint8_t inventoryY)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U8(rackType);
        body.U8(rackIndex);
        body.U8(itemNum);
        body.U8(inventoryX);
        body.U8(inventoryY);
        return BuildPacket(PacketId::CGShopRequestBuy, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGShopRequestSell(
        const std::uint32_t objectId,
        const std::uint32_t itemObjectId,
        const std::uint8_t opCode)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U32(itemObjectId);
        body.U8(opCode);
        return BuildPacket(PacketId::CGShopRequestSell, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGUseItemFromInventory(
        const std::uint32_t objectId,
        const std::uint32_t inventoryItemObjectId,
        const std::uint8_t inventoryX,
        const std::uint8_t inventoryY)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U32(inventoryItemObjectId);
        body.U8(inventoryX);
        body.U8(inventoryY);
        return BuildPacket(PacketId::CGUseItemFromInventory, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGUseItemFromGear(
        const std::uint32_t objectId,
        const std::uint8_t part)
    {
        PacketWriter body;
        body.U32(objectId);
        body.U8(part);
        return BuildPacket(PacketId::CGUseItemFromGear, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGUseItemFromGQuestInventory(const std::uint8_t index)
    {
        PacketWriter body;
        body.U8(index);
        return BuildPacket(PacketId::CGUseItemFromGQuestInventory, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGGQuestAccept(const std::uint32_t questId)
    {
        PacketWriter body;
        body.U32(questId);
        return BuildPacket(PacketId::CGGQuestAccept, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGGQuestCancel(const std::uint32_t questId)
    {
        PacketWriter body;
        body.U32(questId);
        return BuildPacket(PacketId::CGGQuestCancel, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGTradePrepare(
        const std::uint32_t targetObjectId,
        const std::uint8_t code)
    {
        PacketWriter body;
        body.U32(targetObjectId);
        body.U8(code);
        return BuildPacket(PacketId::CGTradePrepare, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGTradeAddItem(
        const std::uint32_t targetObjectId,
        const std::uint32_t itemObjectId)
    {
        PacketWriter body;
        body.U32(targetObjectId);
        body.U32(itemObjectId);
        return BuildPacket(PacketId::CGTradeAddItem, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGTradeRemoveItem(
        const std::uint32_t targetObjectId,
        const std::uint32_t itemObjectId)
    {
        PacketWriter body;
        body.U32(targetObjectId);
        body.U32(itemObjectId);
        return BuildPacket(PacketId::CGTradeRemoveItem, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGTradeMoney(
        const std::uint32_t targetObjectId,
        const std::uint32_t amount,
        const std::uint8_t code)
    {
        PacketWriter body;
        body.U32(targetObjectId);
        body.U32(amount);
        body.U8(code);
        return BuildPacket(PacketId::CGTradeMoney, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGTradeFinish(
        const std::uint32_t targetObjectId,
        const std::uint8_t code)
    {
        PacketWriter body;
        body.U32(targetObjectId);
        body.U8(code);
        return BuildPacket(PacketId::CGTradeFinish, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGStashList(const std::uint32_t objectId)
    {
        PacketWriter body;
        body.U32(objectId);
        return BuildPacket(PacketId::CGStashList, body.Bytes());
    }

    std::vector<std::uint8_t> EncodeCGVerifyTime()
    {
        return BuildPacket(PacketId::CGVerifyTime, {});
    }

    std::vector<std::uint8_t> EncodeCGRequestPing()
    {
        return BuildPacket(PacketId::CGRequestPing, {});
    }

    std::optional<LoginOk> DecodeLCLoginOK(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        LoginOk ok;
        if (!reader.Bool(ok.isAdult) ||
            !reader.Bool(ok.underFifteen) ||
            !reader.Bool(ok.family) ||
            !reader.U8(ok.stat))
        {
            return std::nullopt;
        }

        if (reader.Remaining() >= 4U)
        {
            const auto optionalByteCount = reader.Remaining() - 4U;
            for (std::size_t i = 0; i < optionalByteCount; ++i)
            {
                std::uint8_t ignored = 0;
                (void)reader.U8(ignored);
            }

            std::uint8_t serverId = 0;
            std::uint8_t serverIdA = 0;
            std::uint8_t serverIdB = 0;
            std::uint8_t serverIdC = 0;
            if (!reader.U8(serverId) ||
                !reader.U8(serverIdA) ||
                !reader.U8(serverIdB) ||
                !reader.U8(serverIdC))
            {
                return std::nullopt;
            }
            ok.serverId = serverId;
            ok.serverIdA = serverIdA;
            ok.serverIdB = serverIdB;
            ok.serverIdC = serverIdC;
        }
        return ok;
    }

    std::optional<LoginError> DecodeLCLoginError(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        LoginError error;
        if (!reader.U8(error.errorId))
        {
            return std::nullopt;
        }
        error.message = LoginErrorMessage(error.errorId);
        return error;
    }

    std::optional<RegisterPlayerOk> DecodeLCRegisterPlayerOK(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        RegisterPlayerOk ok;
        if (!reader.String8AllowEmpty(ok.groupName, 255))
        {
            return std::nullopt;
        }
        std::uint8_t adult = 0;
        if (!reader.U8(adult) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        ok.isAdult = adult != 0;
        return ok;
    }

    std::optional<RegisterPlayerError> DecodeLCRegisterPlayerError(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        RegisterPlayerError error;
        if (!reader.U8(error.errorId) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return error;
    }

    std::optional<WorldList> DecodeLCWorldList(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        WorldList list;
        std::uint8_t count = 0;
        if (!reader.U8(list.currentWorldId) || !reader.U8(count))
        {
            return std::nullopt;
        }

        list.worlds.reserve(count);
        for (std::uint8_t i = 0; i < count; ++i)
        {
            WorldInfo info;
            if (!reader.U8(info.id) || !reader.String8(info.name, 255) || !reader.U8(info.status))
            {
                return std::nullopt;
            }
            list.worlds.push_back(info);
        }
        return list;
    }

    std::optional<ServerList> DecodeLCServerList(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ServerList list;
        std::uint8_t count = 0;
        if (!reader.U8(list.currentServerGroupId) || !reader.U8(count))
        {
            return std::nullopt;
        }

        list.servers.reserve(count);
        for (std::uint8_t i = 0; i < count; ++i)
        {
            ServerGroupInfo info;
            if (!reader.U8(info.id) ||
                !reader.String8(info.name, 255) ||
                !reader.U8(info.status) ||
                !reader.Bool(info.nonPk))
            {
                return std::nullopt;
            }

            if (reader.Remaining() >= 6U)
            {
                (void)reader.U16(info.slayerCount);
                (void)reader.U16(info.vampireCount);
                (void)reader.U16(info.oustersCount);
            }
            list.servers.push_back(info);
        }
        return list;
    }

    std::optional<CharacterList> DecodeLCPCList(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        CharacterList result;
        std::array<char, 3> types{ '0', '0', '0' };
        for (char& type : types)
        {
            std::uint8_t raw = 0;
            if (!reader.U8(raw))
            {
                return std::nullopt;
            }
            type = static_cast<char>(raw);
        }

        for (const auto type : types)
        {
            switch (type)
            {
            case 'S':
                if (!DecodeSlayerInfo(reader, result))
                {
                    return std::nullopt;
                }
                break;
            case 'V':
                if (!DecodeVampireInfo(reader, result))
                {
                    return std::nullopt;
                }
                break;
            case 'O':
                if (!DecodeOustersInfo(reader, result))
                {
                    return std::nullopt;
                }
                break;
            case '0':
                break;
            default:
                return std::nullopt;
            }
        }

        return result;
    }

    std::optional<ReconnectInfo> DecodeLCReconnect(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ReconnectInfo info;
        if (!reader.String8(info.gameServerIp, 15) ||
            !reader.U32(info.gameServerPort) ||
            !reader.U32(info.authKey))
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<LoginReconnectInfo> DecodeGCReconnectLogin(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        LoginReconnectInfo info;
        if (!reader.String8(info.loginServerIp, 15) ||
            !reader.U32(info.loginServerPort) ||
            !reader.U32(info.authKey) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<UpdateInfo> DecodeGCUpdateInfo(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        UpdateInfo info;
        if (!DecodeUpdateInfoBody(reader, info) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.bytesConsumed = reader.Offset();
        info.strictAlignment = true;
        return info;
    }

    std::optional<ModifyInfoRecord> DecodeGCModifyInformation(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ModifyInfoRecord info;
        if (!ReadModifyInfo(reader, info) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<SetPosition> DecodeGCSetPosition(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        SetPosition position;
        if (!reader.U8(position.x) || !reader.U8(position.y) || !reader.U8(position.direction) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return position;
    }

    std::optional<MoveInfo> DecodeGCMove(const std::vector<std::uint8_t>& body)
    {
        if (auto legacyPacket = legacy_packet::DecodeGCMove(body))
        {
            return *legacyPacket;
        }

        PacketReader reader(body);
        MoveInfo move;
        std::uint8_t x = 0;
        std::uint8_t y = 0;
        if (!reader.U32(move.objectId) ||
            !reader.U8(x) ||
            !reader.U8(y) ||
            !reader.U8(move.direction) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        move.x = x;
        move.y = y;
        return move;
    }

    std::optional<SetPosition> DecodeGCMoveOK(const std::vector<std::uint8_t>& body)
    {
        // TcpClient has already applied the Legacy incoming decrypt/shuffle.
        // GCMoveOK::read requires SocketEncryptInputStream in encrypted builds,
        // while the bridge owns only a plain in-memory stream.
        return DecodeGCSetPosition(body);
    }

    std::optional<SetPosition> DecodeGCMoveError(const std::vector<std::uint8_t>& body)
    {
        // The transport-decrypted Legacy body is X,Y. Direction is not present
        // in GCMoveError and remains owned by the caller's correction state.
        PacketReader reader(body);
        SetPosition position;
        if (!reader.U8(position.x) || !reader.U8(position.y) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return position;
    }

    std::optional<FastMoveInfo> DecodeGCFastMove(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        FastMoveInfo info;
        if (!reader.U32(info.objectId) ||
            !reader.U8(info.fromX) ||
            !reader.U8(info.fromY) ||
            !reader.U8(info.toX) ||
            !reader.U8(info.toY) ||
            !reader.U16(info.skillType) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<KnockBackInfo> DecodeGCKnockBack(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        KnockBackInfo info;
        if (!reader.U32(info.objectId) ||
            !reader.U16(info.originX) ||
            !reader.U16(info.originY) ||
            !reader.U16(info.targetX) ||
            !reader.U16(info.targetY) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<ObjectPositionInfo> DecodeGCFakeMove(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ObjectPositionInfo info;
        if (!reader.U32(info.objectId) || !reader.U8(info.x) || !reader.U8(info.y) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<ObjectPositionInfo> DecodeGCEnterVampirePortal(const std::vector<std::uint8_t>& body)
    {
        return DecodeGCFakeMove(body);
    }

    std::optional<KnockTargetBackInfo> DecodeGCKnocksTargetBackOK1(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        KnockTargetBackInfo info;
        std::uint8_t success = 0;
        if (!reader.U16(info.skillType) ||
            !reader.U8(info.direction) ||
            !reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !reader.U32(info.objectId) ||
            !reader.U8(info.bullet) ||
            !reader.U8(success) ||
            !ReadModifyInfo(reader, info.modify) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.success = success != 0U;
        return info;
    }

    std::optional<KnockTargetBackInfo> DecodeGCKnocksTargetBackOK2(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        KnockTargetBackInfo info;
        if (!reader.U16(info.skillType) ||
            !reader.U8(info.direction) ||
            !reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !reader.U32(info.objectId) ||
            !ReadModifyInfo(reader, info.modify) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<KnockTargetBackInfo> DecodeGCKnocksTargetBackOK4(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        KnockTargetBackInfo info;
        if (!reader.U32(info.targetObjectId) ||
            !reader.U16(info.skillType) ||
            !reader.U8(info.direction) ||
            !reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<KnockTargetBackInfo> DecodeGCKnocksTargetBackOK5(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        KnockTargetBackInfo info;
        std::uint8_t success = 0;
        if (!reader.U32(info.objectId) ||
            !reader.U32(info.targetObjectId) ||
            !reader.U8(success) ||
            !reader.U16(info.skillType) ||
            !reader.U8(info.direction) ||
            !reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.success = success != 0U;
        return info;
    }

    std::optional<DeleteObjectInfo> DecodeGCDeleteObject(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        DeleteObjectInfo info;
        if (!reader.U32(info.objectId) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<WorldObjectInfo> DecodeGCAddSlayer(const std::vector<std::uint8_t>& body)
    {
        const auto decode = [&body](const bool readContributePoint) -> std::optional<WorldObjectInfo> {
            PacketReader reader(body);
            WorldObjectInfo info;
            const auto start = reader.Offset();
            if (!ReadSlayerWorldObject(reader, info, readContributePoint) ||
                !ReadPlayerAddTrailer(reader, info) ||
                !reader.AtEnd())
            {
                return std::nullopt;
            }
            info.raw = reader.RawFrom(start);
            return info;
        };

        if (auto withContribution = decode(true))
        {
            return withContribution;
        }
        return decode(false);
    }

    std::optional<WorldObjectInfo> DecodeGCAddVampire(const std::vector<std::uint8_t>& body)
    {
        const auto decode = [&body](const bool readContributePoint) -> std::optional<WorldObjectInfo> {
            PacketReader reader(body);
            WorldObjectInfo info;
            const auto start = reader.Offset();
            if (!ReadVampireWorldObject(reader, info, readContributePoint) ||
                !ReadPlayerAddTrailer(reader, info) ||
                !reader.U8(info.fromFlag) ||
                !reader.AtEnd())
            {
                return std::nullopt;
            }
            info.raw = reader.RawFrom(start);
            return info;
        };

        if (auto withContribution = decode(true))
        {
            return withContribution;
        }
        return decode(false);
    }

    std::optional<WorldObjectInfo> DecodeGCAddOusters(const std::vector<std::uint8_t>& body)
    {
        const auto decode = [&body](const bool readContributePoint) -> std::optional<WorldObjectInfo> {
            PacketReader reader(body);
            WorldObjectInfo info;
            const auto start = reader.Offset();
            if (!ReadOustersWorldObject(reader, info, readContributePoint) ||
                !ReadPlayerAddTrailer(reader, info) ||
                !reader.AtEnd())
            {
                return std::nullopt;
            }
            info.raw = reader.RawFrom(start);
            return info;
        };

        if (auto withContribution = decode(true))
        {
            return withContribution;
        }
        return decode(false);
    }

    namespace
    {
        [[nodiscard]] std::optional<WorldObjectInfo> DecodeMonsterObjectBody(
            const std::vector<std::uint8_t>& body,
            const bool hasFromFlag,
            const bool dead)
        {
            PacketReader reader(body);
            WorldObjectInfo info;
            const auto start = reader.Offset();
            std::uint8_t x = 0;
            std::uint8_t y = 0;
            if (!reader.U32(info.objectId) ||
                !reader.U16(info.typeId) ||
                !reader.String8AllowEmpty(info.name, 100) ||
                !reader.U16(info.mainColor) ||
                !reader.U16(info.subColor) ||
                !reader.U8(x) ||
                !reader.U8(y) ||
                !reader.U8(info.direction) ||
                !ReadEffectInfo(reader, info.effects) ||
                !reader.U32(info.currentHp) ||
                !reader.U32(info.maxHp))
            {
                return std::nullopt;
            }
            if (hasFromFlag && !reader.U8(info.fromFlag))
            {
                return std::nullopt;
            }
            if (!reader.AtEnd())
            {
                return std::nullopt;
            }
            info.kind = WorldObjectKind::Monster;
            info.x = x;
            info.y = y;
            info.dead = dead;
            info.raw = reader.RawFrom(start);
            return info;
        }
    }

    std::optional<WorldObjectInfo> DecodeGCAddMonster(const std::vector<std::uint8_t>& body)
    {
        return DecodeMonsterObjectBody(body, true, false);
    }

    std::optional<WorldObjectInfo> DecodeGCAddMonsterFromBurrowing(const std::vector<std::uint8_t>& body)
    {
        return DecodeMonsterObjectBody(body, false, false);
    }

    std::optional<WorldObjectInfo> DecodeGCAddMonsterFromTransformation(const std::vector<std::uint8_t>& body)
    {
        return DecodeMonsterObjectBody(body, false, false);
    }

    std::optional<WorldObjectInfo> DecodeGCAddMonsterCorpse(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        WorldObjectInfo info;
        const auto start = reader.Offset();
        std::uint8_t x = 0;
        std::uint8_t y = 0;
        if (!reader.U32(info.objectId) ||
            !reader.U16(info.typeId) ||
            !reader.String8AllowEmpty(info.name, 100) ||
            !reader.U8(x) ||
            !reader.U8(y) ||
            !reader.U8(info.direction))
        {
            return std::nullopt;
        }
        std::uint8_t hasHead = 0;
        std::uint8_t treasureCount = 0;
        std::uint32_t lastKiller = 0;
        if (!reader.U8(hasHead) ||
            !reader.U8(treasureCount) ||
            !reader.U32(lastKiller) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        (void)hasHead;
        (void)treasureCount;
        (void)lastKiller;
        info.kind = WorldObjectKind::Monster;
        info.x = x;
        info.y = y;
        info.dead = true;
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<WorldObjectInfo> DecodeGCAddVampireCorpse(const std::vector<std::uint8_t>& body)
    {
        if (auto legacyPacket = legacy_packet::DecodeGCAddVampireCorpse(body))
        {
            return *legacyPacket;
        }

        const auto decode = [&body](const bool readContributePoint) -> std::optional<WorldObjectInfo> {
            PacketReader reader(body);
            WorldObjectInfo info;
            const auto start = reader.Offset();
            std::uint8_t treasureCount = 0;
            if (!ReadVampireWorldObject(reader, info, readContributePoint) ||
                !reader.U8(treasureCount) ||
                !reader.AtEnd())
            {
                return std::nullopt;
            }
            (void)treasureCount;
            info.dead = true;
            info.raw = reader.RawFrom(start);
            return info;
        };

        if (auto withContribution = decode(true))
        {
            return withContribution;
        }
        return decode(false);
    }

    std::optional<WorldObjectInfo> DecodeGCAddVampireFromBurrowing(const std::vector<std::uint8_t>& body)
    {
        const auto decode = [&body](const bool readContributePoint) -> std::optional<WorldObjectInfo> {
            PacketReader reader(body);
            WorldObjectInfo info;
            const auto start = reader.Offset();
            if (!ReadVampireWorldObject(reader, info, readContributePoint) ||
                !ReadEffectInfo(reader, info.effects) ||
                !reader.AtEnd())
            {
                return std::nullopt;
            }
            info.raw = reader.RawFrom(start);
            return info;
        };

        if (auto withContribution = decode(true))
        {
            return withContribution;
        }
        return decode(false);
    }

    std::optional<WorldObjectInfo> DecodeGCAddVampireFromTransformation(const std::vector<std::uint8_t>& body)
    {
        return DecodeGCAddVampireFromBurrowing(body);
    }

    std::optional<WorldObjectInfo> DecodeGCAddWolf(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        WorldObjectInfo info;
        const auto start = reader.Offset();
        std::uint8_t x = 0;
        std::uint8_t y = 0;
        std::uint16_t guildId = 0;
        if (!reader.U32(info.objectId) ||
            !reader.String8(info.name, 112) ||
            !reader.U16(info.mainColor) ||
            !reader.U16(info.typeId) ||
            !reader.U8(x) ||
            !reader.U8(y) ||
            !reader.U8(info.direction) ||
            !reader.U32(info.currentHp) ||
            !reader.U32(info.maxHp) ||
            !reader.U16(guildId))
        {
            return std::nullopt;
        }
        if (reader.Remaining() >= 2U)
        {
            (void)reader.U16(info.subColor);
        }
        if (!reader.AtEnd())
        {
            return std::nullopt;
        }
        (void)guildId;
        info.kind = WorldObjectKind::Monster;
        info.x = x;
        info.y = y;
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<WorldObjectInfo> DecodeGCAddBat(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        WorldObjectInfo info;
        const auto start = reader.Offset();
        std::uint8_t x = 0;
        std::uint8_t y = 0;
        std::uint16_t guildId = 0;
        if (!reader.U32(info.objectId) ||
            !reader.String8(info.name, 112) ||
            !reader.U16(info.typeId) ||
            !reader.U8(x) ||
            !reader.U8(y) ||
            !reader.U8(info.direction) ||
            !reader.U32(info.currentHp) ||
            !reader.U32(info.maxHp) ||
            !reader.U16(guildId))
        {
            return std::nullopt;
        }
        if (reader.Remaining() >= 1U)
        {
            std::uint8_t batType = 0;
            (void)reader.U8(batType);
            if (reader.Remaining() >= 4U)
            {
                (void)reader.U16(info.mainColor);
                (void)reader.U16(info.subColor);
            }
        }
        if (!reader.AtEnd())
        {
            return std::nullopt;
        }
        (void)guildId;
        info.kind = WorldObjectKind::Monster;
        info.x = x;
        info.y = y;
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<WorldObjectInfo> DecodeGCAddNPC(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        WorldObjectInfo info;
        const auto start = reader.Offset();
        std::uint8_t x = 0;
        std::uint8_t y = 0;
        if (!reader.U32(info.objectId) ||
            !reader.String8AllowEmpty(info.name, 100) ||
            !reader.U16(info.typeId) ||
            !reader.U16(info.spriteType) ||
            !reader.U16(info.mainColor) ||
            !reader.U16(info.subColor) ||
            !reader.U8(x) ||
            !reader.U8(y) ||
            !reader.U8(info.direction))
        {
            return std::nullopt;
        }
        if (reader.Remaining() > 0U)
        {
            if (!ReadEffectInfo(reader, info.effects))
            {
                return std::nullopt;
            }
        }
        if (!reader.AtEnd())
        {
            return std::nullopt;
        }
        info.kind = WorldObjectKind::Npc;
        info.x = x;
        info.y = y;
        if (info.name.empty())
        {
            info.name = "NPC " + std::to_string(info.typeId);
        }
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<StatusCurrentHP> DecodeGCStatusCurrentHP(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        StatusCurrentHP status;
        if (!reader.U32(status.objectId) || !reader.U32(status.currentHp) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return status;
    }

    std::optional<SystemMessage> DecodeGCSystemMessage(const std::vector<std::uint8_t>& body)
    {
        return legacy_packet::DecodeGCSystemMessage(body);
    }

    std::optional<ChatMessage> DecodeGCSay(const std::vector<std::uint8_t>& body)
    {
        return legacy_packet::DecodeGCSay(body);
    }

    std::optional<ChatMessage> DecodeGCGlobalChat(const std::vector<std::uint8_t>& body)
    {
        return legacy_packet::DecodeGCGlobalChat(body);
    }

    std::optional<ChatMessage> DecodeGCWhisper(const std::vector<std::uint8_t>& body)
    {
        return legacy_packet::DecodeGCWhisper(body);
    }

    bool DecodeGCWhisperFailed(const std::vector<std::uint8_t>& body)
    {
        return legacy_packet::DecodeGCWhisperFailed(body);
    }

    std::optional<ChatMessage> DecodeGCPartySay(const std::vector<std::uint8_t>& body)
    {
        return legacy_packet::DecodeGCPartySay(body);
    }

    std::optional<ChatMessage> DecodeGCGuildChat(const std::vector<std::uint8_t>& body)
    {
        return legacy_packet::DecodeGCGuildChat(body);
    }

    std::optional<PartyJoinedInfo> DecodeGCPartyJoined(const std::vector<std::uint8_t>& body)
    {
        return legacy_packet::DecodeGCPartyJoined(body);
    }

    std::optional<PartyLeaveInfo> DecodeGCPartyLeave(const std::vector<std::uint8_t>& body)
    {
        return legacy_packet::DecodeGCPartyLeave(body);
    }

    std::optional<RankBonusInfo> DecodeGCRankBonusInfo(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        std::uint8_t count = 0;
        if (!reader.U8(count))
        {
            return std::nullopt;
        }

        RankBonusInfo info;
        info.rankBonusTypes.reserve(count);
        for (std::uint8_t i = 0; i < count; ++i)
        {
            std::uint32_t rankBonusType = 0;
            if (!reader.U32(rankBonusType))
            {
                return std::nullopt;
            }
            info.rankBonusTypes.push_back(rankBonusType);
        }
        if (!reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<RankBonusSelectionResult> DecodeGCSelectRankBonusOK(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        RankBonusSelectionResult result;
        if (!reader.U32(result.rankBonusType) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return result;
    }

    std::optional<RankBonusSelectionResult> DecodeGCSelectRankBonusFailed(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        RankBonusSelectionResult result;
        if (!reader.U32(result.rankBonusType) ||
            !reader.U8(result.failureCode) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return result;
    }

    std::optional<NpcDialogInfo> DecodeGCNPCAsk(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        NpcDialogInfo info;
        const auto start = reader.Offset();
        if (!reader.U32(info.objectId) ||
            !reader.U32(info.scriptId) ||
            !reader.U16(info.npcId) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<NpcDialogInfo> DecodeGCNPCSay(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        NpcDialogInfo info;
        const auto start = reader.Offset();
        if (!reader.U32(info.objectId) ||
            !reader.U32(info.scriptId) ||
            !reader.U8(info.subjectId) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<NpcDialogInfo> DecodeGCNPCAskDynamic(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        NpcDialogInfo info;
        const auto start = reader.Offset();
        std::uint8_t count = 0;
        if (!reader.U32(info.objectId) ||
            !reader.U32(info.scriptId) ||
            !reader.String16(info.subject, 1024) ||
            !reader.U8(count))
        {
            return std::nullopt;
        }

        info.contents.reserve(count);
        for (std::uint8_t i = 0; i < count; ++i)
        {
            std::string content;
            if (!reader.String16AllowEmpty(content, 10240))
            {
                return std::nullopt;
            }
            if (!content.empty())
            {
                info.answerIds.push_back(static_cast<std::uint8_t>(i + 1U));
                info.contents.push_back(std::move(content));
            }
        }

        if (!reader.AtEnd())
        {
            return std::nullopt;
        }
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<NpcDialogInfo> DecodeGCNPCSayDynamic(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        NpcDialogInfo info;
        const auto start = reader.Offset();
        if (!reader.U32(info.objectId) ||
            !reader.String8(info.message, 255) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<NpcDialogInfo> DecodeGCNPCAskVariable(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        NpcDialogInfo info;
        const auto start = reader.Offset();
        std::uint8_t count = 0;
        if (!reader.U32(info.objectId) ||
            !reader.U32(info.scriptId) ||
            !reader.U8(count))
        {
            return std::nullopt;
        }

        info.parameters.reserve(count);
        for (std::uint8_t i = 0; i < count; ++i)
        {
            NpcScriptParameter parameter;
            if (!reader.String8(parameter.name, 255) ||
                !reader.String8(parameter.value, 255))
            {
                return std::nullopt;
            }
            info.parameters.push_back(std::move(parameter));
        }

        if (!reader.AtEnd())
        {
            return std::nullopt;
        }
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<NpcDialogInfo> DecodeGCNPCResponse(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        NpcDialogInfo info;
        const auto start = reader.Offset();
        if (!reader.U8(info.responseCode))
        {
            return std::nullopt;
        }
        if (reader.Remaining() == 4U)
        {
            if (!reader.U32(info.responseParameter))
            {
                return std::nullopt;
            }
            info.hasResponseParameter = true;
        }
        else if (reader.Remaining() != 0U)
        {
            return std::nullopt;
        }
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<std::vector<NpcRuntimeInfo>> DecodeGCNPCInfo(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        std::uint8_t count = 0;
        if (!reader.U8(count))
        {
            return std::nullopt;
        }

        std::vector<NpcRuntimeInfo> npcs;
        npcs.reserve(count);
        for (std::uint8_t i = 0; i < count; ++i)
        {
            NpcRuntimeInfo npc;
            if (!ReadNpcInfo(reader, npc))
            {
                return std::nullopt;
            }
            if (!npc.name.empty())
            {
                npcs.push_back(std::move(npc));
            }
        }

        if (!reader.AtEnd())
        {
            return std::nullopt;
        }

        return npcs;
    }

    std::optional<ShopVersionInfo> DecodeGCShopVersion(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ShopVersionInfo info;
        if (!reader.U32(info.objectId))
        {
            return std::nullopt;
        }

        const auto remainingWithNpcId = 2U + info.versions.size() * 4U + 2U;
        if (reader.Remaining() == remainingWithNpcId)
        {
            std::uint16_t npcId = 0;
            if (!reader.U16(npcId))
            {
                return std::nullopt;
            }
            info.npcId = npcId;
        }

        for (auto& version : info.versions)
        {
            if (!ReadI32(reader, version))
            {
                return std::nullopt;
            }
        }

        if (!ReadI16(reader, info.marketCondSell) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<ShopListInfo> DecodeGCShopList(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ShopListInfo info;
        const auto start = reader.Offset();
        std::uint8_t total = 0;
        if (!reader.U32(info.objectId) ||
            !ReadI32(reader, info.version) ||
            !reader.U8(info.rackType) ||
            !reader.U8(total))
        {
            return std::nullopt;
        }

        for (std::uint8_t i = 0; i < total; ++i)
        {
            ShopItemInfo item;
            if (!ReadShopRackItem(reader, item))
            {
                return std::nullopt;
            }
            if (item.rackIndex < info.rack.size())
            {
                info.rack[item.rackIndex] = std::move(item);
            }
        }

        if (!ReadI16(reader, info.marketCondBuy) ||
            !ReadI16(reader, info.marketCondSell) ||
            !reader.U8(info.shopType) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<ShopListInfo> DecodeGCShopListMysterious(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ShopListInfo info;
        const auto start = reader.Offset();
        std::uint8_t total = 0;
        if (!reader.U32(info.objectId) ||
            !ReadI32(reader, info.version) ||
            !reader.U8(info.rackType) ||
            !reader.U8(total))
        {
            return std::nullopt;
        }

        info.mysterious = true;
        for (std::uint8_t i = 0; i < total; ++i)
        {
            ShopItemInfo item;
            if (!ReadMysteriousShopRackItem(reader, item))
            {
                return std::nullopt;
            }
            if (item.rackIndex < info.rack.size())
            {
                info.rack[item.rackIndex] = std::move(item);
            }
        }

        if (!ReadI16(reader, info.marketCondBuy) ||
            !ReadI16(reader, info.marketCondSell) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<ShopBuyOkInfo> DecodeGCShopBuyOK(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ShopBuyOkInfo info;
        std::uint8_t optionCount = 0;
        if (!reader.U32(info.objectId) ||
            !ReadI32(reader, info.version) ||
            !reader.U32(info.item.objectId) ||
            !reader.U8(info.item.itemClass) ||
            !reader.U16(info.item.itemType) ||
            !reader.U8(optionCount))
        {
            return std::nullopt;
        }

        info.item.options.reserve(optionCount);
        for (std::uint8_t index = 0; index < optionCount; ++index)
        {
            std::uint16_t optionType = 0;
            if (!reader.U16(optionType))
            {
                return std::nullopt;
            }
            info.item.options.push_back(optionType);
        }

        if (!reader.U32(info.item.durability) ||
            !reader.U8(info.item.itemNum) ||
            !reader.U16(info.item.silver) ||
            !reader.U32(info.item.grade) ||
            !reader.U8(info.item.enchantLevel) ||
            !reader.U32(info.price) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.item.price = info.price;
        info.item.present = true;
        return info;
    }

    std::optional<ShopBuyFailInfo> DecodeGCShopBuyFail(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ShopBuyFailInfo info;
        if (!reader.U32(info.objectId) ||
            !reader.U8(info.code) ||
            !reader.U32(info.amount) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<ShopSellOkInfo> DecodeGCShopSellOK(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ShopSellOkInfo info;
        if (!reader.U32(info.objectId) ||
            !ReadI32(reader, info.version) ||
            !reader.U32(info.itemObjectId) ||
            !reader.U32(info.price) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<std::uint32_t> DecodeGCShopSellFail(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        std::uint32_t objectId = 0;
        if (!reader.U32(objectId) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return objectId;
    }

    std::optional<ShopRackDeltaInfo> DecodeGCShopSold(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ShopRackDeltaInfo info;
        if (!reader.U32(info.objectId) ||
            !ReadI32(reader, info.version) ||
            !reader.U8(info.rackType) ||
            !reader.U8(info.rackIndex) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<ShopRackDeltaInfo> DecodeGCShopBought(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ShopRackDeltaInfo info;
        if (!reader.U32(info.objectId) ||
            !ReadI32(reader, info.version) ||
            !reader.U8(info.rackType) ||
            !reader.U8(info.rackIndex) ||
            !ReadShopItemCore(reader, info.item) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.item.rackIndex = info.rackIndex;
        return info;
    }

    std::optional<ShopMarketConditionInfo> DecodeGCShopMarketCondition(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ShopMarketConditionInfo info;
        if (!reader.U32(info.objectId) ||
            !ReadI16(reader, info.marketCondBuy) ||
            !ReadI16(reader, info.marketCondSell) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<ModifyInfoRecord> DecodeGCUseOK(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ModifyInfoRecord info;
        if (!ReadModifyInfo(reader, info) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.raw = body;
        return info;
    }

    std::optional<TradeCodeInfo> DecodeGCTradePrepare(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        TradeCodeInfo info;
        if (!reader.U32(info.targetObjectId) || !reader.U8(info.code) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<TradeItemInfo> DecodeGCTradeAddItem(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        TradeItemInfo info;
        if (!reader.U32(info.targetObjectId) ||
            !reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !ReadPCItemInfo(reader, info.item))
        {
            return std::nullopt;
        }
        if (reader.Remaining() == 1U && !reader.U8(info.cashItem))
        {
            return std::nullopt;
        }
        if (!reader.AtEnd())
        {
            return std::nullopt;
        }
        info.item.slotOrX = info.x;
        info.item.y = info.y;
        return info;
    }

    std::optional<TradeRemoveItemInfo> DecodeGCTradeRemoveItem(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        TradeRemoveItemInfo info;
        if (!reader.U32(info.targetObjectId) ||
            !reader.U32(info.itemObjectId) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<TradeMoneyInfo> DecodeGCTradeMoney(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        TradeMoneyInfo info;
        if (!reader.U32(info.targetObjectId) ||
            !reader.U32(info.amount) ||
            !reader.U8(info.code) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<TradeCodeInfo> DecodeGCTradeFinish(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        TradeCodeInfo info;
        if (!reader.U32(info.targetObjectId) || !reader.U8(info.code) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<TradeCodeInfo> DecodeGCTradeError(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        TradeCodeInfo info;
        if (!reader.U32(info.targetObjectId) || !reader.U8(info.code) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<TradeVerifyInfo> DecodeGCTradeVerify(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        TradeVerifyInfo info;
        if (!reader.U8(info.code) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<StashListInfo> DecodeGCStashList(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        StashListInfo info;
        info.raw = body;

        if (!reader.U8(info.stashNum) || !reader.U8(info.totalItemCount))
        {
            return std::nullopt;
        }

        info.items.reserve(info.totalItemCount);
        for (std::uint8_t i = 0; i < info.totalItemCount; ++i)
        {
            StashItemInfo item;
            if (!reader.U8(item.rack) ||
                !reader.U8(item.index) ||
                !ReadStashItemInfo(reader, item.item, item.cashItem))
            {
                return std::nullopt;
            }
            item.item.slotOrX = item.rack;
            item.item.y = item.index;
            info.items.push_back(std::move(item));
        }

        if (!reader.U32(info.gold) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<std::uint32_t> DecodeGCStashSell(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        std::uint32_t price = 0;
        if (!reader.U32(price) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return price;
    }

    std::optional<ZoneItemInfo> DecodeGCAddNewItemToZone(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ZoneItemInfo info;
        const auto start = reader.Offset();
        if (!reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !ReadPCItemInfo(reader, info.item) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.item.slotOrX = info.x;
        info.item.y = info.y;
        info.item.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<ZoneItemInfo> DecodeGCDropItemToZone(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ZoneItemInfo info;
        const auto start = reader.Offset();
        if (!reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !ReadPCItemInfo(reader, info.item) ||
            !reader.U32(info.dropPetObjectId) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.item.slotOrX = info.x;
        info.item.y = info.y;
        info.item.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<DeleteObjectInfo> DecodeGCDeleteAndPickUpOK(const std::vector<std::uint8_t>& body)
    {
        return DecodeGCDeleteObject(body);
    }

    std::optional<InventoryItemInfo> DecodeGCCreateItem(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        InventoryItemInfo info;
        const auto start = reader.Offset();
        if (!reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !ReadPCItemInfo(reader, info.item) ||
            !reader.U32(info.inventoryObjectId) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.item.slotOrX = info.x;
        info.item.y = info.y;
        info.item.raw = reader.RawFrom(start);
        return info;
    }

    std::optional<GearToInventoryInfo> DecodeGCAddGearToInventory(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        GearToInventoryInfo info;
        if (!reader.U8(info.slotId) ||
            !reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<CannotAddInfo> DecodeGCCannotAdd(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        CannotAddInfo info;
        if (!reader.U32(info.objectId))
        {
            return std::nullopt;
        }
        if (reader.Remaining() >= 4U)
        {
            std::uint32_t rawReason = 0;
            if (!reader.U32(rawReason))
            {
                return std::nullopt;
            }
            info.reason = static_cast<std::int32_t>(rawReason);
            info.hasReason = true;
        }
        if (!reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<RemoveFromGearInfo> DecodeGCRemoveFromGear(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        RemoveFromGearInfo info;
        if (!reader.U8(info.slotId) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<ChangeGearSlotInfo> DecodeGCChangeGearSlot(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ChangeGearSlotInfo info;
        if (!reader.U16(info.requestedGearSlotId) ||
            !ReadGearInfo(reader, info.gearItems, info.activeGearSlotId, info.active) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<DeleteObjectInfo> DecodeGCDeleteInventoryItem(const std::vector<std::uint8_t>& body)
    {
        return DecodeGCDeleteObject(body);
    }

    std::optional<ItemToItemVerify> DecodeGCAddItemToItemVerify(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ItemToItemVerify verify;
        if (!reader.U8(verify.code))
        {
            return std::nullopt;
        }
        if (reader.Remaining() >= 4U && !reader.U32(verify.parameter))
        {
            return std::nullopt;
        }
        if (!reader.AtEnd())
        {
            return std::nullopt;
        }
        return verify;
    }

    std::optional<ObjectEffectInfo> DecodeGCAddEffect(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ObjectEffectInfo info;
        if (!reader.U32(info.objectId) ||
            !reader.U16(info.effectId) ||
            !reader.U32(info.duration) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.effectIds.push_back(info.effectId);
        return info;
    }

    std::optional<ObjectEffectInfo> DecodeGCRemoveEffect(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ObjectEffectInfo info;
        std::uint8_t count = 0;
        if (!reader.U32(info.objectId) || !reader.U8(count))
        {
            return std::nullopt;
        }

        info.effectIds.reserve(count);
        for (std::uint8_t i = 0; i < count; ++i)
        {
            std::uint16_t effectId = 0;
            if (!reader.U16(effectId))
            {
                return std::nullopt;
            }
            info.effectIds.push_back(effectId);
        }

        if (!reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<TileEffectInfo> DecodeGCAddEffectToTile(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        TileEffectInfo info;
        if (!reader.U32(info.objectId) ||
            !reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !reader.U16(info.effectId) ||
            !reader.U32(info.duration) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<TileEffectInfo> DecodeGCDeleteEffectFromTile(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        TileEffectInfo info;
        if (!reader.U32(info.objectId) ||
            !reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !reader.U16(info.effectId) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<DeleteObjectInfo> DecodeGCCreatureDied(const std::vector<std::uint8_t>& body)
    {
        return DecodeGCDeleteObject(body);
    }

    std::optional<DamageInfo> DecodeGCGetDamage(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        DamageInfo info;
        if (!reader.U32(info.objectId) ||
            !reader.U16(info.damage) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<CreatureAttackInfo> DecodeGCAttack(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        CreatureAttackInfo info;
        if (!reader.U32(info.objectId) ||
            !reader.U8(info.x) ||
            !reader.U8(info.y) ||
            !reader.U8(info.direction) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.direction &= 0x07U;
        return info;
    }

    std::optional<BloodDrainInfo> DecodeGCBloodDrainOK3(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        BloodDrainInfo info;
        if (!reader.U32(info.objectId) ||
            !reader.U32(info.targetObjectId) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<ChangeDarkLightInfo> DecodeGCChangeDarkLight(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ChangeDarkLightInfo info;
        if (!reader.U8(info.darkLevel) ||
            !reader.U8(info.lightLevel) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<ChangeWeatherInfo> DecodeGCChangeWeather(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        ChangeWeatherInfo info;
        if (!reader.U8(info.weather) ||
            !reader.U8(info.weatherLevel) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<SkillInfoSummary> DecodeGCSkillInfo(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        SkillInfoSummary info;
        if (!ReadSkillSummary(reader, info) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.raw = body;
        return info;
    }

    std::optional<LearnSkillFailure> DecodeGCLearnSkillFailed(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        LearnSkillFailure info;
        if (!reader.U16(info.skillType) || !reader.U8(info.description) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<LearnSkillResult> DecodeGCLearnSkillOK(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        LearnSkillResult info;
        if (!reader.U16(info.skillType) || !reader.U8(info.domainType) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<LearnSkillReady> DecodeGCLearnSkillReady(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        LearnSkillReady info;
        if (!reader.U8(info.domainType) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<RankGemDataList> DecodeGCSendRankGemData(const std::vector<std::uint8_t>& body)
    {
        constexpr std::uint8_t kLegacyMaximumRankGemData = 56;
        PacketReader reader(body);
        std::uint8_t count = 0;
        if (!reader.U8(count) || count > kLegacyMaximumRankGemData)
        {
            return std::nullopt;
        }

        RankGemDataList result;
        result.entries.reserve(count);
        for (std::uint8_t index = 0; index < count; ++index)
        {
            RankGemDataEntry entry;
            if (!reader.U32(entry.slot) || !reader.U32(entry.type) || !reader.U32(entry.grade))
            {
                return std::nullopt;
            }
            result.entries.push_back(entry);
        }
        return reader.AtEnd() ? std::optional<RankGemDataList>{ std::move(result) } : std::nullopt;
    }

    std::optional<RealWearingInfo> DecodeGCRealWearingInfo(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        RealWearingInfo info;
        if (!reader.U32(info.info) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return info;
    }

    std::optional<PetInfoRecord> DecodeGCPetInfo(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        PetInfoRecord info;
        if (!ReadPetInfo(reader, info) ||
            !reader.U32(info.summonObjectId) ||
            !reader.AtEnd())
        {
            return std::nullopt;
        }
        info.raw = body;
        return info;
    }

    namespace
    {
        [[nodiscard]] bool ReadGQuestStatus(PacketReader& reader, GQuestStatusInfo& quest)
        {
            std::uint8_t missionCount = 0;
            if (!reader.U32(quest.questId) || !reader.U8(quest.status) || !reader.U8(missionCount))
            {
                return false;
            }
            quest.missions.clear();
            quest.missions.reserve(missionCount);
            for (std::uint8_t i = 0; i < missionCount; ++i)
            {
                GQuestMissionInfo mission;
                if (!reader.U8(mission.condition) ||
                    !reader.U16(mission.index) ||
                    !reader.U8(mission.status) ||
                    !reader.String8AllowEmpty(mission.stringArgument, 255U) ||
                    !reader.U32(mission.numericArgument))
                {
                    return false;
                }
                quest.missions.push_back(std::move(mission));
            }
            return true;
        }
    }

    std::optional<GQuestStatusList> DecodeGCGQuestStatusInfo(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        GQuestStatusList result;
        std::uint8_t questCount = 0;
        if (!reader.U8(questCount))
        {
            return std::nullopt;
        }
        result.quests.reserve(questCount);
        for (std::uint8_t i = 0; i < questCount; ++i)
        {
            GQuestStatusInfo quest;
            if (!ReadGQuestStatus(reader, quest))
            {
                return std::nullopt;
            }
            result.quests.push_back(std::move(quest));
        }
        return reader.AtEnd() ? std::optional<GQuestStatusList>{ std::move(result) } : std::nullopt;
    }

    std::optional<GQuestStatusModify> DecodeGCGQuestStatusModify(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        GQuestStatusModify result;
        if (!reader.U8(result.type) || !ReadGQuestStatus(reader, result.quest) || !reader.AtEnd())
        {
            return std::nullopt;
        }
        return result;
    }

    std::optional<GQuestInventory> DecodeGCGQuestInventory(const std::vector<std::uint8_t>& body)
    {
        PacketReader reader(body);
        GQuestInventory result;
        std::uint8_t count = 0;
        if (!reader.U8(count))
        {
            return std::nullopt;
        }
        result.itemTypes.reserve(count);
        for (std::uint8_t i = 0; i < count; ++i)
        {
            std::uint16_t itemType = 0;
            if (!reader.U16(itemType))
            {
                return std::nullopt;
            }
            result.itemTypes.push_back(itemType);
        }
        return reader.AtEnd() ? std::optional<GQuestInventory>{ std::move(result) } : std::nullopt;
    }

    std::uint8_t CalculateDefaultEncryptCode(const std::uint16_t zoneId, const std::uint8_t serverId) noexcept
    {
        return static_cast<std::uint8_t>(((zoneId >> 8U) ^ zoneId) ^ ((static_cast<std::uint16_t>(serverId) + 1U) << 4U));
    }

    bool ApplyLegacyIncomingEncryption(const PacketId id, std::vector<std::uint8_t>& body, const std::uint8_t code)
    {
        if (code == 0U)
        {
            return true;
        }

        using Field = LegacyEncryptedField;
        using Type = LegacyEncryptedFieldType;
        switch (id)
        {
        case PacketId::GCMoveOK:
            return TransformLegacyEncryptedFields(body, { Field{ Type::U8 }, Field{ Type::U8 }, Field{ Type::U8 } }, code, true);
        case PacketId::GCMoveError:
            return TransformLegacyEncryptedFields(body, { Field{ Type::U8 }, Field{ Type::U8 } }, code, true);
        default:
            return true;
        }
    }

    bool ApplyLegacyOutgoingEncryption(const PacketId id, std::vector<std::uint8_t>& body, const std::uint8_t code)
    {
        if (code == 0U)
        {
            return true;
        }

        using Field = LegacyEncryptedField;
        using Type = LegacyEncryptedFieldType;
        switch (id)
        {
        case PacketId::CGMove:
            return TransformLegacyEncryptedFields(body, { Field{ Type::U8 }, Field{ Type::U8 }, Field{ Type::U8 } }, code, false);
        case PacketId::CGNPCAskAnswer:
            return TransformLegacyEncryptedFields(body, { Field{ Type::U32 }, Field{ Type::U32 }, Field{ Type::U8 } }, code, false);
        case PacketId::CGAddZoneToMouse:
        case PacketId::CGAddItemToItem:
            return TransformLegacyEncryptedFields(
                body,
                { Field{ Type::U32 }, Field{ Type::U8 }, Field{ Type::U8 } },
                code,
                false);
        case PacketId::CGAddZoneToInventory:
        case PacketId::CGAttack:
            return TransformLegacyEncryptedFields(
                body,
                { Field{ Type::U32 }, Field{ Type::U8 }, Field{ Type::U8 }, Field{ Type::U8 }, Field{ Type::U8 } },
                code,
                false);
        case PacketId::CGAddInventoryToMouse:
        case PacketId::CGAddMouseToInventory:
        case PacketId::CGUseItemFromInventory:
            return TransformLegacyEncryptedFields(
                body,
                { Field{ Type::U32 }, Field{ Type::U32 }, Field{ Type::U8 }, Field{ Type::U8 } },
                code,
                false);
        case PacketId::CGAddGearToMouse:
        case PacketId::CGUseItemFromGear:
            return TransformLegacyEncryptedFields(
                body,
                { Field{ Type::U32 }, Field{ Type::U8 } },
                code,
                false);
        case PacketId::CGAddMouseToGear:
            return TransformLegacyEncryptedFields(
                body,
                { Field{ Type::U16 }, Field{ Type::U32 }, Field{ Type::U8 } },
                code,
                false);
        case PacketId::CGChangeGearSlot:
        case PacketId::CGCastingSkill:
            return TransformLegacyEncryptedFields(body, { Field{ Type::U16 } }, code, false);
        case PacketId::CGSkillToSelf:
            return TransformLegacyEncryptedFields(body, { Field{ Type::U16 }, Field{ Type::U16 } }, code, false);
        case PacketId::CGSkillToObject:
            return TransformLegacyEncryptedFields(
                body,
                { Field{ Type::U16 }, Field{ Type::U16 }, Field{ Type::U32 }, Field{ Type::U8 } },
                code,
                false);
        case PacketId::CGSkillToTile:
            return TransformLegacyEncryptedFields(
                body,
                { Field{ Type::U16 }, Field{ Type::U16 }, Field{ Type::U8 }, Field{ Type::U8 } },
                code,
                false);
        case PacketId::CGSkillToInventory:
            return TransformLegacyEncryptedFields(
                body,
                {
                    Field{ Type::U16 },
                    Field{ Type::U32 },
                    Field{ Type::U8 },
                    Field{ Type::U8 },
                    Field{ Type::U8 },
                    Field{ Type::U8 },
                    Field{ Type::U32 }
                },
                code,
                false);
        default:
            return true;
        }
    }
}
