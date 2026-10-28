#include "Core/BinaryReader.h"

#include <algorithm>
#include <limits>

namespace phoenix::core
{
    BinaryReader::BinaryReader(const std::byte* data, std::size_t size) noexcept
        : data_(data)
        , size_(size)
    {
    }

    std::size_t BinaryReader::Position() const noexcept
    {
        return position_;
    }

    std::size_t BinaryReader::Size() const noexcept
    {
        return size_;
    }

    std::size_t BinaryReader::Remaining() const noexcept
    {
        return size_ - position_;
    }

    bool BinaryReader::CanRead(std::size_t byteCount) const noexcept
    {
        return data_ != nullptr && byteCount <= Remaining();
    }

    bool BinaryReader::Seek(std::uint64_t offset) noexcept
    {
        if (offset > static_cast<std::uint64_t>(size_))
        {
            SetError(L"Seek offset is outside the buffer.");
            return false;
        }

        position_ = static_cast<std::size_t>(offset);
        return true;
    }

    bool BinaryReader::Skip(std::uint64_t byteCount) noexcept
    {
        const auto remaining = static_cast<std::uint64_t>(Remaining());
        if (byteCount > remaining)
        {
            SetError(L"Skip count is outside the remaining buffer.");
            return false;
        }

        position_ += static_cast<std::size_t>(byteCount);
        return true;
    }

    std::optional<std::uint8_t> BinaryReader::ReadU8() noexcept
    {
        if (!CanRead(1))
        {
            SetError(L"Unable to read uint8: end of buffer.");
            return std::nullopt;
        }

        const auto value = static_cast<std::uint8_t>(data_[position_]);
        ++position_;
        return value;
    }

    std::optional<std::uint16_t> BinaryReader::ReadU16LE() noexcept
    {
        if (!CanRead(2))
        {
            SetError(L"Unable to read uint16: end of buffer.");
            return std::nullopt;
        }

        const auto low = static_cast<std::uint16_t>(static_cast<std::uint8_t>(data_[position_]));
        const auto high = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(static_cast<std::uint8_t>(data_[position_ + 1])) << 8U);
        const auto value = static_cast<std::uint16_t>(low | high);
        position_ += 2;
        return value;
    }

    std::optional<std::uint32_t> BinaryReader::ReadU32LE() noexcept
    {
        if (!CanRead(4))
        {
            SetError(L"Unable to read uint32: end of buffer.");
            return std::nullopt;
        }

        std::uint32_t value = 0;
        for (std::size_t index = 0; index < 4; ++index)
        {
            value |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(data_[position_ + index])) << (index * 8U);
        }

        position_ += 4;
        return value;
    }

    std::optional<std::uint64_t> BinaryReader::ReadU64LE() noexcept
    {
        if (!CanRead(8))
        {
            SetError(L"Unable to read uint64: end of buffer.");
            return std::nullopt;
        }

        std::uint64_t value = 0;
        for (std::size_t index = 0; index < 8; ++index)
        {
            value |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(data_[position_ + index])) << (index * 8U);
        }

        position_ += 8;
        return value;
    }

    std::optional<std::vector<std::byte>> BinaryReader::ReadBytes(std::size_t byteCount)
    {
        if (!CanRead(byteCount))
        {
            SetError(L"Unable to read byte block: end of buffer.");
            return std::nullopt;
        }

        std::vector<std::byte> bytes(byteCount);
        std::copy_n(data_ + position_, byteCount, bytes.begin());
        position_ += byteCount;
        return bytes;
    }

    std::wstring BinaryReader::LastError() const
    {
        return lastError_;
    }

    void BinaryReader::SetError(std::wstring message)
    {
        lastError_ = std::move(message);
    }
}
