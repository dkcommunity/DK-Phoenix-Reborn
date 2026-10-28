#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <filesystem>

namespace phoenix::ui
{

using ItemListProfile = settings::GameItemListProfile;

// Item data for display in the list
struct ItemListEntry
{
    std::unique_ptr<MItem> pItem;
    bool selected{ false };
    std::int32_t timeLimit{ 0 };  // seconds remaining for timed items
};

class ItemListWindow
{
public:
    explicit ItemListWindow() noexcept;

    // State
    void Open() noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const ItemListProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // Item management
    bool AddItem(std::unique_ptr<MItem> item, std::int32_t timeLimit = 0) noexcept;
    bool RemoveItem(std::uint32_t itemID) noexcept;
    void ClearItems() noexcept;
    [[nodiscard]] std::size_t GetItemCount() const noexcept { return items_.size(); }

    // Current focus
    [[nodiscard]] std::int32_t GetCurrentItemIndex() const noexcept;
    void SetCurrentItemIndex(std::int32_t index) noexcept;
    [[nodiscard]] const MItem* GetCurrentItem() const noexcept;

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleMouseWheel(int delta) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Drag support
    bool BeginDrag(int x, int y) noexcept;
    void UpdateDrag(int x, int y) noexcept;
    void EndDrag() noexcept;
    [[nodiscard]] bool IsDragging() const noexcept { return isDragging_; }
    [[nodiscard]] bool IsDraggable() const noexcept { return draggable_; }

private:
    // State
    bool isOpen_{ false };
    bool draggable_{ true };
    bool isDragging_{ false };
    std::int32_t dragOffsetX_{ 0 };
    std::int32_t dragOffsetY_{ 0 };

    // Config
    ItemListProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // Items
    std::vector<std::unique_ptr<MItem>> items_;
    std::vector<bool> itemSelected_;
    std::vector<std::int32_t> itemTimeLimits_;
    std::int32_t currentItemIndex_{ -1 };
    std::int32_t mouseFocusIndex_{ -1 };

    // Scroll
    std::int32_t scrollPos_{ 0 };
    std::int32_t maxScrollPos_{ 0 };
    std::int32_t visibleLines_{ 8 };
    std::int32_t lineGap_{ 20 };

    // UI Elements
    std::unique_ptr<ButtonGroup> buttonGroup_;
    std::unique_ptr<ScrollBar> scrollBar_;

    // SPK archives
    std::unique_ptr<SpriteArchive> assembleBoxButtonSpk_;
    std::unique_ptr<SpriteArchive> globalResourceSpk_;

    // Window position
    std::int32_t windowX_{ 0 };
    std::int32_t windowY_{ 0 };
    std::int32_t windowWidth_{ 450 };
    std::int32_t windowHeight_{ 400 };

    // Helpers
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;
    [[nodiscard]] std::string GetItemName(std::size_t index) const noexcept;
    [[nodiscard]] std::string GetItemTimeString(std::size_t index) const noexcept;
    void ClampScrollPos() noexcept;
    [[nodiscard]] bool IsPointInItemList(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInWindow(int x, int y) const noexcept;
    void UpdateScrollBar() noexcept;

    // Callbacks
    void OnOkClicked() noexcept;
    void OnCancelClicked() noexcept;
};

} // namespace phoenix::ui