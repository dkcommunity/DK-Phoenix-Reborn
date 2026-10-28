#include "UI/Windows/ClientUIEditor.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <sstream>

namespace phoenix::ui
{
    namespace
    {
        bool Contains(const UiRect& rect, const std::int32_t x, const std::int32_t y) noexcept
        {
            return x >= rect.x && y >= rect.y && x < rect.x + rect.width && y < rect.y + rect.height;
        }

        UiRect NormalizeRect(const UiRect rect) noexcept
        {
            const auto left = std::min(rect.x, rect.x + rect.width);
            const auto top = std::min(rect.y, rect.y + rect.height);
            const auto right = std::max(rect.x, rect.x + rect.width);
            const auto bottom = std::max(rect.y, rect.y + rect.height);
            return UiRect{ left, top, right - left, bottom - top };
        }

        bool Intersects(const UiRect& lhs, const UiRect& rhs) noexcept
        {
            return lhs.x < rhs.x + rhs.width &&
                lhs.x + lhs.width > rhs.x &&
                lhs.y < rhs.y + rhs.height &&
                lhs.y + lhs.height > rhs.y;
        }

        std::string Trim(const std::string& value)
        {
            const auto begin = std::find_if_not(value.begin(), value.end(), [](const unsigned char ch) { return std::isspace(ch) != 0; });
            const auto end = std::find_if_not(value.rbegin(), value.rend(), [](const unsigned char ch) { return std::isspace(ch) != 0; }).base();
            if (begin >= end)
            {
                return {};
            }

            return std::string(begin, end);
        }

        bool IsLayoutSection(const std::string& section)
        {
            return section == "Login_1280x960" ||
                section == "ServerSelect_1280x960" ||
                section == "Option_1280x960" ||
                section == "OptionHotkey_1280x960" ||
                section == "CharacterSelect_1280x960" ||
                section == "CharacterCreate_1280x960";
        }

        std::string SectionForKey(const std::string& key)
        {
            const auto separator = key.find('.');
            return separator == std::string::npos ? std::string{} : key.substr(0, separator);
        }

        std::string NameForKey(const std::string& key)
        {
            const auto separator = key.find('.');
            return separator == std::string::npos ? key : key.substr(separator + 1);
        }

        std::optional<UiRect> ParseRect(const std::string& value)
        {
            std::array<std::int32_t, 4> parts{};
            std::stringstream stream(value);
            std::string token;
            std::size_t index = 0;
            while (std::getline(stream, token, ','))
            {
                if (index >= parts.size())
                {
                    return std::nullopt;
                }

                token = Trim(token);
                if (token.empty())
                {
                    return std::nullopt;
                }

                try
                {
                    parts[index++] = static_cast<std::int32_t>(std::stoi(token));
                }
                catch (const std::exception&)
                {
                    return std::nullopt;
                }
            }

            if (index != parts.size())
            {
                return std::nullopt;
            }

            return UiRect{ parts[0], parts[1], parts[2], parts[3] };
        }

        std::string FormatRect(const UiRect rect)
        {
            return std::to_string(rect.x) + "," +
                std::to_string(rect.y) + "," +
                std::to_string(rect.width) + "," +
                std::to_string(rect.height);
        }
    }

    void ClientUIEditor::Load(const std::filesystem::path& path)
    {
        overrides_.clear();

        std::ifstream input(path);
        std::string section;
        std::string line;
        while (std::getline(input, line))
        {
            line = Trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#')
            {
                continue;
            }

            if (line.front() == '[' && line.back() == ']')
            {
                section = line.substr(1, line.size() - 2);
                continue;
            }

            if (!IsLayoutSection(section))
            {
                continue;
            }

            const auto separator = line.find('=');
            if (separator == std::string::npos)
            {
                continue;
            }

            const auto key = Trim(line.substr(0, separator));
            const auto value = Trim(line.substr(separator + 1));
            if (const auto rect = ParseRect(value))
            {
                overrides_[section + "." + key] = *rect;
            }
        }
    }
    
    void ClientUIEditor::Reload(const std::filesystem::path& path)
    {
    Load(path);  // Load() zaten overrides_.clear() yapıp yeniden doldurur
    }

    void ClientUIEditor::Save(const std::filesystem::path& path, const std::vector<ClientUIEditorEntry>& knownEntries) const
    {
        std::vector<std::string> preservedLines;
        std::ifstream input(path);
        std::string line;
        std::string section;
        bool skippingLayoutSection = false;
        while (std::getline(input, line))
        {
            const auto trimmed = Trim(line);
            if (!trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']')
            {
                section = trimmed.substr(1, trimmed.size() - 2);
                skippingLayoutSection = IsLayoutSection(section);
            }

            if (!skippingLayoutSection)
            {
                preservedLines.push_back(line);
            }
        }

        std::map<std::string, std::vector<ClientUIEditorEntry>> sections;
        for (const auto& entry : knownEntries)
        {
            sections[SectionForKey(entry.key)].push_back(ClientUIEditorEntry{ NameForKey(entry.key), Rect(entry.key, entry.rect) });
        }

        std::ofstream output(path, std::ios::trunc);
        for (const auto& preserved : preservedLines)
        {
            output << preserved << '\n';
        }

        for (const auto& [sectionName, entries] : sections)
        {
            if (!IsLayoutSection(sectionName))
            {
                continue;
            }

            output << '\n' << '[' << sectionName << ']' << '\n';
            for (const auto& entry : entries)
            {
                output << entry.key << " = " << FormatRect(entry.rect) << '\n';
            }
        }
    }

