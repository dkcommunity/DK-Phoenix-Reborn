#include "UI/Windows/LoginScreen.h"

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

LoginScreen::LoginScreen() noexcept
    : isOpen_(false)
    , idFocused_(true)
    , windowX_(0)
    , windowY_(0)
    , windowWidth_(0)
    , windowHeight_(0)
    , highRes_(false)
    , convX_(0)
    , convY_(0)
{
}

void LoginScreen::Open() noexcept
{
    isOpen_ = true;
    idFocused_ = true;

    // Initialize SPKs
    InitializeSPKs();
    UpdateResolution();

    // Initialize UI elements
    InitializeLineEditors();
    InitializeButtons();
    PositionButtons();
    PositionLineEditors();
}

void LoginScreen::Close() noexcept
{
    isOpen_ = false;
    loginSpk_.reset();
    menuSpk_.reset();
    buttonGroup_.reset();
    idEdit_.reset();
    passwordEdit_.reset();
}

void LoginScreen::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open();
}

void LoginScreen::LoadFromConfig(const settings::LoginProfile& profile) noexcept
{
    config_ = profile;
    windowX_ = config_.windowX;
    windowY_ = config_.windowY;
    windowWidth_ = config_.windowWidth;
    windowHeight_ = config_.windowHeight;
    UpdateResolution();
}

void LoginScreen::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

void LoginScreen::InitializeSPKs() noexcept
{
    loginSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.loginSpkName.empty())
    {
        loginSpk_->Open(config_.loginSpkName.c_str());
    }

    menuSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.menuSpkName.empty())
    {
        menuSpk_->Open(config_.menuSpkName.c_str());
    }
}

void LoginScreen::InitializeButtons() noexcept
{
    buttonGroup_ = std::make_unique<ButtonGroup>();

    // New Account button
    buttonGroup_->AddButton(static_cast<int>(0), // NEW_ID
        config_.newButtonX, config_.newButtonY,
        menuSpk_ ? menuSpk_->GetWidth(config_.newButtonFrame) : 16,
        menuSpk_ ? menuSpk_->GetHeight(config_.newButtonFrame) : 16,
        [this]() { OnNewAccountClicked(); });

    // OK button
    buttonGroup_->AddButton(static_cast<int>(1), // OK
        config_.okButtonX, config_.okButtonY,
        menuSpk_ ? menuSpk_->GetWidth(config_.okButtonFrame) : 16,
        menuSpk_ ? menuSpk_->GetHeight(config_.okButtonFrame) : 16,
        [this]() { OnOkClicked(); });

    // Cancel button
    buttonGroup_->AddButton(static_cast<int>(2), // CANCEL
        config_.cancelButtonX, config_.cancelButtonY,
        menuSpk_ ? menuSpk_->GetWidth(config_.cancelButtonFrame) : 16,
        menuSpk_ ? menuSpk_->GetHeight(config_.cancelButtonFrame) : 16,
        [this]() { OnCancelClicked(); });
}

void LoginScreen::InitializeLineEditors() noexcept
{
    idEdit_ = std::make_unique<LineEditor>();
    idEdit_->SetByteLimit(config_.idByteLimit);

    passwordEdit_ = std::make_unique<LineEditor>();
    passwordEdit_->SetPasswordMode(true);
    passwordEdit_->SetByteLimit(config_.passwordByteLimit);
}

void LoginScreen::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    int newBtnX = config_.newButtonX + convX_;
    int newBtnY = config_.newButtonY + convY_;
    int okBtnX = config_.okButtonX + convX_;
    int okBtnY = config_.okButtonY + convY_;
    int cancelBtnX = config_.cancelButtonX + convX_;
    int cancelBtnY = config_.cancelButtonY + convY_;

    buttonGroup_->SetPosition(0, newBtnX, newBtnY);
    buttonGroup_->SetPosition(1, okBtnX, okBtnY);
    buttonGroup_->SetPosition(2, cancelBtnX, cancelBtnY);
}

void LoginScreen::PositionLineEditors() noexcept
{
    if (idEdit_)
    {
        idEdit_->SetPosition(windowX_ + config_.idX + convX_, windowY_ + config_.idY + convY_);
    }
    if (passwordEdit_)
    {
        passwordEdit_->SetPosition(windowX_ + config_.passwordX + convX_, windowY_ + config_.passwordY + convY_);
    }
}

