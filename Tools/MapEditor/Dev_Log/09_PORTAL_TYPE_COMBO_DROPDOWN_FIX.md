# Portal Type ComboBox dropdown açılmıyor

**Tarih:** 2026-08-26  
**Semptom:** Portal Type satırında dropdown menü hiç açılmıyor; sayılar görünmüyor. Enum eksikliği değil.

## Kök neden

`CBS_DROPDOWNLIST` açılırken `WM_KILLFOCUS` sık sık `wParam=NULL` (veya list HWND henüz yok) ile gelir. Property edit subclass bunu commit sanıp `DestroyWindow` ediyordu → liste hiç görünmeden yok oluyordu.

## Düzeltme

- Combo edit: `WM_KILLFOCUS` ile commit yok
- Commit yalnız `CBN_SELCHANGE` / `CBN_CLOSEUP` / Enter
- Açılışta `CB_SHOWDROPDOWN` + `HWND_TOP` z-order
- Properties çift tık: kolon 0/1 fark etmeksizin edit
- Commit handler: dropdown hâlâ açıksa destroy etme

**Binary:** `Tool\MapEditor\bin\PhoenixMapEditor_Release.exe`
