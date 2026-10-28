#include "UI/Windows/OustersSkillInfoWindow.h"

#include "Settings/InterfaceProfileLoader.h"
#include "Protocol/LegacyProtocol.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

OustersSkillInfoWindow::OustersSkillInfoWindow() noexcept
    : isOpen_(false)
    , skillID_(0)
    , downSkill_(false)
    , windowX_(100)
    , windowY_(100)
    , windowWidth_(300)
    , windowHeight_(350)
    , isDragging_(false)
    , dragOffsetX_(0)
    , dragOffsetY_(0)
    , descYDistance_(5)
{
}

void OustersSkillInfoWindow::Open(std::uint32_t skillID, std::int32_t windowX, std::int32_t windowY, bool downSkill) noexcept
{
    isOpen_ = true;
    skillID_ = skillID;
    downSkill_ = downSkill;
    windowX_ = windowX;
    windowY_ = windowY;
    descYDistance_ = 5;

    // Initialize SPK archives
    backSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.backSpkName.empty())
    {
        backSpk_->Open(config_.backSpkName.c_str());
    }

    assembleBoxButtonSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.assembleBoxButtonSpkName.empty())
    {
        assembleBoxButtonSpk_->Open(config_.assembleBoxButtonSpkName.c_str());
    }

    // Window size from config
    windowWidth_ = config_.windowWidth;
    windowHeight_ = config_.windowHeight;

    InitializeButtons();
    PositionButtons();
    InitializeScrollBar();

    // In legacy: g_CurrentSkillID = 0xFFFFFFFF;
}

void OustersSkillInfoWindow::Close() noexcept
{
    isOpen_ = false;
    isDragging_ = false;
    backSpk_.reset();
    assembleBoxButtonSpk_.reset();
    buttonGroup_.reset();
    scrollBar_.reset();
}

void OustersSkillInfoWindow::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open(0, windowX_, windowY_, downSkill_);
}

void OustersSkillInfoWindow::LoadFromConfig(const settings::OustersSkillInfoProfile& profile) noexcept
{
    config_ = profile;
    windowX_ = config_.windowX;
    windowY_ = config_.windowY;
    windowWidth_ = config_.windowWidth;
    windowHeight_ = config_.windowHeight;
}

void OustersSkillInfoWindow::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

bool OustersSkillInfoWindow::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    // Check buttons
    if (buttonGroup_)
    {
        if (buttonGroup_->HandleMouseDown(x, y))
            return true;
    }

    // Check scroll bar
    if (scrollBar_)
    {
        if (scrollBar_->HitTest(x, y))
        {
            scrollBar_->HandleMouseDown(x, y);
            return true;
        }
    }

    // Check title bar for dragging
    if (IsPointInTitleBar(x, y))
    {
        isDragging_ = true;
        dragOffsetX_ = x - windowX_;
        dragOffsetY_ = y - windowY_;
        return true;
    }

    return false;
}

bool OustersSkillInfoWindow::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    if (scrollBar_)
    {
        scrollBar_->HandleMouseUp(x, y);
    }

    EndDrag();
    return false;
}

bool OustersSkillInfoWindow::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    if (scrollBar_)
    {
        scrollBar_->HandleMouseMove(x, y);
    }

    if (isDragging_)
    {
        UpdateDrag(x, y);
    }

    return false;
}

bool OustersSkillInfoWindow::HandleMouseWheel(int delta) noexcept
{
    if (!isOpen_)
        return false;

    if (scrollBar_)
    {
        if (delta > 0)
            scrollBar_->ScrollUp();
        else
            scrollBar_->ScrollDown();
        return true;
    }

    return false;
}

bool OustersSkillInfoWindow::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    if (key == VK_ESCAPE)
    {
        OnCloseClicked();
        return true;
    }

    return false;
}

void OustersSkillInfoWindow::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    if (buttonGroup_)
    {
        buttonGroup_->Update(deltaTime);
    }

    if (scrollBar_)
    {
        scrollBar_->Update(deltaTime);
    }
}

