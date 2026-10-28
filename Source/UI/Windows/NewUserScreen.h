#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <functional>

namespace phoenix::ui
{

// NewUser states
enum class NewUserState : std::uint8_t
{
    None = 0,
    Success = 1,
    Failed = 2,
    AlreadyExists = 3
};

// NewUser result
enum class NewUserResult : std::uint8_t
{
    None = 0,
    Success = 1,
    Failed = 2,
    AccountAlreadyExists = 3,
    NetworkError = 4
};

// NewUser config from interface.ini [NewUser_1280x960]
struct NewUserProfile
{
    // Window
    std::int32_t windowX{ 0 };
    std::int32_t windowY{ 0 };
    std::int32_t windowWidth{ 320 };
    std::int32_t windowHeight{ 200 };
    bool topmost{ true };
    bool keyboardControl{ true };

    // SPK files
    std::string newUserSpkName{ "NewUser.spk" };
    std::string newUserTitleSpkName{ "NewUserTitle.spk" };
    std::string newUserTitle1024SpkName{ "NewUserTitle1024.spk" };
    std::string commonSpkName{ "Common.spk" };

    // ID/Password edit positions
    std::int32_t idEditX{ 0 };
    std::int32_t idEditY{ 0 };
    std::int32_t idEditWidth{ 123 };
    std::int32_t idEditHeight{ 17 };
    std::int32_t idByteLimit{ 12 };

    std::int32_t passwordEditX{ 0 };
    std::int32_t passwordEditY{ 0 };
    std::int32_t passwordEditWidth{ 123 };
    std::int32_t passwordEditHeight{ 17 };
    std::int32_t passwordByteLimit{ 10 };

    // Buttons
    std::int32_t createButtonX{ 0 };
    std::int32_t createButtonY{ 0 };
    std::int32_t closeButtonX{ 0 };
    std::int32_t closeButtonY{ 0 };

    // Result buttons
    std::int32_t resultCloseButtonX{ 0 };
    std::int32_t resultCloseButtonY{ 0 };

    // SPK frames
    std::int32_t bgFrame{ 0 };
    std::int32_t accountCreatedFrame{ 1 };
    std::int32_t accountNotCreatedFrame{ 2 };
    std::int32_t accountAlreadyExistsFrame{ 3 };
    std::int32_t createNormalFrame{ 4 };
    std::int32_t createHighlightFrame{ 5 };
    std::int32_t createPressedFrame{ 6 };
    std::int32_t resultCloseButtonFrame{ 7 };
    std::int32_t closeButtonFrame{ 8 };
    std::int32_t closeButtonHighlightFrame{ 9 };
    std::int32_t closeButtonPressedFrame{ 10 };

    // Title mode frames
    std::int32_t titleBgFrame{ 0 };
    std::int32_t titleAccountCreatedFrame{ 1 };
    std::int32_t titleAccountNotCreatedFrame{ 2 };
    std::int32_t titleAccountAlreadyExistsFrame{ 3 };
    std::int32_t titleCreateNormalFrame{ 4 };
    std::int32_t titleCreateHighlightFrame{ 5 };
    std::int32_t titleCreatePressedFrame{ 6 };
    std::int32_t titleResultCloseButtonFrame{ 7 };
    std::int32_t titleCloseButtonFrame{ 8 };
    std::int32_t titleCloseButtonHighlightFrame{ 9 };
    std::int32_t titleCloseButtonPressedFrame{ 10 };
};

class NewUserScreen
{
public:
    NewUserScreen() noexcept;
    ~NewUserScreen() = default;

    // Lifecycle
    void Open() noexcept;
    void Close() noexcept;
    void Toggle() noexcept { if (isOpen_) Close(); else Open(); }

    // Config
    void LoadFromConfig(const settings::NewUserProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // State
    bool IsOpen() const noexcept { return isOpen_; }
    NewUserState GetState() const noexcept { return state_; }
    const std::string& GetAccountName() const noexcept { return accountName_; }
    const std::string& GetPassword() const noexcept { return password_; }

    // Input handling
    bool HandleMouseDown(int x, int y, bool rightButton = false) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton = false) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;
    bool HandleCharInput(wchar_t c) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Callbacks
    using CloseCallback = std::function<void()>;
    using RegisterCallback = std::function<void(const std::string&, const std::string&)>;
    using ResultCallback = std::function<void(NewUserResult)>;

    void SetCloseCallback(CloseCallback cb) noexcept { closeCallback_ = std::move(cb); }
    void SetRegisterCallback(RegisterCallback cb) noexcept { registerCallback_ = std::move(cb); }
    void SetResultCallback(ResultCallback cb) noexcept { resultCallback_ = std::move(cb); }

    // Network result handlers (called from network thread)
    void OnRegistrationSuccess() noexcept;
    void OnRegistrationFailed(bool accountAlreadyExists = false) noexcept;
    void OnRegistrationNetworkError() noexcept;

private:
    // Initialization
    void InitializeSPKs() noexcept;
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;
    void UpdateLayout() noexcept;

    // Helpers
    bool IsPointInWindow(int x, int y) const noexcept;
    bool IsPointInIdEdit(int x, int y) const noexcept;
    bool IsPointInPasswordEdit(int x, int y) const noexcept;
    bool IsPointInCreateButton(int x, int y) const noexcept;
    bool IsPointInCloseButton(int x, int y) const noexcept;
    bool IsPointInResultCloseButton(int x, int y) const noexcept;

    // Callbacks
    void OnCreateClicked() noexcept;
    void OnCloseClicked() noexcept;
    void OnResultCloseClicked() noexcept;
    void OnTabPressed() noexcept;
    void OnEscapePressed() noexcept;
    void OnEnterPressed() noexcept;

    // State
    bool isOpen_{ false };
    NewUserState state_{ NewUserState::None };
    bool waitingResponse_{ false };
    bool idHasFocus_{ true };
    settings::NewUserProfile config_;

    // Input
    std::string accountName_;
    std::string password_;

    // SPK archives
    std::unique_ptr<SpriteArchive> mainSpk_;
    std::unique_ptr<SpriteArchive> commonSpk_;

    // UI components
    std::unique_ptr<ButtonGroup> buttonGroup_;
    std::unique_ptr<LineEditor> idEditor_;
    std::unique_ptr<LineEditor> passwordEditor_;

    // Dialog for network errors
    std::unique_ptr<DialogWindow> dialog_;

    // Callbacks
    CloseCallback closeCallback_;
    RegisterCallback registerCallback_;
    ResultCallback resultCallback_;
};

} // namespace phoenix::ui