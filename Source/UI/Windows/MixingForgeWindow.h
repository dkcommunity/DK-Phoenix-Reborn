#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "Protocol/LegacyProtocol.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <array>

namespace phoenix::ui
{

// Forge class (matching Legacy FORGE_CLASS)
enum class ForgeClass : std::uint8_t
{
    Weapon = 0,
    Armor = 1,
    Accessory = 2,
    CoreZap = 3,
    CueOfAdam = 4,
    Count
};

// Forge type (matching Legacy FORGE_TYPE)
enum class ForgeType : std::uint8_t
{
    TypeA = 0,
    TypeB = 1,
    TypeC = 2,
    TypeD = 3,
    TypeE = 4,
    Count
};

// Item class enum (matching Legacy ITEM_CLASS) - Moved to MinimalUIComponents.h
// enum class ItemClass : std::uint8_t

// Item type enum (matching Legacy TYPE_ITEMTYPE)
enum class ItemType : std::uint8_t
{
    Type0 = 0,
    Type1 = 1,
    Type2 = 2,
    Type3 = 3,
    Type4 = 4,
    Type5 = 5,
    Type6 = 6,
    Type7 = 7,
    Type8 = 8,
    Type9 = 9,
    Type10 = 10,
    Type11 = 11,
    Type12 = 12,
    Type13 = 13,
    Type14 = 14,
    Type15 = 15,
    Type16 = 16,
    Type17 = 17,
    Type18 = 18,
    Type19 = 19,
    Type20 = 20,
    Type21 = 21,
    Type22 = 22,
    Type23 = 23,
    Type24 = 24,
    Type25 = 25,
    Type26 = 26,
    Type27 = 27,
    Type28 = 28,
    Type29 = 29,
    Type30 = 30,
    Type31 = 31,
    Type32 = 32,
    Type33 = 33,
    Type34 = 34,
    Type35 = 35,
    Type36 = 36,
    Type37 = 37,
    Type38 = 38,
    Type39 = 39,
    Type40 = 40,
    Type41 = 41,
    Type42 = 42,
    Type43 = 43,
    Type44 = 44,
    Type45 = 45,
    Type46 = 46,
    Type47 = 47,
    Type48 = 48,
    Type49 = 49,
    Count
};

// Item option part enum (matching Legacy ITEMOPTION_TABLE::PART)
enum class ItemOptionPart : std::uint8_t
{
    Durability = 0,
    StrToDex = 1,
    StrToInt = 2,
    StrToCon = 3,
    DexToStr = 4,
    DexToInt = 5,
    DexToCon = 6,
    IntToStr = 7,
    IntToDex = 8,
    IntToCon = 9,
    ConToStr = 10,
    ConToDex = 11,
    ConToInt = 12,
    Count
};

// MixingForge config from interface.ini [MixingForge_1280x960]
struct MixingForgeProfile
{
    std::int32_t windowX{ 100 };
    std::int32_t windowY{ 150 };
    std::int32_t windowWidth{ 400 };
    std::int32_t windowHeight{ 300 };
    bool topmost{ true };

    // Buttons
    std::int32_t okButtonX{ 26 };
    std::int32_t okButtonY{ 123 };
    std::int32_t cancelButtonX{ 113 };
    std::int32_t cancelButtonY{ 123 };

    // Item slots
    std::int32_t slot1X{ 30 };
    std::int32_t slot1Y{ 27 };
    std::int32_t slot2X{ 107 };
    std::int32_t slot2Y{ 27 };

    // Grid
    std::int32_t gridX{ 13 };
    std::int32_t gridY{ 26 };
    std::int32_t gridWidth{ 10 };
    std::int32_t gridHeight{ 8 };

    // SPK frames
    std::int32_t mainWindowFrame{ 0 };
    std::int32_t buttonOkFrame{ 0 };
    std::int32_t buttonCancelFrame{ 1 };
    std::int32_t inventoryWindowFrame{ 0 };
    std::int32_t inventoryBottomFrame{ 1 };

    // Race-specific offsets
    std::int32_t slayerInventoryX{ 350 };
    std::int32_t slayerInventoryY{ 150 };
    std::int32_t slayerGridX{ 13 };
    std::int32_t slayerGridY{ 176 };

