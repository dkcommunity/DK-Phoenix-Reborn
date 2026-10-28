# Effectlist and AFPK Count Audit

Date: 2026-07-14

The Windows editor was not launched. Visual behavior is
`USER_MANUAL_VALIDATION_REQUIRED`.

## Root cause

The left Animation palette and the right `Animationlist` both enumerated the
same `animationobject.afpk` body sequences. The visible count of five was not a
parser truncation:

- `animationobject.afpk`: 2568 bytes, header sequence count 5.
- `animationobjectshadow.afpk`: 2568 bytes, header sequence count 5.
- Frame counts in both files: 42, 96, 96, 96, 96 (426 total frames).
- Layout consumed by the verified parser: `2 + (5 * 2) + (426 * 6) = 2568`.
- Remaining bytes after parse: 0.

Changing the AFPK count, relaxing EOF checks, or fabricating additional frame
IDs was rejected because the source files prove that five is the complete valid
top-level sequence count.

## Change

- The left Animation palette remains the map Animation Object placement source.
- The right Sprite Browser button is now `Effectlist`.
- `Effectlist` enumerates direction-zero frame sets from `effect.efpk` through
  the shared `LegacyMapAnimationRenderer`.
- The available corpus contains 2994 valid effect frame IDs.
- Representative thumbnails resolve through `effect.aspk`, `effect.aspki`, and
  `effect.ppk` using the existing decoded-sprite and thumbnail caches.
- Selecting an effect chooses the Animation placement tool. Placement writes a
  Legacy animation object with `bltType=1`, the selected effect frame ID,
  verified frame count, and the resolved representative sprite ID.
- A frame that exists in EFPK but cannot decode its representative sprite is
  rejected explicitly; invalid map data is not created.

## Performance

The effect renderer previously used a linear sequence search for every
direction-zero resolve. `LegacyMapAnimationRenderer::Load` now creates one
persistent frame-ID-to-sequence-index lookup per alpha, shadow, and screen
effect pack. Runtime effect resolve, browser frame count, and visible thumbnail
metadata therefore use O(1) sequence lookup. The lookup lifetime matches the
loaded frame packs and is cleared only by `Clear`/resource reload.

## Files

- `Source/World/LegacyMapAnimationRenderer.h/.cpp`
- `Tool/MapEditor/MapEditorUiModel.h/.cpp`
- `Tool/MapEditor/MapEditorApp.cpp`
- `Source/Tests/MapEditorCoreTests.cpp`

## Validation

- Debug PhoenixMapEditor: PASS
- Release PhoenixMapEditor: PASS
- Debug PhoenixClient: PASS
- Release PhoenixClient: PASS
- Debug MapEditorCoreTests: PASS
- Release MapEditorCoreTests: PASS
- Map corpus: 378/378 PASS
- AFPK exact count/EOF validation: PASS
- Effect frame IDs: 2994
- Effect representative frame resolve and shared alpha-SPK decode: PASS
- Existing round-trip, split-SPK 8230, animation timing/origin, draw-list parity,
  screenshot, minimap, and Client shared-core tests: PASS

## Manual validation

- Effectlist label, item count, scrolling, thumbnails, search, and selection:
  `USER_MANUAL_VALIDATION_REQUIRED`.
- Effect placement and visual animation parity:
  `USER_MANUAL_VALIDATION_REQUIRED`.

No remaining technical parser defect was found.
