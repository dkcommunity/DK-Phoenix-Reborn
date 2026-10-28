#include "UI/Windows/ItemListWindow.h"

#include "Settings/InterfaceProfileLoader.h"
#include "Protocol/LegacyProtocol.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace phoenix::ui
{

ItemListWindow::ItemListWindow() noexcept
    : isOpen_(false)
    , draggable_(true)
    , isDragging_(false)
    , dragOffsetX_(0)
    , dragOffsetY_(0)
    , windowX_(0)
    , windowY_(0)
    , windowWidth_(450)
    , windowHeight_(400)
    , currentItemIndex_(-1)
    , mouseFocusIndex_(-1)
    , scrollPos_(0)
    , maxScrollPos_(0)
    , visibleLines_(8)
    , lineGap_(20)
{
}

void ItemListWindow::Open() noexcept
{
    isOpen_ = true;
    scrollPos_ = 0;
    currentItemIndex_ = -1;
    mouseFocusIndex_ = -1;
}

void ItemListWindow::Close() noexcept
{
    isOpen_ = false;
}

void ItemListWindow::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open();
}

void ItemListWindow::LoadFromConfig(const ItemListProfile& profile) noexcept
{
    config_ = profile;
    windowX_ = profile.windowX;
    windowY_ = profile.windowY;
    windowWidth_ = profile.windowWidth;
    windowHeight_ = profile.windowHeight;
    draggable_ = profile.draggable;
    visibleLines_ = profile.visibleLines;
    lineGap_ = profile.lineGap;

    InitializeButtons();
    PositionButtons();
}

void ItemListWindow::ReloadConfig() noexcept
{
    // Reload from interface.ini
    // TODO: Implement ini reload
}

bool ItemListWindow::AddItem(std::unique_ptr<MItem> item, std::int32_t timeLimit) noexcept
{
    if (!item)
        return false;

    items_.push_back(std::move(item));
    itemSelected_.push_back(false);
    itemTimeLimits_.push_back(0);

    // Update scroll bar max position
    UpdateScrollBar();
    return true;
}

bool ItemListWindow::RemoveItem(std::uint32_t itemID) noexcept
{
    for (std::size_t i = 0; i < items_.size(); ++i)
    {
        if (items_[i] && items_[i]->GetID() == itemID)
        {
            items_.erase(items_.begin() + i);
            itemSelected_.erase(itemSelected_.begin() + i);
            itemTimeLimits_.erase(itemTimeLimits_.begin() + i);
            UpdateScrollBar();
            return true;
        }
    }
    return false;
}

void ItemListWindow::ClearItems() noexcept
{
    items_.clear();
    itemSelected_.clear();
    itemTimeLimits_.clear();
    currentItemIndex_ = -1;
    mouseFocusIndex_ = -1;
    scrollPos_ = 0;
    UpdateScrollBar();
}

void ItemListWindow::SetCurrentItemIndex(std::int32_t index) noexcept
{
    if (index >= -1 && index < static_cast<std::int32_t>(items_.size()))
    {
        currentItemIndex_ = index;
    }
}

const MItem* ItemListWindow::GetCurrentItem() const noexcept
{
    if (currentItemIndex_ >= 0 && currentItemIndex_ < static_cast<std::int32_t>(items_.size()))
    {
        return items_[currentItemIndex_].get();
    }
    return nullptr;
}

bool ItemListWindow::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    // Check if point is in window
    if (!IsPointInWindow(x, y))
        return false;

    // Check scroll bar first
    if (scrollBar_ && scrollBar_->HitTest(x, y))
    {
        scrollBar_->HandleMouseDown(x, y);
        return true;
    }

    // Check buttons
    if (buttonGroup_)
    {
        if (buttonGroup_->HandleMouseDown(x, y))
            return true;
    }

    // Check item list area
    if (IsPointInItemList(x, y))
    {
        int relX = x - (windowX_ + config_.itemListX);
        int relY = y - (windowY_ + config_.itemListY);

        int select = relY / lineGap_;
        if (select >= 0 && select < visibleLines_)
        {
            int itemIndex = select + scrollPos_;
            if (itemIndex >= 0 && itemIndex < static_cast<std::int32_t>(items_.size()))
            {
                currentItemIndex_ = itemIndex;
                return true;
            }
        }
    }

    // Check drag
    if (draggable_ && IsPointInWindow(x, y))
    {
        BeginDrag(x, y);
        return true;
    }

    return false;
}

bool ItemListWindow::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (scrollBar_)
    {
        scrollBar_->HandleMouseUp(x, y);
    }

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    EndDrag();
    return false;
}

bool ItemListWindow::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (scrollBar_)
    {
        scrollBar_->HandleMouseMove(x, y);
    }

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    // Update mouse focus for hover highlight
    if (IsPointInItemList(x, y))
    {
        int relY = y - (windowY_ + config_.itemListY);
        int select = (y - (windowY_ + config_.itemListY)) / lineGap_;
        if (select >= 0 && select < visibleLines_)
        {
            int itemIndex = select + scrollPos_;
            if (itemIndex >= 0 && itemIndex < static_cast<std::int32_t>(items_.size()))
            {
                mouseFocusIndex_ = itemIndex;
            }
            else
            {
                mouseFocusIndex_ = -1;
            }
        }
        else
        {
            mouseFocusIndex_ = -1;
        }
    }
    else
    {
        mouseFocusIndex_ = -1;
    }

    // Handle drag
    if (isDragging_)
    {
        UpdateDrag(x, y);
    }

    return false;
}

