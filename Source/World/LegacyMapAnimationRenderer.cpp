#include "World/LegacyMapAnimationRenderer.h"

#include "Assets/AnimationObjectFramePackReader.h"
#include "Assets/EffectFramePackReader.h"

#include <algorithm>
#include <limits>

namespace phoenix::world
{
    namespace
    {
        constexpr std::size_t MissingEffectSequence = std::numeric_limits<std::size_t>::max();

        std::vector<std::size_t> BuildDirectionZeroLookup(
            const std::vector<assets::LegacyEffectFrameSequence>& sequences)
        {
            std::size_t count = 0U;
            for (const auto& sequence : sequences)
                count = std::max(count, static_cast<std::size_t>(sequence.frameId) + 1U);
            std::vector<std::size_t> lookup(count, MissingEffectSequence);
            for (std::size_t index = 0; index < sequences.size(); ++index)
                if (sequences[index].directionId.ValueOf() == 0U)
                    lookup[sequences[index].frameId] = index;
            return lookup;
        }
    }

    bool LegacyMapAnimationRenderer::Load(const LegacyMapAnimationFramePackPaths& paths)
    {
        Clear();
        assets::AnimationObjectFramePackReader reader;
        auto body = reader.Read(paths.imageObject);
        if (!body.Succeeded())
        {
            lastError_ = "Animation object body frame pack failed to load.";
            return false;
        }
        auto shadow = reader.Read(paths.imageObjectShadow);
        if (!shadow.Succeeded())
        {
            lastError_ = "Animation object shadow frame pack failed to load.";
            return false;
        }
        body_ = std::move(*body.value);
        shadow_ = std::move(*shadow.value);

        assets::EffectFramePackReader effectReader;
        auto alpha = effectReader.ReadEffectFrames(paths.effectAlpha);
        auto effectShadow = effectReader.ReadEffectFrames(paths.effectShadow);
        auto screen = effectReader.ReadEffectFrames(paths.effectScreen);
        if (!alpha.Succeeded() || !effectShadow.Succeeded() || !screen.Succeeded())
        {
            Clear();
            lastError_ = "Map effect frame packs failed to load.";
            return false;
        }
        effectAlpha_ = std::move(*alpha.value);
        effectShadow_ = std::move(*effectShadow.value);
        effectScreen_ = std::move(*screen.value);
        effectAlphaDirectionZeroLookup_ = BuildDirectionZeroLookup(effectAlpha_);
        effectShadowDirectionZeroLookup_ = BuildDirectionZeroLookup(effectShadow_);
        effectScreenDirectionZeroLookup_ = BuildDirectionZeroLookup(effectScreen_);
        return true;
    }

    void LegacyMapAnimationRenderer::Clear() noexcept
    {
        body_.clear();
        shadow_.clear();
        effectAlpha_.clear();
        effectShadow_.clear();
        effectScreen_.clear();
        effectAlphaDirectionZeroLookup_.clear();
        effectShadowDirectionZeroLookup_.clear();
        effectScreenDirectionZeroLookup_.clear();
        lastError_.clear();
    }

    bool LegacyMapAnimationRenderer::Loaded() const noexcept
    {
        return !body_.empty();
    }

    const std::string& LegacyMapAnimationRenderer::LastError() const noexcept
    {
        return lastError_;
    }

    std::size_t LegacyMapAnimationRenderer::BodyFrameCount(const std::uint16_t frameId) const noexcept
    {
        return frameId < body_.size() ? body_[frameId].frames.size() : 0U;
    }

    std::size_t LegacyMapAnimationRenderer::BodySequenceCount() const noexcept
    {
        return body_.size();
    }

    std::vector<std::uint16_t> LegacyMapAnimationRenderer::EffectFrameIds(
        const LegacyMapAnimationSpriteSource source) const
    {
        const auto* sequences = EffectSequences(source);
        const auto* lookup = EffectDirectionZeroLookup(source);
        std::vector<std::uint16_t> result;
        if (sequences == nullptr || lookup == nullptr) return result;
        result.reserve(lookup->size());
        for (std::size_t frameId = 0; frameId < lookup->size(); ++frameId)
        {
            const auto index = (*lookup)[frameId];
            if (index == MissingEffectSequence || index >= sequences->size() || (*sequences)[index].frames.empty()) continue;
            result.push_back(static_cast<std::uint16_t>(frameId));
        }
        return result;
    }

