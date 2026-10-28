#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <functional>
#include <Windows.h>

namespace phoenix::ui
{

// Minimal UI components shared between TitleScreen and ItemListWindow

// Button - minimal implementation
class Button
{
public:
    using Callback = std::function<void()>;
    enum class State { Normal, Hover, Press };

    Button([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int w, [[maybe_unused]] int h, [[maybe_unused]] const Callback& cb = {}) {}
    void SetPosition([[maybe_unused]] int x, [[maybe_unused]] int y) {}
    void SetCallback([[maybe_unused]] const Callback& cb) {}
    void Render() {}
    bool HitTest([[maybe_unused]] int x, [[maybe_unused]] int y) { return false; }
    void HandleMouseDown([[maybe_unused]] int x, [[maybe_unused]] int y) {}
    void HandleMouseUp([[maybe_unused]] int x, [[maybe_unused]] int y) {}
    void HandleMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) {}
    bool IsHovered() const { return false; }
    bool IsPressed() const { return false; }
};

// ButtonGroup - minimal implementation
class ButtonGroup
{
public:
    using ButtonCallback = std::function<void()>;
    
    void AddButton([[maybe_unused]] int role, [[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int w, [[maybe_unused]] int h, [[maybe_unused]] const ButtonCallback& cb = {}) {}
    void SetPosition([[maybe_unused]] int role, [[maybe_unused]] int x, [[maybe_unused]] int y) {}
    bool HandleMouseDown([[maybe_unused]] int x, [[maybe_unused]] int y) { return false; }
    bool HandleMouseUp([[maybe_unused]] int x, [[maybe_unused]] int y) { return false; }
    void HandleMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) {}
    void Render() {}
    bool HitTest([[maybe_unused]] int role, [[maybe_unused]] int x, [[maybe_unused]] int y) { return false; }
    void Update([[maybe_unused]] float deltaTime) {}
};

// ScrollBar - minimal implementation
class ScrollBar
{
public:
    enum class Orientation { Vertical, Horizontal };
    ScrollBar([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int w, [[maybe_unused]] int h, [[maybe_unused]] Orientation orient = Orientation::Vertical) {}
    void SetPosition([[maybe_unused]] int x, [[maybe_unused]] int y) {}
    void SetScrollPos([[maybe_unused]] int pos) {}
    void SetPosMax([[maybe_unused]] int max) {}
    int GetScrollPos() const { return 0; }
    int GetPosMax() const { return 0; }
    bool HitTest([[maybe_unused]] int x, [[maybe_unused]] int y) { return false; }
    void HandleMouseDown([[maybe_unused]] int x, [[maybe_unused]] int y) {}
    void HandleMouseUp([[maybe_unused]] int x, [[maybe_unused]] int y) {}
    void HandleMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) {}
    void ScrollUp() {}
    void ScrollDown() {}
    void Render() {}
    void Update([[maybe_unused]] float deltaTime) {}
};

// SpriteArchive - minimal implementation
class SpriteArchive
{
public:
    void Open([[maybe_unused]] const char*) {}
    void Draw([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int id) {}
    void DrawAlpha([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int id, [[maybe_unused]] std::uint8_t alpha) {}
    int GetWidth([[maybe_unused]] int id) { return 0; }
    int GetHeight([[maybe_unused]] int id) { return 0; }
    void Render([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int id) {}
};

// DialogWindow - minimal implementation
class DialogWindow
{
public:
    enum class DialogType { ExitConfirm };
    explicit DialogWindow([[maybe_unused]] DialogType) {}
    void Show() {}
    bool HandleMouseDown([[maybe_unused]] int, [[maybe_unused]] int, [[maybe_unused]] bool) { return false; }
    bool HandleMouseUp([[maybe_unused]] int, [[maybe_unused]] int, [[maybe_unused]] bool) { return false; }
    bool HandleKeyDown([[maybe_unused]] std::uint16_t) { return false; }
    void Update([[maybe_unused]] float) {}
    void Render() {}
};

// LineEditor - minimal implementation
class LineEditor
{
public:
    void SetPosition([[maybe_unused]] int, [[maybe_unused]] int) {}
    void SetPasswordMode([[maybe_unused]] bool) {}
    void SetByteLimit([[maybe_unused]] int) {}
    void AddString([[maybe_unused]] const char*) {}
    void EraseAll() {}
    bool HitTest([[maybe_unused]] int, [[maybe_unused]] int) { return false; }
    void SetFocus([[maybe_unused]] bool) {}
    bool HasFocus() const { return false; }
    bool AddChar([[maybe_unused]] wchar_t) { return true; }
    std::size_t Size() const { return 0; }
    const char* GetString() const { return ""; }
    std::string GetText() const { return ""; }
    std::size_t GetCaretPos() const { return 0; }
    void ClearText() {}
    void Render() {}
    void Update([[maybe_unused]] float deltaTime) {}
    std::size_t GetLineCount() const { return 1; }
    void EndCursor() {}
    bool InsertChar([[maybe_unused]] char c) { return true; }
    void Acquire() {}
    void Unacquire() {}
};

// MItem - minimal implementation for inventory items
struct MItem
{
    std::uint32_t id{ 0 };
    std::uint16_t itemClass{ 0 };
    std::uint16_t itemType{ 0 };
    std::uint16_t gridX{ 0 };
    std::uint16_t gridY{ 0 };
    std::uint8_t gridWidth{ 1 };
    std::uint8_t gridHeight{ 1 };
    std::int32_t number{ 1 };
    std::vector<std::uint16_t> optionList;

    [[nodiscard]] std::uint32_t GetID() const noexcept { return id; }
    [[nodiscard]] std::uint16_t GetItemClass() const noexcept { return itemClass; }
    [[nodiscard]] std::uint16_t GetItemType() const noexcept { return itemType; }
    [[nodiscard]] int GetGridX() const noexcept { return static_cast<int>(gridX); }
    [[nodiscard]] int GetGridY() const noexcept { return static_cast<int>(gridY); }
    [[nodiscard]] std::uint8_t GetGridWidth() const noexcept { return gridWidth; }
    [[nodiscard]] std::uint8_t GetGridHeight() const noexcept { return gridHeight; }
    [[nodiscard]] std::int32_t GetNumber() const noexcept { return number; }
    [[nodiscard]] const std::vector<std::uint16_t>& GetItemOptionList() const noexcept { return optionList; }
    [[nodiscard]] std::size_t GetItemOptionListCount() const noexcept { return optionList.size(); }
    [[nodiscard]] bool IsQuestItem() const noexcept { return false; }
    [[nodiscard]] bool IsUniqueItem() const noexcept { return false; }
    [[nodiscard]] bool IsPileItem() const noexcept { return false; }
    [[nodiscard]] bool IsChargeItem() const noexcept { return false; }
    [[nodiscard]] bool IsAffectStatus() const noexcept { return false; }
    [[nodiscard]] std::uint16_t GetInventoryFrameID() const noexcept { return 0; }
};

// Item class enum (matching Legacy ITEM_CLASS) - shared with MixingForgeWindow.h
enum class ItemClass : std::uint8_t
{
    Sword = 0,
    Blade = 1,
    Cross = 2,
    Mace = 3,
    SG = 4,
    SMG = 5,
    AR = 6,
    SR = 7,
    Coat = 8,
    Trouser = 9,
    Shoes = 10,
    Glove = 11,
    Helm = 12,
    Belt = 13,
    Shield = 14,
    Ring = 15,
    Bracelet = 16,
    Necklace = 17,
    VampireWeapon = 18,
    VampireCoat = 19,
    VampireRing = 20,
    VampireBracelet = 21,
    VampireNecklace = 22,
    VampireAmulet = 23,
    VampireEarring = 24,
    OustersChakram = 25,
    OustersWristlet = 26,
    OustersCoat = 27,
    OustersArmsband = 28,
    OustersCirclet = 29,
    OustersBoots = 30,
    OustersPendant = 31,
    OustersRing = 32,
    OustersStone = 33,
    CoreZap = 34,
    CueOfAdam = 35,
    MixingItem = 36,
    Count
};

} // namespace phoenix::ui