#pragma once

#include <cstdint>
#include <string>

#include "Protocol/LegacyProtocol.h"
#include "Settings/InterfaceProfileLoader.h"

namespace phoenix::ui {

enum class HpBarMode : std::uint8_t {
    Width = 0,    // Horizontal (HP/MP side by side)
    Height,       // Vertical (HP/MP stacked)
    Count
};

class HpBarWindow {
public:
    explicit HpBarWindow(protocol::PcType pcType) noexcept;

    void SetRace(protocol::PcType pcType) noexcept { pcType_ = pcType; }
    void SetResolution(std::uint32_t width, std::uint32_t height) noexcept;

    // Load from interface.ini config
    void LoadFromConfig(const settings::GameHpBarProfile& profile, std::uint32_t screenWidth, std::uint32_t screenHeight) noexcept;

    // Mode toggles
    void ToggleWidthMode() noexcept { widthMode_ = !widthMode_; }
    void ToggleSmallMode() noexcept { smallMode_ = !smallMode_; }
    void SetWidthMode(bool width) noexcept { widthMode_ = width; }
    void SetSmallMode(bool small) noexcept { smallMode_ = small; }

    [[nodiscard]] bool IsWidthMode() const noexcept { return widthMode_; }
    [[nodiscard]] bool IsSmallMode() const noexcept { return smallMode_; }

    // Position/Movability
    void SetPosition(std::int32_t x, std::int32_t y) noexcept { baseX_ = x; baseY_ = y; }
    [[nodiscard]] std::int32_t GetX() const noexcept { return baseX_; }
    [[nodiscard]] std::int32_t GetY() const noexcept { return baseY_; }
    [[nodiscard]] std::int32_t GetWidth() const noexcept { return windowW_; }
    [[nodiscard]] std::int32_t GetHeight() const noexcept { return windowH_; }

    // Mode/Race
    [[nodiscard]] protocol::PcType GetPcType() const noexcept { return pcType_; }

    // Sprite offset getters (hot-reload from interface.ini)
        [[nodiscard]] std::int32_t GetHpFillOffsetX() const noexcept { return hpFillOffsetX_; }
        [[nodiscard]] std::int32_t GetHpFillOffsetY() const noexcept { return hpFillOffsetY_; }
        [[nodiscard]] std::int32_t GetMpFillOffsetX() const noexcept { return mpFillOffsetX_; }
        [[nodiscard]] std::int32_t GetMpFillOffsetY() const noexcept { return mpFillOffsetY_; }
        [[nodiscard]] std::int32_t GetCenterOffsetX() const noexcept { return centerOffsetX_; }
        [[nodiscard]] std::int32_t GetCenterOffsetY() const noexcept { return centerOffsetY_; }
        [[nodiscard]] std::int32_t GetTextOffsetY() const noexcept { return textOffsetY_; }
        [[nodiscard]] std::int32_t GetHpTextOffsetX() const noexcept { return hpTextOffsetX_; }
        [[nodiscard]] std::int32_t GetMpTextOffsetX() const noexcept { return mpTextOffsetX_; }
        [[nodiscard]] std::int32_t GetLevelLabelOffsetX() const noexcept { return levelLabelOffsetX_; }
        [[nodiscard]] std::int32_t GetRightEdgeGapX() const noexcept { return rightEdgeGapX_; }
        // ── Extended offsets (GameHpBar_1280x960) ──
        [[nodiscard]] std::int32_t GetLeftEdgeOffsetX() const noexcept { return leftEdgeOffsetX_; }
        [[nodiscard]] std::int32_t GetLeftEdgeOffsetY() const noexcept { return leftEdgeOffsetY_; }
        [[nodiscard]] std::int32_t GetRightEdgeOffsetY() const noexcept { return rightEdgeOffsetY_; }
        [[nodiscard]] std::int32_t GetGreyBarOffsetX() const noexcept { return greyBarOffsetX_; }
        [[nodiscard]] std::int32_t GetGreyBarOffsetY() const noexcept { return greyBarOffsetY_; }
        [[nodiscard]] std::int32_t GetButtonOffsetX() const noexcept { return buttonOffsetX_; }
        [[nodiscard]] std::int32_t GetButtonOffsetY() const noexcept { return buttonOffsetY_; }
        [[nodiscard]] std::int32_t GetLevelBarOffsetX() const noexcept { return levelBarOffsetX_; }
        [[nodiscard]] std::int32_t GetLevelBarOffsetY() const noexcept { return levelBarOffsetY_; }

    // Mouse drag support
    void StartDrag(std::int32_t mouseX, std::int32_t mouseY) noexcept;
    void UpdateDrag(std::int32_t mouseX, std::int32_t mouseY) noexcept;
    void EndDrag() noexcept;
    [[nodiscard]] bool IsDragging() const noexcept;
    [[nodiscard]] bool IsPointInside(std::int32_t x, std::int32_t y) const noexcept;
    [[nodiscard]] bool IsDraggable() const noexcept { return draggable_; }

    // Render - delegates to Application's DrawLegacyHpMpBarHud
    void Render() noexcept;

private:
    protocol::PcType pcType_{ protocol::PcType::Slayer };
    bool widthMode_{ true };   // true = horizontal, false = vertical
    bool smallMode_{ false };
    bool draggable_{ true };

    std::string spkKey_;       // e.g., "SPK_HPBAR_SLAYER"

    // Position (resolution-aware)
    std::int32_t baseX_{ 0 };
    std::int32_t baseY_{ 0 };
    std::int32_t windowW_{ 535 };
    std::int32_t windowH_{ 43 };

    // Sprite offsets (hot-reload from interface.ini)
    std::int32_t hpFillOffsetX_{ 26 };
    std::int32_t hpFillOffsetY_{ 19 };
    std::int32_t mpFillOffsetX_{ 320 };
    std::int32_t mpFillOffsetY_{ 19 };
    std::int32_t centerOffsetX_{ 1 };
    std::int32_t centerOffsetY_{ 13 };
    std::int32_t textOffsetY_{ 19 };
    std::int32_t hpTextOffsetX_{ 26 };
    std::int32_t mpTextOffsetX_{ 318 };
    std::int32_t levelLabelOffsetX_{ 236 };
        std::int32_t rightEdgeGapX_{ 1 };
        // ── Extended offsets (sprite 7/9/11/0/1-6) ──
        std::int32_t leftEdgeOffsetX_{ 0 };
        std::int32_t leftEdgeOffsetY_{ 0 };
        std::int32_t rightEdgeOffsetY_{ 0 };
        std::int32_t greyBarOffsetX_{ 320 };
        std::int32_t greyBarOffsetY_{ 17 };
        std::int32_t buttonOffsetX_{ 10 };
        std::int32_t buttonOffsetY_{ 15 };
        std::int32_t levelBarOffsetX_{ 236 };
        std::int32_t levelBarOffsetY_{ 19 };

    // Drag state
    bool isDragging_{ false };
    std::int32_t dragOffsetX_{ 0 };
    std::int32_t dragOffsetY_{ 0 };

    void UpdateSpkKey() noexcept;
    void UpdatePosition() noexcept;
};

} // namespace phoenix::ui