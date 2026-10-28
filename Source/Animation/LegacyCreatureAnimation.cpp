#include "Animation/LegacyCreatureAnimation.h"

#include <algorithm>
#include <cstdlib>

namespace phoenix::animation
{
    namespace
    {
        constexpr std::uint8_t kDirectionMask = 0x07U;
        constexpr std::uint8_t kActionIdle = 0U;
        constexpr std::uint8_t kActionWalk = 1U;
        constexpr std::uint8_t kActionAttack = 2U;
        constexpr std::uint8_t kActionMagic = 3U;
        constexpr std::uint8_t kActionDamage = 4U;
        constexpr std::uint8_t kActionDrained = 5U;
        constexpr std::uint8_t kActionDie = 6U;
        constexpr std::uint8_t kActionVampire = 7U;
        constexpr std::uint8_t kActionOustersDrain = 11U;
    }

    void LegacyCreatureAnimation::Configure(
        const LegacyCreatureAnimationConfig& config,
        const std::uint8_t direction,
        const bool alive) noexcept
    {
        config_ = config;
        currentDirection_ = direction & kDirectionMask;
        serverDirection_ = currentDirection_;
        movementDirection_ = currentDirection_;
        actionCount_ = 0U;
        moveCount_ = 0U;
        moveCountMax_ = 0U;
        lastUpdateMs_ = 0U;
        accumulatedMs_ = 0U;
        totalTicks_ = 0U;
        configured_ = true;
        alive_ = alive;
        moving_ = false;
        queuedIdle_ = false;
        ActivateIdle();
        if (!alive_)
        {
            (void)Request(CreatureAnimationState::Die, currentDirection_);
        }
    }

    void LegacyCreatureAnimation::ResetClock(const std::uint64_t nowMs) noexcept
    {
        lastUpdateMs_ = nowMs;
        accumulatedMs_ = 0U;
    }

    CreatureAnimationRequestResult LegacyCreatureAnimation::Request(
        const CreatureAnimationState state,
        const std::optional<std::uint8_t> serverDirection) noexcept
    {
        if (serverDirection)
        {
            SetServerDirection(*serverDirection);
        }

        if (!alive_ && state != CreatureAnimationState::Die)
        {
            return CreatureAnimationRequestResult::RejectedDead;
        }
        if (state == CreatureAnimationState::Walk)
        {
            return StartMovement(serverDirection.value_or(serverDirection_));
        }
        if (state == CreatureAnimationState::Damage &&
            state_ != CreatureAnimationState::Damage &&
            state_ != CreatureAnimationState::Idle &&
            state_ != CreatureAnimationState::Walk)
        {
            queuedIdle_ = true;
            return CreatureAnimationRequestResult::QueuedIdle;
        }

        const auto action = ConfiguredActionId(state);
        if (!action)
        {
            return CreatureAnimationRequestResult::UnsupportedAction;
        }

        if (state == CreatureAnimationState::Die)
        {
            alive_ = false;
            moving_ = false;
        }

        if (FrameCount(*action) == 0U)
        {
            ActivateIdle();
            return CreatureAnimationRequestResult::MissingFrames;
        }

        queuedIdle_ = false;
        Activate(state, *action);
        return CreatureAnimationRequestResult::Applied;
    }

    CreatureAnimationRequestResult LegacyCreatureAnimation::StartMovement(const std::uint8_t movementDirection) noexcept
    {
        if (!alive_)
        {
            return CreatureAnimationRequestResult::RejectedDead;
        }

        const auto action = ConfiguredActionId(CreatureAnimationState::Walk);
        if (!action)
        {
            return CreatureAnimationRequestResult::UnsupportedAction;
        }
        const auto frameCount = FrameCount(*action);
        if (frameCount == 0U)
        {
            ActivateIdle();
            return CreatureAnimationRequestResult::MissingFrames;
        }

        movementDirection_ = movementDirection & kDirectionMask;
        serverDirection_ = movementDirection_;
        moving_ = true;
        moveCount_ = 0U;
        moveCountMax_ = frameCount;
        if (!NonLoopActionInProgress())
        {
            Activate(CreatureAnimationState::Walk, *action);
        }
        return CreatureAnimationRequestResult::Applied;
    }

