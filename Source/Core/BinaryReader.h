#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::core
{
    class BinaryReader final
    {
    public:
        BinaryReader(const std::byte* data, std::size_t size) noexcept;

        [[nodiscard]] std::size_t Position() const noexcept;
        [[nodiscard]] std::size_t Size() const noexcept;
        [[nodiscard]] std::size_t Remaining() const noexcept;
        [[nodiscard]] bool CanRead(std::size_t byteCount) const noexcept;

        bool Seek(std::uint64_t offset) noexcept;
        bool Skip(std::uint64_t byteCount) noexcept;

        std::optional<std::uint8_t> ReadU8() noexcept;
        std::optional<std::uint16_t> ReadU16LE() noexcept;
        std::optional<std::uint32_t> ReadU32LE() noexcept;
        std::optional<std::uint64_t> ReadU64LE() noexcept;
        std::optional<std::vector<std::byte>> ReadBytes(std::size_t byteCount);

        [[nodiscard]] std::wstring LastError() const;

    private:
        void SetError(std::wstring message);

        const std::byte* data_{ nullptr };
        std::size_t size_{ 0 };
        std::size_t position_{ 0 };
        std::wstring lastError_;
    };
}
