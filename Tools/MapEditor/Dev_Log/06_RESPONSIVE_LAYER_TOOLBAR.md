# Responsive Layer Visibility Toolbar

Date: 2026-07-14

The Windows GUI was not launched by Codex. Visual confirmation is
`USER_MANUAL_VALIDATION_REQUIRED`.

## Root cause

Tiles, Objects, and Animations toggle controls had been created as toolbar child
windows, and the old left `Layer Visibility` panel had been removed. However,
the toolbar used one fixed horizontal row. At higher DPI or narrower window
widths, the complete toolbar width exceeded the client area, allowing the layer
controls to be clipped. A control that cannot remain visible does not satisfy
the layer-toolbar requirement.

## Fix

- Added a visible `Layers:` toolbar group label.
- Tiles, Objects, and Animations remain Win32 `BS_AUTOCHECKBOX | BS_PUSHLIKE`
  controls with keyboard focus support and persistent pressed/unpressed state.
- At sufficient width, all toolbar controls remain on one row.
- Before the layer controls can leave the client area, the toolbar switches to
  two rows:
  - row 1: Select, Paint, Erase, Object, Anim, Move, Layers, Tiles, Objects,
    Animations;
  - row 2: New, Open, Save, Undo, Redo, zoom controls, Fit.
- Toolbar height is included in dock and viewport bounds, so wrapping cannot
  overlap the map viewport or side panels.
- `CalculateToolbarRowCount` is a pure tested layout calculation and receives
  the DPI-scaled required width.

The toggle event path is unchanged: it only updates `showTiles_`, `showImages_`,
or `showAnimations_` and invalidates the relevant cached minimap bitmap. It does
not reload resources, rebuild the map, clear textures, decode sprites, or reorder
the Legacy draw list.

## Files

- `Tool/MapEditor/MapEditorApp.cpp`
  - `CreateUi`
  - `Layout`
  - `HandleCommand`
- `Tool/MapEditor/MapEditorUiModel.h/.cpp`
  - `CalculateToolbarRowCount`
- `Source/Tests/MapEditorCoreTests.cpp`

## Validation

- Debug PhoenixMapEditor build: PASS
- Release PhoenixMapEditor build: PASS
- Debug MapEditorCoreTests: PASS
- Release MapEditorCoreTests: PASS
- Corpus and existing parity tests: 378/378 PASS
- Toolbar wrap boundary unit test: PASS
- GUI visibility/pressed-state validation: `USER_MANUAL_VALIDATION_REQUIRED`
