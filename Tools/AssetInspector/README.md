# AssetInspector

AssetInspector is the planned Windows-native inspection tool for legacy DK Phoenix Reborn assets.

## Panels

- Creature browser
- Animation preview
- Archive route viewer
- Shadow preview
- Skill/effect preview
- Item visual preview
- UI/background preview
- Audio preview

## Rules

- Legacy project roots are read-only.
- All generated output stays under `E:\DK Phoenix Reborn`.
- The tool uses shared `Source` readers, validators, content models, render queue, texture cache, audio, UI, and media contracts.
- No external dependencies are allowed.
