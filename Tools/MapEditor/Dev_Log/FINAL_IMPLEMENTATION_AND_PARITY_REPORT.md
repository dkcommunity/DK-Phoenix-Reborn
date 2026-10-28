# Phoenix Map Editor - Final Implementation and Parity Report

Latest implementation addendum: `06_RESPONSIVE_LAYER_TOOLBAR.md`.
It records the DPI-responsive top toolbar placement for the Tiles, Objects, and
Animations visibility toggles.

Date: 2026-07-13

## Execution Boundary

The Windows GUI was implemented and compiled. Neither
`PhoenixMapEditor_Debug.exe` nor `PhoenixMapEditor_Release.exe` was launched.
No GUI automation, screenshot capture, mouse/keyboard simulation, or visual
inspection was performed. All visual and interactive checks are marked
`USER_MANUAL_VALIDATION_REQUIRED`.

## Implemented GUI

The editor is a native Win32 application backed by a DirectX 11 viewport and
the shared `PhoenixMapCore` implementation.

- Resizable, maximizable main window with per-monitor-v2 DPI awareness
- File/Edit/View/Tools menu bar
- Command and tool toolbar with New/Open/Save, Undo/Redo, Select, Paint,
  Erase, Object, Animation, Move, and zoom controls
- Left dock containing virtual Tile, Object, and Animation archive browsers
  plus explicit layer visibility controls
- Right dock containing Sprite Browser, Property Panel, and Minimap
- View-menu visibility toggles for Sprite Browser, Property Panel, and Minimap
- Multi-part status bar showing map name/size, tile coordinate, world pixel
  coordinate, zoom, selected sprite/object, active tool, and modified state
- Modal New Legacy Map form with name, width, height, and validation
- Native Open, Save As, single-SPK, and split-folder dialogs
- Save/Discard/Cancel unsaved-changes flow
- Keyboard accelerators that do not override active property/search text input
- Move-tool delta-time camera movement with held W/A/S/D or arrow keys
- Select-tool Arrow Key nudge for selected objects and tile-coordinate navigation
- Ctrl+A/C/V/X/Z/Y, Delete, +/- zoom, 0 actual size, and F fit-map shortcuts
- Minimum window sizing, DPI font refresh, minimize-safe swap-chain resizing,
  and child-control focus isolation

## Menu Commands

File:

- New, Open, Save, Save As, Exit

Edit:

- Undo, Redo, Copy, Paste, Delete

View:

- Grid, Minimap, Property Panel, Sprite Browser

Tools:

- Select, Paint Tile, Erase Tile, Place Object, Place Animation, Move

## Viewport Tools

- Client-shared `WorldDrawListBuilder` and `LegacyMapAnimationRenderer`
- DirectX 11 rendering with an Editor viewport scissor rectangle
- Camera pan with middle-button capture only while dragging
- Mouse-wheel and toolbar zoom from 2% to 300%
- Persistent tile selection and tile highlight
- Object/animation hit testing against rendered frame rectangles
- Ctrl multi-selection and rectangle selection
- Separate tile paint and tile erase modes
- Separate image-object and animation-object placement modes
- Separate object Move mode with one history entry per drag
- Object and animation deletion
- Copy/paste for selected object sets
- Grid, selected-tile, selected-object, and rectangle-selection overlays
- Tiles, Objects, Animations, and Grid visibility controls
- Disabled `Not implemented` status controls for Collision, NPC, and Monsters
- Viewport input is rejected outside the calculated viewport rectangle
- Child controls receive their own input and cannot initiate viewport actions

The runtime draw contract remains:

```text
pre-background Image Objects -> Tile surface -> row layers
```

Layer filtering does not reorder draw-list submission.

## Sprite Browser

- Native virtual owner-data ListView
- Single `.spk`, `.sspk`, or `.ispk` source selection
- Split-SPK folder selection through the shared split lookup path
- Exact Sprite ID search without rebuilding/decompressing the full archive
- Lazy decode only when metadata/image data is requested for a visible row
- Shared decoded-image cache, texture cache, metadata cache, and 48x48 thumbnail image-list cache
- Selected-sprite preview
- Sprite ID, source path, split/archive base, internal index, frame count,
  dimensions, frame-pack `cX/cY`, and timing columns
- Static SPK records report one frame and static timing
- Sprites referenced by animation frame packs report real sequence frame count,
  `cX/cY`, and the shared 62 ms frame timing
- Selecting a browser or palette item updates the active placement sprite/frame ID

