#include "UI/Windows/MixingForgeWindow.h"

#include "Settings/InterfaceProfileLoader.h"
#include "Protocol/LegacyProtocol.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

MixingForgeWindow::MixingForgeWindow() noexcept
    : isOpen_(false)
    , isDragging_(false)
    , dragOffsetX_(0)
    , dragOffsetY_(0)
    , forgeClass_(ForgeClass::Weapon)
    , forgeType_(ForgeType::TypeA)
    , focusGridX_(-1)
    , focusGridY_(-1)
    , windowX_(100)
    , windowY_(150)
    , windowWidth_(400)
    , windowHeight_(300)
{
    // Initialize selected items
    selectedItems_[0].reset();
    selectedItems_[1].reset();
    itemCheck_[0] = false;
    itemCheck_[1] = false;
}

void MixingForgeWindow::Open(ForgeClass forgeClass, ForgeType forgeType) noexcept
{
    isOpen_ = true;
    forgeClass_ = forgeClass;
    forgeType_ = forgeType;
    focusGridX_ = -1;
    focusGridY_ = -1;

    // Initialize SPK archives
    backSpk_ = std::make_unique<SpriteArchive>();
    backSpk_->Open(config_.backSpkName.c_str());

    inventorySpk_ = std::make_unique<SpriteArchive>();
    UpdateRaceLayout();

    // Window size from SPK
    if (backSpk_)
    {
        windowWidth_ = backSpk_->GetWidth(config_.mainWindowFrame);
        windowHeight_ = backSpk_->GetHeight(config_.mainWindowFrame);
    }

    // Initialize grid
    myGridRect_.x = windowX_ + GetInventoryX() + config_.slayerGridX;
    myGridRect_.y = windowY_ + GetInventoryY() + config_.slayerGridY;
    myGridRect_.w = config_.gridX * GetGridUnitX();
    myGridRect_.h = config_.gridY * GetGridUnitY();

    InitializeButtons();
    PositionButtons();

    // Initialize check vector (one per inventory item)
    checkVector_.clear();
}

void MixingForgeWindow::Close() noexcept
{
    isOpen_ = false;
    isDragging_ = false;
    backSpk_.reset();
    inventorySpk_.reset();
    buttonGroup_.reset();
    ClearSelection();
}

void MixingForgeWindow::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open(forgeClass_, forgeType_);
}

void MixingForgeWindow::LoadFromConfig(const settings::MixingForgeProfile& profile) noexcept
{
    config_ = profile;
    windowX_ = config_.windowX;
    windowY_ = config_.windowY;
}

void MixingForgeWindow::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

bool MixingForgeWindow::SelectItem(std::uint32_t itemID, int slotIndex) noexcept
{
    if (slotIndex < 0 || slotIndex > 1)
        return false;

    // In real implementation, we would look up the item from inventory
    // For now, we just mark the slot as selected
    itemCheck_[slotIndex] = true;
    return true;
}

bool MixingForgeWindow::DeselectItem(int slotIndex) noexcept
{
    if (slotIndex < 0 || slotIndex > 1)
        return false;

    itemCheck_[slotIndex] = false;
    return true;
}

void MixingForgeWindow::ClearSelection() noexcept
{
    selectedItems_[0].reset();
    selectedItems_[1].reset();
    itemCheck_[0] = false;
    itemCheck_[1] = false;
}

void MixingForgeWindow::SetFocusGrid(int gridX, int gridY) noexcept
{
    focusGridX_ = gridX;
    focusGridY_ = gridY;
}

bool MixingForgeWindow::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    // Check buttons
    if (buttonGroup_)
    {
        if (buttonGroup_->HandleMouseDown(x, y))
            return true;
    }

    // Check slots
    for (int i = 0; i < 2; ++i)
    {
        if (IsPointInSlot(i, x, y))
        {
            // Click on slot - could select/deselect
            return true;
        }
    }

    // Check grid
    if (IsPointInMyGrid(x, y))
    {
        if (rightButton)
        {
            HandleRightButtonDown(x, y);
        }
        return true;
    }

    // Check drag
    if (IsPointInWindow(x, y))
    {
        isDragging_ = true;
        dragOffsetX_ = x - windowX_;
        dragOffsetY_ = y - windowY_;
        return true;
    }

    return false;
}

bool MixingForgeWindow::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    isDragging_ = false;
    return false;
}