    std::size_t LegacyMapAnimationRenderer::EffectFrameCount(const std::uint16_t frameId,
        const LegacyMapAnimationSpriteSource source, const std::uint8_t direction) const noexcept
    {
        const auto* sequences = EffectSequences(source);
        if (sequences == nullptr) return 0U;
        if (direction == 0U)
        {
            const auto* lookup = EffectDirectionZeroLookup(source);
            if (lookup == nullptr || frameId >= lookup->size()) return 0U;
            const auto index = (*lookup)[frameId];
            return index == MissingEffectSequence || index >= sequences->size() ? 0U : (*sequences)[index].frames.size();
        }
        const auto found = std::find_if(sequences->begin(), sequences->end(), [frameId, direction](const auto& sequence) {
            return sequence.frameId == frameId && sequence.directionId.ValueOf() == direction;
        });
        return found == sequences->end() ? 0U : found->frames.size();
    }

    const std::vector<assets::LegacyEffectFrameSequence>* LegacyMapAnimationRenderer::EffectSequences(
        const LegacyMapAnimationSpriteSource source) const noexcept
    {
        if (source == LegacyMapAnimationSpriteSource::EffectAlpha) return &effectAlpha_;
        if (source == LegacyMapAnimationSpriteSource::EffectShadow) return &effectShadow_;
        if (source == LegacyMapAnimationSpriteSource::EffectScreen) return &effectScreen_;
        return nullptr;
    }

    const std::vector<std::size_t>* LegacyMapAnimationRenderer::EffectDirectionZeroLookup(
        const LegacyMapAnimationSpriteSource source) const noexcept
    {
        if (source == LegacyMapAnimationSpriteSource::EffectAlpha) return &effectAlphaDirectionZeroLookup_;
        if (source == LegacyMapAnimationSpriteSource::EffectShadow) return &effectShadowDirectionZeroLookup_;
        if (source == LegacyMapAnimationSpriteSource::EffectScreen) return &effectScreenDirectionZeroLookup_;
        return nullptr;
    }

    std::optional<LegacyMapAnimationSpriteMetadata> LegacyMapAnimationRenderer::FindSpriteMetadata(
        const std::uint16_t spriteId, const LegacyMapAnimationSpriteSource source,
        const std::optional<std::uint16_t> preferredFrameId) const noexcept
    {
        const auto findAnimation = [spriteId, source, preferredFrameId](const auto& sequences)
            -> std::optional<LegacyMapAnimationSpriteMetadata> {
            for (const auto& sequence : sequences)
            {
                if (preferredFrameId && sequence.frameId != *preferredFrameId) continue;
                for (std::size_t index = 0; index < sequence.frames.size(); ++index)
                {
                    const auto& frame = sequence.frames[index];
                    if (frame.spriteId.ValueOf() == spriteId)
                        return LegacyMapAnimationSpriteMetadata{ source, sequence.frameId, index,
                            sequence.frames.size(), frame.originX, frame.originY };
                }
            }
            return std::nullopt;
        };
        if (source == LegacyMapAnimationSpriteSource::ImageObject) return findAnimation(body_);
        if (source == LegacyMapAnimationSpriteSource::ImageObjectShadow) return findAnimation(shadow_);
        if (source == LegacyMapAnimationSpriteSource::EffectAlpha) return findAnimation(effectAlpha_);
        if (source == LegacyMapAnimationSpriteSource::EffectShadow) return findAnimation(effectShadow_);
        if (source == LegacyMapAnimationSpriteSource::EffectScreen) return findAnimation(effectScreen_);
        return std::nullopt;
    }

