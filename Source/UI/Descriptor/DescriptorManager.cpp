// DescriptorManager.cpp
//
// Legacy VS_UI_Descriptor.cpp port — Modern projeye uyarlandi.
// Kaynak: F:\Project Phoenix\Designed_Common\Project\NEW - VS_UI\src\
//   VS_UI_Descriptor.cpp
//
// Port notlari:
//   - CT_Donne trace logger kaldirildi (Modern tracing ayrica eklenebilir)
//   - gpC_base->m_p_DDSurface_back surface lock check kaldirildi (DX11)
//   - SimpleDataList<DESCRIBED_UNIT*> yerine std::vector<DescribedUnit*>
//   - assert() kaldi (debug build'de aktif)
//   - Legacy null-guard semantics korundu

#include "DescriptorManager.h"

namespace legacy {

DescriptorManager& DescriptorManager::Instance()
{
    static DescriptorManager instance;
    return instance;
}

DescriptorManager::~DescriptorManager()
{
    for (auto* unit : m_units)
        delete unit;
    m_units.clear();
}

void DescriptorManager::AddDescribedUnit(id_t id, TooltipRectCalcFn fp_calc, TooltipShowFn fp_show, bool bl_immediate)
{
    // Aynı id tekrar register edilmez
    for (auto* unit : m_units)
    {
        if (unit && unit->id == id)
            return;
    }

    auto* p_unit = new DescribedUnit;
    p_unit->id                 = id;
    p_unit->bl_immediate       = bl_immediate;
    p_unit->fp_rect_calculator = fp_calc;
    p_unit->fp_show            = fp_show;

    m_units.push_back(p_unit);
}

void DescriptorManager::Set(id_t id, int x, int y, void* void_ptr, long left, long right, bool add, int z_order)
{
    // add=false ise ve zaten active tooltip varsa atla (Legacy: "doing?" check)
    if (!add && !m_descMap.empty())
        return;

    for (auto* unit : m_units)
    {
        if (unit && unit->id == id)
        {
            // Legacy: gpC_base surface NULL guard — Modern'de gerekmez
            if (unit->fp_rect_calculator != nullptr)
            {
                unit->fp_rect_calculator(unit->fp_show, x, y, void_ptr, left, right, z_order);
            }
            return;
        }
    }
    // id bulunamadi — sessiz geç (Legacy'de CT_Done log var idi)
}

const DescriptorManager::DESC* DescriptorManager::GetDesc(void* void_ptr) const
{
    for (auto it = m_descMap.begin(); it != m_descMap.end(); ++it)
    {
        const DESC& desc = it->second;
        if (desc.m_fp_show_param.void_ptr == void_ptr)
            return &desc;
    }
    return nullptr;
}

void DescriptorManager::Unset(void* pPtr)
{
    if (pPtr == nullptr)
    {
        m_descMap.clear();
        return;
    }

    for (auto it = m_descMap.begin(); it != m_descMap.end(); ++it)
    {
        const DESC& desc = it->second;
        if (desc.m_fp_show_param.void_ptr == pPtr)
        {
            m_descMap.erase(it);
            break;
        }
    }
}

void DescriptorManager::Show()
{
    for (auto it = m_descMap.begin(); it != m_descMap.end(); ++it)
    {
        DESC& desc = it->second;
        if (desc.m_fp_show)
        {
            desc.m_fp_show(
                desc.m_fp_show_param.rect,
                desc.m_fp_show_param.void_ptr,
                desc.m_fp_show_param.left,
                desc.m_fp_show_param.right
            );
        }
    }
}

void DescriptorManager::RectCalculationFinished(
    TooltipShowFn fp_show,
    Rect          rect,
    void*         void_ptr,
    long          left,
    long          right,
    int           z_order)
{
    DESC desc;
    desc.m_fp_show                = fp_show;
    desc.m_fp_show_param.void_ptr = void_ptr;
    desc.m_fp_show_param.rect      = rect;
    desc.m_fp_show_param.left      = left;
    desc.m_fp_show_param.right     = right;
    m_descMap.insert(DescMap::value_type(z_order, desc));
}

// Global alias — Legacy uyumu için
DescriptorManager& g_descriptor_manager()
{
    return DescriptorManager::Instance();
}

} // namespace legacy