bool MixingForgeWindow::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    if (isDragging_)
    {
        windowX_ = x - dragOffsetX_;
        windowY_ = y - dragOffsetY_;
        PositionButtons();
        return true;
    }

    // Update grid focus
    if (IsPointInMyGrid(x, y))
    {
        int gridUnitX = GetGridUnitX();
        int gridUnitY = GetGridUnitY();
        focusGridX_ = (x - myGridRect_.x) / gridUnitX;
        focusGridY_ = (y - myGridRect_.y) / gridUnitY;
    }
    else
    {
        focusGridX_ = -1;
        focusGridY_ = -1;
    }

    return false;
}

bool MixingForgeWindow::HandleKeyDown(std::uint16_t key) noexcept
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
        if (HasValidSelection())
        {
            OnOkClicked();
        }
        return true;
    }

    return false;
}

bool MixingForgeWindow::HandleRightButtonDown(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    // Check if right-click on selected item in grid to deselect
    if (IsPointInMyGrid(x, y))
    {
        // In legacy: Check() function handles right-click on items
        // We would need to check if the item at this grid position is selected
    }

    return true;
}

void MixingForgeWindow::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    if (buttonGroup_)
    {
        buttonGroup_->Update(deltaTime);
    }
}

void MixingForgeWindow::Render() noexcept
{
    if (!isOpen_)
        return;

    if (!backSpk_)
        return;

    // Draw background
    backSpk_->Draw(windowX_, windowY_, config_.mainWindowFrame);

    // Draw inventory background
    if (inventorySpk_)
    {
        inventorySpk_->Draw(GetInventoryX(), GetInventoryY(), config_.inventoryWindowFrame);

        // Vampire has bottom frame too
        // if (race_ == PcType::Vampire)
        // {
        //     inventorySpk_->Draw(GetInventoryX(), GetInventoryY() + inventorySpk_->GetHeight(config_.inventoryWindowFrame), config_.inventoryBottomFrame);
        // }
    }

    // Draw selected items in slots
    // Draw grid items from inventory
    // Draw check marks on selected items

    // Draw buttons
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }
}

bool MixingForgeWindow::HasValidSelection() const noexcept
{
    return itemCheck_[0] && itemCheck_[1];
}

std::pair<std::uint32_t, std::uint32_t> MixingForgeWindow::GetSelectedItems() const noexcept
{
    return { 0, 0 }; // Would return actual item IDs
}

void MixingForgeWindow::InitializeButtons() noexcept
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

void MixingForgeWindow::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(0, config_.okButtonX, config_.okButtonY);
    buttonGroup_->SetPosition(1, config_.cancelButtonX, config_.cancelButtonY);
}

void MixingForgeWindow::UpdateGridPosition() noexcept
{
    myGridRect_.x = windowX_ + GetInventoryX() + config_.slayerGridX;
    myGridRect_.y = windowY_ + GetInventoryY() + config_.slayerGridY;
    myGridRect_.w = config_.gridX * GetGridUnitX();
    myGridRect_.h = config_.gridY * GetGridUnitY();
}

bool MixingForgeWindow::IsPointInWindow(int x, int y) const noexcept
{
    return x >= windowX_ && x < windowX_ + windowWidth_ &&
           y >= windowY_ && y < windowY_ + windowHeight_;
}

bool MixingForgeWindow::IsPointInSlot(int slotIndex, int x, int y) const noexcept
{
    int slotX = (slotIndex == 0) ? config_.slot1X : config_.slot2X;
    int slotY = (slotIndex == 0) ? config_.slot1Y : config_.slot2Y;

    int left = windowX_ + slotX;
    int top = windowY_ + slotY;
    int right = left + 30;  // slot width
    int bottom = top + 30;  // slot height

    return x >= left && x < right && y >= top && y < bottom;
}

bool MixingForgeWindow::IsPointInGrid(int x, int y) const noexcept
{
    int invX = GetInventoryX();
    int invY = GetInventoryY();
    int gridLeft = windowX_ + invX + config_.slayerGridX;
    int gridTop = windowY_ + invY + config_.slayerGridY;
    int gridRight = gridLeft + config_.gridX * GetGridUnitX();
    int gridBottom = gridTop + config_.gridY * GetGridUnitY();

    return x >= gridLeft && x < gridRight && y >= gridTop && y < gridBottom;
}

