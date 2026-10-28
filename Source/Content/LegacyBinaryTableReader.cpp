#include "Content/LegacyBinaryTableReader.h"

#include <fstream>
#include <iterator>
#include <utility>

namespace phoenix::content
{
    LegacyBinaryTableReader::LegacyBinaryTableReader(std::vector<std::uint8_t> bytes)
        : bytes_(std::move(bytes))
    {
    }

    bool LegacyBinaryTableReader::U8(std::uint8_t& value) noexcept
    {
        if (offset_ + 1U > bytes_.size())
        {
            return false;
        }
        value = bytes_[offset_++];
        return true;
    }

    bool LegacyBinaryTableReader::U16(std::uint16_t& value) noexcept
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

    bool LegacyBinaryTableReader::I32(std::int32_t& value) noexcept
    {
        std::uint32_t raw = 0;
        if (!U32(raw))
        {
            return false;
        }
        value = static_cast<std::int32_t>(raw);
        return true;
    }

    bool LegacyBinaryTableReader::U32(std::uint32_t& value) noexcept
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

    bool LegacyBinaryTableReader::BoolByte(bool& value) noexcept
    {
        std::uint8_t raw = 0;
        if (!U8(raw))
        {
            return false;
        }
        value = raw != 0;
        return true;
    }

    bool LegacyBinaryTableReader::String(std::string& value)
    {
        std::uint32_t length = 0;
        if (!U32(length) || length > 65535U || offset_ + length > bytes_.size())
        {
            return false;
        }
        value.assign(
            reinterpret_cast<const char*>(bytes_.data() + offset_),
            reinterpret_cast<const char*>(bytes_.data() + offset_ + length));
        offset_ += length;
        return true;
    }

    bool LegacyBinaryTableReader::Skip(const std::size_t size) noexcept
    {
        if (offset_ + size > bytes_.size())
        {
            return false;
        }
        offset_ += size;
        return true;
    }

    std::size_t LegacyBinaryTableReader::Offset() const noexcept
    {
        return offset_;
    }

    std::size_t LegacyBinaryTableReader::Size() const noexcept
    {
        return bytes_.size();
    }

    std::optional<std::vector<std::uint8_t>> ReadBinaryFile(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            return std::nullopt;
        }
        return std::vector<std::uint8_t>(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
    }
}