    LegacyMapResolvedAnimation LegacyMapAnimationRenderer::Resolve(
        const LegacyMapImageObject& object,
        const std::uint64_t globalFrame) const noexcept
    {
        LegacyMapResolvedAnimation result;
        if (!object.animation)
        {
            if (object.spriteId != 0xFFFFU)
                result.body = LegacyMapResolvedFrame{ object.wrapperType, LegacyMapAnimationSpriteSource::ImageObject,
                    LegacyMapAnimationBlendMode::Alpha, object.spriteId, 0xFFFFU, 0, 0, 0, 1 };
            return result;
        }

        if (object.frameId < shadow_.size())
        {
            const auto& shadowFrames = shadow_[object.frameId].frames;
            const auto legacyMax = object.maxFrame == 0U ? shadowFrames.size() : static_cast<std::size_t>(object.maxFrame);
            const auto cycle = std::min(shadowFrames.size(), legacyMax);
            if (cycle != 0U)
            {
                const auto index = static_cast<std::size_t>(globalFrame % cycle);
                const auto& frame = shadowFrames[index];
                const auto spriteId = static_cast<std::uint16_t>(frame.spriteId.ValueOf());
                if (spriteId != 0xFFFFU)
                    result.shadow = LegacyMapResolvedFrame{ 6U, LegacyMapAnimationSpriteSource::ImageObjectShadow,
                        LegacyMapAnimationBlendMode::Alpha, spriteId, 0xFFFFU, frame.originX, frame.originY, index, cycle };
            }
        }

        if (object.bltType == 0U)
        {
            if (object.frameId >= body_.size()) return result;
            const auto& bodyFrames = body_[object.frameId].frames;
            if (bodyFrames.empty()) return result;
            const auto index = static_cast<std::size_t>(globalFrame % bodyFrames.size());
            const auto& frame = bodyFrames[index];
            const auto spriteId = static_cast<std::uint16_t>(frame.spriteId.ValueOf());
            if (spriteId != 0xFFFFU)
                result.body = LegacyMapResolvedFrame{ 5U, LegacyMapAnimationSpriteSource::ImageObject,
                    LegacyMapAnimationBlendMode::Alpha, spriteId, 0xFFFFU, frame.originX, frame.originY, index, bodyFrames.size() };
            return result;
        }

        const std::vector<assets::LegacyEffectFrameSequence>* sequences = nullptr;
        auto source = LegacyMapAnimationSpriteSource::EffectAlpha;
        if (object.bltType == 1U) sequences = &effectAlpha_;
        else if (object.bltType == 2U) { sequences = &effectShadow_; source = LegacyMapAnimationSpriteSource::EffectShadow; }
        else if (object.bltType == 3U) { sequences = &effectScreen_; source = LegacyMapAnimationSpriteSource::EffectScreen; }
        if (sequences == nullptr) return result;
        const auto* lookup = EffectDirectionZeroLookup(source);
        if (lookup == nullptr || object.frameId >= lookup->size()) return result;
        const auto sequenceIndex = (*lookup)[object.frameId];
        if (sequenceIndex == MissingEffectSequence || sequenceIndex >= sequences->size()) return result;
        const auto& sequence = (*sequences)[sequenceIndex];
        if (sequence.frames.empty()) return result;
        const auto legacyMax = object.maxFrame == 0U ? sequence.frames.size() : static_cast<std::size_t>(object.maxFrame);
        const auto cycle = std::min(sequence.frames.size(), legacyMax);
        if (cycle == 0U) return result;
        const auto index = static_cast<std::size_t>(globalFrame % cycle);
        const auto& frame = sequence.frames[index];
        const auto spriteId = static_cast<std::uint16_t>(frame.spriteId.ValueOf());
        if (spriteId != 0xFFFFU)
            result.body = LegacyMapResolvedFrame{ 5U, source,
                source == LegacyMapAnimationSpriteSource::EffectScreen
                    ? LegacyMapAnimationBlendMode::Screen : LegacyMapAnimationBlendMode::Alpha,
                spriteId, object.frameId,
                frame.originX, frame.originY, index, cycle };
        return result;
    }

    LegacyMapOrderedFrames LegacyMapAnimationRenderer::OrderedFrames(
        const LegacyMapResolvedAnimation& animation)
    {
        LegacyMapOrderedFrames result;
        if (animation.shadow) result.values[result.count++] = *animation.shadow;
        if (animation.body) result.values[result.count++] = *animation.body;
        return result;
    }
}
