#include "UI/Windows/CharacterDeleteScreen.h"

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

CharacterDeleteScreen::CharacterDeleteScreen() noexcept
    : isOpen_(false)
    , slotIndex_(-1)
    , netmarbleMode_(false)
{
}

void CharacterDeleteScreen::Open(int slotIndex) noexcept
{
    isOpen_ = true;
    slotIndex_ = slotIndex;

    InitializeSPKs();
    InitializeButtons();
    PositionButtons();

    // Initialize SSN editors
    ssnPart1Editor_ = std::make_unique<LineEditor>();
    ssnPart1Editor_->SetByteLimit(config_.ssnPart1ByteLimit);
    ssnPart1Editor_->SetPosition(config_.ssnPart1X, config_.ssnPart1Y);
    ssnPart1Editor_->EraseAll();
    ssnPart1Editor_->Acquire();

    ssnPart2Editor_ = std::make_unique<LineEditor>();
    ssnPart2Editor_->SetByteLimit(config_.ssnPart2ByteLimit);
    ssnPart2Editor_->SetPosition(config_.ssnPart2X, config_.ssnPart2Y);
    ssnPart2Editor_->EraseAll();
    ssnPart2Editor_->SetPasswordMode(true);
}

void CharacterDeleteScreen::Close() noexcept
{
    isOpen_ = false;
    slotIndex_ = -1;
    assembleBoxButtonSpk_.reset();
    buttonGroup_.reset();
    ssnPart1Editor_.reset();
    ssnPart2Editor_.reset();
}

void CharacterDeleteScreen::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open(0);
}

void CharacterDeleteScreen::LoadFromConfig(const settings::CharacterDeleteProfile& profile) noexcept
{
    config_ = profile;
}

void CharacterDeleteScreen::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

void CharacterDeleteScreen::InitializeSPKs() noexcept
{
    assembleBoxButtonSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.assembleBoxButtonSpkName.empty())
        assembleBoxButtonSpk_->Open(config_.assembleBoxButtonSpkName.c_str());
}

void CharacterDeleteScreen::InitializeButtons() noexcept
{
    buttonGroup_ = std::make_unique<ButtonGroup>();

    // OK button
    buttonGroup_->AddButton(0, // DELETE_OK
        config_.okButtonX, config_.okButtonY,
        assembleBoxButtonSpk_ ? assembleBoxButtonSpk_->GetWidth(config_.okButtonFrame) : 64,
        assembleBoxButtonSpk_ ? assembleBoxButtonSpk_->GetHeight(config_.okButtonFrame) : 24,
        [this]() { OnOkClicked(); });

    // Cancel button
    buttonGroup_->AddButton(1, // DELETE_CANCEL
        config_.cancelButtonX, config_.cancelButtonY,
        assembleBoxButtonSpk_ ? assembleBoxButtonSpk_->GetWidth(config_.cancelButtonFrame) : 64,
        assembleBoxButtonSpk_ ? assembleBoxButtonSpk_->GetHeight(config_.cancelButtonFrame) : 24,
        [this]() { OnCancelClicked(); });
}

void CharacterDeleteScreen::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(0, config_.okButtonX, config_.okButtonY);
    buttonGroup_->SetPosition(1, config_.cancelButtonX, config_.cancelButtonY);
}

bool CharacterDeleteScreen::IsPointInWindow(int x, int y) const noexcept
{
    return x >= config_.windowX && x < config_.windowX + config_.windowWidth &&
           y >= config_.windowY && y < config_.windowY + config_.windowHeight;
}

bool CharacterDeleteScreen::IsPointInSsnPart1(int x, int y) const noexcept
{
    return x >= config_.ssnPart1X && x < config_.ssnPart1X + 165 &&
           y >= config_.ssnPart1Y && y < config_.ssnPart1Y + 25;
}

bool CharacterDeleteScreen::IsPointInSsnPart2(int x, int y) const noexcept
{
    return x >= config_.ssnPart2X && x < config_.ssnPart2X + 165 &&
           y >= config_.ssnPart2Y && y < config_.ssnPart2Y + 25;
}

void CharacterDeleteScreen::OnOkClicked() noexcept
{
    if (confirmCallback_)
    {
        std::string ssn1 = ssnPart1Editor_ ? ssnPart1Editor_->GetText() : "";
        std::string ssn2 = ssnPart2Editor_ ? ssnPart2Editor_->GetText() : "";
        confirmCallback_(slotIndex_, ssn1, ssn2);
    }
    Close();
}

void CharacterDeleteScreen::OnCancelClicked() noexcept
{
    if (cancelCallback_)
        cancelCallback_();
    Close();
}

