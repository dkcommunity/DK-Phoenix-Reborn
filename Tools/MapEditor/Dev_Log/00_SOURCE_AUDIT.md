# New Client Map Editor Source Audit

Date: 2026-07-13

Status: EVIDENCE_COLLECTED

## Required Prior Records Read

- `Dev_Log/00_PROJECT_MEMORY_RULE.md`
- `Dev_Log/01_DIAGNOSTICS_POLICY.md`
- `Dev_Log/EnterWorldInvestigation/00_MASTER_STATUS.md`
- `Dev_Log/EnterWorldInvestigation/12_WORLD_OBJECT_OCCLUSION_DRAW_ORDER_PARITY.md`
- `Dev_Log/EnterWorldInvestigation/18_COMBINED_INDEX_CACHE_FIX.md`
- `Dev_Log/EnterWorldInvestigation/22_MAP_TILE_WORLD_OBJECT_RESOURCE_PARITY_FIX.md`
- `Dev_Log/EnterWorldInvestigation/23_PHASE22_TILE_OBJECT_RUNTIME_REGRESSION.md`
- `Dev_Log/EnterWorldInvestigation/24_LEGACY_SECTOR_RESOURCE_ADMISSION_FIX.md`
- `Dev_Log/EnterWorldInvestigation/27_IMAGE_OBJECT_LEGACY_VISIBLE_SECTOR_FIX.md`
- `Dev_Log/EnterWorldInvestigation/28_TILE_OBJECT_Z_ORDER_RUNTIME_CAPTURE.md`
- `Dev_Log/EnterWorldInvestigation/29_TILE_OBJECT_Z_ORDER_CAPTURE_RESULT.md`
- `Dev_Log/EnterWorldInvestigation/30_RAW_MAP_RECORD_RESOURCE_IDENTITY_AUDIT.md`
- `Dev_Log/UI/Minimap_ZoneSprite_LegacyParity_20260713.md`

Legacy source was used only to verify contracts. No Legacy editor UI or MFC code is copied.

## 1. Map Load And Save

New Client load entry is `Application::PrepareWorldRuntimeAfterUpdate` in
`Source/Platform/Application.cpp`. It resolves the zone path, starts the map worker and calls
`world::LegacyMapReader::Load`. The format implementation is in
`Source/World/LegacyMapReader.{h,cpp}`. It accepts only `=MAP_2000_05_10=` and reads the zone
header, two sprite-list pointers, dimensions, row-major sectors, object count, object records,
per-object sector positions and both map-owned sprite file-position arrays.

New Client had no map writer before this task. The source-verified Legacy writer is
`MZoneEdit::SaveToFile` plus `SaveToFileViewInfo`; object records are written by
`MImageObject::SaveToFile`, `MAnimationObject::SaveToFile` and derived classes. The two pointer
slots are patched after the sprite file-position arrays are appended. The new shared writer must
preserve original bytes for an unchanged document and must not invent another format.

## 2. Tile And Object Structures

`LegacyMapSector` is exactly `{uint16 SpriteID, uint8 property, uint8 light}`. A map object starts
with wrapper type, then the `MObject` fields `(objectType, objectId, x, y)`, followed by
`(imageObjectId, spriteId, pixelX, pixelY, viewpoint, animation, transparent)`. Wrapper 5/6/7
adds frame ID, max frame, blit type, direction, sound frame, sound ID, loop, min/max delay and
start/end hour. Wrapper 7 adds interaction type. Every record ends with a uint16 count and that
many `(uint16 x,uint16 y)` sector positions. `LegacyMap`, `LegacyMapImageObject` and
`LegacyMapSector` are in `Source/World/LegacyMapReader.h`.

## 3. SPK And Split-SPK Lookup

`assets::SpriteArchiveReader` in `Source/Assets/SpriteArchiveReader.{h,cpp}` is authoritative.
For a directory archive it reads `header.inf`, computes
`splitBase=(SpriteID/unitCount)*unitCount`, resolves the zero-padded chunk and localizes the
combined `.spki` offsets. `ReadSplitSprite565` and `ReadSplitIndexedSprite565` use the immutable
combined-index cache introduced by Phase 18. Single archives use `ReadSprite565` and their
companion index directly.

