#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <functional>

#include "Settings/InterfaceProfileLoader.h"

// Forward declare legacy packet structs (in global namespace)
struct QuestStatusInfo;
struct GCGQuestStatusInfo;
namespace phoenix::content {
    class LegacyQuestCatalog;
}
namespace phoenix::game {
    struct ClientPlayer;
}

namespace phoenix::ui {

// Callback types for rendering
using SpriteDrawCallback = std::function<void(const char* textureKey, int x, int y, int w, int h, float alpha)>;
using TextDrawCallback = std::function<void(const wchar_t* text, int x, int y, int color, int fontSize)>;

struct QuestManagerState {
    bool isOpen{ false };
    std::uint8_t tab{ 0 };  // 0=Active, 1=Completed
    std::size_t selection{ 0 };
    std::uint32_t selectedQuestId{ 0xFFFFFFFF };
    bool detailOpen{ false };
    bool missionOpen{ false };
    bool itemOpen{ false };
    bool iconVisible{ false };
    int scrollPos{ 0 };
    
    // Window positions (legacy coordinates)
    struct { int x, y; } listPos{ 0, 37 };
    struct { int x, y; } detailPos{ 0, 197 };
    struct { int x, y; } missionPos{ 0, 357 };
    struct { int x, y; } itemPos{ 482, 360 };  // Centered
    struct { int x, y; } iconPos{ 1220, 10 };  // Top-right corner
};

class QuestManager {
public:
    // State management
    static void Toggle(QuestManagerState& state) noexcept;
    static void Open(QuestManagerState& state) noexcept;
    static void Close(QuestManagerState& state) noexcept;

    // Config loading
    static void LoadStateFromConfig(QuestManagerState& state, const settings::GameQuestProfile& profile) noexcept;

    // Input handling - returns true if click was consumed
    static bool HandleClick(
        QuestManagerState& state,
        int screenX, int screenY,
        int clickX, int clickY,
        bool rightButton
    ) noexcept;
    
    // Rendering
    static void Render(
        const QuestManagerState& state,
        const std::vector<const ::QuestStatusInfo*>& quests,
        const content::LegacyQuestCatalog& catalog,
        SpriteDrawCallback drawSprite,
        TextDrawCallback drawText
    );
    
private:
    // Helper to find quest by ID
    static const ::QuestStatusInfo* FindQuestById(
        const std::vector<const ::QuestStatusInfo*>& quests,
        std::uint32_t questId
    );
    
    // Individual window rendering
    static void RenderListWindow(
        const QuestManagerState& state,
        const std::vector<const ::QuestStatusInfo*>& quests,
        const content::LegacyQuestCatalog& catalog,
        SpriteDrawCallback drawSprite,
        TextDrawCallback drawText
    );
    
    static void RenderDetailWindow(
        const QuestManagerState& state,
        const ::QuestStatusInfo* selectedQuest,
        const content::LegacyQuestCatalog& catalog,
        SpriteDrawCallback drawSprite,
        TextDrawCallback drawText
    );
    
    static void RenderMissionWindow(
        const QuestManagerState& state,
        const ::QuestStatusInfo* selectedQuest,
        const content::LegacyQuestCatalog& catalog,
        SpriteDrawCallback drawSprite,
        TextDrawCallback drawText
    );
    
    static void RenderItemWindow(
        const QuestManagerState& state,
        const std::vector<std::uint8_t>& questItemTypes,
        SpriteDrawCallback drawSprite
    );
    
    static void RenderIcon(SpriteDrawCallback drawSprite) noexcept;
    
    // Individual window hit tests
    static bool HandleListClick(QuestManagerState& state, int x, int y, bool rightButton) noexcept;
    static bool HandleDetailClick(QuestManagerState& state, int x, int y, bool rightButton) noexcept;
    static bool HandleMissionClick(QuestManagerState& state, int x, int y, bool rightButton) noexcept;
    static bool HandleItemClick(QuestManagerState& state, int x, int y, bool rightButton) noexcept;
};

} // namespace phoenix::ui