The left Tile Browser and Object Browser expose every ID from their shared SPK
indices without decoding the archive. The Animation Browser exposes every
non-empty body sequence from the shared animation frame-pack renderer. These
owner-draw `LBS_NODATA` controls format only visible rows and always show an
explicit resource-status row when their source is unavailable.

SPK records do not contain a separate archive origin field. The browser reports
that part as `n/a` rather than fabricating a value; frame-pack `cX/cY` is shown
when a real reference exists.

## Property Panel

The panel displays real map data in a native ListView. Double-clicking an
editable value creates a control appropriate to the field type:

- Edit boxes for text
- Edit plus native up-down controls for numeric values
- Combo boxes for wrapper type, BLT type, and direction
- Check boxes for animation, transparency, and loop flags
- Read-only rows for identity, count, and structural fields

Editable map fields:

- map name, zone ID, zone group, zone type, zone level, description

Editable tile fields:

- sprite ID, property, light

Editable Image Object fields:

- object type, wrapper type, sprite ID, raw wall direction/value
- pixel X/Y, viewpoint, animation, transparency
- every sector-position record X/Y

Editable Animation Object fields:

- frame ID, max frame, BLT type, direction
- sound frame/ID, loop, min/max delay
- start/end hour and wrapper-7 interaction type

Every commit uses `LegacyMapPropertyEditor`. Validation occurs before mutation;
invalid values do not change map data, history, or dirty-state. Successful
changes update history, resource lists, object indices, render invalidation, palettes,
minimap, title, status, and the Legacy save path.

## Runtime Performance Audit

Removed bottlenecks:

- The Editor no longer calls `WorldDrawListBuilder::Build` every frame. A full,
  world-space draw list is retained by `MapEditorDrawListCache`.
- Camera movement does not traverse map sectors to regenerate the minimap.
  `MinimapProc` stretches a retained DIB and computes only the camera rectangle.
- Frame and overlay quad vectors persist and retain capacity instead of being
  allocated in every render call.
- `LegacyMapAnimationRenderer::OrderedFrames` returns a fixed two-frame array
  instead of allocating a vector per animation object.
- `DirectX11Renderer::DrawTexturedQuads` reuses renderer-owned drawable scratch
  storage instead of allocating a pointer vector per batch call.
- Hover movement updates only status text and no longer rebuilds the Property Panel.
- A tile paint drag creates one undo snapshot for the stroke instead of copying
  the complete map once for every painted tile.
- Undo, redo, and property edits no longer release unchanged GPU textures.
- Animation AFPK/EFPK packs are not reloaded when the resource root is unchanged.

Persistent caches and exact invalidation conditions:

- World draw list: rebuilt on map replacement, undo/redo, tile/object mutation,
  or rendering-relevant property mutation. Pan, zoom, timer ticks, animation
  frame changes, and repaint reuse it.
- Visible tile/layer indices: rebuilt when the map content revision, integer
  camera tile, zoom, or viewport dimensions change. Sub-tile movement reuses
  the set and changes only coordinate transformation and clipping.
- Minimap DIB and memory DC: rebuilt only for a different map, actual map-content
  changes, or Tile/Image/Animation minimap layer visibility changes. Pan, zoom,
  resize, and scrolling reuse the bitmap and redraw only the camera rectangle.
  Saving an unchanged map preserves the cached minimap revision.
- Decoded sprites: keyed by archive, index, palette, format, sprite ID, palette
  ID, and alpha mode. Successes and failures are cached, so browser thumbnails
  and viewport texture creation cannot decode the same source record twice.
- GPU textures: keyed by tile/object source, wrapper, sprite, palette, and blend
  source. Upload occurs only on a texture-cache miss or resource-root change.
- Sprite Browser metadata and thumbnails: generated lazily for owner-data rows;
  filtering and scrolling do not decode the full archive.
- SPK bytes, atlas metadata, and combined split indices retain the shared bounded
  caches in `SpriteArchiveReader`.
- Animation frame-pack metadata remains resident until the resource root changes.

Debug-only profiling records average, maximum, and call count for map rendering,
draw-list generation, minimap generation, Sprite Browser work, Property Panel
updates, SPK decode misses, and texture upload misses. The status bar rotates
through these counters. Profiler types, labels, scopes, and display code are
compiled only under `_DEBUG`.

## Event Handlers And Connections

- `WindowProc` / `HandleMessage`: resize, DPI, timer render, commands,
  notifications, mouse capture, wheel zoom, keyboard, close, destroy
- `HandleCommand`: menu, toolbar, palette, layer, zoom, browser, and property
  control dispatch
