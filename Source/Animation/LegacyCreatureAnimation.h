#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace phoenix::animation
{
    constexpr std::uint32_t LegacyCreatureTickMs = 62U;
    constexpr std::size_t LegacyCreatureActionCapacity = 40U;

    enum class CreatureAnimationState : std::uint8_t
    {
        Idle,
        Walk,
        Attack,
        Magic,
        Damage,
        Drained,
        Die,
        Vampire
    };

    enum class CreatureAnimationProfile : std::uint8_t
    {
        Creature,
        Slayer,
        Vampire,
        Ousters
    };

    enum class CreatureAnimationRequestResult : std::uint8_t
    {
        Applied,
        QueuedIdle,
        RejectedDead,
        UnsupportedAction,
        MissingFrames
    };

    struct LegacyCreatureAnimationConfig
    {
        CreatureAnimationProfile profile{ CreatureAnimationProfile::Creature };
        std::array<std::uint16_t, LegacyCreatureActionCapacity> actionFrameCounts{};
        std::array<std::uint8_t, 8> actionIdOverrides{
            0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU
        };
    };

    class LegacyCreatureAnimation final
    {
    public:
        void Configure(const LegacyCreatureAnimationConfig& config, std::uint8_t direction, bool alive = true) noexcept;
        void ResetClock(std::uint64_t nowMs = 0U) noexcept;

        [[nodiscard]] CreatureAnimationRequestResult Request(
            CreatureAnimationState state,
            std::optional<std::uint8_t> serverDirection = std::nullopt) noexcept;
        [[nodiscard]] CreatureAnimationRequestResult StartMovement(std::uint8_t movementDirection) noexcept;
        void StopMovement() noexcept;
        void SetServerDirection(std::uint8_t direction) noexcept;
        void AdvanceTo(std::uint64_t nowMs) noexcept;
        void AdvanceTicks(std::uint32_t ticks) noexcept;

        [[nodiscard]] bool Configured() const noexcept { return configured_; }
        [[nodiscard]] bool Alive() const noexcept { return alive_; }
        [[nodiscard]] bool Moving() const noexcept { return moving_; }
        [[nodiscard]] CreatureAnimationState State() const noexcept { return state_; }
        [[nodiscard]] CreatureAnimationProfile Profile() const noexcept { return config_.profile; }
        [[nodiscard]] std::uint8_t ActionId() const noexcept { return actionId_; }
        [[nodiscard]] std::uint8_t CurrentDirection() const noexcept { return currentDirection_; }
        [[nodiscard]] std::uint8_t ServerDirection() const noexcept { return serverDirection_; }
        [[nodiscard]] std::uint16_t CurrentFrame() const noexcept;
        [[nodiscard]] std::uint16_t ActionFrameCount() const noexcept { return actionFrameCount_; }
        [[nodiscard]] std::uint16_t MovementFrame() const noexcept { return moveCount_; }
        [[nodiscard]] std::uint16_t MovementFrameCount() const noexcept { return moveCountMax_; }
        [[nodiscard]] std::uint64_t LastUpdateMs() const noexcept { return lastUpdateMs_; }
        [[nodiscard]] std::uint64_t AccumulatedMs() const noexcept { return accumulatedMs_; }
        [[nodiscard]] std::uint64_t TotalTicks() const noexcept { return totalTicks_; }
        [[nodiscard]] bool IsLooping() const noexcept;
        [[nodiscard]] std::optional<std::uint8_t> ActionIdForState(CreatureAnimationState state) const noexcept
        {
            return ConfiguredActionId(state);
        }

        [[nodiscard]] static std::optional<std::uint8_t> ResolveActionId(
            CreatureAnimationProfile profile,
            CreatureAnimationState state) noexcept;
        [[nodiscard]] static const char* StateName(CreatureAnimationState state) noexcept;

    private:
        void Tick() noexcept;
        void Activate(CreatureAnimationState state, std::uint8_t actionId) noexcept;
        void ActivateIdle() noexcept;
        void RotateToward(std::uint8_t direction) noexcept;
        [[nodiscard]] std::uint16_t FrameCount(std::uint8_t actionId) const noexcept;
        [[nodiscard]] std::optional<std::uint8_t> ConfiguredActionId(CreatureAnimationState state) const noexcept;
        [[nodiscard]] bool NonLoopActionInProgress() const noexcept;

        LegacyCreatureAnimationConfig config_;
        CreatureAnimationState state_{ CreatureAnimationState::Idle };
        std::uint8_t actionId_{ 0U };
        std::uint8_t currentDirection_{ 0U };
        std::uint8_t serverDirection_{ 0U };
        std::uint8_t movementDirection_{ 0U };
        std::uint16_t actionCount_{ 0U };
        std::uint16_t actionFrameCount_{ 0U };
        std::uint16_t moveCount_{ 0U };
        std::uint16_t moveCountMax_{ 0U };
        std::uint64_t lastUpdateMs_{ 0U };
        std::uint64_t accumulatedMs_{ 0U };
        std::uint64_t totalTicks_{ 0U };
        bool configured_{ false };
        bool alive_{ true };
        bool moving_{ false };
        bool queuedIdle_{ false };
    };
}
