#include "Animation/AnimationController.h"

#include <utility>

namespace phoenix::animation
{
    void AnimationController::Play(AnimationClip clip)
    {
        clip_ = std::move(clip);
        currentFrame_ = 0;
        elapsedInFrameMs_ = 0;
        finished_ = clip_->frames.empty();
    }

    void AnimationController::Stop() noexcept
    {
        clip_.reset();
        currentFrame_ = 0;
        elapsedInFrameMs_ = 0;
        finished_ = false;
    }

    void AnimationController::Update(std::uint32_t elapsedMs) noexcept
    {
        if (!clip_.has_value() || finished_ || clip_->frames.empty())
        {
            return;
        }

        elapsedInFrameMs_ += elapsedMs;
        while (elapsedInFrameMs_ >= CurrentFrameDuration() && !finished_)
        {
            elapsedInFrameMs_ -= CurrentFrameDuration();
            ++currentFrame_;

            if (currentFrame_ >= clip_->frames.size())
            {
                if (clip_->loop)
                {
                    currentFrame_ = 0;
                }
                else
                {
                    currentFrame_ = static_cast<std::uint32_t>(clip_->frames.size() - 1);
                    finished_ = true;
                }
            }
        }
    }

    std::optional<AnimationSample> AnimationController::CurrentSample() const noexcept
    {
        if (!clip_.has_value() || clip_->frames.empty())
        {
            return std::nullopt;
        }

        const auto& frame = clip_->frames[currentFrame_];
        return AnimationSample{
            frame.visual,
            core::FrameIndex(currentFrame_),
            finished_
        };
    }

    bool AnimationController::IsPlaying() const noexcept
    {
        return clip_.has_value() && !finished_;
    }

    std::uint32_t AnimationController::CurrentFrameDuration() const noexcept
    {
        const auto duration = clip_->frames[currentFrame_].durationMs;
        return duration == 0 ? 1U : duration;
    }
}