- `HandleNotify`: virtual Sprite Browser data, sprite selection, property edit,
  and palette tabs
- `HandleLeftDown`, `HandleMouseMove`, `HandleLeftUp`: viewport tools,
  rectangle selection, painting, panning, placement, and movement
- `PropertyEditProc`: Enter/Escape/focus handling and numeric-control commit
- `NewMapWindowProc`: modal New Map fields and dimension validation
- `MinimapProc`: map-derived minimap and camera marker
- `Render` / `DrawOverlays`: shared draw list, shared animation frames, DX11
  textures, scissor clipping, layers, grid, and selection overlays
- `OpenMap`, `Save`, `ConfirmDiscard`: shared Legacy reader/writer, backup,
  immediate reload verification, and unsaved-state handling

## Backend Connections

- Map model/load: `LegacyMapReader`
- Legacy save/backup/unchanged bytes: `LegacyMapWriter`
- Property validation/history: `PhoenixMapEditorCore`
- SPK/split-SPK decode and lookup: `SpriteArchiveReader`
- Animation AFPK/EFPK metadata and timing: `LegacyMapAnimationRenderer`
- Runtime pass order and coordinates: `WorldDrawListBuilder`
- Rendering, alpha/screen blend, and viewport scissor: `DirectX11Renderer`
- Client and Editor link the same `PhoenixMapCore` static library

## Source Files

Primary GUI and editor model:

- `Tool/MapEditor/MapEditorApp.cpp`
- `Tool/MapEditor/MapEditorDocument.h`
- `Tool/MapEditor/MapEditorDocument.cpp`
- `Tool/MapEditor/MapEditorRenderCache.h`
- `Tool/MapEditor/MapEditorRenderCache.cpp`
- `Tool/MapEditor/MapEditorMinimapModel.h`
- `Tool/MapEditor/MapEditorMinimapModel.cpp`

Shared paths used or extended by the GUI:

- `Source/Backends/DirectX11/DirectX11Renderer.h/.cpp`
- `Source/World/LegacyMapAnimationRenderer.h/.cpp`
- `Source/World/WorldDrawListBuilder.h/.cpp`
- `Source/World/LegacyMapReader.h/.cpp`
- `Source/World/LegacyMapWriter.h/.cpp`
- `Source/Assets/SpriteArchiveReader.h/.cpp`
- `Source/Assets/AnimationObjectFramePackReader.h/.cpp`
- `Source/Assets/EffectFramePackReader.h/.cpp`
- `Source/Tests/MapEditorCoreTests.cpp`
- `CMakeLists.txt`

Unrelated pre-existing working-tree changes were not reverted or included as
Map Editor implementation work.

## Build Results

Debug: **PASS**

- `Tool/MapEditor/bin/PhoenixMapEditor_Debug.exe`
- Size: 2,041,344 bytes
- SHA-256: `147C19C3FB35F385A186BD7D81D3468475DA4BDF28C1FC3515097F35772D8305`
- `GameData/MapEditorCoreTests_Debug.exe`
- `GameData/PhoenixClient_Debug.exe`

Release: **PASS**

- `Tool/MapEditor/bin/PhoenixMapEditor_Release.exe`
- Size: 385,024 bytes
- SHA-256: `A0B6551494C42C802730210C54E6A42A35042853FA73F72A06EA4535C7D20453`
- `GameData/MapEditorCoreTests_Release.exe`
- `GameData/PhoenixClient.exe`

Build commands:

```powershell
cmake --preset windows-vs2022-x64
cmake --build --preset windows-vs2022-x64-debug --target PhoenixMapEditor MapEditorCoreTests PhoenixClient -- /m
cmake --build --preset windows-vs2022-x64-release --target PhoenixMapEditor MapEditorCoreTests PhoenixClient -- /m
```

## Automated Test Results

Debug and Release headless tests: **PASS**

```text
PASS map=256x256 objects=8803 tiles=2576 layers=650 animation_map=gdr_illusion_01.map animation_frame=2 animation_frames=96 maps=378 animation_blt=4346,5138,0,1744 sprite8230=48x24
```

Passed coverage:

