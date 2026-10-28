// CharInfoWindow.cpp
//
// Legacy C_VS_UI_INFO port implementation.

#include "UI/Windows/CharInfoWindow.h"
#include "Platform/Application.h"
#include "UI/Descriptor/DescriptorManager.h"
#include "Runtime/GameRuntimeModels.h"
#include "Protocol/LegacyProtocol.h"
#include "Content/ItemInfoCatalog.h"
#include "UI/Resources/LegacyUiSpriteResourceLoader.h"

#include <algorithm>

namespace phoenix::ui
{

using legacy::DescriptorManager;
using legacy::DID_ITEM;
using legacy::DID_SKILL;
using legacy::DID_HELP;

void CharInfoWindow::LoadFromConfig(const CharInfoWindowConfig& cfg)
{
    cfg_ = cfg;
}

void CharInfoWindow::Open(CharInfoMode mode)
{
    mode_ = mode;
    open_ = true;
    dragging_ = false;

    // Reset skill domain based on race when opening
    switch (race_)
    {
        case protocol::PcType::Slayer:   skillDomain_ = SkillDomain::Slayer_Blade;   break;
        case protocol::PcType::Vampire:  skillDomain_ = SkillDomain::Vampire_Poison; break;
        case protocol::PcType::Ousters:  skillDomain_ = SkillDomain::Ousters_Combat;  break;
        default: skillDomain_ = SkillDomain::Slayer_Blade; break;
    }
}

void CharInfoWindow::Close()
{
    open_ = false;
    dragging_ = false;
}

void CharInfoWindow::SetMode(CharInfoMode mode)
{
    if (mode < CharInfoMode::Count)
        mode_ = mode;
}

void CharInfoWindow::SetSkillDomain(SkillDomain domain)
{
    if (domain < SkillDomain::Count)
        skillDomain_ = domain;
}

void CharInfoWindow::SetRace(protocol::PcType race)
{
    race_ = race;
    // Reset skill domain on race change
    switch (race_)
    {
        case protocol::PcType::Slayer:   skillDomain_ = SkillDomain::Slayer_Blade;   break;
        case protocol::PcType::Vampire:  skillDomain_ = SkillDomain::Vampire_Poison; break;
        case protocol::PcType::Ousters:  skillDomain_ = SkillDomain::Ousters_Combat;  break;
        default: skillDomain_ = SkillDomain::Slayer_Blade; break;
    }
}

void CharInfoWindow::UpdateStats(const runtime::PlayerRuntimeModel& player)
{
    stats_.str        = static_cast<int32_t>(player.str);
    stats_.dex        = static_cast<int32_t>(player.dex);
    stats_.int_       = static_cast<int32_t>(player.intelligence);
    stats_.hp         = static_cast<int32_t>(player.hp);
    stats_.maxHp      = static_cast<int32_t>(player.maxHp);
    stats_.mp         = static_cast<int32_t>(player.mp);
    stats_.maxMp      = static_cast<int32_t>(player.maxMp);
    stats_.level      = static_cast<int32_t>(player.level);
    stats_.fame       = 0;  // PlayerRuntimeModel doesn't have fame
    stats_.pk         = 0;
    stats_.alignment  = static_cast<int32_t>(player.alignment);
    stats_.bonusPoints= static_cast<int32_t>(player.bonus);  // field is 'bonus'
    stats_.rank       = static_cast<int32_t>(player.rank);
}

void CharInfoWindow::Render(platform::Application* app)
{
    if (!open_ || !app)
        return;

    // Window background
    // TODO: Use SPK_INFO_* background sprite via DescriptorManager / LegacyUiSpriteResourceLoader
    // For now, placeholder render

    // Tabs
    // Close button
    // Help button
    // Alpha button

    // Content based on mode_
    switch (mode_)
    {
        case CharInfoMode::CharacterInfo:
            RenderCharacterInfo(app);
            break;
        case CharInfoMode::SkillInfo:
            RenderSkillInfo(app);
            break;
        case CharInfoMode::Grade1:
        case CharInfoMode::Grade2:
        case CharInfoMode::Grade3:
            RenderGradeInfo(app);
            break;
        case CharInfoMode::RareSkill:
            RenderRareSkillInfo(app);
            break;
        default:
            break;
    }
}

void CharInfoWindow::RenderCharacterInfo(platform::Application* app)
{
    // Draw portrait, stats, rank buttons (Vampire), bonus point + buttons
    // Stat labels at cfg_.statPos positions
    // Vampire-only: STR/DEX/INT + buttons + rank buttons
    (void)app;
}

void CharInfoWindow::RenderSkillInfo(platform::Application* app)
{
    // Domain tabs + skill list with scroll
    // Skill level, exp, name
    (void)app;
}

void CharInfoWindow::RenderGradeInfo(platform::Application* app)
{
    // Grade skill list (learnable grade skills)
    (void)app;
}

void CharInfoWindow::RenderRareSkillInfo(platform::Application* app)
{
    // Rare skill list
    (void)app;
}

bool CharInfoWindow::OnMouseMove(int32_t x, int32_t y)
{
    if (!open_) return false;

    if (dragging_)
    {
        cfg_.windowX = x - dragOffsetX_;
        cfg_.windowY = y - dragOffsetY_;
        return true;
    }

    // Hover effects for buttons/tabs
    return false;
}

bool CharInfoWindow::OnMouseDown(int32_t x, int32_t y)
{
    if (!open_) return false;

    // Drag start
    if (x >= cfg_.windowX && x < cfg_.windowX + cfg_.windowW &&
        y >= cfg_.windowY && y < cfg_.windowY + 20)  // title bar area
    {
        dragging_ = true;
        dragOffsetX_ = x - cfg_.windowX;
        dragOffsetY_ = y - cfg_.windowY;
        return true;
    }

    // Close button
    if (HitTestClose(x, y))
    {
        Close();
        return true;
    }

    // Alpha button
    if (HitTestAlpha(x, y))
    {
        // Toggle alpha
        return true;
    }

    // Tabs
    CharInfoMode tabMode;
    if (HitTestTab(x, y, tabMode))
    {
        SetMode(tabMode);
        return true;
    }

    // Skill domain tabs
    SkillDomain domain;
    if (HitTestSkillDomain(x, y, domain))
    {
        SetSkillDomain(domain);
        return true;
    }

    // Stat plus buttons (Vampire only)
    int statType;
    if (HitTestStatPlus(x, y, statType))
    {
        // Send CGUseBonusPoint packet (INC_STR=1, INC_DEX=2, INC_INT=0)
        // via Application::SendUseBonusPoint(statType)
        return true;
    }

    // Rank buttons (Vampire only)
    int rank;
    if (HitTestRank(x, y, rank))
    {
        // Filter grade skills by rank
        return true;
    }

    return false;
}

bool CharInfoWindow::OnMouseUp(int32_t x, int32_t y)
{
    if (dragging_)
    {
        dragging_ = false;
        return true;
    }
    return false;
}

bool CharInfoWindow::OnKeyDown(uint32_t key)
{
    if (!open_) return false;

    if (key == 0x1B) // ESC
    {
        Close();
        return true;
    }
    return false;
}

bool CharInfoWindow::HitTestTab(int32_t x, int32_t y, CharInfoMode& outMode)
{
    // Check tab buttons
    auto test = [&](const CharInfoWindowConfig::TabBtn& btn, CharInfoMode mode) {
        return x >= cfg_.windowX + btn.x && x < cfg_.windowX + btn.x + btn.w &&
               y >= cfg_.windowY + btn.y && y < cfg_.windowY + btn.y + btn.h;
    };

    if (test(cfg_.tabCharacter, CharInfoMode::CharacterInfo)) { outMode = CharInfoMode::CharacterInfo; return true; }
    if (test(cfg_.tabSkill, CharInfoMode::SkillInfo))         { outMode = CharInfoMode::SkillInfo;         return true; }
    if (test(cfg_.tabGrade, CharInfoMode::Grade1))        { outMode = CharInfoMode::Grade1;        return true; }
    if (test(cfg_.tabRare, CharInfoMode::RareSkill))      { outMode = CharInfoMode::RareSkill;     return true; }
    return false;
}

bool CharInfoWindow::HitTestClose(int32_t x, int32_t y)
{
    return x >= cfg_.windowX + cfg_.btnClose.x && x < cfg_.windowX + cfg_.btnClose.x + cfg_.btnClose.w &&
           y >= cfg_.windowY + cfg_.btnClose.y && y < cfg_.windowY + cfg_.btnClose.y + cfg_.btnClose.h;
}

bool CharInfoWindow::HitTestAlpha(int32_t x, int32_t y)
{
    return x >= cfg_.windowX + cfg_.btnAlpha.x && x < cfg_.windowX + cfg_.btnAlpha.x + cfg_.btnAlpha.w &&
           y >= cfg_.windowY + cfg_.btnAlpha.y && y < cfg_.windowY + cfg_.btnAlpha.y + cfg_.btnAlpha.h;
}

bool CharInfoWindow::HitTestSkillDomain(int32_t x, int32_t y, SkillDomain& outDomain)
{
    const CharInfoWindowConfig::SkillTabLayout* layout = nullptr;
    switch (race_)
    {
        case protocol::PcType::Slayer:   layout = &cfg_.slayerSkill;   break;
        case protocol::PcType::Vampire:  layout = &cfg_.vampireSkill;  break;
        case protocol::PcType::Ousters:  layout = &cfg_.oustersSkill;  break;
        default: return false;
    }

    int32_t tabX = cfg_.windowX + layout->domainTabX;
    int32_t tabY = cfg_.windowY + layout->domainTabY;
    int32_t tabW = layout->domainTabW;
    int32_t tabH = layout->domainTabH;

    int numDomains = 0;
    SkillDomain startDomain = SkillDomain::Count;
    switch (race_)
    {
        case protocol::PcType::Slayer:
            numDomains = 6; startDomain = SkillDomain::Slayer_Blade; break;
        case protocol::PcType::Vampire:
            numDomains = 6; startDomain = SkillDomain::Vampire_Poison; break;
        case protocol::PcType::Ousters:
            numDomains = 2; startDomain = SkillDomain::Ousters_Combat; break;
        default: return false;
    }

    for (int i = 0; i < numDomains; ++i)
    {
        int32_t dx = tabX + i * tabW;
        if (x >= dx && x < dx + tabW && y >= tabY && y < tabY + tabH)
        {
            outDomain = static_cast<SkillDomain>(static_cast<uint8_t>(startDomain) + i);
            return true;
        }
    }
    return false;
}

bool CharInfoWindow::HitTestStatPlus(int32_t x, int32_t y, int& outStatType)
{
    // Vampire only: STR/DEX/INT plus buttons
    if (race_ != protocol::PcType::Vampire) return false;

    // TODO: check against cfg_ stat positions + button size
    // For now placeholder
    return false;
}

bool CharInfoWindow::HitTestRank(int32_t x, int32_t y, int& outRank)
{
    if (race_ != protocol::PcType::Vampire) return false;

    for (int i = 0; i < 14; ++i)
    {
        const auto& btn = cfg_.rankButtons[i];
        if (x >= cfg_.windowX + btn.x && x < cfg_.windowX + btn.x + btn.w &&
            y >= cfg_.windowY + btn.y && y < cfg_.windowY + btn.y + btn.h)
        {
            outRank = i;
            return true;
        }
    }
    return false;
}

} // namespace phoenix::ui