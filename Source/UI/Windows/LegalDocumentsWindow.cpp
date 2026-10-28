#include "UI/Windows/LegalDocumentsWindow.h"

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

LegalDocumentsWindow::LegalDocumentsWindow() noexcept
    : isOpen_(false)
    , currentTab_(LegalDocumentType::EULA)
    , checkBoxChecked_(false)
    , isDragging_(false)
    , dragOffsetX_(0)
    , dragOffsetY_(0)
    , windowX_(440)
    , windowY_(330)
    , windowWidth_(400)
    , windowHeight_(300)
{
    // Initialize empty document texts
    documentTexts_.fill("");
}

void LegalDocumentsWindow::Open() noexcept
{
    isOpen_ = true;

    // Initialize SPK archives
    backSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.backSpkName.empty())
    {
        backSpk_->Open(config_.backSpkName.c_str());
    }

    checkBoxSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.checkBoxSpkName.empty())
    {
        checkBoxSpk_->Open(config_.checkBoxSpkName.c_str());
    }

    InitializeButtons();
    PositionButtons();
    InitializeCheckBox();
}

void LegalDocumentsWindow::Close() noexcept
{
    isOpen_ = false;
    backSpk_.reset();
    checkBoxSpk_.reset();
    buttonGroup_.reset();
    checkBox_.reset();
}

void LegalDocumentsWindow::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open();
}

void LegalDocumentsWindow::LoadFromConfig(const settings::LegalDocumentsProfile& profile) noexcept
{
    config_ = profile;
    windowX_ = config_.windowX;
    windowY_ = config_.windowY;
    windowWidth_ = config_.windowWidth;
    windowHeight_ = config_.windowHeight;
}

void LegalDocumentsWindow::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

void LegalDocumentsWindow::SetDocumentText(LegalDocumentType type, const std::string& text) noexcept
{
    size_t idx = static_cast<size_t>(type);
    if (idx < documentTexts_.size())
    {
        documentTexts_[idx] = text;
    }
}

bool LegalDocumentsWindow::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    // Check tabs
    for (int i = 0; i < static_cast<int>(LegalDocumentType::Count); ++i)
    {
        LegalDocumentType tab = static_cast<LegalDocumentType>(i);
        if (IsPointInTab(x, y, tab))
        {
            OnTabClicked(tab);
            return true;
        }
    }

    // Check checkbox
    if (IsPointInCheckBox(x, y))
    {
        OnCheckBoxToggled();
        return true;
    }

    // Check OK button
    if (IsPointInOkButton(x, y))
    {
        OnOkClicked();
        return true;
    }

    // Check Cancel button
    if (IsPointInCancelButton(x, y))
    {
        OnCancelClicked();
        return true;
    }

    // Check drag
    if (IsPointInWindow(x, y) && y < windowY_ + 30) // Title bar area
    {
        isDragging_ = true;
        dragOffsetX_ = x - windowX_;
        dragOffsetY_ = y - windowY_;
        return true;
    }

    return false;
}

bool LegalDocumentsWindow::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    // Reset checkbox if clicked outside
    if (checkBox_)
    {
        // checkbox handled in HandleMouseDown
    }

    EndDrag();
    return false;
}

bool LegalDocumentsWindow::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (isDragging_)
    {
        UpdateDrag(x, y);
        return true;
    }

    return false;
}

bool LegalDocumentsWindow::HandleKeyDown(std::uint16_t key) noexcept
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
        if (checkBoxChecked_)
        {
            OnOkClicked();
        }
        return true;
    }
    if (key == VK_TAB)
    {
        // Cycle tabs
        int nextTab = (static_cast<int>(currentTab_) + 1) % static_cast<int>(LegalDocumentType::Count);
        OnTabClicked(static_cast<LegalDocumentType>(nextTab));
        return true;
    }
    if (key == VK_SPACE)
    {
        // Toggle checkbox
        OnCheckBoxToggled();
        return true;
    }

    return false;
}

bool LegalDocumentsWindow::HandleChar(wchar_t ch) noexcept
{
    return false;
}

void LegalDocumentsWindow::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    if (buttonGroup_)
    {
        buttonGroup_->Update(deltaTime);
    }
}