bool CharacterDeleteScreen::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    // Check SSN part 1 field
    if (IsPointInSsnPart1(x, y))
    {
        if (ssnPart1Editor_)
            ssnPart1Editor_->SetFocus(true);
        if (ssnPart2Editor_)
            ssnPart2Editor_->SetFocus(false);
        return true;
    }

    // Check SSN part 2 field
    if (IsPointInSsnPart2(x, y))
    {
        if (ssnPart2Editor_)
            ssnPart2Editor_->SetFocus(true);
        if (ssnPart1Editor_)
            ssnPart1Editor_->SetFocus(false);
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

bool CharacterDeleteScreen::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    return false;
}

bool CharacterDeleteScreen::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    return false;
}

bool CharacterDeleteScreen::HandleKeyDown(std::uint16_t key) noexcept
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

bool CharacterDeleteScreen::HandleChar(char c) noexcept
{
    if (!isOpen_)
        return false;

    // Legacy character filtering - only digits allowed
    if (c < '0' || c > '9')
        return false;

    // Forward to active editor
    if (ssnPart1Editor_ && ssnPart1Editor_->HasFocus())
    {
        ssnPart1Editor_->InsertChar(c);
        // Auto-move to part 2 when part 1 is full
        if (ssnPart1Editor_->Size() >= static_cast<size_t>(config_.ssnPart1ByteLimit))
        {
            if (ssnPart2Editor_)
            {
                ssnPart1Editor_->SetFocus(false);
                ssnPart2Editor_->SetFocus(true);
                ssnPart2Editor_->Acquire();
            }
        }
        return true;
    }

    if (ssnPart2Editor_ && ssnPart2Editor_->HasFocus())
    {
        ssnPart2Editor_->InsertChar(c);
        return true;
    }

    return false;
}

void CharacterDeleteScreen::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    if (buttonGroup_)
    {
        buttonGroup_->Update(deltaTime);
    }

    if (ssnPart1Editor_)
    {
        ssnPart1Editor_->Update(deltaTime);
    }

    if (ssnPart2Editor_)
    {
        ssnPart2Editor_->Update(deltaTime);
    }
}

void CharacterDeleteScreen::Render() noexcept
{
    if (!isOpen_)
        return;

    // Render background box with color and alpha
    // Legacy: gpC_base->m_p_DDSurface_back->DrawRect(&mssageBox, wLine_RGB);
    //         gpC_base->m_p_DDSurface_back->BltColorAlpha(&mssageBox, wBox_RGB, wBoxRGB_Alpha);

    int boxX = config_.windowX;
    int boxY = config_.windowY;
    int boxW = config_.windowWidth;
    int boxH = config_.windowHeight;

    // Render box frame
    if (assembleBoxButtonSpk_)
    {
        assembleBoxButtonSpk_->Draw(boxX, boxY, config_.messageBoxFrame);
    }

    // Render message text
    // g_PrintColorStr(boxX + boxW/2 - titleWidth/2, boxY + 30, titleString, ...);
    // g_PrintColorStr(boxX + boxW/2 - msgWidth/2, boxY + 50, messageString, ...);

    // Render SSN part 1 field
    if (ssnPart1Editor_)
    {
        // Draw name bar background
        if (assembleBoxButtonSpk_)
        {
            assembleBoxButtonSpk_->Draw(config_.ssnPart1X - 10, config_.ssnPart1Y - 4, 0); // AB_NAME_BAR frame
        }
        ssnPart1Editor_->Render();
    }

    // Render SSN part 2 field (if not netmarble mode)
    if (!netmarbleMode_ && ssnPart2Editor_)
    {
        if (assembleBoxButtonSpk_)
        {
            assembleBoxButtonSpk_->Draw(config_.ssnPart2X - 10, config_.ssnPart2Y - 4, 0);
        }
        ssnPart2Editor_->Render();
    }

    // Render Chinese/netmarble specific box
    if (netmarbleMode_ && ssnPart1Editor_)
    {
        // Draw outbox for netmarble
        if (assembleBoxButtonSpk_)
        {
            // Rect rect(ssnPart1X - 10, ssnPart1Y - 4, 165, 25);
            // gpC_global_resource->DrawOutBoxLocked(rect);
        }
    }

    // Render buttons
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }

    // Render button labels
    // g_PrintColorStr(okButtonX + offset, okButtonY + offset, okButtonString, ...);
    // g_PrintColorStr(cancelButtonX + offset, cancelButtonY + offset, cancelButtonString, ...);
}

} // namespace phoenix::ui