    bool ClientUIEditor::Enabled() const noexcept
    {
        return enabled_;
    }

    void ClientUIEditor::ToggleEnabled() noexcept
    {
        enabled_ = !enabled_;
        dragging_ = false;
        resizing_ = false;
        marqueeSelecting_ = false;
        dragStartRects_.clear();
        pointerEntries_.clear();
    }

    UiRect ClientUIEditor::Rect(const std::string& key, const UiRect fallback) const
    {
        const auto it = overrides_.find(key);
        if (it == overrides_.end())
        {
            return fallback;
        }

        auto rect = it->second;
        if ((rect.width <= 0 || rect.height <= 0) && fallback.width > 0 && fallback.height > 0)
        {
            rect.width = fallback.width;
            rect.height = fallback.height;
        }
        rect.width = std::max(1, rect.width);
        rect.height = std::max(1, rect.height);
        return rect;
    }

    const std::optional<std::string>& ClientUIEditor::SelectedKey() const noexcept
    {
        return selectedKey_;
    }

    const std::set<std::string>& ClientUIEditor::SelectedKeys() const noexcept
    {
        return selectedKeys_;
    }

    std::optional<UiRect> ClientUIEditor::MarqueeSelection() const noexcept
    {
        if (!marqueeSelecting_)
        {
            return std::nullopt;
        }

        return NormalizeRect(UiRect{
            marqueeStartX_,
            marqueeStartY_,
            marqueeCurrentX_ - marqueeStartX_,
            marqueeCurrentY_ - marqueeStartY_ });
    }

    void ClientUIEditor::SelectKey(std::string key)
    {
        selectedKey_ = std::move(key);
        selectedKeys_.clear();
        if (selectedKey_)
        {
            selectedKeys_.insert(*selectedKey_);
        }
        dragging_ = false;
        resizing_ = false;
        marqueeSelecting_ = false;
        dragStartRects_.clear();
        pointerEntries_.clear();
    }

    void ClientUIEditor::ToggleSelectionKey(std::string key)
    {
        dragging_ = false;
        resizing_ = false;
        marqueeSelecting_ = false;
        dragStartRects_.clear();
        pointerEntries_.clear();

        const auto it = selectedKeys_.find(key);
        if (it == selectedKeys_.end())
        {
            selectedKey_ = key;
            selectedKeys_.insert(std::move(key));
            return;
        }

        const auto removingPrimary = selectedKey_ && *selectedKey_ == key;
        selectedKeys_.erase(it);
        if (selectedKeys_.empty())
        {
            selectedKey_.reset();
        }
        else if (removingPrimary)
        {
            selectedKey_ = *selectedKeys_.begin();
        }
    }

    bool ClientUIEditor::BeginPointer(const std::int32_t screenX, const std::int32_t screenY, const std::vector<ClientUIEditorEntry>& entries, const LayoutTransform transform)
    {
        if (!enabled_)
        {
            return false;
        }

        dragging_ = false;
        resizing_ = false;
        marqueeSelecting_ = false;
        dragStartRects_.clear();
        pointerEntries_.clear();

        for (auto it = entries.rbegin(); it != entries.rend(); ++it)
        {
            const auto screenRect = LayoutViewport::ScaleRect(Rect(it->key, it->rect), transform);
            if (!Contains(screenRect, screenX, screenY))
            {
                continue;
            }

            selectedKey_ = it->key;
            if (selectedKeys_.find(it->key) == selectedKeys_.end())
            {
                selectedKeys_.clear();
                selectedKeys_.insert(it->key);
            }
            dragStartX_ = screenX;
            dragStartY_ = screenY;
            for (const auto& entry : entries)
            {
                if (selectedKeys_.find(entry.key) != selectedKeys_.end())
                {
                    dragStartRects_[entry.key] = Rect(entry.key, entry.rect);
                }
            }
            constexpr std::int32_t resizeGrip = 10;
            resizing_ = screenX >= screenRect.x + screenRect.width - resizeGrip ||
                screenY >= screenRect.y + screenRect.height - resizeGrip;
            dragging_ = !resizing_;
            return true;
        }

        selectedKey_.reset();
        selectedKeys_.clear();
        pointerEntries_ = entries;
        const auto point = ScreenToReferencePoint(screenX, screenY, transform);
        marqueeStartX_ = point.first;
        marqueeStartY_ = point.second;
        marqueeCurrentX_ = point.first;
        marqueeCurrentY_ = point.second;
        marqueeSelecting_ = true;
        return true;
    }

