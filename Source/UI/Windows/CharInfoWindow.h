// CharInfoWindow.h
//
// Legacy C_VS_UI_INFO port — Character / Skill / Grade info window.
// Three mode tabs: Character, Skill, Grade/Rare skill.

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include "Runtime/GameRuntimeModels.h"
#include "Content/ItemInfoCatalog.h"

namespace phoenix::platform
{
class Application;
}

namespace phoenix::ui
{

// Legacy INFO_MODE enum (VS_UI_GameCommon.h:4021)
enum class CharInfoMode : uint8_t
{
    CharacterInfo = 0,   // CHARACTER_INFO_MODE
    SkillInfo,           // SKILL_INFO_MODE
    Grade1,              // GRADE1_INFO_MODE
    Grade2,              // GRADE2_INFO_MODE
    Grade3,              // GRADE3_INFO_MODE
    RareSkill,           // RARE_SKILL_INFO_MODE
    Count
};

// Race-specific skill domains (Slayer/Vampire/Ousters)
enum class SkillDomain : uint8_t
{
    // Slayer
    Slayer_Blade = 0,
    Slayer_Sword,
    Slayer_Gun,
    Slayer_Heal,
    Slayer_Enchant,
    Slayer_Total,
    // Vampire
    Vampire_Poison,
    Vampire_Acid,
    Vampire_Curse,
    Vampire_Summon,
    Vampire_Blood,
    Vampire_Innate,
    // Ousters
    Ousters_Combat,
    Ousters_Elemental,
    Count
};

struct CharInfoWindowConfig
{
    // Window position & size (per resolution from interface.ini)
    int32_t windowX = 0;
    int32_t windowY = 0;
    int32_t windowW = 0;
    int32_t windowH = 0;
    bool    draggable = true;

    // Tab button positions (relative to window)
    struct TabBtn { int32_t x, y, w, h; };
    TabBtn tabCharacter{};
    TabBtn tabSkill{};
    TabBtn tabGrade{};
    TabBtn tabRare{};

    // Character info stat label positions
    struct StatPos { int32_t labelX, labelY, valueX, valueY; };
    StatPos strPos{};
    StatPos dexPos{};
    StatPos intPos{};
    StatPos hpPos{};
    StatPos mpPos{};
    StatPos levelPos{};
    StatPos famePos{};
    StatPos pkPos{};
    StatPos alignmentPos{};

    // Vampire rank buttons
    struct RankBtn { int32_t x, y, w, h; };
    RankBtn rankButtons[14]{};  // Private ~ Marshal

    // Skill tab: domain tab buttons + list rect
    struct SkillTabLayout
    {
        int32_t domainTabX = 0;
        int32_t domainTabY = 0;
        int32_t domainTabW = 0;
        int32_t domainTabH = 0;
        int32_t listX = 0;
        int32_t listY = 0;
        int32_t listW = 0;
        int32_t listH = 0;
        int32_t itemH = 20;
    };
    SkillTabLayout slayerSkill{};
    SkillTabLayout vampireSkill{};
    SkillTabLayout oustersSkill{};

    // Close / Help / Alpha buttons
    struct ButtonPos { int32_t x, y, w, h; };
    ButtonPos btnClose{};
    ButtonPos btnHelp{};
    ButtonPos btnAlpha{};

    // Portrait area
    int32_t portraitX = 0;
    int32_t portraitY = 0;
    int32_t portraitW = 0;
    int32_t portraitH = 0;
};

class CharInfoWindow
{
public:
    CharInfoWindow() = default;
    ~CharInfoWindow() = default;

    // Initialize with config from interface.ini
    void LoadFromConfig(const CharInfoWindowConfig& cfg);

    // Open/close
    void Open(CharInfoMode mode = CharInfoMode::CharacterInfo);
    void Close();
    bool IsOpen() const { return open_; }

    // Mode/tab switching
    void SetMode(CharInfoMode mode);
    CharInfoMode GetMode() const { return mode_; }
    void SetSkillDomain(SkillDomain domain);
    SkillDomain GetSkillDomain() const { return skillDomain_; }

    // Race-specific
    void SetRace(protocol::PcType race);
    protocol::PcType GetRace() const { return race_; }

    // Update from runtime (called when stats change)
    void UpdateStats(const runtime::PlayerRuntimeModel& player);

    // Render
    void Render(platform::Application* app);

    // Input
    bool OnMouseMove(int32_t x, int32_t y);
    bool OnMouseDown(int32_t x, int32_t y);
    bool OnMouseUp(int32_t x, int32_t y);
    bool OnKeyDown(uint32_t key);

private:
    // Render helpers (one per mode)
    void RenderCharacterInfo(platform::Application* app);
    void RenderSkillInfo(platform::Application* app);
    void RenderGradeInfo(platform::Application* app);
    void RenderRareSkillInfo(platform::Application* app);
    CharInfoWindowConfig cfg_;
    CharInfoMode mode_ = CharInfoMode::CharacterInfo;
    SkillDomain skillDomain_ = SkillDomain::Slayer_Blade;
    protocol::PcType race_ = protocol::PcType::Slayer;
    bool open_ = false;
    bool dragging_ = false;
    int32_t dragOffsetX_ = 0;
    int32_t dragOffsetY_ = 0;

    // Cached stats
    struct CachedStats
    {
        int32_t str = 0, dex = 0, int_ = 0;
        int32_t hp = 0, maxHp = 0;
        int32_t mp = 0, maxMp = 0;
        int32_t level = 0;
        int32_t fame = 0;
        int32_t pk = 0;
        int32_t alignment = 0;
        int32_t bonusPoints = 0;
        int32_t rank = 0;  // Vampire rank
    } stats_;

    // Hit-test helpers
    bool HitTestTab(int32_t x, int32_t y, CharInfoMode& outMode);
    bool HitTestClose(int32_t x, int32_t y);
    bool HitTestAlpha(int32_t x, int32_t y);
    bool HitTestSkillDomain(int32_t x, int32_t y, SkillDomain& outDomain);
    bool HitTestStatPlus(int32_t x, int32_t y, int& outStatType);  // 0=STR,1=DEX,2=INT
    bool HitTestRank(int32_t x, int32_t y, int& outRank);
};

} // namespace phoenix::ui