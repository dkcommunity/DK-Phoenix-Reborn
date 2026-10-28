#pragma once

#include "UI/Layout/LayoutViewport.h"
#include "UI/UiTypes.h"

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace phoenix::ui
{
    struct ClientUIEditorEntry
    {
        std::string key;
        UiRect rect;
    };

    class ClientUIEditor final
    {
    public:
        void Load(const std::filesystem::path& path);
        void Reload(const std::filesystem::path& path);
        void Save(const std::filesystem::path& path, const std::vector<ClientUIEditorEntry>& knownEntries) const;

        [[nodiscard]] bool Enabled() const noexcept;
        void ToggleEnabled() noexcept;

        [[nodiscard]] UiRect Rect(const std::string& key, UiRect fallback) const;
        [[nodiscard]] const std::optional<std::string>& SelectedKey() const noexcept;
        [[nodiscard]] const std::set<std::string>& SelectedKeys() const noexcept;
        [[nodiscard]] std::optional<UiRect> MarqueeSelection() const noexcept;
        void SelectKey(std::string key);
        void ToggleSelectionKey(std::string key);

        bool BeginPointer(std::int32_t screenX, std::int32_t screenY, const std::vector<ClientUIEditorEntry>& entries, LayoutTransform transform);
        void UpdatePointer(std::int32_t screenX, std::int32_t screenY, LayoutTransform transform) noexcept;
        void EndPointer() noexcept;
        void SelectNext(const std::vector<ClientUIEditorEntry>& entries);
        bool NudgeSelected(std::int32_t dx, std::int32_t dy, bool resize, const std::vector<ClientUIEditorEntry>& entries);

    private:
        [[nodiscard]] UiRect ScreenToReferenceRect(UiRect rect, LayoutTransform transform) const noexcept;
        [[nodiscard]] std::pair<std::int32_t, std::int32_t> ScreenToReferencePoint(std::int32_t screenX, std::int32_t screenY, LayoutTransform transform) const noexcept;

        std::map<std::string, UiRect> overrides_;
        std::optional<std::string> selectedKey_;
        std::set<std::string> selectedKeys_;
        std::map<std::string, UiRect> dragStartRects_;
        std::vector<ClientUIEditorEntry> pointerEntries_;
        std::int32_t dragStartX_{ 0 };
        std::int32_t dragStartY_{ 0 };
        std::int32_t marqueeStartX_{ 0 };
        std::int32_t marqueeStartY_{ 0 };
        std::int32_t marqueeCurrentX_{ 0 };
        std::int32_t marqueeCurrentY_{ 0 };
        bool enabled_{ false };
        bool dragging_{ false };
        bool resizing_{ false };
        bool marqueeSelecting_{ false };
    };
}
