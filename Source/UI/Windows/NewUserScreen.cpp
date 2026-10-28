#include "UI/Windows/NewUserScreen.h"

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

NewUserScreen::NewUserScreen() noexcept
    : isOpen_(false)
    , state_(NewUserState::None)
    , waitingResponse_(false)
    , idHasFocus_(true)
{
}

void NewUserScreen::Open() noexcept
{
    isOpen_ = true;
    state_ = NewUserState::None;
    waitingResponse_ = false;
    idHasFocus_ = true;
    accountName_.clear();
    password_.clear();

    InitializeSPKs();
    InitializeButtons();
    PositionButtons();
    UpdateLayout();

    if (idEditor_)
    {
        idEditor_->SetPosition(config_.idEditX, config_.idEditY);
        idEditor_->SetByteLimit(config_.idByteLimit);
        idEditor_->Acquire();
    }
    if (passwordEditor_)
    {
        passwordEditor_->SetPosition(config_.passwordEditX, config_.passwordEditY);
        passwordEditor_->SetByteLimit(config_.passwordByteLimit);
        passwordEditor_->SetPasswordMode(true);
    }
}

void NewUserScreen::Close() noexcept
{
    isOpen_ = false;
    state_ = NewUserState::None;
    waitingResponse_ = false;
    mainSpk_.reset();
    commonSpk_.reset();
    buttonGroup_.reset();
    idEditor_.reset();
    passwordEditor_.reset();
    dialog_.reset();
}

void NewUserScreen::LoadFromConfig(const settings::NewUserProfile& profile) noexcept
{
    config_ = profile;
}

void NewUserScreen::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

void NewUserScreen::InitializeSPKs() noexcept
{
    mainSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.newUserSpkName.empty())
    {
        mainSpk_->Open(config_.newUserSpkName.c_str());
    }

    commonSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.commonSpkName.empty())
    {
        commonSpk_->Open(config_.commonSpkName.c_str());
    }
}

void NewUserScreen::InitializeButtons() noexcept
{
    buttonGroup_ = std::make_unique<ButtonGroup>();

    // Create button
    if (mainSpk_)
    {
        buttonGroup_->AddButton(1, // CREATE_ID
            config_.createButtonX, config_.createButtonY,
            mainSpk_->GetWidth(config_.createNormalFrame),
            mainSpk_->GetHeight(config_.createNormalFrame),
            [this]() { OnCreateClicked(); });

        // Close button (X)
        buttonGroup_->AddButton(2, // CLOSE_ID
            config_.closeButtonX, config_.closeButtonY,
            mainSpk_->GetWidth(config_.closeButtonFrame),
            mainSpk_->GetHeight(config_.closeButtonFrame),
            [this]() { OnCloseClicked(); });

        // Result close button
        buttonGroup_->AddButton(3, // RESULT_CLOSE_ID
            config_.resultCloseButtonX, config_.resultCloseButtonY,
            mainSpk_->GetWidth(config_.resultCloseButtonFrame),
            mainSpk_->GetHeight(config_.resultCloseButtonFrame),
            [this]() { OnResultCloseClicked(); });
    }

    // ID editor
    idEditor_ = std::make_unique<LineEditor>();

    // Password editor
    passwordEditor_ = std::make_unique<LineEditor>();
}

void NewUserScreen::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(1, config_.createButtonX, config_.createButtonY);
    buttonGroup_->SetPosition(2, config_.closeButtonX, config_.closeButtonY);
    buttonGroup_->SetPosition(3, config_.resultCloseButtonX, config_.resultCloseButtonY);
}

void NewUserScreen::UpdateLayout() noexcept
{
    // Window position/size from config or centered
    if (config_.windowWidth > 0 && config_.windowHeight > 0)
    {
        // Use config values
    }
}

bool NewUserScreen::IsPointInWindow(int x, int y) const noexcept
{
    int w = config_.windowWidth > 0 ? config_.windowWidth : (mainSpk_ ? mainSpk_->GetWidth(config_.bgFrame) : 320);
    int h = config_.windowHeight > 0 ? config_.windowHeight : (mainSpk_ ? mainSpk_->GetHeight(config_.bgFrame) : 200);
    return x >= config_.windowX && x < config_.windowX + w &&
           y >= config_.windowY && y < config_.windowY + h;
}

