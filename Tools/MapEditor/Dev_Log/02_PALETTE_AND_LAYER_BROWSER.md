# Palette And Layer Browser Update

Date: 2026-07-13

## Root Cause

The Tile, Object, and Animation palette controls were populated only from
resources referenced by the currently open map. New maps and sparse maps
therefore produced empty controls, and the controls did not represent the
complete Legacy resource archives.

The Layers section also exposed only three implemented render switches. Grid
was available only through the View menu, while unsupported layer categories
had no visible status. This made an absent feature indistinguishable from an
empty or broken panel.

## Correct Fix

- Tile Browser count comes from the shared `tile.spki` index.
- Object Browser count comes from the shared `imageobject.spki` index.
- Animation Browser frame IDs come from the body sequences already owned by
  `LegacyMapAnimationRenderer`.
- All three controls use `LBS_NODATA | LBS_OWNERDRAWFIXED`; only visible rows
  are formatted and no sprite is decoded while scrolling.
- Resource counts are refreshed only when the resolved resource root changes.
  Map edits, camera movement, repaint, and selection do not rebuild them.
- Before resources are loaded, and when a resource is unavailable, each browser
  renders an explicit status row instead of an empty control.
- Layer Visibility contains Tiles, Objects, Animations, and Grid as functional
  visibility switches.
- Collision, NPC, and Monsters are visible disabled controls labeled
  `Not implemented`; no behavior is fabricated for unsupported overlays.
- The Grid checkbox and View/Grid menu command update the same `showGrid_`
  state.

## Regression Analysis

The map model, writer, draw-list ordering, sprite decoder, texture cache, and
viewport renderer were not duplicated or replaced. Selecting a row still feeds
the existing placement tool and validation path. Animation IDs use the same
frame-pack renderer used by Client/Editor rendering.

The virtual controls avoid creating one Win32 string item per archive entry and
avoid archive-wide sprite decoding. Existing SPK/index caches remain the owner
of archive metadata lifetime.

## Rejected Alternatives

- Keeping map-referenced resources and adding an empty-state label was rejected
  because it would hide the root cause and still would not show all sprites.
- Decoding every archive sprite to build thumbnail palettes was rejected due to
  startup time, memory use, and duplicate work with the existing Sprite Browser.
- Adding fake Collision/NPC/Monster toggles was rejected because no verified
  render overlay exists for those categories.
- Rebuilding palette contents after every map edit was rejected because archive
  membership is independent of map content.

## Validation

- Debug `PhoenixMapEditor`: PASS
- Release `PhoenixMapEditor`: PASS
- Debug `MapEditorCoreTests`: PASS
- Release `MapEditorCoreTests`: PASS
- 378-map round-trip/parity corpus: PASS
- Tile Browser shared index count and sprite 8230 coverage: PASS
- Object Browser shared index count: PASS
- Animation Browser shared frame-sequence count: PASS
- Release profiler marker scan: PASS, zero hits
- GUI execution: not performed
- Visual and interaction validation: `USER_MANUAL_VALIDATION_REQUIRED`

