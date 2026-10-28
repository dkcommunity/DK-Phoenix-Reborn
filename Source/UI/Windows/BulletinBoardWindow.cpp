#include "UI/Windows/BulletinBoardWindow.h"

#include "Settings/InterfaceProfileLoader.h"
#include "Protocol/LegacyProtocol.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

BulletinBoardWindow::BulletinBoardWindow() noexcept
    : isOpen_(false)
    , draggable_(true)
    , isDragging_(false)
    , dragOffsetX_(0)
    , dragOffsetY_(0)
    , itemID_(0)
    , windowX_(0)
    , windowY_(0)
    , windowWidth_(400)
    , windowHeight_(200)
{
}

void BulletinBoardWindow::Open() noexcept
{
    isOpen_ = true;

    // Initialize SPK archive
    backSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.backSpkName.empty())
    {
        backSpk_->Open(config_.backSpkName.c_str());
    }

    // Initialize text edits
    msgEdit_ = std::make_unique<LineEditor>();
    fromEdit_ = std::make_unique<LineEditor>();

    PositionEdits();
    InitializeButtons();
    PositionButtons();

    // Set focus to message edit
    msgEdit_->SetFocus(true);
}

void BulletinBoardWindow::Close() noexcept
{
    isOpen_ = false;
    isDragging_ = false;

    msgEdit_.reset();
    fromEdit_.reset();
    buttonGroup_.reset();
    backSpk_.reset();
}

void BulletinBoardWindow::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open();
}

void BulletinBoardWindow::LoadFromConfig(const settings::BulletinBoardProfile& profile) noexcept
{
    config_ = profile;
    windowX_ = config_.windowX;
    windowY_ = config_.windowY;
    windowWidth_ = config_.windowWidth;
    windowHeight_ = config_.windowHeight;
    draggable_ = config_.draggable;

    InitializeButtons();
    PositionButtons();
    PositionEdits();
}

void BulletinBoardWindow::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

void BulletinBoardWindow::SetItem(std::uint32_t itemID) noexcept
{
    itemID_ = itemID;
}

bool BulletinBoardWindow::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    // Check if point is in window
    if (!IsPointInWindow(x, y))
        return false;

    // Check buttons first
    if (buttonGroup_)
    {
        if (buttonGroup_->HandleMouseDown(x, y))
            return true;
    }

    // Check text edits
    if (msgEdit_ && IsPointInMsgEdit(x, y))
    {
        msgEdit_->SetFocus(true);
        fromEdit_->SetFocus(false);
        return true;
    }
    if (fromEdit_ && IsPointInFromEdit(x, y))
    {
        fromEdit_->SetFocus(false);
        msgEdit_->SetFocus(true);
        return true;
    }

    // Check drag
    if (draggable_ && !IsPointInMsgEdit(x, y) && !IsPointInFromEdit(x, y))
    {
        isDragging_ = true;
        dragOffsetX_ = x - windowX_;
        dragOffsetY_ = y - windowY_;
        return true;
    }

    return true;
}

bool BulletinBoardWindow::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    EndDrag();
    return false;
}

bool BulletinBoardWindow::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    if (isDragging_)
    {
        UpdateDrag(x, y);
    }

    return false;
}

bool BulletinBoardWindow::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    switch (key)
    {
        case VK_ESCAPE:
            OnCancelClicked();
            return true;
        case VK_RETURN:
            if (msgEdit_ && msgEdit_->HasFocus())
            {
                // Check line count
                if (msgEdit_->GetLineCount() > 2)
                {
                    // Move focus to from edit
                    fromEdit_->SetFocus(true);
                    msgEdit_->SetFocus(false);
                }
            }
            return true;
        case VK_BACK:
            if (fromEdit_ && fromEdit_->HasFocus() && fromEdit_->Size() == 0)
            {
                fromEdit_->SetFocus(false);
                msgEdit_->SetFocus(true);
            }
            return true;
        case VK_UP:
            if (fromEdit_ && fromEdit_->HasFocus())
            {
                fromEdit_->SetFocus(false);
                msgEdit_->SetFocus(true);
                msgEdit_->EndCursor();
            }
            return true;
        case VK_DOWN:
        case VK_TAB:
            if (msgEdit_ && msgEdit_->HasFocus())
            {
                // Focus to from edit
                fromEdit_->SetFocus(true);
                msgEdit_->SetFocus(false);
            }
            return true;
    }
    return false;
}

