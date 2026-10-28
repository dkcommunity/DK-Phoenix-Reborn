// ItemTooltip.cpp
//
// DID_ITEM tooltip implementation for DescriptorManager.
// Uses legacy::TooltipShowFn / TooltipRectCalcFn signatures.

#include "UI/Descriptor/ItemTooltip.h"
#include "UI/Descriptor/DescriptorManager.h"
#include "Runtime/GameRuntimeModels.h"
#include "Content/ItemInfoCatalog.h"

#include <string>
#include <vector>
#include <algorithm>

namespace phoenix::ui
{

ItemTooltipContext* g_itemTooltipCtx = nullptr;
bool g_itemTooltipCtxOwned = false;  // true if we new'd it (need to delete)

// Build tooltip text lines from ItemRuntimeRecord + ItemInfo
std::vector<std::string> BuildItemTooltipLines(const phoenix::runtime::ItemRuntimeRecord& item,
                                                const phoenix::content::ItemInfoCatalogResult* catalog)
{
    std::vector<std::string> lines;

    // 1. Item name
    std::string name;
    if (catalog && catalog->Succeeded())
    {
        if (const auto* info = catalog->Find(item.itemClass, item.itemType))
        {
            name = info->displayName.empty() ? info->englishName : info->displayName;
        }
    }
    if (name.empty())
    {
        name = "Item(" + std::to_string(item.itemClass) + "," + std::to_string(item.itemType) + ")";
    }
    lines.push_back(name);

    // 2. Durability
    if (item.durability > 0 || item.maxNumber > 0)
    {
        lines.push_back("Durability: " + std::to_string(item.durability) + " / " + std::to_string(item.maxNumber));
    }

    // 3. Silver
    if (item.silver > 0)
    {
        lines.push_back("Silver: " + std::to_string(item.silver));
    }

    // 4. Grade
    if (item.grade > 0)
    {
        lines.push_back("Grade: " + std::to_string(item.grade));
    }

    // 5. Enchant
    if (item.enchantLevel > 0)
    {
        lines.push_back("Enchant: +" + std::to_string(item.enchantLevel));
    }

    // 6. Options
    if (!item.options.empty())
    {
        lines.push_back("Options:");
        for (auto opt : item.options)
        {
            lines.push_back("  Option " + std::to_string(opt));
        }
    }

    // 7. Requirements
    if (item.requireStr > 0 || item.requireDex > 0 || item.requireInt > 0 || item.requireLevel > 0)
    {
        std::string req = "Req:";
        if (item.requireStr > 0) req += " STR " + std::to_string(item.requireStr);
        if (item.requireDex > 0) req += " DEX " + std::to_string(item.requireDex);
        if (item.requireInt > 0) req += " INT " + std::to_string(item.requireInt);
        if (item.requireLevel > 0) req += " Lv " + std::to_string(item.requireLevel);
        lines.push_back(req);
    }

    return lines;
}

// Calculate tooltip rect from text lines
legacy::Rect CalculateTooltipRect(int mouseX, int mouseY, const std::vector<std::string>& lines,
                                  int charW = 8, int lineH = 14, int padX = 8, int padY = 4)
{
    int maxW = 0;
    for (const auto& l : lines)
        maxW = std::max(maxW, static_cast<int>(l.size()));

    int w = maxW * charW + padX * 2;
    int h = lines.size() * lineH + padY * 2;

    int baseX = mouseX + 16;
    int baseY = mouseY + 8;

    return { baseX, baseY, w, h };
}

// Rect calculator callback (fp_rect_calculator)
// Called from DescriptorManager::Set -> finds item, builds text, computes rect, calls RectCalculationFinished
void ItemTooltipRectCalculator(legacy::TooltipShowFn fp_show, int x, int y, void* void_ptr, long left, long right, int z_order)
{
    if (!g_itemTooltipCtx || !void_ptr)
        return;

    const phoenix::runtime::ItemRuntimeRecord* item = nullptr;

    // void_ptr can be:
    // - ItemRuntimeRecord* (direct pointer from grid hover)
    // - uint32_t* (objectId pointer from gear/quickslot hover)
    auto* ptrAsItem = static_cast<const phoenix::runtime::ItemRuntimeRecord*>(void_ptr);
    if (ptrAsItem && ptrAsItem->itemClass > 0 && ptrAsItem->itemClass < 255)
    {
        item = ptrAsItem;
    }
    else if (g_itemTooltipCtx->findItemById)
    {
        // Try as objectId
        auto* ptrAsObjId = static_cast<const std::uint32_t*>(void_ptr);
        if (ptrAsObjId && *ptrAsObjId != 0)
        {
            item = g_itemTooltipCtx->findItemById(*ptrAsObjId);
        }
    }

    if (!item)
        return;

    // Build tooltip text
    auto lines = BuildItemTooltipLines(*item, g_itemTooltipCtx->itemCatalog);

    // Calculate rect at mouse position
    legacy::Rect rect = CalculateTooltipRect(x, y, lines);

    // Register with DescriptorManager
    legacy::DescriptorManager::Instance().RectCalculationFinished(fp_show, rect, void_ptr, left, right, z_order);
}

// Show callback (fp_show) — called per frame from DescriptorManager::Show()
void ItemTooltipShow(legacy::Rect rect, void* void_ptr, long left, long right)
{
    (void)left; (void)right;

    if (!g_itemTooltipCtx || !void_ptr)
        return;

    const phoenix::runtime::ItemRuntimeRecord* item = nullptr;
    auto* ptrAsItem = static_cast<const phoenix::runtime::ItemRuntimeRecord*>(void_ptr);
    if (ptrAsItem && ptrAsItem->itemClass > 0 && ptrAsItem->itemClass < 255)
    {
        item = ptrAsItem;
    }
    else if (g_itemTooltipCtx->findItemById)
    {
        auto* ptrAsObjId = static_cast<const std::uint32_t*>(void_ptr);
        if (ptrAsObjId && *ptrAsObjId != 0)
        {
            item = g_itemTooltipCtx->findItemById(*ptrAsObjId);
        }
    }

    if (!item)
        return;

    // Build lines again for rendering
    auto lines = BuildItemTooltipLines(*item, g_itemTooltipCtx->itemCatalog);

    // Render via callback (bound from Application)
    if (g_itemTooltipCtx->renderFn)
    {
        // Recalculate rect for render (rect from DESC already has position)
        g_itemTooltipCtx->renderFn(lines, rect.x, rect.y, rect.w, rect.h);
    }
}

// Public API
void RegisterItemTooltip(const ItemTooltipContext& ctx)
{
    // Delete previous context if owned
    if (g_itemTooltipCtxOwned && g_itemTooltipCtx)
    {
        delete g_itemTooltipCtx;
    }
    // Heap-allocate a copy so it survives beyond the caller's scope
    g_itemTooltipCtx = new ItemTooltipContext(ctx);
    g_itemTooltipCtxOwned = true;

    legacy::DescriptorManager::Instance().AddDescribedUnit(
        legacy::DID_ITEM,
        &ItemTooltipRectCalculator,
        &ItemTooltipShow,
        true  // immediate
    );
}

void UnregisterItemTooltip()
{
    if (g_itemTooltipCtxOwned && g_itemTooltipCtx)
    {
        delete g_itemTooltipCtx;
    }
    g_itemTooltipCtx = nullptr;
    g_itemTooltipCtxOwned = false;
}

} // namespace phoenix::ui