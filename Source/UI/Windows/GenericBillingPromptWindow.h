#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <functional>

namespace phoenix::ui
{

// Billing prompt modes (matching Legacy BILING_MSG_LIST)
enum class BillingPromptMode : std::uint8_t
{
    CannotPlay = 0,           // Cannot play - go to store?
    CannotPlayByAttr = 1,     // Cannot play due to attribute - go to store?
    Custom = 2                // Custom message
};

// GenericBillingPrompt config from interface.ini [BillingPrompt_1280x960]
struct GenericBillingPromptProfile
{
    std::int32_t windowX{ 465 };
    std::int32_t windowY{ 380 };
    std::int32_t windowWidth{ 350 };
    std::int32_t windowHeight{ 200 };
    bool topmost{ true };
    bool keyboardControl{ true };

    // Buttons
    std::int32_t okButtonX{ 200 };
    std::int32_t okButtonY{ 140 };
    std::int32_t cancelButtonX{ 275 };
    std::int32_t cancelButtonY{ 140 };

    // Text
    std::int32_t textX{ 30 };
    std::int32_t textY{ 40 };
    std::int32_t textWidth{ 290 };
    std::int32_t textLineHeight{ 20 };

    // SPK
    std::string backSpkName{ "AssembleBox.spk" };
    std::int32_t okButtonFrame{ 0 };
    std::int32_t cancelButtonFrame{ 1 };
};

class GenericBillingPromptWindow
{
public:
    explicit GenericBillingPromptWindow() noexcept;

    // State
    void Open(uint8_t mode, const std::string& customMessage = "") noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const settings::GenericBillingPromptProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // Callback for when user confirms (go to store)
    using StoreCallback = std::function<void()>;
    void SetStoreCallback(std::function<void()> cb) noexcept { storeCallback_ = std::move(cb); }

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

private:
    // State
    bool isOpen_{ false };
    std::uint8_t mode_{ 0 };
    std::string customMessage_;

    // Config
    settings::GenericBillingPromptProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // Callback
    std::function<void()> storeCallback_;

    // UI Elements
    std::unique_ptr<ButtonGroup> buttonGroup_;
    std::unique_ptr<SpriteArchive> backSpk_;

    // Window position
    std::int32_t windowX_{ 465 };
    std::int32_t windowY_{ 380 };
    std::int32_t windowWidth_{ 350 };
    std::int32_t windowHeight_{ 200 };

    // Helpers
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;
    [[nodiscard]] bool IsPointInWindow(int x, int y) const noexcept;
    [[nodiscard]] std::string GetMessageForMode(uint8_t mode) const noexcept;

    // Callbacks
    void OnOkClicked() noexcept;
    void OnCancelClicked() noexcept;
};

} // namespace phoenix::ui