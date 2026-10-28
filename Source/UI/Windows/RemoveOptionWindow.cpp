#include "UI/Windows/RemoveOptionWindow.h"

#include "Settings/InterfaceProfileLoader.h"
#include "Protocol/LegacyProtocol.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

RemoveOptionWindow::RemoveOptionWindow() noexcept
    : isOpen_(false)
    , status_(RemoveOptionStatus::SelectOption)
    , selectedOption_(-1)
    , isPressed_(false)
    , isBokjory_(false)
    , mouseItem_(nullptr)
    , curItem_(nullptr)
    , windowX_(0)
    , windowY_(0)
    , windowWidth_(340)
    , windowHeight_(180)
{
    optionText_[0].clear();
    optionText_[1].clear();
}

void RemoveOptionWindow::Open(const MItem* pItem, const MItem* pCurItem) noexcept
{
    if (!pItem || !pCurItem)
        return;

    isOpen_ = true;
    status_ = RemoveOptionStatus::SelectOption;
    selectedOption_ = -1;
    isPressed_ = false;

    mouseItem_ = pItem;
    curItem_ = pCurItem;

    // Check if bokjory
    isBokjory_ = false;
    if (pItem->GetItemClass() == static_cast<std::uint16_t>(ItemClass::MixingItem) &&
        (pItem->GetItemType() == 18 || pItem->GetItemType() == 25))
    {
        isBokjory_ = true;
    }

    // Initialize SPK
    backSpk_ = std::make_unique<SpriteArchive>();
    backSpk_->Open(config_.backSpkName.c_str());

    // Window position (centered)
    // In legacy: Set(g_pUserInformation->iResolution_x/2-window_w/2, g_pUserInformation->iResolution_y/2 - window_h/2, window_w, window_h);
    // We'll use config values

    UpdateOptionText();
    InitializeButtons();
    PositionButtons();
}

void RemoveOptionWindow::Close() noexcept
{
    isOpen_ = false;
    backSpk_.reset();
    buttonGroup_.reset();
    mouseItem_ = nullptr;
    curItem_ = nullptr;
}

void RemoveOptionWindow::Toggle() noexcept
{
    if (isOpen_)
        Close();
}

void RemoveOptionWindow::LoadFromConfig(const settings::RemoveOptionProfile& profile) noexcept
{
    config_ = profile;
    windowX_ = config_.windowX;
    windowY_ = config_.windowY;
    windowWidth_ = config_.windowWidth;
    windowHeight_ = config_.windowHeight;
}

void RemoveOptionWindow::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

bool RemoveOptionWindow::HandleMouseDown(int x, int y, bool rightButton) noexcept
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

    // Check option selection area
    if (status_ == RemoveOptionStatus::SelectOption)
    {
        for (int i = 0; i < 2; ++i)
        {
            if (optionText_[i].empty())
                continue;

            if (IsPointInOption(i, x, y))
            {
                selectedOption_ = i;
                return true;
            }
        }
    }

    return false;
}

bool RemoveOptionWindow::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    isPressed_ = false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    if (status_ == RemoveOptionStatus::SelectOption && selectedOption_ >= 0)
    {
        status_ = RemoveOptionStatus::Question;
    }

    return false;
}

bool RemoveOptionWindow::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    if (status_ == RemoveOptionStatus::SelectOption)
    {
        selectedOption_ = -1;
        for (int i = 0; i < 2; ++i)
        {
            if (optionText_[i].empty())
                continue;

            if (IsPointInOption(i, x, y))
            {
                selectedOption_ = i;
                break;
            }
        }
    }

    return false;
}

bool RemoveOptionWindow::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    if (key == VK_ESCAPE)
    {
        OnCancelClicked();
        return true;
    }

    return false;
}

void RemoveOptionWindow::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    if (buttonGroup_)
    {
        buttonGroup_->Update(deltaTime);
    }
}

