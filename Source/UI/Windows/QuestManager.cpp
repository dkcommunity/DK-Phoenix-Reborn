#include "UI/Windows/QuestManager.h"

namespace phoenix::ui
{

void QuestManager::Toggle(QuestManagerState& state) noexcept
{
    if (state.isOpen)
        Close(state);
    else
        Open(state);
}

void QuestManager::Open(QuestManagerState& state) noexcept
{
    state.isOpen = true;
}

void QuestManager::Close(QuestManagerState& state) noexcept
{
    state.isOpen = false;
}

void QuestManager::LoadStateFromConfig(QuestManagerState& state,
                                        const settings::GameQuestProfile& profile) noexcept
{
    state.listPos = { profile.listX, profile.listY };
    state.detailPos = { profile.detailX, profile.detailY };
    state.missionPos = { profile.missionX, profile.missionY };
    state.itemPos = { profile.itemX, profile.itemY };
    state.iconPos = { profile.iconX, profile.iconY };
}

bool QuestManager::HandleClick(
    QuestManagerState& /*state*/,
    int /*screenX*/, int /*screenY*/,
    int /*clickX*/, int /*clickY*/,
    bool /*rightButton*/) noexcept
{
    // Stub: full implementation deferred to parity mapping
    return false;
}

void QuestManager::Render(
    const QuestManagerState& /*state*/,
    const std::vector<const ::QuestStatusInfo*>& /*quests*/,
    const content::LegacyQuestCatalog& /*catalog*/,
    SpriteDrawCallback /*drawSprite*/,
    TextDrawCallback /*drawText*/)
{
    // Stub: full implementation deferred to parity mapping
}

// ── Private stubs ─────────────────────────────────────────────────────

const ::QuestStatusInfo* QuestManager::FindQuestById(
    const std::vector<const ::QuestStatusInfo*>& /*quests*/,
    std::uint32_t /*questId*/)
{
    return nullptr;
}

void QuestManager::RenderListWindow(
    const QuestManagerState& /*state*/,
    const std::vector<const ::QuestStatusInfo*>& /*quests*/,
    const content::LegacyQuestCatalog& /*catalog*/,
    SpriteDrawCallback /*drawSprite*/,
    TextDrawCallback /*drawText*/)
{
}

void QuestManager::RenderDetailWindow(
    const QuestManagerState& /*state*/,
    const ::QuestStatusInfo* /*selectedQuest*/,
    const content::LegacyQuestCatalog& /*catalog*/,
    SpriteDrawCallback /*drawSprite*/,
    TextDrawCallback /*drawText*/)
{
}

void QuestManager::RenderMissionWindow(
    const QuestManagerState& /*state*/,
    const ::QuestStatusInfo* /*selectedQuest*/,
    const content::LegacyQuestCatalog& /*catalog*/,
    SpriteDrawCallback /*drawSprite*/,
    TextDrawCallback /*drawText*/)
{
}

void QuestManager::RenderItemWindow(
    const QuestManagerState& /*state*/,
    const std::vector<std::uint8_t>& /*questItemTypes*/,
    SpriteDrawCallback /*drawSprite*/)
{
}

void QuestManager::RenderIcon(SpriteDrawCallback /*drawSprite*/) noexcept
{
}

bool QuestManager::HandleListClick(QuestManagerState& /*state*/, int /*x*/, int /*y*/, bool /*rightButton*/) noexcept
{
    return false;
}

bool QuestManager::HandleDetailClick(QuestManagerState& /*state*/, int /*x*/, int /*y*/, bool /*rightButton*/) noexcept
{
    return false;
}

bool QuestManager::HandleMissionClick(QuestManagerState& /*state*/, int /*x*/, int /*y*/, bool /*rightButton*/) noexcept
{
    return false;
}

bool QuestManager::HandleItemClick(QuestManagerState& /*state*/, int /*x*/, int /*y*/, bool /*rightButton*/) noexcept
{
    return false;
}

} // namespace phoenix::ui