    void LegacyCreatureAnimation::StopMovement() noexcept
    {
        moving_ = false;
        moveCount_ = moveCountMax_;
        if (state_ == CreatureAnimationState::Walk)
        {
            ActivateIdle();
        }
    }

    void LegacyCreatureAnimation::SetServerDirection(const std::uint8_t direction) noexcept
    {
        serverDirection_ = direction & kDirectionMask;
    }

    void LegacyCreatureAnimation::AdvanceTo(const std::uint64_t nowMs) noexcept
    {
        if (lastUpdateMs_ == 0U)
        {
            lastUpdateMs_ = nowMs;
            return;
        }
        if (nowMs < lastUpdateMs_)
        {
            ResetClock(nowMs);
            return;
        }

        accumulatedMs_ += nowMs - lastUpdateMs_;
        lastUpdateMs_ = nowMs;
        while (accumulatedMs_ >= LegacyCreatureTickMs)
        {
            accumulatedMs_ -= LegacyCreatureTickMs;
            Tick();
        }
    }

    void LegacyCreatureAnimation::AdvanceTicks(const std::uint32_t ticks) noexcept
    {
        for (std::uint32_t index = 0U; index < ticks; ++index)
        {
            Tick();
        }
    }

    std::uint16_t LegacyCreatureAnimation::CurrentFrame() const noexcept
    {
        if (state_ == CreatureAnimationState::Walk)
        {
            return moveCountMax_ == 0U ? 0U : std::min<std::uint16_t>(moveCount_, moveCountMax_ - 1U);
        }
        return actionFrameCount_ == 0U ? 0U : std::min<std::uint16_t>(actionCount_, actionFrameCount_ - 1U);
    }

    bool LegacyCreatureAnimation::IsLooping() const noexcept
    {
        return state_ == CreatureAnimationState::Idle || state_ == CreatureAnimationState::Walk;
    }

    std::optional<std::uint8_t> LegacyCreatureAnimation::ResolveActionId(
        const CreatureAnimationProfile profile,
        const CreatureAnimationState state) noexcept
    {
        if (profile == CreatureAnimationProfile::Ousters)
        {
            switch (state)
            {
            case CreatureAnimationState::Idle: return kActionIdle;
            case CreatureAnimationState::Walk: return kActionWalk;
            case CreatureAnimationState::Attack: return kActionAttack;
            case CreatureAnimationState::Magic: return kActionMagic;
            case CreatureAnimationState::Damage: return kActionDamage;
            case CreatureAnimationState::Drained: return kActionDrained;
            case CreatureAnimationState::Die: return kActionDie;
            case CreatureAnimationState::Vampire: return kActionOustersDrain;
            }
        }

        switch (state)
        {
        case CreatureAnimationState::Idle: return kActionIdle;
        case CreatureAnimationState::Walk: return kActionWalk;
        case CreatureAnimationState::Attack: return kActionAttack;
        case CreatureAnimationState::Magic: return kActionMagic;
        case CreatureAnimationState::Damage: return kActionDamage;
        case CreatureAnimationState::Drained: return kActionDrained;
        case CreatureAnimationState::Die: return kActionDie;
        case CreatureAnimationState::Vampire:
            return profile == CreatureAnimationProfile::Slayer ? std::nullopt : std::optional<std::uint8_t>{ kActionVampire };
        }
        return std::nullopt;
    }

    const char* LegacyCreatureAnimation::StateName(const CreatureAnimationState state) noexcept
    {
        switch (state)
        {
        case CreatureAnimationState::Idle: return "Idle";
        case CreatureAnimationState::Walk: return "Walk";
        case CreatureAnimationState::Attack: return "Attack";
        case CreatureAnimationState::Magic: return "Magic";
        case CreatureAnimationState::Damage: return "Damage";
        case CreatureAnimationState::Drained: return "Drained";
        case CreatureAnimationState::Die: return "Die";
        case CreatureAnimationState::Vampire: return "Vampire";
        }
        return "Unknown";
    }

