#include "UI/Windows/HpBarWindow.h"
#include "Diagnostics/Logger.h"
#include "Diagnostics/PhoenixDiagnostics.h"

namespace phoenix::ui {

HpBarWindow::HpBarWindow(protocol::PcType pcType) noexcept
    : pcType_(pcType) {
    UpdateSpkKey();
    UpdatePosition();
}

void HpBarWindow::UpdateSpkKey() noexcept {
    switch (pcType_) {
    case protocol::PcType::Slayer:
        spkKey_ = "SPK_HPBAR_SLAYER";
        break;
    case protocol::PcType::Vampire:
        spkKey_ = "SPK_HPBAR_VAMPIRE";
        break;
    case protocol::PcType::Ousters:
        spkKey_ = "SPK_HPBAR_OUSTERS";
        break;
    default:
        spkKey_ = "SPK_HPBAR_SLAYER";
        break;
    }
}

void HpBarWindow::SetResolution(std::uint32_t width, std::uint32_t height) noexcept {
    // Legacy centering: (resolution_x - 800) / 2 offset from 246 base
    const std::int32_t centerOffset = (static_cast<std::int32_t>(width) - 800) / 2;
    baseX_ = 246 + centerOffset;

    // Y position: resolution_y - 105 (Legacy)
    baseY_ = static_cast<std::int32_t>(height) - 105;

    // Set the window size to match the full drawn HP bar sprite (left + center + right + 1px gaps)
    // These dimensions match the large-width mode sprites:
    //   LeftEdge  : 229x37
    //   Center    : 77x24  (height 24, but we use max height 37 for hitbox)
    //   RightEdge : 229x37
    const int leftWidth  = 229;
    const int centerWidth = 77;
    const int rightWidth = 229;
    const int gap = 1; // 1 pixel gap between parts as used in DrawLegacyHpMpBarHud
    windowW_ = leftWidth + gap + centerWidth + gap + rightWidth; // 229+1+77+1+229 = 535
    // The drawn HP/MP fill sprites are rendered at Y+19 and have a height of 24,
    // making the total visible height 19 + 24 = 43.
    windowH_ = 43; // total visible height of the HP bar sprite

    UpdatePosition();
}

// HpBarWindow.cpp ~line 54
void HpBarWindow::LoadFromConfig(const settings::GameHpBarProfile& profile,
                                 std::uint32_t screenWidth,
                                 std::uint32_t screenHeight) noexcept
{
    PHX_LOG_INFO(L"[HPBAR] LoadFromConfig IN: levelBarOffsetX=" + std::to_wstring(profile.levelBarOffsetX));  // ← EKLE (ilk satır)

    // Config provides absolute coordinates relative to 1280x960 reference canvas.
    // Scale to current resolution using the same centering logic.
    const std::int32_t centerOffset = (static_cast<std::int32_t>(screenWidth) - 800) / 2;
    baseX_ = profile.positionX + centerOffset;

    // Y: config absolute Y, adjusted for screen height difference
    const std::int32_t screenHeightDiff = static_cast<std::int32_t>(screenHeight) - 960;
    baseY_ = profile.positionY + screenHeightDiff;

    widthMode_ = profile.defaultWidthMode;
    smallMode_ = profile.defaultSmallMode;
    windowW_ = profile.panelWidth;
    windowH_ = profile.panelHeight;
    draggable_ = profile.draggable;

    // Sprite offsets (hot-reload editable)
    hpFillOffsetX_ = profile.hpFillOffsetX;
    hpFillOffsetY_ = profile.hpFillOffsetY;
    mpFillOffsetX_ = profile.mpFillOffsetX;
    mpFillOffsetY_ = profile.mpFillOffsetY;
    centerOffsetX_ = profile.centerOffsetX;
    centerOffsetY_ = profile.centerOffsetY;
    textOffsetY_ = profile.textOffsetY;
    hpTextOffsetX_ = profile.hpTextOffsetX;
    mpTextOffsetX_ = profile.mpTextOffsetX;
    levelLabelOffsetX_ = profile.levelLabelOffsetX;
    rightEdgeGapX_ = profile.rightEdgeGapX;
    // ── Extended offsets ──
    leftEdgeOffsetX_ = profile.leftEdgeOffsetX;
    leftEdgeOffsetY_ = profile.leftEdgeOffsetY;
    rightEdgeOffsetY_ = profile.rightEdgeOffsetY;
    greyBarOffsetX_ = profile.greyBarOffsetX;
    greyBarOffsetY_ = profile.greyBarOffsetY;
    buttonOffsetX_ = profile.buttonOffsetX;
    buttonOffsetY_ = profile.buttonOffsetY;
    levelBarOffsetX_ = profile.levelBarOffsetX;
    levelBarOffsetY_ = profile.levelBarOffsetY;

    PHX_LOG_INFO(L"[HPBAR] LoadFromConfig OUT: levelBarOffsetX_=" + std::to_wstring(levelBarOffsetX_));  // ← EKLE (son satır, UpdatePosition() öncesi)

    UpdatePosition();
}

void HpBarWindow::UpdatePosition() noexcept {
    // Window dimensions are set in SetResolution; position is set by baseX_/baseY_
    // This function is a placeholder for any additional positioning logic
}

// Mouse drag support
void HpBarWindow::StartDrag(std::int32_t mouseX, std::int32_t mouseY) noexcept {
    if (IsPointInside(mouseX, mouseY)) {
        isDragging_ = true;
        dragOffsetX_ = mouseX - baseX_;
        dragOffsetY_ = mouseY - baseY_;
    }
}

void HpBarWindow::UpdateDrag(std::int32_t mouseX, std::int32_t mouseY) noexcept {
    if (isDragging_) {
        baseX_ = mouseX - dragOffsetX_;
        baseY_ = mouseY - dragOffsetY_;
    }
}

void HpBarWindow::EndDrag() noexcept {
    isDragging_ = false;
}

[[nodiscard]] bool HpBarWindow::IsDragging() const noexcept { return isDragging_; }

[[nodiscard]] bool HpBarWindow::IsPointInside(std::int32_t x, std::int32_t y) const noexcept {
    return x >= baseX_ && x < baseX_ + windowW_ &&
           y >= baseY_ && y < baseY_ + windowH_;
}

// Render - delegates to Application::DrawLegacyHpMpBarHud
void HpBarWindow::Render() noexcept {
    // Delegate to Application's DrawLegacyHpMpBarHud which handles
    // all the Legacy-specific frame indices and rendering logic
    // This window class just manages state (position, mode, race)
    // The actual rendering is done by Application::DrawLegacyHpMpBarHud()
}

} // namespace phoenix::ui