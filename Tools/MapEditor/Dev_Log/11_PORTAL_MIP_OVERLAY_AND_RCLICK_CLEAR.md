# Portal MIP overlay + RClick clear — 2026-08-31

## Niyet

MapEditor `.mip` sidecar portal kaydını (ör. `tigers_lair` (20,11) → 59999) haritada göstermiyordu.
Portal aracı seçiliyken bir kez eklenen portal da silinemiyordu.

## Root cause

Portal `PORTAL` etiketi `visibleTileIndices_` üzerinden çiziliyordu. Bu liste yalnızca
`EditorWorldDrawListBuilder` içinde çözülebilen tile sprite’ları içerir (`0xFFFF` / arşiv dışı ID
`MissingIndex`). Sidecar `.mip` portalı tile sprite’ına bağlı değildir; boş veya çözülemeyen
karedeki MIP kaydı modelde durur, overlay atlanır.

Silme: Delete/Erase tile sprite veya object siler. Portal aracı RClick hiçbir şey yapmazdı.
Property `Portal enabled=0` sidecar’ı temizler ama görünmeyen karede seçilemezdi.

## Fix

- Overlay: görünür sektör karesini sidecar `LegacyServerSector::portal` ile tarar; draw-list
  sprite şartı yok.
- Minimap: `Portals` katmanı açıkken portal kareleri mavi işaretlenir.
- Portal aracı + sağ tık: `ClearPortalOnTile` — yalnızca `portal.reset()`, tile sprite/light/block
  dokunulmaz. Undo/redo history’de.

## Race parity

Portal type 0–7 / 200–203 / 255 aynı sidecar modelinde. Overlay ve RClick ırk ayırmaz; type 1/2/7
kayıtları da aynı şekilde görünür ve silinir.

## Doğrulama

PhoenixMapEditor Debug derlemesi başarılı (`Tool/MapEditor/bin/PhoenixMapEditor_Debug.exe`).
Release, MapCore `PHOENIX_ENABLE_DIAGNOSTICS` PUBLIC iken editor stub’unun yalnızca `_DEBUG` olması yüzünden LNK2019 veriyordu; `EditorPhoenixDiagnostics.cpp` artık aynı makroya bağlı.
MapEditorCoreTests CMake’de kapalı; headless test çalıştırılmadı.
Kullanıcı: `tigers_lair` (20,11) PORTAL etiketi + RClick sonrası kaydın gitmesi.
