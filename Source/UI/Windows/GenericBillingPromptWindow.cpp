#include "UI/Windows/GenericBillingPromptWindow.h"

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

GenericBillingPromptWindow::GenericBillingPromptWindow() noexcept
    : isOpen_(false)
    , mode_(0)
    , windowX_(465)
    , windowY_(380)
    , windowWidth_(350)
    , windowHeight_(200)
{
}

void GenericBillingPromptWindow::Open(uint8_t mode, const std::string& customMessage) noexcept
{
    isOpen_ = true;
    mode_ = mode;
    customMessage_ = customMessage;

    // Initialize SPK
    backSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.backSpkName.empty())
    {
        backSpk_->Open(config_.backSpkName.c_str());
    }

    InitializeButtons();
    PositionButtons();
}

void GenericBillingPromptWindow::Close() noexcept
{
    isOpen_ = false;
    backSpk_.reset();
    buttonGroup_.reset();
}

void GenericBillingPromptWindow::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open(0, "");
}

void GenericBillingPromptWindow::LoadFromConfig(const settings::GenericBillingPromptProfile& profile) noexcept
{
    config_ = profile;
    windowX_ = config_.windowX;
    windowY_ = config_.windowY;
    windowWidth_ = config_.windowWidth;
    windowHeight_ = config_.windowHeight;
}

void GenericBillingPromptWindow::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

bool GenericBillingPromptWindow::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    if (buttonGroup_)
    {
        if (buttonGroup_->HandleMouseDown(x, y))
            return true;
    }

    return false;
}

bool GenericBillingPromptWindow::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    return false;
}

bool GenericBillingPromptWindow::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    return false;
}

bool GenericBillingPromptWindow::HandleKeyDown(std::uint16_t key) noexcept
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
        OnOkClicked();
        return true;
    }

    return false;
}

void GenericBillingPromptWindow::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    if (buttonGroup_)
    {
        buttonGroup_->Update(deltaTime);
    }
}

void GenericBillingPromptWindow::Render() noexcept
{
    if (!isOpen_)
        return;

    if (!backSpk_)
        return;

    // Draw background
    backSpk_->Draw(windowX_, windowY_, 0);

    // Draw message text
    std::string message = GetMessageForMode(mode_);
    // In legacy: g_PrintColorStr with word wrapping
    // For now, placeholder

    // Draw buttons
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }
}

void GenericBillingPromptWindow::InitializeButtons() noexcept
{
    buttonGroup_ = std::make_unique<ButtonGroup>();

    // OK button
    buttonGroup_->AddButton(static_cast<int>(0), // OK_ID
        config_.okButtonX, config_.okButtonY,
        backSpk_ ? backSpk_->GetWidth(config_.okButtonFrame) : 16,
        backSpk_ ? backSpk_->GetHeight(config_.okButtonFrame) : 16,
        [this]() { OnOkClicked(); });

    // Cancel button
    buttonGroup_->AddButton(static_cast<int>(1), // CANCEL_ID
        config_.cancelButtonX, config_.cancelButtonY,
        backSpk_ ? backSpk_->GetWidth(config_.cancelButtonFrame) : 16,
        backSpk_ ? backSpk_->GetHeight(config_.cancelButtonFrame) : 16,
        [this]() { OnCancelClicked(); });
}

void GenericBillingPromptWindow::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(0, config_.okButtonX, config_.okButtonY);
    buttonGroup_->SetPosition(1, config_.cancelButtonX, config_.cancelButtonY);
}

bool GenericBillingPromptWindow::IsPointInWindow(int x, int y) const noexcept
{
    return x >= windowX_ && x < windowX_ + windowWidth_ &&
           y >= windowY_ && y < windowY_ + windowHeight_;
}

std::string GenericBillingPromptWindow::GetMessageForMode(uint8_t mode) const noexcept
{
    switch (mode)
    {
        case 0: // CANNOT_PLAY_MSG
            return "You cannot play this character.\nWould you like to go to the billing page?";
        case 1: // CANNOT_PLAY_BY_ATTR_MSG
            return "You cannot play this character due to attribute restrictions.\nWould you like to go to the billing page?";
        case 2: // CUSTOM
            return customMessage_.empty() ? "Custom message not set." : customMessage_;
        default:
            return "Unknown message mode.";
    }
}

void GenericBillingPromptWindow::OnOkClicked() noexcept
{
    // Send packet to go to billing page
    // Legacy: gpC_base->SendMessage(UI_GO_BILING_PAGE, TRUE);

    // Call store callback
    if (storeCallback_)
    {
        storeCallback_();
    }

    Close();
}

void GenericBillingPromptWindow::OnCancelClicked() noexcept
{
    Close();
}

} // namespace phoenix::ui