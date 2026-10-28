# Legacy Server Sidecars And Portal Editing

## Root cause

The editor document represented only the client `.map` stream. Portal destinations, safety
regions and server-sector records therefore had no owner in the shared model, and `Save` called
only `LegacyMapWriter`.

The fix adds an explicit sidecar model to `LegacyMap`, while keeping sidecar loading opt-in so the
Client's existing `.map` load behavior is unchanged.

## Source-verified Legacy contract

- `.smp`: zone header, width/height, one server flag byte per sector, followed immediately by
  portal type/count and `(zoneId, x, y)` targets when bit `0x80` is present.
- `.mip`: width/height, portal rectangles with type and zone IDs, safety rectangles, then any
  trailing Legacy payload.
- `.ssi`: safety rectangle count and packed five-byte records.
- Tile property bits `0x01/0x02/0x04` are the underground/ground/flying flags used to generate
  `.smp` collision bytes.
- Portal types below `200` are server portals. Types `200..203` and `255` remain client-only
  `.mip` records.
- Legacy `.mip` loading is file ordered: a later overlapping portal rectangle replaces earlier
  sector values. Out-of-map cells are skipped.

## Implementation

- `Source/World/LegacyMapSidecar.*` owns sidecar parsing, validation, serialization and atomic
  four-file bundle commits.
- Existing sidecar bytes are preserved when unchanged. Known portal/safety sections are rebuilt
  only after an actual sidecar edit; trailing payload remains byte-preserved.
- Existing `.map/.smp/.ssi/.mip` files receive individual `.bak` files before commit.
- Missing server portal destination X/Y is reported and save is rejected. No fallback destination
  is synthesized.
- `MapEditHistory` captures sidecar state, so portal, safety and server collision edits participate
  in undo/redo and dirty-state handling.
- The Portal tool selects sectors without triggering object or paint actions. The property panel
  exposes collision flags, four safety flags, portal enabled state, portal type and targets in
  `zone:x:y;zone:x:y` form.
- The toolbar includes Portal editing and an independent Portals visibility toggle. Portal markers
  are editor overlays and are not rendered into the minimap.

## Validation

- Debug Editor, Debug Client and Debug headless tests: build passed.
- Release Editor, Release Client and Release headless tests: build passed.
- 378-map client corpus: passed.
- All 352 companion `.mip` files: parsed successfully.
- `.map/.smp/.ssi/.mip` bundle round-trip: passed.
- Portal destination, safety flags, client-only portal exclusion, backup identity and portal
  undo/redo tests: passed.
- Unknown server destination rejection: passed.
- Release diagnostic string scan: passed.
- GUI executable was not launched. Visual interaction remains `USER_MANUAL_VALIDATION_REQUIRED`.

## Performance

Sidecars are read once when a map opens and written only on Save/Save As. Portal overlays reuse the
cached world draw list and visible-tile indices. Portal property edits do not rebuild the world draw
list, regenerate the minimap or repopulate sprite palettes.

## Toolbar and portal label follow-up

- The Layers group is anchored to the right edge of the top toolbar. On narrow windows it remains
  right-aligned on the first row while file and zoom commands use the second row.
- Portal sectors render a persistent `PORTAL` bitmap label instead of the previous white square.
- The label texture is generated and uploaded once during renderer initialization, reused for every
  visible portal, and released during renderer shutdown.
- Debug and Release Editor builds and both headless parity test configurations passed after the
  change. Visual placement remains `USER_MANUAL_VALIDATION_REQUIRED`.
