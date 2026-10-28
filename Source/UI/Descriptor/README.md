# Descriptor / Tooltip Sistemi — Modern Port

Legacy `DescriptorManager` (hover tooltip) ve `C_VS_UI_DESC_DIALOG` (popup description) sistemlerinin Modern port'u.

## Dosyalar

### Çekirdek (port edildi)

| Dosya | Legacy Kaynak | Açıklama |
|-------|---------------|----------|
| `DescriptorManager.h` | `VS_UI_Descriptor.h` | Singleton tooltip motoru — `Set/Unset/Show/AddDescribedUnit` API |
| `DescriptorManager.cpp` | `VS_UI_Descriptor.cpp` | Implementasyon — Legacy bağımlılıkları temizlendi (DXLib, gpC_base, CT_Done) |

### Henüz port edilmedi (referans)

| Legacy Dosya | Açıklama | Port Durumu |
|-------------|----------|-------------|
| `VS_UI_Description.h/.cpp` | `g_InstallDescriptor()` — DID_* callback'leri register eder (item/skill/help tooltip render) | ❌ Bekliyor — Legacy `VS_UI_GameCommon.h` bağımlılığı çok ağır |
| `VS_UI_Desc.h/.cpp` | `C_VS_UI_DESC` — text + sprite description helper (scroll, color, title) | ❌ Bekliyor — `RarFile.h` bağımlılığı |
| `VS_UI_Dialog.h/.cpp` | `C_VS_UI_DIALOG` — genel dialog widget (scroll bar, button group) | ❌ Bekliyor — `Unicorn.h` bağımlılığı |
| `VS_UI_ExtraDialog.h/.cpp` | `C_VS_UI_DESC_DIALOG` — popup description penceresi | ❌ Bekliyor — `VS_UI_Dialog` + `VS_UI_Desc` bağımlı |

## Kullanım

```cpp
#include "UI/Descriptor/DescriptorManager.h"

// Init — tooltip type'larını register et (bir kere)
legacy::DescriptorManager::Instance().AddDescribedUnit(
    legacy::DID_ITEM,
    &MyItemRectCalculator,    // tooltip rect + text build
    &MyItemTooltipRender,     // final render
    true                       // immediate
);

// Mouse hover — tooltip göster
legacy::DescriptorManager::Instance().Set(
    legacy::DID_ITEM, x, y, (void*)pItem
);

// Mouse leave — tooltip kapat
legacy::DescriptorManager::Instance().Unset((void*)pItem);

// Render loop — her frame çağrılır
legacy::DescriptorManager::Instance().Show();

// Legacy uyumlu global alias
legacy::g_descriptor_manager().Set(legacy::DID_ITEM, x, y, ptr);
```

## Sonraki Adım

`DID_ITEM` tooltip render callback'i yazılmadı — `MyItemRectCalculator` ve `MyItemTooltipRender` fonksiyonları Modern item modelini (`gameSession_.items`) kullanarak tooltip text + rect build etmeli.
