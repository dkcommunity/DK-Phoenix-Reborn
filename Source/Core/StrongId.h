#pragma once

#include <cstdint>
#include <type_traits>

namespace phoenix::core
{
    template <typename Tag, typename Value = std::uint32_t>
    class StrongId final
    {
        static_assert(std::is_integral_v<Value>, "StrongId value type must be integral.");

    public:
        using value_type = Value;

        constexpr StrongId() noexcept = default;
        explicit constexpr StrongId(Value value) noexcept
            : value_(value)
        {
        }

        [[nodiscard]] constexpr Value ValueOf() const noexcept
        {
            return value_;
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value_ != Value{};
        }

        friend constexpr bool operator==(StrongId left, StrongId right) noexcept
        {
            return left.value_ == right.value_;
        }

        friend constexpr bool operator!=(StrongId left, StrongId right) noexcept
        {
            return !(left == right);
        }

        friend constexpr bool operator<(StrongId left, StrongId right) noexcept
        {
            return left.value_ < right.value_;
        }

    private:
        Value value_{};
    };

    struct BodyIdTag;
    struct ActionIdTag;
    struct DirectionIdTag;
    struct FrameIndexTag;
    struct SpriteIdTag;
    struct ArchiveIdTag;
    struct AudioIdTag;

    using BodyId = StrongId<BodyIdTag>;
    using ActionId = StrongId<ActionIdTag>;
    using DirectionId = StrongId<DirectionIdTag>;
    using FrameIndex = StrongId<FrameIndexTag>;
    using SpriteId = StrongId<SpriteIdTag>;
    using ArchiveId = StrongId<ArchiveIdTag>;
    using AudioId = StrongId<AudioIdTag>;
}