void RemoveOptionWindow::Render() noexcept
{
    if (!isOpen_)
        return;

    if (!backSpk_)
        return;

    // Draw dialog background
    backSpk_->Draw(windowX_, windowY_, 0); // Main dialog frame

    // Draw buttons
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }

    // Draw text based on status
    // In legacy: g_FL2_GetDC() then g_PrintColorStrOut, g_PrintColorStr
    // We'll skip text rendering for now as it requires font system
}

void RemoveOptionWindow::InitializeButtons() noexcept
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

void RemoveOptionWindow::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(0, config_.okButtonX, config_.okButtonY);
    buttonGroup_->SetPosition(1, config_.cancelButtonX, config_.cancelButtonY);
}

void RemoveOptionWindow::UpdateOptionText() noexcept
{
    if (!curItem_)
        return;

    const auto& optionList = curItem_->GetItemOptionList();
    auto itr = optionList.begin();

    for (int i = 0; i < 2 && itr != optionList.end(); ++itr, ++i)
    {
        if (*itr > 1000 || i >= 2) // g_pItemOptionTable->GetSize() approximated
            break;

        // ITEMOPTION_INFO& optionInfo = (*g_pItemOptionTable)[*itr];
        // m_str_option[i] = g_pItemOptionTable->ITEMOPTION_PARTNAME[optionInfo.Part].GetString();
        
        // For now, use placeholder text
        optionText_[i] = "Option " + std::to_string(i + 1);
        
        // char temp[256] = {0,};
        // int Point = optionInfo.PlusPoint - 100;
        // 
        // if (optionInfo.Part == ITEMOPTION_TABLE::PART_DURABILITY)
        // {
        //     if (Point > 0) wsprintf(temp, " +%d", Point);
        //     else wsprintf(temp, " %d", Point);
        // }
        // else if (optionInfo.Part >= ITEMOPTION_TABLE::PART_STR_TO_DEX && 
        //          optionInfo.Part <= ITEMOPTION_TABLE::PART_INT_TO_DEX)
        // {
        //     if (Point > 0) wsprintf(temp, " +%d%%", Point);
        //     else wsprintf(temp, " %d%%", Point);
        // }
        // else
        // {
        //     if (Point + 100 > 0) wsprintf(temp, " +%d", Point + 100);
        //     else wsprintf(temp, " %d", Point + 100);
        // }
        // m_str_option[i] += temp;
    }
}

bool RemoveOptionWindow::IsPointInWindow(int x, int y) const noexcept
{
    return x >= windowX_ && x < windowX_ + windowWidth_ &&
           y >= windowY_ && y < windowY_ + windowHeight_;
}

bool RemoveOptionWindow::IsPointInOption(int optionIndex, int x, int y) const noexcept
{
    int rectLeft = windowX_ + windowWidth_ / 2 - (windowWidth_ - 60) / 2;
    int rectTop = windowY_ + config_.fieldY + optionIndex * config_.gapY;
    int rectRight = rectLeft + windowWidth_ - 60;
    int rectBottom = rectTop + 23; // 23 height from legacy

    return x >= rectLeft && x < rectRight && y >= rectTop && y < rectBottom;
}

void RemoveOptionWindow::OnOkClicked() noexcept
{
    if (status_ == RemoveOptionStatus::Question && selectedOption_ >= 0 && selectedOption_ <= 1)
    {
        // Send UI_SEND_REMOVE_OPTION packet
        // gpC_base->SendMessage(UI_SEND_REMOVE_OPTION, m_select_option, m_pCurItem->GetID(), (void*)m_pMouseItem);
    }
    Close();
}

void RemoveOptionWindow::OnCancelClicked() noexcept
{
    // Send UI_CLOSE_REMOVE_OPTION
    // gpC_base->SendMessage(UI_CLOSE_REMOVE_OPTION);
    Close();
}

const std::string& RemoveOptionWindow::GetOptionText(int index) const noexcept
{
    static const std::string empty;
    if (index >= 0 && index < 2)
        return optionText_[index];
    return empty;
}

} // namespace phoenix::ui