void OustersSkillInfoWindow::Render() noexcept
{
    if (!isOpen_)
        return;

    if (!backSpk_)
        return;

    // Draw background dialog
    backSpk_->Draw(windowX_, windowY_, 0); // Main dialog frame

    // Draw skill icon background
    if (IsSkillLearned())
    {
        backSpk_->Draw(windowX_ + 43, windowY_ + 173, config_.oustersSkillBack2Frame);
    }
    else
    {
        backSpk_->Draw(windowX_ + 43, windowY_ + 173, config_.oustersSkillBackFrame);
    }

    // Draw out boxes
    // In legacy: gpC_global_resource->DrawOutBoxLocked(rect)
    // We'll use SPK frames if available

    // Draw buttons
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }

    // Draw scroll bar
    if (scrollBar_)
    {
        scrollBar_->Render();
    }

    // Draw skill name
    // In legacy: g_PrintColorStr(x+30, y+35, sInfo.GetName(), ...)
    // Would need font system

    // Draw description
    ShowDesc(windowX_, windowY_ + 20);
}

void OustersSkillInfoWindow::InitializeButtons() noexcept
{
    buttonGroup_ = std::make_unique<ButtonGroup>();

    // Close button
    buttonGroup_->AddButton(static_cast<int>(0), // CLOSE_ID
        config_.closeButtonX, config_.closeButtonY,
        assembleBoxButtonSpk_ ? assembleBoxButtonSpk_->GetWidth(config_.closeButtonFrame) : 16,
        assembleBoxButtonSpk_ ? assembleBoxButtonSpk_->GetHeight(config_.closeButtonFrame) : 16,
        [this]() { OnCloseClicked(); });

    // Help button
    buttonGroup_->AddButton(static_cast<int>(1), // HELP_ID
        config_.helpButtonX, config_.helpButtonY,
        assembleBoxButtonSpk_ ? assembleBoxButtonSpk_->GetWidth(config_.helpButtonFrame) : 16,
        assembleBoxButtonSpk_ ? assembleBoxButtonSpk_->GetHeight(config_.helpButtonFrame) : 16,
        [this]() { OnHelpClicked(); });

    // Alpha button (commented out in legacy)
    // buttonGroup_->AddButton(static_cast<int>(2), // ALPHA_ID
    //     config_.alphaButtonX, config_.alphaButtonY,
    //     assembleBoxButtonSpk_ ? assembleBoxButtonSpk_->GetWidth(config_.alphaButtonFrame) : 16,
    //     assembleBoxButtonSpk_ ? assembleBoxButtonSpk_->GetHeight(config_.alphaButtonFrame) : 16,
    //     [this]() { OnAlphaClicked(); });

    // Learn/DownSkill button
    int learnFrame = downSkill_ ? config_.oustersDownSkillButtonFrame : config_.learnButtonFrame;
    buttonGroup_->AddButton(static_cast<int>(3), // LEARN_ID
        config_.learnButtonX, config_.learnButtonY,
        backSpk_ ? backSpk_->GetWidth(learnFrame) : 16,
        backSpk_ ? backSpk_->GetHeight(learnFrame) : 16,
        [this]() { OnLearnClicked(); });
}

void OustersSkillInfoWindow::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(0, config_.closeButtonX, config_.closeButtonY);
    buttonGroup_->SetPosition(1, config_.helpButtonX, config_.helpButtonY);
    // buttonGroup_->SetPosition(2, config_.alphaButtonX, config_.alphaButtonY);
    buttonGroup_->SetPosition(3, config_.learnButtonX, config_.learnButtonY);
}

void OustersSkillInfoWindow::InitializeScrollBar() noexcept
{
    scrollBar_ = std::make_unique<ScrollBar>(
        windowX_ + config_.scrollBarX,
        windowY_ + config_.scrollBarY,
        config_.scrollBarWidth,
        config_.scrollBarHeight,
        ScrollBar::Orientation::Vertical
    );
    scrollBar_->SetPosMax(100); // Would be set based on description length
}

