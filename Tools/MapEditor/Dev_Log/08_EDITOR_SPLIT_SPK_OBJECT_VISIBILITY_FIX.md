# Editor Split-SPK Object Visibility Fix

Date: 2026-07-15
Status: IMPLEMENTED_AND_USER_VISUALLY_VERIFIED

## Reported Regression

Map: `GameData/data/Map/eslania_nw.map`

Objects reported as invisible:

- ObjectID 21014
- ObjectID 21015
- ObjectID 21016
- ObjectID 21017
- ObjectID 21018
- ObjectID 21019

## Verified Root Cause

All six map records were parsed correctly and admitted to the Editor full-map
draw list. The failure occurred during sprite decoding.

The records reference Image Object sprites 5813 through 5818 in
`imageobject.spk/05808.spk`. The combined `imageobject.spki` offsets selected
incorrect positions inside this split chunk. The split chunk's own Legacy
layout, consisting of a U16 record count followed by sequential sprite
records, resolves all six sprites correctly.

## Editor-Only Fix

Added:

- `Tool/MapEditor/Core/EditorSpriteArchiveReader.h`
- `Tool/MapEditor/Diagnostics/InspectMapObjects.cpp`

Updated:

- `Tool/MapEditor/MapEditorApp.cpp`

The Editor now uses `EditorSpriteArchiveReader`. For directory-based regular
and shadow split-SPK archives, it uses the chunk-local record order instead of
the incompatible combined-index offsets. Single-file SPK and paletted effect
archive behavior remains unchanged.

No Client `Source` file was modified for this fix. The New Client was not
built or executed.

## Headless Regression Result

| ObjectID | SpriteID | Decode result |
|---:|---:|---:|
| 21014 | 5813 | 46x750 |
| 21015 | 5814 | 46x936 |
| 21016 | 5815 | 46x975 |
| 21017 | 5816 | 46x995 |
| 21018 | 5817 | 46x1028 |
| 21019 | 5818 | 46x1093 |

Result: `PASS editor_object_visibility failures=0`

## Build Result

- PhoenixMapEditor Debug x64: PASS
- PhoenixMapEditor Release x64: PASS

## Visual Acceptance

Codex did not launch the Windows GUI. The user manually verified the rebuilt
Editor and confirmed: `Şimdi herşey görünüyor.`

