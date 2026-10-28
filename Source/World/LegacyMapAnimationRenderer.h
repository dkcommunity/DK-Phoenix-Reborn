#pragma once

#include "Assets/LegacyAssetTypes.h"
#include "World/LegacyMapReader.h"

#include <cstdint>
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::world
{
    inline constexpr std::uint64_t LegacyMapAnimationTickMs = 62U;

    enum class LegacyMapAnimationSpriteSource : std::uint8_t
    {
        ImageObject,
        ImageObjectShadow,
        EffectAlpha,
        EffectShadow,
        EffectScreen
    };

    enum class LegacyMapAnimationBlendMode : std::uint8_t
    {
        Alpha,
        Screen
    };

    struct LegacyMapAnimationFramePackPaths
    {
        std::filesystem::path imageObject;
        std::filesystem::path imageObjectShadow;
        std::filesystem::path effectAlpha;
        std::filesystem::path effectShadow;
        std::filesystem::path effectScreen;
    };

    struct LegacyMapResolvedFrame
    {
        std::uint8_t wrapperType{ 0 };
        LegacyMapAnimationSpriteSource source{ LegacyMapAnimationSpriteSource::ImageObject };
        LegacyMapAnimationBlendMode blendMode{ LegacyMapAnimationBlendMode::Alpha };
        std::uint16_t spriteId{ 0xFFFFU };
        std::uint16_t paletteId{ 0xFFFFU };
        std::int32_t cX{ 0 };
        std::int32_t cY{ 0 };
        std::size_t frameIndex{ 0 };
        std::size_t frameCount{ 0 };
    };

    struct LegacyMapResolvedAnimation
    {
        std::optional<LegacyMapResolvedFrame> shadow;
        std::optional<LegacyMapResolvedFrame> body;
    };

    struct LegacyMapOrderedFrames
    {
        std::array<LegacyMapResolvedFrame, 2> values{};
        std::size_t count{ 0 };

        [[nodiscard]] const LegacyMapResolvedFrame* begin() const noexcept { return values.data(); }
        [[nodiscard]] const LegacyMapResolvedFrame* end() const noexcept { return values.data() + count; }
        [[nodiscard]] bool empty() const noexcept { return count == 0U; }
        [[nodiscard]] std::size_t size() const noexcept { return count; }
        [[nodiscard]] const LegacyMapResolvedFrame& front() const noexcept { return values.front(); }
        [[nodiscard]] const LegacyMapResolvedFrame& back() const noexcept { return values[count - 1U]; }
    };

    struct LegacyMapAnimationSpriteMetadata
    {
        LegacyMapAnimationSpriteSource source{ LegacyMapAnimationSpriteSource::ImageObject };
        std::uint16_t frameId{ 0 };
        std::size_t frameIndex{ 0 };
        std::size_t frameCount{ 0 };
        std::int32_t cX{ 0 };
        std::int32_t cY{ 0 };
    };

    class LegacyMapAnimationRenderer final
    {
    public:
        bool Load(const LegacyMapAnimationFramePackPaths& paths);
        void Clear() noexcept;
        [[nodiscard]] bool Loaded() const noexcept;
        [[nodiscard]] const std::string& LastError() const noexcept;
        [[nodiscard]] LegacyMapResolvedAnimation Resolve(
            const LegacyMapImageObject& object,
            std::uint64_t globalFrame) const noexcept;
        [[nodiscard]] static LegacyMapOrderedFrames OrderedFrames(
            const LegacyMapResolvedAnimation& animation);
        [[nodiscard]] std::size_t BodyFrameCount(std::uint16_t frameId) const noexcept;
        [[nodiscard]] std::size_t BodySequenceCount() const noexcept;
        [[nodiscard]] std::vector<std::uint16_t> EffectFrameIds(
            LegacyMapAnimationSpriteSource source) const;
        [[nodiscard]] std::size_t EffectFrameCount(
            std::uint16_t frameId, LegacyMapAnimationSpriteSource source,
            std::uint8_t direction = 0U) const noexcept;
        [[nodiscard]] std::optional<LegacyMapAnimationSpriteMetadata> FindSpriteMetadata(
            std::uint16_t spriteId, LegacyMapAnimationSpriteSource source,
            std::optional<std::uint16_t> preferredFrameId = std::nullopt) const noexcept;

    private:
        [[nodiscard]] const std::vector<assets::LegacyEffectFrameSequence>* EffectSequences(
            LegacyMapAnimationSpriteSource source) const noexcept;
        [[nodiscard]] const std::vector<std::size_t>* EffectDirectionZeroLookup(
            LegacyMapAnimationSpriteSource source) const noexcept;
        std::vector<assets::LegacyAnimationObjectFrameSequence> body_;
        std::vector<assets::LegacyAnimationObjectFrameSequence> shadow_;
        std::vector<assets::LegacyEffectFrameSequence> effectAlpha_;
        std::vector<assets::LegacyEffectFrameSequence> effectShadow_;
        std::vector<assets::LegacyEffectFrameSequence> effectScreen_;
        std::vector<std::size_t> effectAlphaDirectionZeroLookup_;
        std::vector<std::size_t> effectShadowDirectionZeroLookup_;
        std::vector<std::size_t> effectScreenDirectionZeroLookup_;
        std::string lastError_;
    };
}