bool NewUserScreen::IsPointInIdEdit(int x, int y) const noexcept
{
    return x >= config_.idEditX && x < config_.idEditX + config_.idEditWidth &&
           y >= config_.idEditY && y < config_.idEditY + config_.idEditHeight;
}

bool NewUserScreen::IsPointInPasswordEdit(int x, int y) const noexcept
{
    return x >= config_.passwordEditX && x < config_.passwordEditX + config_.passwordEditWidth &&
           y >= config_.passwordEditY && y < config_.passwordEditY + config_.passwordEditHeight;
}

bool NewUserScreen::IsPointInCreateButton(int x, int y) const noexcept
{
    if (!mainSpk_) return false;
    int w = mainSpk_->GetWidth(config_.createNormalFrame);
    int h = mainSpk_->GetHeight(config_.createNormalFrame);
    return x >= config_.createButtonX && x < config_.createButtonX + w &&
           y >= config_.createButtonY && y < config_.createButtonY + h;
}

bool NewUserScreen::IsPointInCloseButton(int x, int y) const noexcept
{
    if (!mainSpk_) return false;
    int w = mainSpk_->GetWidth(config_.closeButtonFrame);
    int h = mainSpk_->GetHeight(config_.closeButtonFrame);
    return x >= config_.closeButtonX && x < config_.closeButtonX + w &&
           y >= config_.closeButtonY && y < config_.closeButtonY + h;
}

bool NewUserScreen::IsPointInResultCloseButton(int x, int y) const noexcept
{
    if (!mainSpk_) return false;
    int w = mainSpk_->GetWidth(config_.resultCloseButtonFrame);
    int h = mainSpk_->GetHeight(config_.resultCloseButtonFrame);
    return x >= config_.resultCloseButtonX && x < config_.resultCloseButtonX + w &&
           y >= config_.resultCloseButtonY && y < config_.resultCloseButtonY + h;
}

void NewUserScreen::OnCreateClicked() noexcept
{
    if (state_ != NewUserState::None)
    {
        OnResultCloseClicked();
        return;
    }

    if (!accountName_.empty() && !password_.empty())
    {
        waitingResponse_ = true;
        if (registerCallback_)
        {
            registerCallback_(accountName_, password_);
        }
    }
    else
    {
        OnRegistrationFailed(false);
    }
}

void NewUserScreen::OnCloseClicked() noexcept
{
    if (closeCallback_)
        closeCallback_();
    Close();
}

void NewUserScreen::OnResultCloseClicked() noexcept
{
    state_ = NewUserState::None;
    waitingResponse_ = false;
    accountName_.clear();
    password_.clear();
    if (idEditor_)
    {
        idEditor_->EraseAll();
        idEditor_->Acquire();
    }
    if (passwordEditor_)
    {
        passwordEditor_->EraseAll();
    }
    idHasFocus_ = true;
}

void NewUserScreen::OnTabPressed() noexcept
{
    if (state_ != NewUserState::None)
        return;

    idHasFocus_ = !idHasFocus_;
    if (idHasFocus_)
    {
        if (idEditor_) idEditor_->Acquire();
        if (passwordEditor_) passwordEditor_->Unacquire();
    }
    else
    {
        if (idEditor_) idEditor_->Unacquire();
        if (passwordEditor_) passwordEditor_->Acquire();
    }
}

void NewUserScreen::OnEscapePressed() noexcept
{
    if (state_ != NewUserState::None)
    {
        OnResultCloseClicked();
    }
    else
    {
        OnCloseClicked();
    }
}

void NewUserScreen::OnEnterPressed() noexcept
{
    if (state_ != NewUserState::None)
    {
        OnResultCloseClicked();
        return;
    }

    if (!accountName_.empty() && !password_.empty())
    {
        waitingResponse_ = true;
        if (registerCallback_)
        {
            registerCallback_(accountName_, password_);
        }
    }
    else
    {
        OnTabPressed();
    }
}

void NewUserScreen::OnRegistrationSuccess() noexcept
{
    state_ = NewUserState::Success;
    waitingResponse_ = false;
    if (resultCallback_)
        resultCallback_(NewUserResult::Success);
}

void NewUserScreen::OnRegistrationFailed(bool accountAlreadyExists) noexcept
{
    state_ = NewUserState::Failed;
    waitingResponse_ = false;
    if (accountAlreadyExists)
    {
        state_ = NewUserState::AlreadyExists;
        if (resultCallback_)
            resultCallback_(NewUserResult::AccountAlreadyExists);
    }
    else
    {
        if (resultCallback_)
            resultCallback_(NewUserResult::Failed);
    }
}