bool MixingForgeWindow::IsPointInMyGrid(int x, int y) const noexcept
{
    return x >= myGridRect_.x && x < myGridRect_.x + myGridRect_.w &&
           y >= myGridRect_.y && y < myGridRect_.y + myGridRect_.h;
}

bool MixingForgeWindow::IsValidItemForSlot(const MItem* item, int slotIndex) const noexcept
{
    if (!item)
        return false;

    // Check class
    if (!IsCorrectClass(static_cast<ItemClass>(item->GetItemClass())))
        return false;

    // Check type
    if (!IsCorrectType(static_cast<ItemType>(item->GetItemType()), static_cast<ItemClass>(item->GetItemClass())))
        return false;

    // Check if already selected in other slot
    if (slotIndex == 0 && itemCheck_[1] && selectedItems_[1] && selectedItems_[1]->GetID() == item->GetID())
        return false;
    if (slotIndex == 1 && itemCheck_[0] && selectedItems_[0] && selectedItems_[0]->GetID() == item->GetID())
        return false;

    // Check with other selected item
    if (slotIndex == 0 && itemCheck_[1] && selectedItems_[1])
    {
        if (!IsCorrectOption(item, selectedItems_[1].get()))
            return false;
    }
    if (slotIndex == 1 && itemCheck_[0] && selectedItems_[0])
    {
        if (!IsCorrectOption(selectedItems_[0].get(), item))
            return false;
    }

    return true;
}

bool MixingForgeWindow::IsCorrectClass(ItemClass itemClass) const noexcept
{
    // Legacy logic from IsCorrectClass()
    // This is a simplified version
    return true;
}

bool MixingForgeWindow::IsCorrectType(ItemType itemType, ItemClass itemClass) const noexcept
{
    // Legacy logic from IsCorrectType()
    // This is a simplified version
    return true;
}

bool MixingForgeWindow::IsCorrectOption(const MItem* item1, const MItem* item2) const noexcept
{
    // Legacy logic from IsCorrectOption()
    if (!item1 || !item2)
        return false;

    // Core Zap and Cue of Adam always OK together
    if (static_cast<ItemClass>(item1->GetItemClass()) == ItemClass::CoreZap && static_cast<ItemClass>(item2->GetItemClass()) == ItemClass::CoreZap)
        return true;
    if (static_cast<ItemClass>(item1->GetItemClass()) == ItemClass::CueOfAdam && static_cast<ItemClass>(item2->GetItemClass()) == ItemClass::CueOfAdam)
        return true;

    // Check option part
    // auto& opt1 = item1->GetItemOptionList();
    // auto& opt2 = item2->GetItemOptionList();
    // if (!opt1.empty() && !opt2.empty())
    // {
    //     auto& info1 = (*g_pItemOptionTable)[*opt1.begin()];
    //     auto& info2 = (*g_pItemOptionTable)[*opt2.begin()];
    //     return info1.Part == info2.Part;
    // }

    return false;
}

void MixingForgeWindow::UpdateRaceLayout() noexcept
{
    // Inventory SPK based on race
    // In real implementation, we'd get the current race from game state
    // For now using Slayer as default
    inventorySpk_ = std::make_unique<SpriteArchive>();
    inventorySpk_->Open(config_.slayerInventorySpkName.c_str());
}

int MixingForgeWindow::GetInventoryX() const noexcept
{
    return config_.slayerInventoryX;
}

int MixingForgeWindow::GetInventoryY() const noexcept
{
    return config_.slayerInventoryY;
}

void MixingForgeWindow::OnOkClicked() noexcept
{
    if (HasValidSelection())
    {
        // Send UI_USE_MIXING_FORGE packet
        // gpC_base->SendMessage(UI_USE_MIXING_FORGE, item1ID, item2ID, NULL);
    }
    Close();
}

void MixingForgeWindow::OnCancelClicked() noexcept
{
    // Send UI_CLOSE_MIXING_FORGE
    // gpC_base->SendMessage(UI_CLOSE_MIXING_FORGE);
    Close();
}

int MixingForgeWindow::GetFocusedItemGridX(const MItem* item) const noexcept
{
    if (!item)
        return 0;
    return myGridRect_.x + GetGridUnitX() * item->GetGridX();
}

int MixingForgeWindow::GetFocusedItemGridY(const MItem* item) const noexcept
{
    if (!item)
        return 0;
    return myGridRect_.y + GetGridUnitY() * item->GetGridY();
}

} // namespace phoenix::ui