    std::int32_t vampireInventoryX{ 350 };
    std::int32_t vampireInventoryY{ 150 };
    std::int32_t vampireGridX{ 17 };
    std::int32_t vampireGridY{ 169 };

    std::int32_t oustersInventoryX{ 350 };
    std::int32_t oustersInventoryY{ 150 };
    std::int32_t oustersGridX{ 25 };
    std::int32_t oustersGridY{ 185 };
};

class MixingForgeWindow
{
public:
    explicit MixingForgeWindow() noexcept;

    // State
    void Open(ForgeClass forgeClass, ForgeType forgeType) noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const settings::MixingForgeProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // Item selection
    bool SelectItem(std::uint32_t itemID, int slotIndex) noexcept;
    bool DeselectItem(int slotIndex) noexcept;
    void ClearSelection() noexcept;

    // Grid position for inventory
    void SetFocusGrid(int gridX, int gridY) noexcept;
    [[nodiscard]] int GetFocusGridX() const noexcept { return focusGridX_; }
    [[nodiscard]] int GetFocusGridY() const noexcept { return focusGridY_; }

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;
    bool HandleRightButtonDown(int x, int y) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Callbacks
    [[nodiscard]] bool HasValidSelection() const noexcept;
    [[nodiscard]] std::pair<std::uint32_t, std::uint32_t> GetSelectedItems() const noexcept;

private:
    // State
    bool isOpen_{ false };
    bool isDragging_{ false };
    std::int32_t dragOffsetX_{ 0 };
    std::int32_t dragOffsetY_{ 0 };

    // Config
    settings::MixingForgeProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // Forge parameters
    ForgeClass forgeClass_{ ForgeClass::Weapon };
    ForgeType forgeType_{ ForgeType::TypeA };

    // Selected items (2 slots)
    std::array<std::unique_ptr<MItem>, 2> selectedItems_;
    std::array<bool, 2> itemCheck_{ false, false };
    std::vector<bool> checkVector_;

    // Grid focus
    int focusGridX_{ -1 };
    int focusGridY_{ -1 };

    // UI Elements
    std::unique_ptr<ButtonGroup> buttonGroup_;
    std::unique_ptr<SpriteArchive> backSpk_;
    std::unique_ptr<SpriteArchive> inventorySpk_;

    // Window position
    std::int32_t windowX_{ 100 };
    std::int32_t windowY_{ 150 };
    std::int32_t windowWidth_{ 400 };
    std::int32_t windowHeight_{ 300 };

    // Grid
    struct GridRect
    {
        std::int32_t x{ 0 };
        std::int32_t y{ 0 };
        std::int32_t w{ 0 };
        std::int32_t h{ 0 };
    } myGridRect_;

    // Helpers
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;
    void UpdateGridPosition() noexcept;
    [[nodiscard]] bool IsPointInWindow(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInSlot(int slotIndex, int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInGrid(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInMyGrid(int x, int y) const noexcept;
    [[nodiscard]] bool IsValidItemForSlot(const MItem* item, int slotIndex) const noexcept;
    [[nodiscard]] bool IsCorrectClass(ItemClass itemClass) const noexcept;
    [[nodiscard]] bool IsCorrectType(ItemType itemType, ItemClass itemClass) const noexcept;
    [[nodiscard]] bool IsCorrectOption(const MItem* item1, const MItem* item2) const noexcept;

    // Race-specific layout
    void UpdateRaceLayout() noexcept;
    [[nodiscard]] int GetInventoryX() const noexcept;
    [[nodiscard]] int GetInventoryY() const noexcept;
    [[nodiscard]] std::int32_t GetGridUnitX() const noexcept { return 30; } // GRID_UNIT_PIXEL_X
    [[nodiscard]] std::int32_t GetGridUnitY() const noexcept { return 30; } // GRID_UNIT_PIXEL_Y

    // Callbacks
    void OnOkClicked() noexcept;
    void OnCancelClicked() noexcept;

    // Grid helpers
    [[nodiscard]] int GetFocusedItemGridX(const MItem* item) const noexcept;
    [[nodiscard]] int GetFocusedItemGridY(const MItem* item) const noexcept;
};

} // namespace phoenix::ui