#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <array>

namespace phoenix::ui
{

// Option tabs
enum class OptionTab : std::uint8_t
{
    Control = 0,
    Graphic = 1,
    Sound = 2,
    Game = 3,
    Other = 4,
    Count = 5
};

// Check types (matching Legacy CHECK_* enums)
enum class CheckId : std::int32_t
{
    // Control tab
    NormalChat = 0,
    EnterChat = 1,
    MouseSpeed = 2,
    IFeel = 3,
    CheckControlMax = 4,

    // Graphic tab (offset by CHECK_CONTROL_MAX)
    CheckGraphicTab = 4,
    AlphaHpBar = 4,
    BloodDrop = 5,
    AlphaDepth = 6,
    DefaultAlpha = 7,
    Gamma = 8,
    ChatBox = 9,
    Fps = 10,
    AutoHideSmooth = 11,
    Resolution800x600 = 12,
    Resolution1024x768 = 13,
    ThreeD = 14,
    CheckGraphicMax = 15,

    // Sound tab
    CheckSoundTab = 15,
    Sound = 15,
    Music = 16,
    Wav = 17,
    Midi = 18,
    Yell = 19,
    CheckSoundMax = 20,

    // Game tab
    CheckGameTab = 20,
    Help = 20,
    Filtering = 21,
    Change = 22,
    PartyInvite = 23,
    PartyRequest = 24,
    PreloadMonster = 25,
    ChatWhite = 26,
    TeenVersion = 27,
    PopupChatByWhisper = 28,
    NotSendMyInfo = 29,
    DoNotShowWarMsg = 30,
    DoNotShowLairMsg = 31,
    DoNotShowHolyLandMsg = 32,
    DoNotShowItembayMessageBox = 33,
    ShowGamemoneyWithHangul = 34,
    OneClickAttackOff = 35,
    CheckGameMax = 36,

    // Other tab (Title mode, 1024+ resolution only)
    CheckOtherTab = 36,
    ShowCriticalHitEffect = 36,
    ShowDamageNumber = 37,
    VSync = 38,
    Vignette = 39,
    CrtScanline = 40,
    CheckOtherMax = 41
};

// Value rect types for sliders
enum class ValueRect : std::int32_t
{
    MouseSpeed = 0,
    Gamma = 1,
    Sound = 2,
    Music = 3,
    Alpha = 4,
    Max = 5
};

class OptionsScreen
{
public:
    OptionsScreen() noexcept;
    ~OptionsScreen() = default;

    // Lifecycle
    void Open(bool isTitleMode = false) noexcept;
    void Close() noexcept;
    void Toggle() noexcept;

    // Config
    void LoadFromConfig(const settings::OptionsProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // State
    bool IsOpen() const noexcept { return isOpen_; }
    bool IsTitleMode() const noexcept { return isTitleMode_; }
    OptionTab GetSelectedTab() const noexcept { return selectedTab_; }

    // Input handling
    bool HandleMouseDown(int x, int y, bool rightButton = false) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton = false) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleMouseWheel(int delta) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Callbacks
    using CloseCallback = std::function<void()>;
    using OptionChangeCallback = std::function<void(CheckId, bool)>;
    using SliderChangeCallback = std::function<void(ValueRect, std::int32_t)>;
    void SetCloseCallback(CloseCallback cb) noexcept { closeCallback_ = std::move(cb); }
    void SetOptionChangeCallback(OptionChangeCallback cb) noexcept { optionChangeCallback_ = std::move(cb); }
    void SetSliderChangeCallback(SliderChangeCallback cb) noexcept { sliderChangeCallback_ = std::move(cb); }

    // Option access
    void SetCheck(CheckId id, bool value) noexcept;
    bool GetCheck(CheckId id) const noexcept;
    void SetSlider(ValueRect rect, std::int32_t value) noexcept;
    std::int32_t GetSlider(ValueRect rect) const noexcept;

    // Hotkey/Accelerator
    void SetAcceleratorMode(std::uint8_t accel) noexcept { acceleratorMode_ = accel; }
    std::uint8_t GetAcceleratorMode() const noexcept { return acceleratorMode_; }

private:
    // Initialization
    void InitializeSPKs() noexcept;
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;
    void UpdateSelectedTabLayout() noexcept;

    // Input helpers
    bool IsPointInWindow(int x, int y) const noexcept;
    bool IsPointInTab(int x, int y, OptionTab tab) const noexcept;
    bool IsPointInSlider(int x, int y, ValueRect rect) const noexcept;
    bool IsPointInCheck(int x, int y, CheckId id) const noexcept;
    bool IsPointInResetButton(int x, int y) const noexcept;

    // Callbacks
    void OnTabSelected(OptionTab tab) noexcept;
    void OnCloseClicked() noexcept;
    void OnResetClicked() noexcept;
    void OnCheckToggled(CheckId id) noexcept;
    void OnSliderDragged(ValueRect rect, std::int32_t value) noexcept;
    void OnScroll(int delta) noexcept;

    // Helpers
    void UpdateScrollRanges() noexcept;
    void LoadOptionValues() noexcept;
    void SaveOptionValues() noexcept;
    void ApplyResolution() noexcept;
    void ApplyGamma() noexcept;
    void ApplyVSync() noexcept;
    void ApplyVignette() noexcept;
    void ApplyCrtScanline() noexcept;

    // State
    bool isOpen_{ false };
    bool isTitleMode_{ false };
    OptionTab selectedTab_{ OptionTab::Control };
    settings::OptionsProfile config_;

    // Checks (all possible options)
    std::array<bool, static_cast<std::size_t>(CheckId::CheckOtherMax)> checks_{};

    // Slider values
    std::array<std::int32_t, static_cast<std::size_t>(ValueRect::Max)> sliderValues_{};

    // Scroll
    std::int32_t controlScrollPos_{ 0 };
    std::int32_t graphicScrollPos_{ 0 };

    // Race for SPK selection (Game mode)
    std::int32_t currentRace_{ 0 }; // 0=Slayer, 1=Vampire, 2=Ousters

    // Accelerator/hotkey
    std::uint8_t acceleratorMode_{ 0 };
    std::int32_t focusedHotkey_{ -1 };

    // Drag state
    bool draggingSlider_{ false };
    ValueRect draggedSlider_{ ValueRect::MouseSpeed };

    // SPK archives
    std::unique_ptr<SpriteArchive> mainSpk_;
    std::unique_ptr<SpriteArchive> etcSpk_;
    std::unique_ptr<SpriteArchive> scrollSpk_;
    std::unique_ptr<SpriteArchive> titleScrollSpk_;

    // UI components
    std::unique_ptr<ButtonGroup> tabButtonGroup_;
    std::unique_ptr<ButtonGroup> controlButtonGroup_;
    std::unique_ptr<ButtonGroup> graphicButtonGroup_;
    std::unique_ptr<ButtonGroup> soundButtonGroup_;
    std::unique_ptr<ButtonGroup> gameButtonGroup_;
    std::unique_ptr<ButtonGroup> otherButtonGroup_;
    std::unique_ptr<ScrollBar> scrollBar_;

    // Dialog
    std::unique_ptr<DialogWindow> dialog_;

    // Callbacks
    CloseCallback closeCallback_;
    OptionChangeCallback optionChangeCallback_;
    SliderChangeCallback sliderChangeCallback_;
};

} // namespace phoenix::ui