void LoginScreen::UpdateResolution() noexcept
{
    // In legacy: check resolution and load appropriate SPK
    // For now, use base resolution
    if (loginSpk_)
    {
        windowWidth_ = loginSpk_->GetWidth(0);
        windowHeight_ = loginSpk_->GetHeight(0);

        // Center window
        windowX_ = (1280 - windowWidth_) / 2;
        windowY_ = (960 - windowHeight_) / 2;
    }

    highRes_ = false;
    convX_ = 0;
    convY_ = 0;
}

bool LoginScreen::IsPointInWindow(int x, int y) const noexcept
{
    return x >= windowX_ && x < windowX_ + windowWidth_ &&
           y >= windowY_ && y < windowY_ + windowHeight_;
}

bool LoginScreen::IsPointInID(int x, int y) const noexcept
{
    int left = windowX_ + config_.idX + convX_;
    int top = windowY_ + config_.idY + convY_;
    int right = left + config_.idWidth;
    int bottom = top + config_.idHeight;

    return x >= left && x < right && y >= top && y < bottom;
}

bool LoginScreen::IsPointInPassword(int x, int y) const noexcept
{
    int left = windowX_ + config_.passwordX + convX_;
    int top = windowY_ + config_.passwordY + convY_;
    int right = left + config_.passwordWidth;
    int bottom = top + config_.passwordHeight;

    return x >= left && x < right && y >= top && y < bottom;
}

void LoginScreen::ChangeFocus() noexcept
{
    if (idFocused_)
    {
        if (idEdit_ && idEdit_->Size() == 0)
        {
            FocusPassword();
        }
        else
        {
            FocusPassword();
        }
    }
    else
    {
        FocusID();
    }
}

void LoginScreen::FocusID() noexcept
{
    if (idEdit_)
    {
        idEdit_->SetFocus(true);
    }
    if (passwordEdit_)
    {
        passwordEdit_->SetFocus(false);
    }
    idFocused_ = true;
}

void LoginScreen::FocusPassword() noexcept
{
    if (idEdit_)
    {
        idEdit_->SetFocus(false);
    }
    if (passwordEdit_)
    {
        passwordEdit_->SetFocus(true);
    }
    idFocused_ = false;
}

bool LoginScreen::ReadySend() const noexcept
{
    if (!idEdit_ || !passwordEdit_)
        return false;

    return idEdit_->Size() > 0 && passwordEdit_->Size() > 0;
}

bool LoginScreen::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    // Check ID field
    if (IsPointInID(x, y))
    {
        FocusID();
        return true;
    }

    // Check Password field
    if (IsPointInPassword(x, y))
    {
        FocusPassword();
        return true;
    }

    // Check buttons
    if (buttonGroup_)
    {
        if (buttonGroup_->HandleMouseDown(x, y))
            return true;
    }

    return false;
}

bool LoginScreen::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    return false;
}

bool LoginScreen::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    return false;
}

bool LoginScreen::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    if (key == VK_ESCAPE)
    {
        OnCancelClicked();
        return true;
    }
    if (key == VK_RETURN)
    {
        if (ReadySend())
        {
            OnOkClicked();
        }
        else
        {
            ChangeFocus();
        }
        return true;
    }
    if (key == VK_TAB)
    {
        ChangeFocus();
        return true;
    }

    return false;
}

bool LoginScreen::HandleChar(wchar_t ch) noexcept
{
    if (!isOpen_)
        return false;

    if (idFocused_ && idEdit_)
    {
        return idEdit_->AddChar(ch);
    }
    else if (!idFocused_ && passwordEdit_)
    {
        return passwordEdit_->AddChar(ch);
    }

    return false;
}

void LoginScreen::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    if (buttonGroup_)
    {
        buttonGroup_->Update(deltaTime);
    }
}

void LoginScreen::Render() noexcept
{
    if (!isOpen_)
        return;

    if (!loginSpk_)
        return;

    // Draw background
    loginSpk_->Draw(windowX_, windowY_, 0);

    // Draw ID field
    if (idEdit_)
    {
        idEdit_->Render();
    }

    // Draw Password field
    if (passwordEdit_)
    {
        passwordEdit_->Render();
    }

    // Draw buttons
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }
}

void LoginScreen::OnNewAccountClicked() noexcept
{
    if (newAccountCallback_)
    {
        newAccountCallback_();
    }
    Close();
}

void LoginScreen::OnOkClicked() noexcept
{
    if (ReadySend() && loginCallback_)
    {
        loginCallback_(idEdit_->GetString(), passwordEdit_->GetString());
    }
    else
    {
        ChangeFocus();
    }
}

void LoginScreen::OnCancelClicked() noexcept
{
    if (cancelCallback_)
    {
        cancelCallback_();
    }
    Close();
}

} // namespace phoenix::ui