void NewUserScreen::OnRegistrationNetworkError() noexcept
{
    state_ = NewUserState::Failed;
    waitingResponse_ = false;
    if (resultCallback_)
        resultCallback_(NewUserResult::NetworkError);
}

bool NewUserScreen::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    // Check ID edit field
    if (IsPointInIdEdit(x, y))
    {
        if (idEditor_)
        {
            idEditor_->Acquire();
            if (passwordEditor_) passwordEditor_->Unacquire();
        }
        idHasFocus_ = true;
        return true;
    }

    // Check password edit field
    if (IsPointInPasswordEdit(x, y))
    {
        if (passwordEditor_)
        {
            passwordEditor_->Acquire();
            if (idEditor_) idEditor_->Unacquire();
        }
        idHasFocus_ = false;
        return true;
    }

    // Check create button
    if (IsPointInCreateButton(x, y))
    {
        if (buttonGroup_)
            buttonGroup_->HandleMouseDown(x - config_.createButtonX, y - config_.createButtonY);
        return true;
    }

    // Check close button
    if (IsPointInCloseButton(x, y))
    {
        if (buttonGroup_)
            buttonGroup_->HandleMouseDown(x - config_.closeButtonX, y - config_.closeButtonY);
        return true;
    }

    // Check result close button
    if (state_ != NewUserState::None && IsPointInResultCloseButton(x, y))
    {
        if (buttonGroup_)
            buttonGroup_->HandleMouseDown(x - config_.resultCloseButtonX, y - config_.resultCloseButtonY);
        return true;
    }

    // Check button group
    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseDown(x, y);
        return true;
    }

    return false;
}

bool NewUserScreen::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    return false;
}

bool NewUserScreen::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    return false;
}

bool NewUserScreen::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    switch (key)
    {
        case VK_TAB:
            OnTabPressed();
            return true;

        case VK_ESCAPE:
            OnEscapePressed();
            return true;

        case VK_RETURN:
            OnEnterPressed();
            return true;
    }

    return false;
}

bool NewUserScreen::HandleCharInput(wchar_t c) noexcept
{
    if (!isOpen_ || state_ != NewUserState::None)
        return false;

    if (c == VK_TAB || c == VK_RETURN || c == VK_ESCAPE)
        return false;

    if (idHasFocus_ && idEditor_)
    {
        return idEditor_->AddChar(c);
    }
    else if (!idHasFocus_ && passwordEditor_)
    {
        return passwordEditor_->AddChar(c);
    }

    return false;
}

void NewUserScreen::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    if (idEditor_)
        idEditor_->Update(deltaTime);
    if (passwordEditor_)
        passwordEditor_->Update(deltaTime);
    if (buttonGroup_)
        buttonGroup_->Update(deltaTime);
    if (dialog_)
        dialog_->Update(deltaTime);

    // Update account name from editor
    if (idEditor_)
    {
        accountName_ = idEditor_->GetText();
    }
    if (passwordEditor_)
    {
        password_ = passwordEditor_->GetText();
    }
}

void NewUserScreen::Render() noexcept
{
    if (!isOpen_)
        return;

    if (!mainSpk_)
        return;

    int bgFrame = config_.bgFrame;
    switch (state_)
    {
        case NewUserState::Success:
            bgFrame = config_.accountCreatedFrame;
            break;
        case NewUserState::Failed:
            bgFrame = config_.accountNotCreatedFrame;
            break;
        case NewUserState::AlreadyExists:
            bgFrame = config_.accountAlreadyExistsFrame;
            break;
        default:
            bgFrame = config_.bgFrame;
            break;
    }

    mainSpk_->Draw(config_.windowX, config_.windowY, bgFrame);

    // Render ID editor
    if (state_ == NewUserState::None && idEditor_)
    {
        idEditor_->Render();
    }

    // Render password editor
    if (state_ == NewUserState::None && passwordEditor_)
    {
        passwordEditor_->Render();
    }

    // Render buttons
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }

    // Render result close button if in result state
    if (state_ != NewUserState::None)
    {
        int closeFrame = config_.resultCloseButtonFrame;
        mainSpk_->Draw(config_.resultCloseButtonX, config_.resultCloseButtonY, closeFrame);
    }
}

} // namespace phoenix::ui