    void LegacyCreatureAnimation::Tick() noexcept
    {
        ++totalTicks_;
        const auto targetDirection = NonLoopActionInProgress()
            ? serverDirection_
            : (moving_ && moveCount_ > 0U && moveCount_ < moveCountMax_ ? movementDirection_ : serverDirection_);
        RotateToward(targetDirection);

        if (moving_ && moveCount_ < moveCountMax_)
        {
            ++moveCount_;
        }

        if (actionFrameCount_ == 0U)
        {
            return;
        }
        if (actionCount_ < actionFrameCount_)
        {
            ++actionCount_;
            return;
        }

        if (state_ == CreatureAnimationState::Die)
        {
            return;
        }
        if (state_ == CreatureAnimationState::Idle)
        {
            actionCount_ = 0U;
            return;
        }
        if (state_ == CreatureAnimationState::Walk)
        {
            if (!moving_)
            {
                ActivateIdle();
            }
            return;
        }

        if (queuedIdle_ || !moving_)
        {
            queuedIdle_ = false;
            ActivateIdle();
            return;
        }

        const auto walkAction = ConfiguredActionId(CreatureAnimationState::Walk);
        if (walkAction && FrameCount(*walkAction) != 0U)
        {
            Activate(CreatureAnimationState::Walk, *walkAction);
        }
        else
        {
            ActivateIdle();
        }
    }

    void LegacyCreatureAnimation::Activate(const CreatureAnimationState state, const std::uint8_t actionId) noexcept
    {
        state_ = state;
        actionId_ = actionId;
        actionCount_ = 0U;
        actionFrameCount_ = FrameCount(actionId);
    }

    void LegacyCreatureAnimation::ActivateIdle() noexcept
    {
        const auto idle = ConfiguredActionId(CreatureAnimationState::Idle).value_or(kActionIdle);
        Activate(CreatureAnimationState::Idle, idle);
    }

    void LegacyCreatureAnimation::RotateToward(const std::uint8_t direction) noexcept
    {
        const auto target = direction & kDirectionMask;
        if (currentDirection_ == target)
        {
            return;
        }

        const auto gap = static_cast<std::uint8_t>((currentDirection_ + 8U - target) & kDirectionMask);
        if (gap == 4U)
        {
            currentDirection_ = static_cast<std::uint8_t>((currentDirection_ + ((std::rand() & 1) != 0 ? 1U : 7U)) & kDirectionMask);
        }
        else if (gap < 4U)
        {
            currentDirection_ = static_cast<std::uint8_t>((currentDirection_ + 7U) & kDirectionMask);
        }
        else
        {
            currentDirection_ = static_cast<std::uint8_t>((currentDirection_ + 1U) & kDirectionMask);
        }
    }

    std::uint16_t LegacyCreatureAnimation::FrameCount(const std::uint8_t actionId) const noexcept
    {
        return actionId < config_.actionFrameCounts.size() ? config_.actionFrameCounts[actionId] : 0U;
    }

    std::optional<std::uint8_t> LegacyCreatureAnimation::ConfiguredActionId(
        const CreatureAnimationState state) const noexcept
    {
        const auto index = static_cast<std::size_t>(state);
        if (index < config_.actionIdOverrides.size() && config_.actionIdOverrides[index] != 0xFFU)
        {
            return config_.actionIdOverrides[index];
        }
        return ResolveActionId(config_.profile, state);
    }

    bool LegacyCreatureAnimation::NonLoopActionInProgress() const noexcept
    {
        return state_ != CreatureAnimationState::Idle &&
            state_ != CreatureAnimationState::Walk &&
            actionCount_ < actionFrameCount_;
    }
}
