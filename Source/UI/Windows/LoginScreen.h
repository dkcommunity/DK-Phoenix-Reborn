#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <functional>

namespace phoenix::ui
{

// LoginScreen config from interface.ini [Login_1280x960]
struct LoginProfile
{
    std::int32_t windowX{ 0 };
    std::int32_t windowY{ 0 };
    std::int32_t windowWidth{ 0 };
    std::int32_t windowHeight{ 0 };
    bool topmost{ true };
    bool keyboardControl{ true };

    // Background SPK
    std::string loginSpkName{ "Login.spk" };
    std::string loginHighResSpkName{ "Login_1024.spk" };
    std::string menuSpkName{ "LoginMenu.spk" };

    // ID/Password positions
    std::int32_t idX{ 0 };
    std::int32_t idY{ 0 };
    std::int32_t idWidth{ 130 };
    std::int32_t idHeight{ 23 };
    std::int32_t idByteLimit{ 12 };

    std::int32_t passwordX{ 0 };
    std::int32_t passwordY{ 0 };
    std::int32_t passwordWidth{ 130 };
    std::int32_t passwordHeight{ 23 };
    std::int32_t passwordByteLimit{ 10 };

    // Buttons
    std::int32_t newButtonX{ 118 };
    std::int32_t newButtonY{ 123 };
    std::int32_t okButtonX{ 52 };
    std::int32_t okButtonY{ 123 };
    std::int32_t cancelButtonX{ 156 };
    std::int32_t cancelButtonY{ 28 };

    // Button frames
    std::int32_t newButtonFrame{ 0 };
    std::int32_t newButtonPushedFrame{ 1 };
    std::int32_t okButtonFrame{ 2 };
    std::int32_t okButtonPushedFrame{ 3 };
    std::int32_t cancelButtonFrame{ 4 };
    std::int32_t cancelButtonPushedFrame{ 5 };

    // High resolution adjustments
    std::int32_t highResConvX{ 128 };
    std::int32_t highResConvY{ 96 };
    std::int32_t highResWidthThreshold{ 1024 };
    std::int32_t highResHeightThreshold{ 960 };
};

class LoginScreen
{
public:
    explicit LoginScreen() noexcept;

    // State
    void Open() noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const settings::LoginProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // Callbacks
    using LoginCallback = std::function<void(const std::string& id, const std::string& password)>;
    using NewAccountCallback = std::function<void()>;
    using CancelCallback = std::function<void()>;

    void SetLoginCallback(LoginCallback cb) noexcept { loginCallback_ = std::move(cb); }
    void SetNewAccountCallback(NewAccountCallback cb) noexcept { newAccountCallback_ = std::move(cb); }
    void SetCancelCallback(CancelCallback cb) noexcept { cancelCallback_ = std::move(cb); }

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;
    bool HandleChar(wchar_t ch) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Focus management
    void ChangeFocus() noexcept;
    void FocusID() noexcept;
    void FocusPassword() noexcept;
    [[nodiscard]] bool IsIDFocused() const noexcept { return idFocused_; }

    // Login validation
    [[nodiscard]] bool ReadySend() const noexcept;

private:
    // State
    bool isOpen_{ false };
    bool idFocused_{ true };

    // Config
    settings::LoginProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // UI Elements
    std::unique_ptr<ButtonGroup> buttonGroup_;
    std::unique_ptr<LineEditor> idEdit_;
    std::unique_ptr<LineEditor> passwordEdit_;
    std::unique_ptr<SpriteArchive> loginSpk_;
    std::unique_ptr<SpriteArchive> menuSpk_;

    // Window position
    std::int32_t windowX_{ 0 };
    std::int32_t windowY_{ 0 };
    std::int32_t windowWidth_{ 0 };
    std::int32_t windowHeight_{ 0 };

    // Resolution
    bool highRes_{ false };
    std::int32_t convX_{ 0 };
    std::int32_t convY_{ 0 };

    // Callbacks
    std::function<void(const std::string&, const std::string&)> loginCallback_;
    std::function<void()> newAccountCallback_;
    std::function<void()> cancelCallback_;

    // Helpers
    void InitializeSPKs() noexcept;
    void InitializeButtons() noexcept;
    void InitializeLineEditors() noexcept;
    void PositionButtons() noexcept;
    void PositionLineEditors() noexcept;
    void UpdateResolution() noexcept;
    [[nodiscard]] bool IsPointInWindow(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInID(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInPassword(int x, int y) const noexcept;

    // Callbacks
    void OnNewAccountClicked() noexcept;
    void OnOkClicked() noexcept;
    void OnCancelClicked() noexcept;
};

} // namespace phoenix::ui