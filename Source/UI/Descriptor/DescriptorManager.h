// DescriptorManager.h
//
// Legacy DescriptorManager port — hover tooltip motoru.
// Kaynak: F:\Project Phoenix\Designed_Common\Project\NEW - VS_UI\src\
//   VS_UI_Descriptor.h / VS_UI_Descriptor.cpp
//
// Modern port: Legacy bağımlılıklari (VS_UI_Base.h, SimpleDataList, DXLib,
// gpC_base, CT_Done) kaldirildi. Cekirdek mantik korundu:
//   - AddDescribedUnit(id, fp_rect_calc, fp_show) ile tooltip type register
//   - Set(id, x, y, ptr) ile tooltip göster
//   - Unset(ptr) ile tooltip kapat
//   - Show() her frame render loop'ta çağrılır
//
// Descriptor ID'leri (VS_UI_Description.h'den):
//   DID_ITEM=1, DID_SKILL=2, DID_HELP=3, DID_SKILLTREE=4, DID_INFO=5,
//   DID_STRINGS=6, DID_BLOOD_BIBLE_STATUS=7, DID_MULTILINE_INFO=8

#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <functional>

namespace legacy {

// Descriptor ID type (Legacy: typedef DWORD id_t)
using id_t = std::uint32_t;

// Tooltip pozisyon rect (Legacy: BasicData.h Rect class)
struct Rect
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

// Descriptor ID sabitleri (VS_UI_Description.h)
inline constexpr id_t DID_ITEM              = 1;
inline constexpr id_t DID_SKILL             = 2;
inline constexpr id_t DID_HELP              = 3;
inline constexpr id_t DID_SKILLTREE         = 4;
inline constexpr id_t DID_INFO              = 5;
inline constexpr id_t DID_STRINGS           = 6;
inline constexpr id_t DID_BLOOD_BIBLE_STATUS = 7;
inline constexpr id_t DID_MULTILINE_INFO    = 8;

// Tooltip render callback imzasi
//   rect   — tooltip konumu (ekran koordinatlari)
//   ptr    — tooltip veri pointer (item/skill/help payload)
//   left   — ekstra left parametre (Legacy uyumu)
//   right  — ekstra right parametre (Legacy uyumu)
using TooltipShowFn = void(*)(Rect rect, void* ptr, long left, long right);

// Tooltip rect hesaplama callback — tooltip text/sprite build + pozisyon hesap
//   fp_show       — final render callback
//   x, y          — mouse pozisyonu
//   ptr           — tooltip veri pointer
//   left, right   — ekstra parametreler
//   z_order       — render z-order
using TooltipRectCalcFn = void(*)(TooltipShowFn fp_show, int x, int y, void* ptr, long left, long right, int z_order);

// Register edilen tooltip type tanimi
struct DescribedUnit
{
    id_t                  id              = 0;
    bool                  bl_immediate    = true;   // immediate(true)/delay(false)
    TooltipRectCalcFn     fp_rect_calculator = nullptr;
    TooltipShowFn         fp_show            = nullptr;
};

// DescriptorManager — singleton tooltip motoru
//
// Legacy: g_descriptor_manager (global). Modern: DescriptorManager::Instance()
// kullanim. Isterseniz global alias tanimlanabilir.
//
// API uyumu Legacy ile birebir:
//   Set(id, x, y, ptr, left=0, right=0, add=false, z_order=0)
//   Unset(ptr=NULL)
//   Show()
//   AddDescribedUnit(id, fp_calc, fp_show, bl_immediate=true)
//   RectCalculationFinished(fp_show, rect, ptr, left, right, z_order)
//   GetDesc(ptr) const
class DescriptorManager
{
public:
    // Active tooltip description
    struct DESC
    {
        TooltipShowFn m_fp_show = nullptr;
        struct FP_SHOW_PARAM
        {
            Rect   rect;
            void*  void_ptr = nullptr;
            long   left  = 0;
            long   right = 0;
        } m_fp_show_param;
    };

    static DescriptorManager& Instance();

    // Tooltip type register (init sirasinda bir kere)
    void AddDescribedUnit(id_t id, TooltipRectCalcFn fp_calc, TooltipShowFn fp_show, bool bl_immediate = true);

    // Tooltip göster — mouse hover event'inden çağrılır
    void Set(id_t id, int x, int y, void* void_ptr, long left = 0, long right = 0, bool add = false, int z_order = 0);

    // Tooltip kapat — mouse leave event'inden çağrılır
    // ptr=NULL ise tüm tooltip'leri temizle
    void Unset(void* pPtr = nullptr);

    // Her frame render loop'ta çağrılır — active tooltip'leri çiz
    void Show();

    // Lookup active tooltip by data pointer
    const DESC* GetDesc(void* void_ptr) const;

    // RectCalculationFinished — fp_rect_calculator içinden çağrılır
    // (回调 köprü: rect hesaplandiktan sonra DESC'yi queue'ya koy)
    void RectCalculationFinished(TooltipShowFn fp_show, Rect rect, void* void_ptr, long left, long right, int z_order);

private:
    DescriptorManager() = default;
    ~DescriptorManager();
    DescriptorManager(const DescriptorManager&) = delete;
    DescriptorManager& operator=(const DescriptorManager&) = delete;

    std::vector<DescribedUnit*>   m_units;      // registered DescribedUnit list
    // Legacy: std::multimap<int, DESC> — z_order'a göre sırali
    using DescMap = std::multimap<int, DESC>;
    DescMap m_descMap;
};

// Global alias — Legacy uyumu için (g_descriptor_manager)
DescriptorManager& g_descriptor_manager();

} // namespace legacy