    void ClientUIEditor::UpdatePointer(const std::int32_t screenX, const std::int32_t screenY, const LayoutTransform transform) noexcept
    {
        if (!enabled_)
        {
            return;
        }

        if (marqueeSelecting_)
        {
            const auto point = ScreenToReferencePoint(screenX, screenY, transform);
            marqueeCurrentX_ = point.first;
            marqueeCurrentY_ = point.second;

            const auto selection = NormalizeRect(UiRect{
                marqueeStartX_,
                marqueeStartY_,
                marqueeCurrentX_ - marqueeStartX_,
                marqueeCurrentY_ - marqueeStartY_ });
            selectedKeys_.clear();
            for (const auto& entry : pointerEntries_)
            {
                const auto rect = Rect(entry.key, entry.rect);
                if (Intersects(selection, rect))
                {
                    selectedKeys_.insert(entry.key);
                }
            }
            selectedKey_ = selectedKeys_.empty() ? std::optional<std::string>{} : std::optional<std::string>{ *selectedKeys_.begin() };
            return;
        }

        if ((!selectedKey_ && selectedKeys_.empty()) || (!dragging_ && !resizing_))
        {
            return;
        }

        const auto scale = transform.uniformScale <= 0.0F ? 1.0F : transform.uniformScale;
        const auto dx = static_cast<std::int32_t>(static_cast<float>(screenX - dragStartX_) / scale);
        const auto dy = static_cast<std::int32_t>(static_cast<float>(screenY - dragStartY_) / scale);

        for (const auto& [key, startRect] : dragStartRects_)
        {
            auto next = startRect;
            if (dragging_)
            {
                next.x += dx;
                next.y += dy;
            }
            else
            {
                next.width = std::max(1, startRect.width + dx);
                next.height = std::max(1, startRect.height + dy);
            }

            overrides_[key] = next;
        }
    }

    void ClientUIEditor::EndPointer() noexcept
    {
        dragging_ = false;
        resizing_ = false;
        marqueeSelecting_ = false;
        dragStartRects_.clear();
        pointerEntries_.clear();
    }

    void ClientUIEditor::SelectNext(const std::vector<ClientUIEditorEntry>& entries)
    {
        if (!enabled_ || entries.empty())
        {
            return;
        }

        if (!selectedKey_)
        {
            selectedKey_ = entries.front().key;
            selectedKeys_.clear();
            selectedKeys_.insert(*selectedKey_);
            return;
        }

        const auto it = std::find_if(entries.begin(), entries.end(), [this](const auto& entry) {
            return entry.key == *selectedKey_;
        });
        if (it == entries.end() || std::next(it) == entries.end())
        {
            selectedKey_ = entries.front().key;
            selectedKeys_.clear();
            selectedKeys_.insert(*selectedKey_);
            return;
        }

        selectedKey_ = std::next(it)->key;
        selectedKeys_.clear();
        selectedKeys_.insert(*selectedKey_);
    }

    bool ClientUIEditor::NudgeSelected(const std::int32_t dx, const std::int32_t dy, const bool resize, const std::vector<ClientUIEditorEntry>& entries)
    {
        if (!enabled_ || (!selectedKey_ && selectedKeys_.empty()))
        {
            return false;
        }

        std::set<std::string> keys = selectedKeys_;
        if (keys.empty() && selectedKey_)
        {
            keys.insert(*selectedKey_);
        }

        bool changed = false;
        for (const auto& key : keys)
        {
            const auto it = std::find_if(entries.begin(), entries.end(), [&key](const auto& entry) {
                return entry.key == key;
            });
            if (it == entries.end())
            {
                continue;
            }

            auto rect = Rect(it->key, it->rect);
            if (resize)
            {
                rect.width = std::max(1, rect.width + dx);
                rect.height = std::max(1, rect.height + dy);
            }
            else
            {
                rect.x += dx;
                rect.y += dy;
            }

            overrides_[it->key] = rect;
            changed = true;
        }
        return changed;
    }

    UiRect ClientUIEditor::ScreenToReferenceRect(const UiRect rect, const LayoutTransform transform) const noexcept
    {
        const auto scale = transform.uniformScale <= 0.0F ? 1.0F : transform.uniformScale;
        return UiRect{
            static_cast<std::int32_t>(static_cast<float>(rect.x - transform.offsetX) / scale),
            static_cast<std::int32_t>(static_cast<float>(rect.y - transform.offsetY) / scale),
            static_cast<std::int32_t>(static_cast<float>(rect.width) / scale),
            static_cast<std::int32_t>(static_cast<float>(rect.height) / scale)
        };
    }

    std::pair<std::int32_t, std::int32_t> ClientUIEditor::ScreenToReferencePoint(const std::int32_t screenX, const std::int32_t screenY, const LayoutTransform transform) const noexcept
    {
        const auto scale = transform.uniformScale <= 0.0F ? 1.0F : transform.uniformScale;
        return {
            static_cast<std::int32_t>(static_cast<float>(screenX - transform.offsetX) / scale),
            static_cast<std::int32_t>(static_cast<float>(screenY - transform.offsetY) / scale)
        };
    }
}