bool BulletinBoardWindow::HandleChar(wchar_t ch) noexcept
{
    if (!isOpen_)
        return false;

    // Ignore special characters
    const char ignoreChars[] = { '%', '\\', '"', '\'' };
    for (char c : ignoreChars)
    {
        if (ch == c)
            return false;
    }

    if (msgEdit_ && msgEdit_->HasFocus())
    {
        return msgEdit_->AddChar(ch);
    }
    if (fromEdit_ && fromEdit_->HasFocus())
    {
        return fromEdit_->AddChar(ch);
    }

    return false;
}

void BulletinBoardWindow::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    if (buttonGroup_)
    {
        buttonGroup_->Update(deltaTime);
    }
}

void BulletinBoardWindow::Render() noexcept
{
    if (!isOpen_)
        return;

    // Draw background
    if (backSpk_)
    {
        backSpk_->Draw(windowX_, windowY_, config_.mainWindowFrame);
    }

    // Draw text edits
    if (msgEdit_)
    {
        msgEdit_->Render();
    }
    if (fromEdit_)
    {
        fromEdit_->Render();
    }

    // Draw buttons
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }
}

void BulletinBoardWindow::OnOkClicked() noexcept
{
    // Get message text
    // Send UI_USE_XMAS_TREE packet
    // Legacy: gpC_base->SendMessage(UI_USE_XMAS_TREE, (int)m_pItem, 1, (void*)m_szTreeMessage.c_str());

    // Close window
    Close();
}

void BulletinBoardWindow::OnCancelClicked() noexcept
{
    Close();
}

void BulletinBoardWindow::InitializeButtons() noexcept
{
    buttonGroup_ = std::make_unique<ButtonGroup>();

    // OK button
    buttonGroup_->AddButton(static_cast<int>(0), // OK_ID
        config_.okButtonX, config_.okButtonY,
        backSpk_ ? backSpk_->GetWidth(config_.buttonOkFrame) : 16,
        backSpk_ ? backSpk_->GetHeight(config_.buttonOkFrame) : 16,
        [this]() { OnOkClicked(); });

    // Cancel button
    buttonGroup_->AddButton(static_cast<int>(1), // CANCEL_ID
        config_.cancelButtonX, config_.cancelButtonY,
        backSpk_ ? backSpk_->GetWidth(config_.buttonCancelFrame) : 16,
        backSpk_ ? backSpk_->GetHeight(config_.buttonCancelFrame) : 16,
        [this]() { OnCancelClicked(); });
}

void BulletinBoardWindow::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(0, config_.okButtonX, config_.okButtonY);
    buttonGroup_->SetPosition(1, config_.cancelButtonX, config_.cancelButtonY);
}

void BulletinBoardWindow::PositionEdits() noexcept
{
    if (msgEdit_)
    {
        msgEdit_->SetPosition(windowX_ + config_.msgX, windowY_ + config_.msgY);
    }
    if (fromEdit_)
    {
        fromEdit_->SetPosition(windowX_ + config_.fromX, windowY_ + config_.fromY);
    }
}

bool BulletinBoardWindow::IsPointInWindow(int x, int y) const noexcept
{
    return x >= windowX_ && x < windowX_ + windowWidth_ &&
           y >= windowY_ && y < windowY_ + windowHeight_;
}

bool BulletinBoardWindow::IsPointInMsgEdit(int x, int y) const noexcept
{
    if (!msgEdit_)
        return false;

    int editX = windowX_ + config_.msgX;
    int editY = windowY_ + config_.msgY;
    int editW = config_.msgWidth;
    int editH = config_.msgHeight;

    return x >= editX && x < editX + editW && y >= editY && y < editY + editH;
}

bool BulletinBoardWindow::IsPointInFromEdit(int x, int y) const noexcept
{
    if (!fromEdit_)
        return false;

    int editX = windowX_ + config_.fromX;
    int editY = windowY_ + config_.fromY;

    return x >= editX && x < editX + config_.fromByteLimit * 8 &&
           y >= editY && y < editY + 20;
}

void BulletinBoardWindow::UpdateDrag(int x, int y) noexcept
{
    if (isDragging_)
    {
        windowX_ = x - dragOffsetX_;
        windowY_ = y - dragOffsetY_;
        PositionEdits();
        PositionButtons();
    }
}

void BulletinBoardWindow::EndDrag() noexcept
{
    isDragging_ = false;
}

} // namespace phoenix::ui