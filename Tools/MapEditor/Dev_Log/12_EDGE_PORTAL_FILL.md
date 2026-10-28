# Kenar portal dolgusu — 2026-09-01

## Niyet

Boydan boya harita kenarı portallarını kare kare `zone:x:y` yazmadan doldurmak.
Seçilen dikdörtgene dest ZoneID + dest start + dest end bir kez verilir; her kareye
`.mip`/`.smp` kare-kare hedef yazılır.

## Root cause

Runtime her sektörü ayrı portal okur. Kenar geçişinde dest X/Y sütun/satır boyunca
artmalıdır. Editor yalnızca tek kare `zone:x:y` kabul ediyordu. MIP rect + bilinmeyen
SMP dest Save’te `X/Y=0,0` yazar (`34:0:0`).

## Fix

- `FillEdgePortals`: uzun eksende tamsayı lerp, kalınlık aynı dest’i kopyalar.
  `destinationKnown=true`. Type 3/4 yok. Sprite/collision dokunulmaz. Tek undo.
- Portal aracı sürükleyerek dikdörtgen seçer. Enter / Tools → Fill Edge Portal.
- Shift+RClick: seçimdeki portal kayıtlarını siler.

## Race

Type 0/1/2/5/6/7 aynı dest şeridi. 1/2/7 ayrı boyanır.

## Doğrulama

Debug + Release `PhoenixMapEditor_*.exe` derlendi.
Zone 61 verisi bu işte patch edilmedi.
