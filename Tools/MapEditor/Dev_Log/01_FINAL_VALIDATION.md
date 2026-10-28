# Phoenix Map Editor - Final Validation

Date: 2026-07-13

## Build results

- Debug PhoenixClient: PASS (`GameData/PhoenixClient_Debug.exe`)
- Debug PhoenixMapEditor: PASS (`Tool/MapEditor/bin/PhoenixMapEditor_Debug.exe`)
- Debug MapEditorCoreTests: PASS
- Release PhoenixClient: PASS (`GameData/PhoenixClient.exe`)
- Release PhoenixMapEditor: PASS (`Tool/MapEditor/bin/PhoenixMapEditor.exe`)
- Release MapEditorCoreTests: PASS
- Debug and Release editor startup smoke: PASS (process remains in the Win32 message loop)

Build command:

```powershell
cmake --build Build/windows-vs2022-x64 --config Debug --target PhoenixClient PhoenixMapEditor MapEditorCoreTests -- /m
cmake --build Build/windows-vs2022-x64 --config Release --target PhoenixClient PhoenixMapEditor MapEditorCoreTests -- /m
```

## Shared runtime paths

- Map read: `Source/World/LegacyMapReader.*`
- Map write/backup/unchanged payload preservation: `Source/World/LegacyMapWriter.*`
- Draw list, camera, projection, pass and Z-order: `Source/World/WorldDrawListBuilder.*`
- SPK/split-SPK lookup, decode and cache: `Source/Assets/SpriteArchiveReader.*`
- Animation object AFPK read: `Source/Assets/AnimationObjectFramePackReader.*`
- Animation frame selection and body/shadow offsets: `Source/World/LegacyMapAnimationRenderer.*`
- DX11 texture lifetime and submission: `Source/Backends/DirectX11/DirectX11Renderer.*`

PhoenixClient and PhoenixMapEditor link the same `PhoenixMapCore` implementation.

## Automated validation

`MapEditorCoreTests` result:

```text
PASS map=256x256 objects=8803 tiles=2576 layers=650 animation_map=gdr_illusion_01.map animation_frame=2 animation_frames=96 sprite8230=48x24
```

Validated behavior:

- `eslania_nw.map` load.
- Unchanged save is byte-identical.
- Modified save reloads and preserves object count.
- Map, tile and object property changes survive save/reload.
- Split-SPK sprite 8230 resolves as 48x24.
- Client/Editor camera, draw-list cardinality, pass and ordered layer fields match.
- Pre-background submissions precede row submissions.
- Real animation map `gdr_illusion_01.map` loads.
- Animation frame ID 2 resolves to the real 96-frame AFPK sequence.
- Body frame advances on the Legacy 62 ms global tick.
- Sprite ID and cX/cY match the AFPK frame record.
- Animation shadow resolves before body and uses the shadow frame pack.
- Loop, delay, hour, frame ID and max-frame fields survive save/reload.
- Animation map ordered draw pass parity matches between Client and Editor.

## Property panel

Editable map fields: name, zone ID, zone group, zone type, zone level and description.

Editable tile fields: sprite ID, property and light.

Editable Image/Animation Object fields: object type, wrapper type, sprite ID, wall direction/value, pixel X/Y, viewpoint, transparency, frame ID, max frame, blt type, direction, sound frame/ID, loop, min/max delay, start/end hour and interaction type.

Every accepted edit enters the snapshot undo/redo history, sets dirty state, refreshes resource/index state where required and uses the Legacy save/reload path. Object movement changes sector position records, pixel anchor and viewpoint without corrupting the Legacy wall fields.

## Release compile-out

Generated Release projects for `PhoenixMapCore`, `PhoenixClient` and `PhoenixMapEditor` contain `NDEBUG` and do not define `PHOENIX_ENABLE_DIAGNOSTICS`. `MapEditorCoreTests` is a separate executable and is not linked into either product binary. Debug-only source offsets and diagnostic counters remain behind `_DEBUG && PHOENIX_ENABLE_DIAGNOSTICS`.

## Windows UI validation

Automated visual interaction was stopped by physical Escape. Per user direction, the final visual interaction matrix is user-owned. This does not invalidate the completed builds, startup smoke tests or automated map/render tests.

The manual matrix is: open/save/save-as, pan/zoom, tile and object tools, multi-select, copy/paste, undo/redo, property editing, single/split SPK browser, minimap, dirty warning, resize/maximize/DPI, mouse capture, keyboard focus and viewport exclusion over child UI.

## Remaining gaps

- Product implementation/build/test gaps found by the automated suite: none.
- Pending external verification: user-run visual Windows interaction matrix above.
