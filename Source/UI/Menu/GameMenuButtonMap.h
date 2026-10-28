#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace phoenix::ui {

    enum class GameMenuItem : uint8_t {
        Gear          = 0,   // Equipment (装备 - zhuangbei)
        Skill         = 1,   // Skill (技能 - jineng)
        Quest         = 2,   // Quest (任务 - renwu)
        Pet           = 3,   // Pet (宠物 - pengwu)
        Guild         = 4,   // Guild (公会 - gonghui)
        GuildList     = 5,   // Union List (联盟 - lianmeng)
        WaitingGuild  = 6,   // Pending Guild (等待工会 - dengdai gonghui)
        GuildMember   = 7,   // Guild Member (公会成员 - gonghui chengyuan)
        Friend        = 8,   // Friend (朋友 - pengyou)
        Party         = 9,   // Party (队伍 - duiwu)
        TMall         = 10,  // T-Mall (天猫 - tianmao)
        Shop          = 11,  // Shop/Store (商店 - shangdian)
        GoodList      = 12,  // Notice Board / Goods List (公告列表 - gonggao lieshi)
        Help          = 13,  // Help (帮助 - bangzhu)
        Option        = 14,  // Option (选项 - xuanxiang)
        Logout        = 15,  // Logout (登出 - dengchu)
        CloseGame     = 16,  // Close Game (关闭游戏 - guanbi youxi)
        Close         = 17,  // X button / dismiss (关闭 - guanbi)

        ItemCount     = 18   // must be last
    };

    struct GameMenuButtonEntry {
        GameMenuItem id;
        const char* labelCN{};      // Chinese CN display text (gb2312/cp936)
        const char* labelEN{};      // English description
        uint32_t    spriteIndex{};  // sheet index for drawing
        uint32_t    colIndex{};     // grid column (0-based)
        uint32_t    rowIndex{};     // grid row (0-based)
        const char* dispatchMsg{};  // legacy msg name trace ("UI_NEW_OPTION", etc.)

        GameMenuButtonEntry(GameMenuItem id_,
                            const char* cn,
                            const char* en,
                            uint32_t spriteIdx,
                            uint32_t col,
                            uint32_t row,
                            const char* msg)
            : id(id_), labelCN(cn), labelEN(en),
              spriteIndex(spriteIdx), colIndex(col), rowIndex(row), dispatchMsg(msg) {}

        // Default constructor for raw-array find path
        GameMenuButtonEntry() = default;
    };

    class GameMenuButtonMap final {
    public:
        static std::vector<GameMenuButtonEntry> CreateDefault();
        static const GameMenuButtonEntry* Find(const std::vector<GameMenuButtonEntry>& map,
                                                GameMenuItem id) noexcept;

        // Layout constants from legacy Start() offset math
        static constexpr int32_t BUTTON_WIDTH       = 0;   // actual size loaded at runtime
        static constexpr int32_t BUTTON_HEIGHT      = 0;   // same
        static constexpr int32_t COL_SPACING        = 68;  // horizontal gap between buttons
        static constexpr int32_t ROW_SPACING        = 63;  // vertical row gap
        static constexpr int32_t MENU_ORIGIN_X      = 75;  // screen_center_W/2 - halfBtnW + 75
        static constexpr int32_t MENU_ORIGIN_Y      = 60;  // screen_center_H/2 - halfBtnH + 60
        static constexpr int32_t BOTTOM_ROW_Y_OFFSET = 50; // row5 y offset override
    };

} // namespace phoenix::ui
