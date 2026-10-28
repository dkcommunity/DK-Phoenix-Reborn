// ItemTooltip.h
//
// Legacy DID_ITEM tooltip registration for DescriptorManager.
// Modern item model: ItemRuntimeRecord (runtime data) + ItemInfoCatalog (static item.inf data).

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Runtime/GameRuntimeModels.h"
#include "Content/ItemInfoCatalog.h"

namespace phoenix::platform
{
class Application;
}

namespace phoenix::ui
{

// Context for item tooltip
struct ItemTooltipContext
{
    phoenix::platform::Application* app = nullptr;
    const content::ItemInfoCatalogResult* itemCatalog = nullptr;
    std::function<const runtime::ItemRuntimeRecord*(std::uint32_t objectId)> findItemById;
    // Render callback: called per frame from DescriptorManager::Show()
    // lines  — tooltip text lines to display
    // x, y   — tooltip top-left position (reference coords)
    // w, h   — tooltip dimensions
    std::function<void(const std::vector<std::string>& lines, int x, int y, int w, int h)> renderFn;
};

// Register DID_ITEM tooltip with DescriptorManager
// Call once at init (e.g., Application::InitializeRuntime)
// ctx is copied to heap internally; caller does not need to keep it alive.
void RegisterItemTooltip(const ItemTooltipContext& ctx);

// Tear down tooltip context (call at shutdown to avoid dangling pointers)
void UnregisterItemTooltip();

} // namespace phoenix::ui