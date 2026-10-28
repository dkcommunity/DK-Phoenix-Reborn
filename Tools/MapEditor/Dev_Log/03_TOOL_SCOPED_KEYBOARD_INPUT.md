# Tool-Scoped Keyboard Input

Date: 2026-07-13

## Root Cause

Navigation keys were routed globally to camera movement. The active editor tool
was not part of keyboard action resolution, so Select and Move could not expose
different keyboard contracts.

The existing object movement path also scanned every map object and rebuilt the
complete sector/object index for each nudge. Key-repeat would amplify that cost
on object-heavy maps.

## Correct Fix

- Select tool + Arrow Keys nudges selected objects by one Legacy tile.
- When Select has a tile but no object selection, Arrow Keys move the selected
  tile coordinate without changing map data or dirty-state.
- Move tool + W/A/S/D or Arrow Keys drives the existing delta-time camera state.
- Navigation keys do not move the camera in other tools.
- Held object-nudge keys share one undo snapshot until all held Arrow Keys are
  released.
- Tool changes and focus loss clear held-key state.
- Object ID to vector-index lookup is cached and rebuilt only when the map or
  object collection changes.
- `CanMoveObject` and `MoveObject` live in `PhoenixMapEditorCore`; the UI does
  not duplicate Legacy position and sector-index mutation.
- Object movement updates only affected sector index entries and preserves
  object-index ordering. It does not rebuild the full sector index.

## Regression Analysis

Object movement still updates pixel X/Y, viewpoint, every position record,
dirty-state, undo/redo, draw-list invalidation, minimap invalidation, property
display, and the Legacy save path. Boundary movement returns without mutation.

Camera movement continues to reuse the cached draw list, minimap terrain, sprite
decodes, and textures. Delta-time behavior is unchanged.

## Rejected Alternatives

- Keeping global camera keys and special-casing only Arrow Keys was rejected
  because the active tool would still not own its input contract.
- Rebuilding the full object index after each nudge was rejected due to avoidable
  object-heavy map cost.
- Moving tile data when only a tile coordinate is selected was rejected because
  no verified Legacy operation defines whether sprite, property, light, or the
  complete sector should be moved.
- Treating W/A/S/D as Select shortcuts was rejected because it conflicts with
  the requested Move-tool camera contract.

## Validation

- Debug `PhoenixMapEditor`: PASS
- Release `PhoenixMapEditor`: PASS
- Debug `MapEditorCoreTests`: PASS
- Release `MapEditorCoreTests`: PASS
- Incremental moved-object sector index equals complete rebuild: PASS
- Boundary nudge does not mutate map: PASS
- 378-map round-trip/parity corpus: PASS
- Release profiler marker scan: PASS, zero hits
- GUI execution: not performed
- Keyboard interaction: `USER_MANUAL_VALIDATION_REQUIRED`