bool ItemListWindow::HandleMouseWheel(int delta) noexcept
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

bool ItemListWindow::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    switch (key)
    {
        case VK_ESCAPE:
            Close();
            return true;
        case VK_UP:
            if (currentItemIndex_ > 0)
            {
                SetCurrentItemIndex(currentItemIndex_ - 1);
                // Scroll if needed
                if (currentItemIndex_ < scrollPos_)
                    scrollPos_ = currentItemIndex_;
                return true;
            }
            break;
        case VK_DOWN:
            if (currentItemIndex_ + 1 < static_cast<std::int32_t>(items_.size()))
            {
                SetCurrentItemIndex(currentItemIndex_ + 1);
                // Scroll if needed
                if (currentItemIndex_ >= scrollPos_ + visibleLines_)
                    scrollPos_ = currentItemIndex_ - visibleLines_ + 1;
                return true;
            }
            break;
        case VK_RETURN:
            // Take out item or use
            if (currentItemIndex_ >= 0 && currentItemIndex_ < static_cast<std::int32_t>(items_.size()))
            {
                // Trigger take-out confirmation
                // g_StartTakeOutConfirmDialog(...)
            }
            return true;
    }
    return false;
}

void ItemListWindow::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    if (buttonGroup_)
        buttonGroup_->Update(deltaTime);
}

void ItemListWindow::Render() noexcept
{
    if (!isOpen_)
        return;

    // Draw window background
    // Draw item list background
    // Draw items
    // Draw scroll bar
    // Draw buttons
}

bool ItemListWindow::BeginDrag(int x, int y) noexcept
{
    if (!draggable_)
        return false;

    if (IsPointInWindow(x, y) && !IsPointInItemList(x, y))
    {
        isDragging_ = true;
        dragOffsetX_ = x - windowX_;
        dragOffsetY_ = y - windowY_;
        return true;
    }
    return false;
}

void ItemListWindow::UpdateDrag(int x, int y) noexcept
{
    if (isDragging_)
    {
        windowX_ = x - dragOffsetX_;
        windowY_ = y - dragOffsetY_;
        // Clamp to screen bounds
    }
}

void ItemListWindow::EndDrag() noexcept
{
    isDragging_ = false;
}

void ItemListWindow::InitializeButtons() noexcept
{
    // OK button (take item)
    // Cancel button (close)
    // TODO: Implement ButtonGroup with OK/Cancel buttons
}

void ItemListWindow::PositionButtons() noexcept
{
    // Position OK and Cancel buttons at bottom of window
}

std::string ItemListWindow::GetItemName(std::size_t index) const noexcept
{
    if (index >= items_.size() || !items_[index])
        return "";

    // Legacy: GetItemName from MItem
    // Format: [Option1] [Option2] ItemName
    std::string name;
    // TODO: Implement using MItem methods
    return name;
}

std::string ItemListWindow::GetItemTimeString(std::size_t index) const noexcept
{
    if (index >= itemTimeLimits_.size())
        return "";

    std::int32_t timeLimit = itemTimeLimits_[index];
    if (timeLimit <= 0)
        return "";

    int days = timeLimit / 86400;
    int hours = (timeLimit % 86400) / 3600;
    int minutes = (timeLimit % 3600) / 60;
    int seconds = timeLimit % 60;

    std::string timeStr;
    bool hasValue = false;

    if (days > 0)
    {
        timeStr += std::to_string(days) + "d ";
        hasValue = true;
    }
    if (hours > 0 || hasValue)
    {
        timeStr += std::to_string(hours) + "h ";
        hasValue = true;
    }
    if (minutes > 0 || hasValue)
    {
        timeStr += std::to_string(minutes) + "m ";
        hasValue = true;
    }
    if (seconds > 0 || hasValue)
    {
        timeStr += std::to_string(seconds) + "s";
    }

    return timeStr;
}

void ItemListWindow::ClampScrollPos() noexcept
{
    if (scrollPos_ < 0)
        scrollPos_ = 0;
    if (scrollPos_ > maxScrollPos_)
        scrollPos_ = maxScrollPos_;
}

bool ItemListWindow::IsPointInItemList(int x, int y) const noexcept
{
    int listX = windowX_ + config_.itemListX;
    int listY = windowY_ + config_.itemListY;
    int listW = config_.itemListWidth;
    int listH = config_.itemListHeight;

    return x >= listX && x < listX + listW && y >= listY && y < listY + listH;
}

bool ItemListWindow::IsPointInWindow(int x, int y) const noexcept
{
    return x >= windowX_ && x < windowX_ + windowWidth_ &&
           y >= windowY_ && y < windowY_ + windowHeight_;
}

void ItemListWindow::UpdateScrollBar() noexcept
{
    if (items_.size() > visibleLines_)
        maxScrollPos_ = static_cast<std::int32_t>(items_.size()) - visibleLines_;
    else
        maxScrollPos_ = 0;

    if (scrollPos_ > maxScrollPos_)
        scrollPos_ = maxScrollPos_;
}

} // namespace phoenix::ui