- real `eslania_nw.map` load
- unchanged save byte identity
- Save As backend and reload
- modified map save/reload
- automatic `.bak` and backup byte identity
- map/tile/image/animation property validation and save/reload
- invalid property mutation/dirty/history rejection
- dirty-state, undo, and redo transitions
- split-SPK sprite 8230 lookup/decode (`48x24`)
- animation frame timing, wrap, loop, origin, `cX/cY`, shadow/body order
- Sprite Browser metadata uses the same frame-pack sequence as rendering
- Tile/Object Browser counts use the complete shared SPK indices
- Animation Browser count uses the shared frame-pack renderer
- effect-alpha and effect-screen palette decode/blend
- viewport rectangle clipping contract
- Legacy draw-list/pass order
- Client/Editor draw-list parity with animation objects
- draw-list revision cache reuse across repeated camera/zoom requests
- draw-list invalidation on map-content revision changes
- O(1) cached sector-to-tile lookup
- minimap terrain-cache reuse across 100 camera rectangle updates
- minimap invalidation on map-content revision changes
- incremental selected-object movement index matches a complete index rebuild
- boundary object nudge rejects mutation
- Debug Client build against shared `PhoenixMapCore`
- `git diff --check`

The real 378-map corpus contains animation BLT counts `4346,5138,0,1744`.
There is no BLT-2 object in that corpus, so the implemented effect-shadow path
cannot be exercised by an existing real map fixture. This is a corpus coverage
limit, not a missing renderer branch.

## Release Diagnostics Audit

Release `PhoenixMapCore`, `PhoenixMapEditor`, and `PhoenixClient` do not define
`PHOENIX_ENABLE_DIAGNOSTICS`.

The Release Editor binary was scanned for profiler, diagnostic, audit, overlay,
and capture markers. No markers were found. Result: **PASS**.

## Manual Validation Checklist

No item below was executed by Codex:

- `USER_MANUAL_VALIDATION_REQUIRED` - main window opening and initial layout
- `USER_MANUAL_VALIDATION_REQUIRED` - menu and toolbar visual/interaction behavior
- `USER_MANUAL_VALIDATION_REQUIRED` - New/Open/Save/Save As dialogs
- `USER_MANUAL_VALIDATION_REQUIRED` - unsaved changes Save/Discard/Cancel dialog
- `USER_MANUAL_VALIDATION_REQUIRED` - dock visibility and panel layout
- `USER_MANUAL_VALIDATION_REQUIRED` - DirectX viewport visual output
- `USER_MANUAL_VALIDATION_REQUIRED` - pan and wheel/toolbar zoom
- `USER_MANUAL_VALIDATION_REQUIRED` - Move-tool held W/A/S/D and arrow-key camera movement
- `USER_MANUAL_VALIDATION_REQUIRED` - Select-tool object nudge and tile-coordinate navigation
- `USER_MANUAL_VALIDATION_REQUIRED` - Ctrl+A/C/V/X/Z/Y, Delete, +/-/0/F shortcuts
- `USER_MANUAL_VALIDATION_REQUIRED` - cached minimap updates only its camera rectangle during pan/zoom
- `USER_MANUAL_VALIDATION_REQUIRED` - tile selection, paint, erase, and highlight
- `USER_MANUAL_VALIDATION_REQUIRED` - image-object select/place/move/delete
- `USER_MANUAL_VALIDATION_REQUIRED` - animation select/place/move/delete
- `USER_MANUAL_VALIDATION_REQUIRED` - Ctrl multi-select and rectangle selection
- `USER_MANUAL_VALIDATION_REQUIRED` - copy/paste and undo/redo interaction
- `USER_MANUAL_VALIDATION_REQUIRED` - typed property controls and error messages
- `USER_MANUAL_VALIDATION_REQUIRED` - Sprite Browser single/split source loading
- `USER_MANUAL_VALIDATION_REQUIRED` - Sprite Browser search, thumbnails, and scroll
- `USER_MANUAL_VALIDATION_REQUIRED` - Tile/Object/Animation palettes
- `USER_MANUAL_VALIDATION_REQUIRED` - layer controls, grid, and selection overlays
- `USER_MANUAL_VALIDATION_REQUIRED` - minimap rendering
- `USER_MANUAL_VALIDATION_REQUIRED` - resize, maximize, restore, and minimum size
- `USER_MANUAL_VALIDATION_REQUIRED` - DPI changes and font/control scaling
- `USER_MANUAL_VALIDATION_REQUIRED` - keyboard focus and accelerators
- `USER_MANUAL_VALIDATION_REQUIRED` - mouse capture during drag/pan/paint only
- `USER_MANUAL_VALIDATION_REQUIRED` - UI control input isolation from viewport
- `USER_MANUAL_VALIDATION_REQUIRED` - general Client/Editor pixel parity

## Remaining Technical Work

No known implementation task remains in the requested GUI, shared backend,
build, or headless-test scope. The only pending work is the user's manual Windows
visual and interaction validation listed above.
