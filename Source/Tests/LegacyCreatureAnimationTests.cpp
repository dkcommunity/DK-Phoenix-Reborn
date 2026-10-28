#include "Animation/LegacyCreatureAnimation.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace
{
    using phoenix::animation::CreatureAnimationProfile;
    using phoenix::animation::CreatureAnimationRequestResult;
    using phoenix::animation::CreatureAnimationState;
    using phoenix::animation::LegacyCreatureAnimation;
    using phoenix::animation::LegacyCreatureAnimationConfig;

    int failures = 0;

    void Check(const bool condition, const char* expression, const int line)
    {
        if (!condition)
        {
            std::cerr << "line " << line << ": " << expression << '\n';
            ++failures;
        }
    }

#define CHECK(expression) Check((expression), #expression, __LINE__)

    LegacyCreatureAnimationConfig MakeConfig(const CreatureAnimationProfile profile)
    {
        LegacyCreatureAnimationConfig config;
        config.profile = profile;
        config.actionFrameCounts.fill(4U);
        return config;
    }

    void VerifyActionMatrix()
    {
        constexpr std::array profiles{
            CreatureAnimationProfile::Creature,
            CreatureAnimationProfile::Slayer,
            CreatureAnimationProfile::Vampire,
            CreatureAnimationProfile::Ousters
        };
        constexpr std::array states{
            CreatureAnimationState::Idle,
            CreatureAnimationState::Walk,
            CreatureAnimationState::Attack,
            CreatureAnimationState::Magic,
            CreatureAnimationState::Damage,
            CreatureAnimationState::Drained,
            CreatureAnimationState::Die,
            CreatureAnimationState::Vampire
        };

        for (const auto profile : profiles)
        {
            for (std::uint8_t direction = 0U; direction < 8U; ++direction)
            {
                for (const auto state : states)
                {
                    for (const bool beginMoving : { false, true })
                    {
                        LegacyCreatureAnimation animation;
                        animation.Configure(MakeConfig(profile), direction);
                        if (beginMoving)
                        {
                            CHECK(animation.StartMovement(direction) == CreatureAnimationRequestResult::Applied);
                        }

                        const auto result = animation.Request(state, direction);
                        if (profile == CreatureAnimationProfile::Slayer && state == CreatureAnimationState::Vampire)
                        {
                            CHECK(result == CreatureAnimationRequestResult::UnsupportedAction);
                            animation.Configure(MakeConfig(profile), direction);
                            CHECK(animation.State() == CreatureAnimationState::Idle);
                            continue;
                        }
                        CHECK(result == CreatureAnimationRequestResult::Applied);
                        CHECK(animation.ActionId() == LegacyCreatureAnimation::ResolveActionId(profile, state).value());
                        CHECK(animation.CurrentDirection() == direction);
                        CHECK(animation.CurrentFrame() == 0U);
                        CHECK(animation.IsLooping() ==
                            (state == CreatureAnimationState::Idle || state == CreatureAnimationState::Walk));

                        if (state == CreatureAnimationState::Die)
                        {
                            animation.AdvanceTicks(8U);
                            CHECK(animation.State() == CreatureAnimationState::Die);
                            CHECK(animation.CurrentFrame() == 3U);
                            CHECK(animation.Request(CreatureAnimationState::Idle) ==
                                CreatureAnimationRequestResult::RejectedDead);
                        }
                        else if (state != CreatureAnimationState::Idle && state != CreatureAnimationState::Walk)
                        {
                            animation.StopMovement();
                            animation.AdvanceTicks(5U);
                            CHECK(animation.State() == CreatureAnimationState::Idle);
                            CHECK(animation.CurrentFrame() == 0U);
                        }

                        animation.Configure(MakeConfig(profile), direction);
                        animation.ResetClock(9000U);
                        CHECK(animation.State() == CreatureAnimationState::Idle);
                        CHECK(animation.CurrentFrame() == 0U);
                        CHECK(animation.CurrentDirection() == direction);
                    }
                }
            }
        }
    }

    void VerifyDirectionRotation()
    {
        LegacyCreatureAnimation animation;
        animation.Configure(MakeConfig(CreatureAnimationProfile::Creature), 0U);
        CHECK(animation.Request(CreatureAnimationState::Attack, static_cast<std::uint8_t>(3U)) == CreatureAnimationRequestResult::Applied);
        animation.AdvanceTicks(1U);
        CHECK(animation.CurrentDirection() == 1U);
        animation.AdvanceTicks(1U);
        CHECK(animation.CurrentDirection() == 2U);
        animation.AdvanceTicks(1U);
        CHECK(animation.CurrentDirection() == 3U);
    }

    void VerifyTimingAndMovementSync()
    {
        auto config = MakeConfig(CreatureAnimationProfile::Vampire);
        config.actionFrameCounts[1U] = 8U;
        LegacyCreatureAnimation animation;
        animation.Configure(config, 4U);
        CHECK(animation.StartMovement(4U) == CreatureAnimationRequestResult::Applied);
        animation.AdvanceTo(1000U);
        animation.AdvanceTo(1061U);
        CHECK(animation.MovementFrame() == 0U);
        animation.AdvanceTo(1062U);
        CHECK(animation.MovementFrame() == 1U);
        animation.AdvanceTo(1496U);
        CHECK(animation.MovementFrame() == 8U);
        CHECK(animation.CurrentFrame() == 7U);
        animation.StopMovement();
        CHECK(animation.State() == CreatureAnimationState::Idle);
    }

    void VerifyLegacyTransitions()
    {
        LegacyCreatureAnimation animation;
        animation.Configure(MakeConfig(CreatureAnimationProfile::Creature), 2U);
        CHECK(animation.Request(CreatureAnimationState::Attack) == CreatureAnimationRequestResult::Applied);
        CHECK(animation.Request(CreatureAnimationState::Damage) == CreatureAnimationRequestResult::QueuedIdle);
        animation.AdvanceTicks(5U);
        CHECK(animation.State() == CreatureAnimationState::Idle);

        animation.Configure(MakeConfig(CreatureAnimationProfile::Creature), 2U);
        CHECK(animation.Request(CreatureAnimationState::Damage) == CreatureAnimationRequestResult::Applied);
        CHECK(animation.State() == CreatureAnimationState::Damage);

        auto missing = MakeConfig(CreatureAnimationProfile::Creature);
        missing.actionFrameCounts[3U] = 0U;
        animation.Configure(missing, 2U);
        CHECK(animation.Request(CreatureAnimationState::Magic) == CreatureAnimationRequestResult::MissingFrames);
        CHECK(animation.State() == CreatureAnimationState::Idle);

        auto speedVariant = MakeConfig(CreatureAnimationProfile::Vampire);
        speedVariant.actionIdOverrides[static_cast<std::size_t>(CreatureAnimationState::Attack)] = 9U;
        animation.Configure(speedVariant, 2U);
        CHECK(animation.Request(CreatureAnimationState::Attack) == CreatureAnimationRequestResult::Applied);
        CHECK(animation.ActionId() == 9U);
    }

    void VerifyMapReloadReset()
    {
        LegacyCreatureAnimation animation;
        const auto config = MakeConfig(CreatureAnimationProfile::Ousters);
        animation.Configure(config, 7U);
        CHECK(animation.Request(CreatureAnimationState::Magic, static_cast<std::uint8_t>(1U)) == CreatureAnimationRequestResult::Applied);
        animation.AdvanceTicks(2U);
        animation.Configure(config, 5U);
        animation.ResetClock(5000U);
        CHECK(animation.State() == CreatureAnimationState::Idle);
        CHECK(animation.ActionId() == 7U);
        CHECK(animation.CurrentFrame() == 0U);
        CHECK(animation.CurrentDirection() == 5U);
        animation.AdvanceTo(5061U);
        CHECK(animation.CurrentFrame() == 0U);
        animation.AdvanceTo(5062U);
        CHECK(animation.CurrentFrame() == 1U);
    }
}

int main()
{
    VerifyActionMatrix();
    VerifyDirectionRotation();
    VerifyTimingAndMovementSync();
    VerifyLegacyTransitions();
    VerifyMapReloadReset();
    if (failures != 0)
    {
        std::cerr << failures << " Legacy creature animation checks failed.\n";
        return 1;
    }
    std::cout << "Legacy creature animation matrix passed.\n";
    return 0;
}
