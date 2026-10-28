#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::content
{
    class LegacyBinaryTableReader final
    {
    public:
        explicit LegacyBinaryTableReader(std::vector<std::uint8_t> bytes);

        [[nodiscard]] bool U8(std::uint8_t& value) noexcept;
        [[nodiscard]] bool U16(std::uint16_t& value) noexcept;
        [[nodiscard]] bool I32(std::int32_t& value) noexcept;
        [[nodiscard]] bool U32(std::uint32_t& value) noexcept;
        [[nodiscard]] bool BoolByte(bool& value) noexcept;
        [[nodiscard]] bool String(std::string& value);
        [[nodiscard]] bool Skip(std::size_t size) noexcept;

        [[nodiscard]] std::size_t Offset() const noexcept;
        [[nodiscard]] std::size_t Size() const noexcept;

    private:
        std::vector<std::uint8_t> bytes_;
        std::size_t offset_{ 0 };
    };

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> ReadBinaryFile(const std::filesystem::path& path);
}