## 4. Sprite ID Resolution

Map SpriteID is not remapped. `LegacyMapReader` stores it verbatim. Client tile/object upload
passes it to `SpriteArchiveReader`. Directory archives resolve combined ID to split base and
local index using the archive unit. Phase 30 verified this chain byte-for-byte for map records,
combined indexes and split chunks.

## 5. Frame, Origin And cX/cY

Raw SPK entries contain dimensions and compressed pixels; they do not contain frame origin.
Origin/cX/cY belongs to frame packs. `LegacyFrameRef` in `Source/Assets/LegacyAssetTypes.h` stores
`spriteId, originX, originY`; `FramePackReader` reads signed 16-bit cX/cY from six-byte frame
records. Legacy `CFrame::LoadFromFile` confirms the same `(SpriteID,cX,cY)` layout. Map animation
objects reference an animation frame sequence by `frameId` and `maxFrame`.

## 6. Coordinates

The active New Client contract is deliberately non-diamond: `IsometricProjection` is 48x24 and
`ProjectIsometric(x,y)` is `origin + (x*48,y*24)`. `WorldDrawListBuilder::Build` centers the
camera by subtracting the player's projected visual position and half one tile. Static object
anchors are `camera.origin + (pixelX,pixelY)`. `Application::ScreenToWorldTile` applies the exact
inverse floor division. The editor must call this shared builder at zoom 1; zoom is a display
transform after the parity list is built.

## 7. Draw Order

The verified runtime order is:

`pre-background Image Objects -> Tile surface -> row layers`

`WorldDrawListBuilder` assigns viewpoint-zero static objects to `PreBackground`, all other
objects to `Row`, and stable-sorts row entries by pass, legacy row, priority, primary value and
object ID. `Application::DrawGameFrame` submits pre-background quads, then tile quads, then skips
pre-background entries while emitting row layers. `DirectX11Renderer::DrawTexturedQuads`
preserves input order and only batches consecutive compatible quads.

## 8. Viewpoint, Layer And Pass

Object `viewpoint==0` means pre-background. Nonzero viewpoint is the row key. The source-verified
Legacy object key is `(viewpoint << 32)|objectId`; row submission is deterministic. Ground items,
creatures and effects have separate row priorities, but an offline map editor submits only map
static/animation objects around the tile surface. No editor-specific texture sort is permitted.

## 9. Cache And Ownership

`SpriteArchiveReader` owns process-wide, mutex-protected immutable archive-byte, atlas and
combined-index caches. Client GPU textures are owned by `Application`, released explicitly via
`DirectX11Renderer::ReleaseTexture` in `ClearWorldRuntimeAssets`, and decode workers transfer CPU
images to render-thread uploads. The editor must retain the same reader caches, own its texture
descriptors, release them before renderer shutdown, and decode only selected/visible sprites.

## 10. Collision, Portal, Spawn, Trigger And Regions

The `.map` client file stores tile `property` bits and object sector-position regions. Wrapper 7
stores `interactionType`. The audited client `.map` path contains no portal destination, spawn or
trigger table. Legacy `MZoneEdit` proves portals, safety regions, server blocking detail, sounds
and related gameplay data are written to sidecar info/server files, not this client map stream.
This editor therefore preserves tile property/light and object position regions and reports that
portal/spawn/trigger sidecars are outside the loaded format; it must not synthesize or clear them.

## Implementation Boundary

- Shared New Client code: map model/reader/writer, sprite reader/cache, frame readers,
  `WorldDrawListBuilder`, `DirectX11Renderer`.
- Editor-only code: document commands, Windows controls, selection, property presentation and
  editor input mapping.
- No Client packet, gameplay, runtime camera, cache policy or draw submission behavior is changed.
- Debug diagnostics use only the established compile-time boundary. Release has no audit overlay
  or verbose capture path.