void LegalDocumentsWindow::Render() noexcept
{
    if (!isOpen_)
        return;

    if (!backSpk_)
        return;

    // Draw background
    backSpk_->Draw(windowX_, windowY_, 0);

    // Draw tabs
    // Tab backgrounds and text
    std::array<const char*, 3> tabNames = { "EULA", "Privacy", "Terms" };
    std::array<int, 3> tabX = { config_.tabEULAX, config_.tabPrivacyX, config_.tabTermsX };
    int tabY = windowY_ + 30;

    for (int i = 0; i < 3; ++i)
    {
        // Draw tab background (highlight if current)
        // Draw tab text
        // In legacy: g_PrintColorStr
    }

    // Draw document text area
    std::string currentText = GetCurrentDocumentText();
    // Render with word wrapping and scroll support
    // In legacy: g_PrintColorStr with scrolling

    // Draw checkbox
    if (checkBox_)
    {
        checkBox_->Render();
    }

    // Draw "I Agree" text next to checkbox
    // g_PrintColorStr(windowX_ + config_.checkBoxX + 18, windowY_ + config_.checkBoxY + 5, "I Agree", ...)

    // Draw OK/Cancel buttons
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }
}

void LegalDocumentsWindow::InitializeButtons() noexcept
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

void LegalDocumentsWindow::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(0, config_.okButtonX, config_.okButtonY);
    buttonGroup_->SetPosition(1, config_.cancelButtonX, config_.cancelButtonY);
}

void LegalDocumentsWindow::InitializeCheckBox() noexcept
{
    checkBox_ = std::make_unique<LineEditor>();
    checkBox_->SetPosition(windowX_ + config_.checkBoxX, windowY_ + config_.checkBoxY);
    checkBox_->SetByteLimit(1); // Just for checkbox state
}

bool LegalDocumentsWindow::IsPointInWindow(int x, int y) const noexcept
{
    return x >= windowX_ && x < windowX_ + windowWidth_ &&
           y >= windowY_ && y < windowY_ + windowHeight_;
}

bool LegalDocumentsWindow::IsPointInTab(int x, int y, LegalDocumentType tab) const noexcept
{
    int tabIndex = static_cast<int>(tab);
    int tabX = windowX_ + (tabIndex == 0 ? config_.tabEULAX :
                           tabIndex == 1 ? config_.tabPrivacyX : config_.tabTermsX);
    int tabY = windowY_ + 30;
    int tabWidth = 80;
    int tabHeight = 25;

    return x >= tabX && x < tabX + tabWidth &&
           y >= tabY && y < tabY + tabHeight;
}

bool LegalDocumentsWindow::IsPointInCheckBox(int x, int y) const noexcept
{
    int checkX = windowX_ + config_.checkBoxX;
    int checkY = windowY_ + config_.checkBoxY;
    int checkW = 16;
    int checkH = 16;

    return x >= checkX && x < checkX + checkW &&
           y >= checkY && y < checkY + checkH;
}

bool LegalDocumentsWindow::IsPointInOkButton(int x, int y) const noexcept
{
    int btnX = windowX_ + config_.okButtonX;
    int btnY = windowY_ + config_.okButtonY;
    int btnW = backSpk_ ? backSpk_->GetWidth(config_.okButtonFrame) : 64;
    int btnH = backSpk_ ? backSpk_->GetHeight(config_.okButtonFrame) : 24;

    return x >= btnX && x < btnX + btnW &&
           y >= btnY && y < btnY + btnH;
}

bool LegalDocumentsWindow::IsPointInCancelButton(int x, int y) const noexcept
{
    int btnX = windowX_ + config_.cancelButtonX;
    int btnY = windowY_ + config_.cancelButtonY;
    int btnW = backSpk_ ? backSpk_->GetWidth(config_.cancelButtonFrame) : 64;
    int btnH = backSpk_ ? backSpk_->GetHeight(config_.cancelButtonFrame) : 24;

    return x >= btnX && x < btnX + btnW &&
           y >= btnY && y < btnY + btnH;
}

void LegalDocumentsWindow::UpdateDrag(int x, int y) noexcept
{
    if (isDragging_)
    {
        windowX_ = x - dragOffsetX_;
        windowY_ = y - dragOffsetY_;
        PositionButtons();
    }
}

void LegalDocumentsWindow::EndDrag() noexcept
{
    isDragging_ = false;
}

std::string LegalDocumentsWindow::GetCurrentDocumentText() const noexcept
{
    size_t idx = static_cast<size_t>(currentTab_);
    if (idx < documentTexts_.size())
    {
        return documentTexts_[idx];
    }
    return "";
}

void LegalDocumentsWindow::OnOkClicked() noexcept
{
    if (checkBoxChecked_)
    {
        if (acceptCallback_)
        {
            acceptCallback_(currentTab_);
        }
        Close();
    }
}

void LegalDocumentsWindow::OnCancelClicked() noexcept
{
    Close();
}

void LegalDocumentsWindow::OnCheckBoxToggled() noexcept
{
    checkBoxChecked_ = !checkBoxChecked_;
    // OK button enabled only when checked
}

void LegalDocumentsWindow::OnTabClicked(LegalDocumentType tab) noexcept
{
    currentTab_ = tab;
}

} // namespace phoenix::ui