# Next Target: C_VS_UI_ITEM_LIST → ItemListWindow

## Source: VS_UI_GameCommon2.cpp lines 348-1280

## Key Functions to Port:

| Function | Line | Purpose |
|----------|------|---------|
| `C_VS_UI_ITEM_LIST::C_VS_UI_ITEM_LIST()` | 352 | Constructor - init button group, scroll bar |
| `~C_VS_UI_ITEM_LIST()` | 397 | Destructor - cleanup buttons, scroll bar |
| `Start()` | 407 | Initialize UI position, load items |
| `Finish()` | 416 | Cleanup |
| `Show()` | 423 | Render window, buttons, items |
| `MouseControl()` | 1004 | Handle mouse hover/click on items/buttons |
| `KeyboardControl()` | 1081 | Handle keyboard (ESC, arrows) |
| `ShowButtonWidget()` | 1107 | Position buttons from config |
| `ShowButtonDescription()` | 1119 | Show tooltip on hover |
| `WindowEventReceiver()` | 1140 | Handle button events |
| `Run()` | 1144 | Execute button commands |
| `Process()` | 1156 | Update scroll, animations |
| `ClearItem()` | 1162 | Clear all items |
| `IsPixel()` | 1102 | Hit test for window area |

## Config Keys Needed in interface.ini [ItemList_1280x960]:

```ini
WindowX = 0
WindowY = 0
WindowWidth = 320
WindowHeight = 480
Draggable = true
ButtonCloseX = ...
ButtonCloseY = ...
ScrollBarX = ...
ScrollBarY = ...
ItemSlotWidth = ...
ItemSlotHeight = ...
```

## Legacy Dependencies:
- `C_VS_UI_EVENT_BUTTON` for buttons
- `C_VS_UI_SCROLL_BAR` for scrolling
- `MItem*` for item data
- `C_VS_UI_EVENT_BUTTON` callbacks

## Modern Pattern:
```cpp
class ItemListWindow {
    void LoadFromConfig(const GameItemListProfile& profile) noexcept;
    void Show() noexcept;
    void Update(float dt) noexcept;
    void Render() noexcept;
    bool HandleMouseDown(int x, int y, bool right) noexcept;
    bool HandleKeyDown(uint16_t key) noexcept;
}
```

## Config Section to Add:
```ini
[ItemList_1280x960]
WindowX = 0
WindowY = 0
WindowWidth = 320
WindowHeight = 480
Draggable = true
CloseButtonX = ...
CloseButtonY = ...
ScrollBarX = ...
ScrollBarY = ...
ItemSlotWidth = 32
ItemSlotHeight = 32
```