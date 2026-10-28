# Map Editor Sprite Browser and Screenshot Export

Date: 2026-07-13

The Windows editor executable was not launched. GUI and visual behavior remains
`USER_MANUAL_VALIDATION_REQUIRED`.

## Root cause

The old Sprite Browser had one untyped, transient `SpriteSource`. The two source
buttons opened generic dialogs, every source activation discarded browser
metadata and thumbnails, and the report-view columns mixed browsing with
metadata inspection. The left palette had virtual rows but rendered text only.
The right dock used a fixed one-third split and also owned the minimap.

The fix keeps the shared `SpriteArchiveReader`, decoded-image cache, texture
cache, animation frame-pack renderer, world draw-list cache, and minimap cache.
It adds source identity, layout calculations, export planning, and streaming PNG
encoding around those existing owners. Generic fallbacks, eager archive decode,
one-child-per-sprite controls, full-map GPU render targets, and camera-driven
cache invalidation were rejected because each would reintroduce the original
ownership or performance defect.

## Tasks in required order

1. `Tilelist` resolves `GameData/data/Image/tile.spk` and `tile.spki` from the
   configured image root. It uses the shared split-SPK reader and no dialog.
2. `Objectlist` resolves `imageobject.spk` and `imageobject.spki` identically.
   The source catalog reuses a previously indexed unchanged source.
3. Sprite report columns were removed. `UpdateProperties` appends a read-only
   sprite section with ID, type, exact chunk path, split base, local index,
   frame count, dimensions, origin (`n/a` when unavailable), cX/cY, timing,
   format, and cache/decode state.
4. Tiles, Objects, and Animations are push-like visibility buttons in the top
   toolbar. They change render flags and relevant minimap validity only.
5. The cached minimap occupies the former left layer area. Camera movement and
   zoom repaint only its camera rectangle; content/layer revisions rebuild the
   clean bitmap.
6. The three left owner-drawn virtual palettes request real cached thumbnails
   only for rows Windows asks them to paint. Animation rows use frame zero from
   `LegacyMapAnimationRenderer`. Decode failures retain a cached error image.
7. A DPI-scaled horizontal splitter resizes Sprite Browser and Properties,
   enforces both minimum sizes, and stores its ratio for the process session.
   Dragging performs layout only.
8. Sprite Browser is an `LVS_OWNERDATA` icon view. Normal 420-pixel dock width
   computes five columns. Native ListView supplies mouse selection, keyboard
   navigation, vertical scrolling, selection highlighting, and virtual item
   requests. Search filters IDs without decoding for layout.
9. `Screenshot -> Export Full Map` builds one deterministic frame-zero command
   list from the cached shared `WorldDrawList` in Legacy pass order:
   pre-background objects, tiles, then row objects. It excludes all editor
   overlays. WIC receives at most 256 rows per write under a 16 MiB temporary
   memory budget; no full-map GPU texture or full output bitmap is allocated.
10. `Screenshot -> Export Game Minimap (200x100)` writes an exact 200x100 PNG.
    Client audit shows `Application::DrawGameMinimapOverlay` draws the selected
    `Minimap.spk` sprite into a fixed 200x100 inner rectangle. Asset audit found
    source sizes from 100x100 through 200x100, including 200x50. For generated
    map imagery, the editor preserves the complete map aspect inside 200x100
    with centered letterboxing, renders at 400x200, then applies a 2x box
    downsample. This avoids inventing a crop or distorting map geometry.

## Source and handlers

- `Tool/MapEditor/MapEditorApp.cpp`
  - `CreateUi`: menus, toolbar toggles, virtual icon grid, palettes, minimap,
    Properties, and splitter controls.
  - `HandleCommand`: fixed source selection, layer toggles, and screenshot menu.
  - `LoadFixedSpriteSource`, `LoadSpriteSource`, `RefreshSpriteFilter`: source
    resolution, source catalog reuse, and virtual ID filtering.
  - `GetSpriteMetadata`, `DrawPaletteItem`: lazy decode/thumbnail and metadata.
  - `SetPanelSplitterFromClientY`, `Layout`: session splitter and responsive grid.
  - `BuildScreenshotCommands`, `CompositeScreenshotCommand`, `ExportScreenshot`:
    shared-order CPU composition and streaming export.
  - `UpdateProperties`: editable map fields plus read-only sprite metadata.
- `Tool/MapEditor/MapEditorUiModel.h/.cpp`: fixed source, source identity reuse,
  five-column grid, visible-range, and splitter calculations.
- `Tool/MapEditor/MapEditorExportModel.h/.cpp`: overlay-free layer policy,
  full-map strip plan, and 200x100 aspect fit.
- `Tool/MapEditor/MapEditorPngWriter.h/.cpp`: shared WIC streaming PNG writer.
- `Source/Tests/MapEditorCoreTests.cpp`: source, split archive, cache reuse,
  layout, minimap cache, screenshot, PNG dimension, and existing parity tests.
- `CMakeLists.txt`: new core sources and WIC/COM linkage.

## Cache ownership and invalidation

- Split source catalog: `MapEditorApp`; reset only when resource root changes.
- Decoded sprites: existing `decodedSprites_`; negative results are cached;
  reset with resource lifetime, never by scroll/source toggle.
- Thumbnail metadata/image list: `MapEditorApp`; populated by visible virtual
  item requests; reset only when resource root changes/application exits.
- GPU textures: existing tile/object maps; screenshot does not upload textures.
- World draw list: existing `MapEditorDrawListCache`; content revision only.
- Minimap bitmap: existing `RevisionCacheState`; map content or relevant layer
  visibility only. Pan/zoom updates only the camera rectangle.
- Splitter: layout ratio only; no render/resource invalidation.

Debug-only counters cover split scans, decode misses, thumbnail misses, texture
upload misses, Properties refreshes, minimap rebuilds, draw-list rebuilds,
screenshot strip count, and peak temporary bytes. They are inside `_DEBUG`.

## Validation

- Debug `PhoenixMapEditor`: PASS
- Debug `PhoenixClient`: PASS
- Debug `MapEditorCoreTests`: PASS
- Release `PhoenixMapEditor`: PASS
- Release `PhoenixClient`: PASS
- Release `MapEditorCoreTests`: PASS
- Corpus: 378/378 maps parsed with complete traversal.
- Existing round-trip, backup, byte identity, reload, animation parity,
  draw-list parity, and split-SPK sprite 8230 tests: PASS.
- New tests: fixed paths, split indexes, source reuse, five-column layout,
  visible thumbnail range, splitter bounds, minimap cache preservation,
  overlay exclusion, bounded strips, state-preserving export planning,
  full-map PNG bounds, and exact 200x100 PNG: PASS.
- Release binary scan found none of the Debug profiling/counter/audit strings.

## Manual GUI validation

All items below are `USER_MANUAL_VALIDATION_REQUIRED`:

- Tilelist/Objectlist/Effectlist control layout and selection (Effectlist replaced
  the duplicate Animationlist in the 2026-07-14 addendum).
- Five-column grid appearance, scrolling, keyboard navigation, search, and DPI.
- Palette thumbnails and selection highlight.
- Sprite metadata presentation beside editable map/object properties.
- Toolbar visibility toggles and left minimap placement/camera rectangle.
- Splitter drag, resize, maximize, restore, and DPI behavior.
- Full-map and game-minimap Save dialogs and visual output parity.

## Remaining technical limitations

None identified by automated validation. Visual and interaction validation is
reserved for the user and was not executed.