bool OustersSkillInfoWindow::IsPointInWindow(int x, int y) const noexcept
{
    return x >= windowX_ && x < windowX_ + windowWidth_ &&
           y >= windowY_ && y < windowY_ + windowHeight_;
}

bool OustersSkillInfoWindow::IsPointInTitleBar(int x, int y) const noexcept
{
    return x >= windowX_ && x < windowX_ + windowWidth_ &&
           y >= windowY_ && y < windowY_ + 30; // Title bar height
}

void OustersSkillInfoWindow::UpdateDrag(int x, int y) noexcept
{
    if (isDragging_)
    {
        windowX_ = x - dragOffsetX_;
        windowY_ = y - dragOffsetY_;
        PositionButtons();
        if (scrollBar_)
        {
            scrollBar_->SetPosition(windowX_ + config_.scrollBarX, windowY_ + config_.scrollBarY);
        }
    }
}

void OustersSkillInfoWindow::EndDrag() noexcept
{
    isDragging_ = false;
}

void OustersSkillInfoWindow::ShowDesc(int x, int y) noexcept
{
    // In legacy: Draws skill description with elemental requirements, etc.
    // We would use the font system to render:
    // - Skill level
    // - Elemental requirements
    // - Can delete / Cannot delete
    // - Passive indicator
    // - Required skill points
    // - Description text
    // With scrolling support via descYDistance_
    (void)x; (void)y;
}

void OustersSkillInfoWindow::SetDescScrollPos(int pos) noexcept
{
    if (scrollBar_)
    {
        scrollBar_->SetScrollPos(pos);
    }
}

bool OustersSkillInfoWindow::IsSkillLearned() const noexcept
{
    // Would query skill manager
    return false;
}

bool OustersSkillInfoWindow::CanLearnSkill() const noexcept
{
    // Would check skill points, prerequisites, level
    return false;
}

bool OustersSkillInfoWindow::CanDownSkill() const noexcept
{
    // Would check if skill can be downgraded
    return false;
}

std::string OustersSkillInfoWindow::GetSkillName() const noexcept
{
    // Would query skill info table
    return "Skill " + std::to_string(skillID_);
}

std::string OustersSkillInfoWindow::GetSkillDescription() const noexcept
{
    return "Description for skill " + std::to_string(skillID_);
}

int OustersSkillInfoWindow::GetSkillLevel() const noexcept
{
    return 1;
}

int OustersSkillInfoWindow::GetRequiredSkillPoints() const noexcept
{
    return 1;
}

bool OustersSkillInfoWindow::IsPassive() const noexcept
{
    return false;
}

void OustersSkillInfoWindow::OnCloseClicked() noexcept
{
    // gpC_base->SendMessage(UI_CLOSE_OUSTERS_SKILL_INFO);
    Close();
}

void OustersSkillInfoWindow::OnHelpClicked() noexcept
{
    // gC_vs_ui.RunDescDialog(DID_HELP, (void *)C_VS_UI_DESC_DIALOG::OUSTERS_SKILL_INFO);
}

void OustersSkillInfoWindow::OnAlphaClicked() noexcept
{
    // AttrAlpha(!GetAttributes()->alpha);
    // EMPTY_MOVE;
}

void OustersSkillInfoWindow::OnLearnClicked() noexcept
{
    if (downSkill_)
    {
        // Down skill logic
        if (CanDownSkill())
        {
            // gpC_base->SendMessage(UI_OUSTERS_DOWN_SKILL, m_skillID);
        }
    }
    else
    {
        // Learn skill logic
        if (CanLearnSkill())
        {
            // gpC_base->SendMessage(UI_OUSTERS_LEARN_SKILL, m_skillID);
        }
    }
}

} // namespace phoenix::ui