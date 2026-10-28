#include "UI/Menu/GameMenuButtonMap.h"

namespace phoenix::ui {

    using Entry = GameMenuButtonEntry;
    using Item  = GameMenuItem;

    std::vector<Entry> GameMenuButtonMap::CreateDefault() {
        std::vector<Entry> entries;
        entries.reserve(17);

        // Row 0 — Gear, Skill, Quest, Pet
        entries.emplace_back(Item::Gear,     nullptr, "Equipment",         0,  0, 0, "UI_NEW_OPTION(param=1)");
        entries.emplace_back(Item::Skill,    nullptr, "Skill",             1,  1, 0, "UI_NEW_OPTION(param=2)");
        entries.emplace_back(Item::Quest,    nullptr, "Quest",             2,  2, 0, "UI_NEW_OPTION(param=3)");
        entries.emplace_back(Item::Pet,      nullptr, "Pet",               3,  3, 0, "UI_NEW_OPTION(param=4)");

        // Row 1 — Guild, GuildList, WaitingGuild, GuildMember
        entries.emplace_back(Item::Guild,        nullptr, "Guild",                4,   0, 1, "UI_REQUEST_GUILD_INFO(param=1)");
        entries.emplace_back(Item::GuildList,    nullptr, "Union List",           5,   1, 1, "UI_REQUEST_UNION_REQ_LIST(param=1)");
        entries.emplace_back(Item::WaitingGuild, nullptr, "Wait Guild",           6,   2, 1, "UI_REQUEST_UNION_REQ_LIST(param=0)");
        entries.emplace_back(Item::GuildMember,  nullptr, "Guild Member",         7,   3, 1, "UI_REQ_GUILD_MEMBER_LIST");

        // Row 2 — Friend, Party (cols 2+ left empty)
        entries.emplace_back(Item::Friend,   nullptr, "Friend",             8,  0, 2, "UI_NEW_OPTION(param=5)");
        entries.emplace_back(Item::Party,    nullptr, "Party",              9,  1, 2, "UI_NEW_OPTION(param=6)");

        // Row 3 — TMall, Shop, GoodList (col 3 left empty)
        entries.emplace_back(Item::TMall,     nullptr, "T-Mall",            10,  0, 3, "UI_NEW_OPTION(param=7)");
        entries.emplace_back(Item::Shop,      nullptr, "Shop/Store",        11,  1, 3, "UI_REQUEST_STORE_INFO(param=1)");
        entries.emplace_back(Item::GoodList,  nullptr, "Notice Board",      12,  2, 3, "UI_MARKET_ACCOUNT(param=1)");

        // Row 4 — Help, Option, Logout, CloseGame (bottom row with slight Y offset)
        entries.emplace_back(Item::Help,      nullptr, "Help",              13,  0, 4, "UI_NEW_OPTION(param=9)");
        entries.emplace_back(Item::Option,    nullptr, "Option",            14,  2, 4, "RunOption dialog");
        entries.emplace_back(Item::Logout,    nullptr, "Logout",            15,  3, 4, "confirm dialog (Finish NOT called before)");
        entries.emplace_back(Item::CloseGame, nullptr, "Close Game",        16,  4, 4, "UI_GAME_EXIT / GameExit");

        // Visual close button — X on top-right of menu frame
        entries.emplace_back(Item::Close,      nullptr, "Close (X)",         17,  0, 5, "window close/dismiss");

        return entries;
    }

    const GameMenuButtonEntry* GameMenuButtonMap::Find(
        const std::vector<GameMenuButtonEntry>& map,
        GameMenuItem id) noexcept {
        for (const auto& e : map) {
            if (e.id == id) return &e;
        }
        return nullptr;
    }

} // namespace phoenix::ui
