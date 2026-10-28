#include "Content/LegacyUiSpriteCatalog.h"

#include <algorithm>

namespace phoenix::content
{
    namespace
    {
        void Add(
            std::vector<LegacyUiSpriteRecord>& records,
            std::string key,
            std::string legacyDefine,
            LegacyUiSpriteScope scope,
            std::filesystem::path relativePath,
            bool required,
            bool smokeDecode = false,
            std::uint16_t expectedMinimumFrames = 0,
            LegacyUiRace race = LegacyUiRace::Shared,
            std::optional<std::filesystem::path> relativeIndexPath = std::nullopt,
            bool indexRequired = true,
            std::uint16_t smokeFrameIndex = 0,
            std::uint16_t splitBaseFrameIndex = 0)
        {
            records.push_back(LegacyUiSpriteRecord{
                std::move(key),
                std::move(legacyDefine),
                LegacyUiSpriteKind::SpritePack,
                scope,
                race,
                std::move(relativePath),
                std::move(relativeIndexPath),
                required,
                indexRequired,
                smokeDecode,
                smokeFrameIndex,
                splitBaseFrameIndex,
                expectedMinimumFrames
            });
        }

        void AddCatalogOnly(
            std::vector<LegacyUiSpriteRecord>& records,
            std::string key,
            std::string legacyDefine,
            LegacyUiSpriteKind kind,
            LegacyUiSpriteScope scope,
            std::filesystem::path relativePath,
            LegacyUiRace race = LegacyUiRace::Shared,
            std::optional<std::filesystem::path> relativeIndexPath = std::nullopt)
        {
            records.push_back(LegacyUiSpriteRecord{
                std::move(key),
                std::move(legacyDefine),
                kind,
                scope,
                race,
                std::move(relativePath),
                std::move(relativeIndexPath),
                false,
                false,
                false,
                0,
                0,
                0,
                true
            });
        }

        void AddRpk(
            std::vector<LegacyUiSpriteRecord>& records,
            std::string key,
            std::string legacyDefine,
            std::filesystem::path relativePath,
            bool required)
        {
            records.push_back(LegacyUiSpriteRecord{
                std::move(key),
                std::move(legacyDefine),
                LegacyUiSpriteKind::ResourcePack,
                LegacyUiSpriteScope::Text,
                LegacyUiRace::Shared,
                std::move(relativePath),
                std::nullopt,
                required,
                false,
                false,
                0,
                0,
                0
            });
        }

        void AddIndexed(
            std::vector<LegacyUiSpriteRecord>& records,
            std::string key,
            std::string legacyDefine,
            LegacyUiSpriteScope scope,
            std::filesystem::path relativePath,
            std::filesystem::path relativeIndexPath,
            bool required,
            LegacyUiRace race = LegacyUiRace::Shared)
        {
            records.push_back(LegacyUiSpriteRecord{
                std::move(key),
                std::move(legacyDefine),
                LegacyUiSpriteKind::IndexedSpritePack,
                scope,
                race,
                std::move(relativePath),
                std::move(relativeIndexPath),
                required,
                true,
                false,
                0,
                0,
                0
            });
        }

        void AddFramePack(
            std::vector<LegacyUiSpriteRecord>& records,
            std::string key,
            std::string legacyDefine,
            LegacyUiSpriteScope scope,
            std::filesystem::path relativePath,
            std::filesystem::path relativeIndexPath,
            bool required,
            LegacyUiRace race = LegacyUiRace::Shared)
        {
            records.push_back(LegacyUiSpriteRecord{
                std::move(key),
                std::move(legacyDefine),
                LegacyUiSpriteKind::FramePack,
                scope,
                race,
                std::move(relativePath),
                std::move(relativeIndexPath),
                required,
                true,
                false,
                0,
                0,
                0
            });
        }

        void AddIfr(
            std::vector<LegacyUiSpriteRecord>& records,
            std::string key,
            std::string legacyDefine,
            std::filesystem::path relativePath,
            bool required)
        {
            records.push_back(LegacyUiSpriteRecord{
                std::move(key),
                std::move(legacyDefine),
                LegacyUiSpriteKind::InterfaceText,
                LegacyUiSpriteScope::Text,
                LegacyUiRace::Shared,
                std::move(relativePath),
                std::nullopt,
                required,
                false,
                false,
                0,
                0,
                0
            });
        }
    }

    const LegacyUiSpriteRecord* LegacyUiSpriteCatalogResult::Find(const std::string_view key) const noexcept
    {
        const auto it = std::find_if(records.begin(), records.end(), [&](const LegacyUiSpriteRecord& record) {
            return record.key == key;
        });
        return it == records.end() ? nullptr : &(*it);
    }

    std::vector<const LegacyUiSpriteRecord*> LegacyUiSpriteCatalogResult::ByScope(const LegacyUiSpriteScope scope) const
    {
        std::vector<const LegacyUiSpriteRecord*> result;
        for (const auto& record : records)
        {
            if (record.scope == scope)
            {
                result.push_back(&record);
            }
        }
        return result;
    }

    std::vector<const LegacyUiSpriteRecord*> LegacyUiSpriteCatalogResult::GlobalForRace(const LegacyUiRace race) const
    {
        std::vector<const LegacyUiSpriteRecord*> result;
        for (const auto& record : records)
        {
            if (record.scope == LegacyUiSpriteScope::Global &&
                (record.race == LegacyUiRace::Shared || record.race == race))
            {
                result.push_back(&record);
            }
        }
        return result;
    }

    std::filesystem::path LegacyUiSpriteCatalogResult::ResolvePath(const LegacyUiSpriteRecord& record) const
    {
        return (dataRoot / record.relativePath).lexically_normal();
    }

    std::filesystem::path LegacyUiSpriteCatalogResult::ResolveIndexPath(const LegacyUiSpriteRecord& record) const
    {
        if (record.relativeIndexPath)
        {
            return (dataRoot / *record.relativeIndexPath).lexically_normal();
        }

        auto index = record.relativePath;
        index.replace_extension(L".spki");
        return (dataRoot / index).lexically_normal();
    }

    LegacyUiSpriteCatalogResult LegacyUiSpriteCatalog::Build(const std::filesystem::path& dataRoot) const
    {
        LegacyUiSpriteCatalogResult result;
        result.dataRoot = dataRoot;
        auto& r = result.records;

        AddRpk(r, "RPK_ITEM", "RPK_ITEM", L"UI\\txt\\item.rpk", true);
        AddRpk(r, "RPK_HELP", "RPK_HELP", L"UI\\txt\\help.rpk", true);
        AddRpk(r, "RPK_SKILL", "RPK_SKILL", L"UI\\txt\\skill.rpk", true);
        AddRpk(r, "RPK_BOOK", "RPK_BOOK", L"UI\\txt\\book.rpk", false);
        AddRpk(r, "RPK_TUTORIAL_ETC", "RPK_TUTORIAL_ETC", L"UI\\txt\\tutorialetc.rpk", false);
        AddRpk(r, "RPK_PROGRESS", "RPK_PROGRESS", L"UI\\txt\\progress.rpk", true);
        AddRpk(r, "RPK_TITLE", "RPK_TITLE", L"UI\\txt\\title.rpk", true);
        AddIfr(r, "IFR_UI", "IFR_UI", L"UI\\txt\\ui.ifr", true);

        Add(r, "SPK_SCROLL_BAR", "SPK_SCROLL_BAR", LegacyUiSpriteScope::Widget, L"UI\\spk\\scrollbar.spk", true, true, 1);
        Add(r, "SPK_SCROLL_BAR_RENEWAL", "SPK_SCROLL_BAR", LegacyUiSpriteScope::Widget, L"UI\\spk\\new\\renewalscrollbar\\00000.spk", false, false, 1);
        Add(r, "SPK_BOX", "SPK_BOX", LegacyUiSpriteScope::Global, L"UI\\spk\\assemblemessagebox.spk", true, true, 1);
        Add(r, "SPK_COMMON", "SPK_COMMON", LegacyUiSpriteScope::Global, L"UI\\spk\\Common.spk", false, false, 1);
        Add(r, "SPK_COMMON_1024", "SPK_COMMON_1024", LegacyUiSpriteScope::Global, L"UI\\spk\\Common1024.spk", false, false, 1);
        Add(r, "SPK_COMMON_BUTTON", "SPK_COMMON_BUTTON", LegacyUiSpriteScope::Widget, L"UI\\spk\\commonbutton.spk", false, false, 1);
        Add(r, "SPK_MOUSE_POINTER", "SPK_MOUSE_POINTER", LegacyUiSpriteScope::Widget, L"UI\\spk\\mousepointer.spk", true, true, 1);
        Add(r, "SPK_FILE_DIALOG", "SPK_FILE_DIALOG", LegacyUiSpriteScope::Widget, L"UI\\spk\\filedialog.spk", true, true, 1);
        Add(r, "SPK_QUESTION", "SPK_QUESTION", LegacyUiSpriteScope::Widget, L"UI\\spk\\question.spk", true, true, 1);
        Add(r, "SPK_USE_GRADE", "SPK_USE_GRADE", LegacyUiSpriteScope::Widget, L"UI\\spk\\usegrade.spk", true, true, 1);
        Add(r, "SPK_USE_GRADE_RENEWAL", "SPK_USE_GRADE", LegacyUiSpriteScope::Widget, L"UI\\spk\\new\\renewalusegrade\\00000.spk", false, false, 1);
        Add(r, "SPK_NEW_NPC_DIALOG", "SPK_NEW_NPC_DIALOG", LegacyUiSpriteScope::Widget, L"UI\\spk\\npcdialog.spk", false, false, 1);
        Add(r, "SPK_NPC_ICON", "SPK_NPC_ICON", LegacyUiSpriteScope::Widget, L"UI\\spk\\npcicon.spk", false, false, 1);
        Add(r, "SPK_WINDOW_RENEWAL_00000", "SPK_WINDOW_RENEWAL", LegacyUiSpriteScope::Widget, L"UI\\spk\\new\\windowrenewal\\00000.spk", false, false, 1);
        Add(r, "SPK_WINDOW_RENEWAL_00016", "SPK_WINDOW_RENEWAL", LegacyUiSpriteScope::Widget, L"UI\\spk\\new\\windowrenewal\\00016.spk", false, false, 1);

        Add(r, "SPK_ASSEMBLE_BOX_SLAYER", "SPK_ASSEMBLE_BOX_SLAYER", LegacyUiSpriteScope::Global, L"UI\\spk\\assemblemessageboxslayer.spk", true, true, 12, LegacyUiRace::Slayer);
        Add(r, "SPK_ASSEMBLE_BOX_BUTTON_SLAYER", "SPK_ASSEMBLE_BOX_BUTTON_SLAYER", LegacyUiSpriteScope::Global, L"UI\\spk\\assemblemessageboxbuttonslayer.spk", true, true, 166, LegacyUiRace::Slayer);
        Add(r, "SPK_ASSEMBLE_BOX_VAMPIRE", "SPK_ASSEMBLE_BOX_VAMPIRE", LegacyUiSpriteScope::Global, L"UI\\spk\\assemblemessageboxvampire.spk", true, true, 12, LegacyUiRace::Vampire);
        Add(r, "SPK_ASSEMBLE_BOX_BUTTON_VAMPIRE", "SPK_ASSEMBLE_BOX_BUTTON_VAMPIRE", LegacyUiSpriteScope::Global, L"UI\\spk\\assemblemessageboxbuttonvampire.spk", true, true, 166, LegacyUiRace::Vampire);
        Add(r, "SPK_ASSEMBLE_BOX_OUSTERS", "SPK_ASSEMBLE_BOX_OUSTERS", LegacyUiSpriteScope::Global, L"UI\\spk\\assemblemessageboxousters.spk", true, true, 12, LegacyUiRace::Ousters);
        Add(r, "SPK_ASSEMBLE_BOX_BUTTON_OUSTERS", "SPK_ASSEMBLE_BOX_BUTTON_OUSTERS", LegacyUiSpriteScope::Global, L"UI\\spk\\assemblemessageboxbuttonousters.spk", true, true, 166, LegacyUiRace::Ousters);
        Add(r, "SPK_INFO_SLAYER", "SPK_INFO_SLAYER", LegacyUiSpriteScope::Global, L"UI\\spk\\infoslayer.spk", true, true, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_INFO_VAMPIRE", "SPK_INFO_VAMPIRE", LegacyUiSpriteScope::Global, L"UI\\spk\\infovampire.spk", true, true, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_INFO_OUSTERS", "SPK_INFO_OUSTERS", LegacyUiSpriteScope::Global, L"UI\\spk\\infoousters.spk", true, true, 1, LegacyUiRace::Ousters);

        Add(r, "SPK_TITLE_MENU_DEFAULT", "SPK_TITLE_MENU_DEFAULT", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\TitleMenuDefault.spk", true, true, 1);
        Add(r, "SPK_TITLE_MENU_SELECT", "SPK_TITLE_MENU_SELECT", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\TitleMenuSelect.spk", false, false, 1);
        Add(r, "SPK_LOGIN_MENU", "SPK_LOGIN_MENU", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\LoginMenu.spk", false, false, 1);
        Add(r, "SPK_TITLE", "SPK_TITLE", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\Title.spk", false, false, 1);
        Add(r, "SPK_TITLE_1024", "SPK_TITLE_1024", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\Title1024.spk", false, false, 1);
        Add(r, "SPK_NEW_TITLE", "SPK_NEW_TITLE", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\new\\renewalgametitle\\00000.spk", true, true, 1, LegacyUiRace::Shared, std::nullopt, false);
        Add(r, "SPK_NEW_ID_CREATE", "SPK_NEW_ID_CREATE", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\new\\newidcreate\\NewIDCreate.spk", true, true, 1, LegacyUiRace::Shared, std::nullopt, false);
        Add(r, "SPK_SERVER_SELECT_RENEWAL", "SPK__NEW_SERVER_SELECT", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\new\\serverselectrenewal\\00000.spk", true, true, 1);
        Add(r, "SPK_LOGIN", "SPK_LOGIN", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\Login.spk", false, false, 1);
        Add(r, "SPK_LOGIN_1024", "SPK_LOGIN_1024", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\Login1024.spk", false, false, 1);
        Add(r, "SPK_PROGRESS", "SPK_PROGRESS", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\Progress.spk", true, true, 1);
        Add(r, "SPK_PROGRESS_1024", "SPK_PROGRESS_1024", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\Progress1024.spk", false, false, 1);
        Add(r, "SPK_PROGRESS_CHARACTER", "SPK_PROGRESS_CHARACTER", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\ProgressChar.spk", true, false, 1);
        Add(r, "SPK_PROGRESS_EVENT", "SPK_PROGRESS_EVENT", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\progressevent.spk\\00000.spk", true, true, 1, LegacyUiRace::Shared, L"UI\\spk\\progressevent.spki", false);
        Add(r, "SPK_PROGRESS_EVENT_1024", "SPK_PROGRESS_EVENT_1024", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\progressevent1024.spk\\00000.spk", false, false, 1, LegacyUiRace::Shared, L"UI\\spk\\progressevent1024.spki", false);
        Add(r, "SPK_PROGRESS_PPL", "SPK_PROGRESS_PPL", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\ProgressPPL.spk", false, false, 1);
        Add(r, "SPK_PROGRESS_PPL_1024", "SPK_PROGRESS_PPL_1024", LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\ProgressPPL1024.spk", false, false, 1);

        Add(r, "SPK_CHAR_MANAGER", "SPK_CHAR_MANAGER", LegacyUiSpriteScope::Character, L"UI\\spk\\CharManager.spk", false, false, 1);
        Add(r, "SPK_CHAR_MANAGER_RENEWAL", "SPK_CHAR_MANAGER_RENEWAL", LegacyUiSpriteScope::Character, L"UI\\spk\\new\\charmanagerrenewal\\00000.spk", true, true, 1);
        Add(r, "SPK_CHAR_CREATE", "SPK_CHAR_CREATE", LegacyUiSpriteScope::Character, L"UI\\spk\\CharCreate.spk", false, false, 1);
        Add(r, "SPK_CHAR_CREATE_NEW", "SPK_CHAR_CREATE_NEW", LegacyUiSpriteScope::Character, L"UI\\spk\\new\\charcreaterenewal\\00000.spk", true, true, 1);
        Add(r, "SPK_FACE", "SPK_FACE", LegacyUiSpriteScope::Character, L"UI\\spk\\face.spk", true, false, 1);
        Add(r, "SPK_FACE_MAKE", "SPK_FACE_MAKE", LegacyUiSpriteScope::Character, L"UI\\spk\\facemake.spk", true, false, 1);
        Add(r, "SPK_PARTY_FACE", "SPK_PARTY_FACE", LegacyUiSpriteScope::Character, L"UI\\spk\\faceparty.spk", true, false, 1);
        AddIndexed(r, "ISPK_SLAYER_MAN", "ISPK_SLAYER_MAN", LegacyUiSpriteScope::Character, L"Image\\AddonMan.ispk", L"Image\\AddonMan.ispki", true, LegacyUiRace::Slayer);
                AddIndexed(r, "ISPK_SLAYER_WOMAN", "ISPK_SLAYER_WOMAN", LegacyUiSpriteScope::Character, L"Image\\AddonWoman.ispk", L"Image\\AddonWoman.ispki", true, LegacyUiRace::Slayer);
                AddIndexed(r, "ISPK_VAMPIRE", "ISPK_VAMPIRE", LegacyUiSpriteScope::Character, L"Image\\Vampire.ispk", L"Image\\Vampire.ispki", true, LegacyUiRace::Vampire);
                AddIndexed(r, "ISPK_OUSTERS", "ISPK_OUSTERS", LegacyUiSpriteScope::Character, L"Image\\Ousters.ispk", L"Image\\Ousters.ispki", true, LegacyUiRace::Ousters);
                AddIndexed(r, "ISPK_ADVANCEMENT_SLAYER_MAN", "ISPK_ADVANCEMENT_SLAYER_MAN", LegacyUiSpriteScope::Character, L"Image\\ACSlayerMan.ispk", L"Image\\ACSlayerMan.ispki", false, LegacyUiRace::Slayer);
                AddIndexed(r, "ISPK_ADVANCEMENT_SLAYER_WOMAN", "ISPK_ADVANCEMENT_SLAYER_WOMAN", LegacyUiSpriteScope::Character, L"Image\\ACSlayerWoman.ispk", L"Image\\ACSlayerWoman.ispki", false, LegacyUiRace::Slayer);
                AddIndexed(r, "ISPK_ADVANCEMENT_VAMPIRE_MAN", "ISPK_ADVANCEMENT_VAMPIRE_MAN", LegacyUiSpriteScope::Character, L"Image\\ACVampireMan.ispk", L"Image\\ACVampireMan.ispki", false, LegacyUiRace::Vampire);
                AddIndexed(r, "ISPK_ADVANCEMENT_VAMPIRE_WOMAN", "ISPK_ADVANCEMENT_VAMPIRE_WOMAN", LegacyUiSpriteScope::Character, L"Image\\ACVampireWoman.ispk", L"Image\\ACVampireWoman.ispki", false, LegacyUiRace::Vampire);
                AddIndexed(r, "ISPK_ADVANCEMENT_OUSTERS", "ISPK_ADVANCEMENT_OUSTERS", LegacyUiSpriteScope::Character, L"Image\\ACOusters.ispk", L"Image\\ACOusters.ispki", false, LegacyUiRace::Ousters);
                AddIndexed(r, "ISPK_FACE", "ISPK_FACE", LegacyUiSpriteScope::Character, L"UI\\spk\\face.ispk", L"UI\\spk\\face.ispki", true);
                AddFramePack(r, "CFPK_SLAYER_MAN", "CFPK_SLAYER_MAN", LegacyUiSpriteScope::Character, L"Image\\AddonMan.cfpk", L"Image\\AddonMan.cfpki", true, LegacyUiRace::Slayer);
        AddFramePack(r, "CFPK_SLAYER_WOMAN", "CFPK_SLAYER_WOMAN", LegacyUiSpriteScope::Character, L"Image\\AddonWoman.cfpk", L"Image\\AddonWoman.cfpki", true, LegacyUiRace::Slayer);
        AddFramePack(r, "CFPK_VAMPIRE", "CFPK_VAMPIRE", LegacyUiSpriteScope::Character, L"Image\\Vampire.cfpk", L"Image\\Vampire.cfpki", true, LegacyUiRace::Vampire);
        AddFramePack(r, "CFPK_OUSTERS", "CFPK_OUSTERS", LegacyUiSpriteScope::Character, L"Image\\Ousters.cfpk", L"Image\\Ousters.cfpki", true, LegacyUiRace::Ousters);
        AddFramePack(r, "CFPK_ADVANCEMENT_SLAYER_MAN", "CFPK_ADVANCEMENT_SLAYER_MAN", LegacyUiSpriteScope::Character, L"Image\\ACSlayerMan.cfpk", L"Image\\ACSlayerMan.cfpki", false, LegacyUiRace::Slayer);
        AddFramePack(r, "CFPK_ADVANCEMENT_SLAYER_WOMAN", "CFPK_ADVANCEMENT_SLAYER_WOMAN", LegacyUiSpriteScope::Character, L"Image\\ACSlayerWoman.cfpk", L"Image\\ACSlayerWoman.cfpki", false, LegacyUiRace::Slayer);
        AddFramePack(r, "CFPK_ADVANCEMENT_VAMPIRE_MAN", "CFPK_ADVANCEMENT_VAMPIRE_MAN", LegacyUiSpriteScope::Character, L"Image\\ACVampireMan.cfpk", L"Image\\ACVampireMan.cfpki", false, LegacyUiRace::Vampire);
        AddFramePack(r, "CFPK_ADVANCEMENT_VAMPIRE_WOMAN", "CFPK_ADVANCEMENT_VAMPIRE_WOMAN", LegacyUiSpriteScope::Character, L"Image\\ACVampireWoman.cfpk", L"Image\\ACVampireWoman.cfpki", false, LegacyUiRace::Vampire);
        AddFramePack(r, "CFPK_ADVANCEMENT_OUSTERS", "CFPK_ADVANCEMENT_OUSTERS", LegacyUiSpriteScope::Character, L"Image\\ACOusters.cfpk", L"Image\\ACOusters.cfpki", false, LegacyUiRace::Ousters);

        Add(r, "SPK_SLAYER_MAIN", "SPK_SLAYER_MAIN", LegacyUiSpriteScope::Hud, L"UI\\spk\\simpleinformation.spk", true, true, 32, LegacyUiRace::Slayer);
        Add(r, "SPK_VAMPIRE_MAIN", "SPK_VAMPIRE_MAIN", LegacyUiSpriteScope::Hud, L"UI\\spk\\simpleinformation.spk", true, true, 32, LegacyUiRace::Vampire);
        Add(r, "SPK_OUSTERS_MAIN", "SPK_OUSTERS_MAIN", LegacyUiSpriteScope::Hud, L"UI\\spk\\simpleinformation.spk", true, true, 32, LegacyUiRace::Ousters);
        // HPBAR: monolithic hpbar.spk (new location: UI\spk\new\hpbar\hpbar.spk)
        // Single file with .spki index, frames 0-49 directly accessible
        Add(r, "SPK_HPBAR", "SPK_HPBAR", LegacyUiSpriteScope::Hud, L"UI\\spk\\new\\hpbar\\hpbar.spk", true, true, 50, LegacyUiRace::Shared, L"UI\\spk\\new\\hpbar\\hpbar.spki", true, 0, 0);
        Add(r, "SPK_EFFECT_STATUS_SLAYER", "SPK_EFFECT_STATUS_SLAYER", LegacyUiSpriteScope::Hud, L"UI\\spk\\effectstatusslayer.spk", true, false, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_EFFECT_STATUS_VAMPIRE", "SPK_EFFECT_STATUS_VAMPIRE", LegacyUiSpriteScope::Hud, L"UI\\spk\\effectstatusvampire.spk", true, false, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_EFFECT_STATUS_OUSTERS", "SPK_EFFECT_STATUS_OUSTERS", LegacyUiSpriteScope::Hud, L"UI\\spk\\effectstatusousters.spk", true, false, 1, LegacyUiRace::Ousters);

        Add(r, "SPK_SLAYER_INVENTORY", "SPK_SLAYER_INVENTORY", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\inventoryslayer.spk", true, true, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_VAMPIRE_INVENTORY", "SPK_VAMPIRE_INVENTORY", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\inventoryvampire.spk", true, true, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_OUSTERS_INVENTORY", "SPK_OUSTERS_INVENTORY", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\inventoryousters.spk", true, true, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_RENEWAL_INVENTORY_FOR_ALL", "SPK_RENEWAL_INVENTORY_FOR_ALL", LegacyUiSpriteScope::InventoryGear,
            L"UI\\spk\\renewal_InventoryForAll\\renewal_InventoryForAll.spk", true, true, 1,
            LegacyUiRace::Shared, L"UI\\spk\\renewal_InventoryForAll\\renewal_InventoryForAll.spki");
        Add(r, "SPK_SLAYER_INVENTORY_SUB", "SPK_SLAYER_INVENTORY_SUB", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\inventoryslayersub.spk", true, true, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_VAMPIRE_INVENTORY_SUB", "SPK_VAMPIRE_INVENTORY_SUB", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\inventoryvampiresub.spk", true, true, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_OUSTERS_INVENTORY_SUB", "SPK_OUSTERS_INVENTORY_SUB", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\inventoryousterssub.spk", true, true, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_SLAYER_INVENTORY_SUB_RENEWAL", "SPK_SLAYER_INVENTORY_SUB", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\inventoryslayersubrenewal.spk\\00000.spk", false, false, 1, LegacyUiRace::Slayer, L"UI\\spk\\inventoryslayersubrenewal.spki", false);
        Add(r, "SPK_VAMPIRE_INVENTORY_SUB_RENEWAL", "SPK_VAMPIRE_INVENTORY_SUB", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\inventoryvampiresubrenewal.spk\\00000.spk", false, false, 1, LegacyUiRace::Vampire, L"UI\\spk\\inventoryvampiresubrenewal.spki", false);
        Add(r, "SPK_OUSTERS_INVENTORY_SUB_RENEWAL", "SPK_OUSTERS_INVENTORY_SUB", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\inventoryousterssubrenewal.spk\\00000.spk", false, false, 1, LegacyUiRace::Ousters, L"UI\\spk\\inventoryousterssubrenewal.spki", false);
        Add(r, "SPK_SLAYER_GEAR", "SPK_SLAYER_GEAR", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\gearslayer.spk", true, true, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_VAMPIRE_GEAR", "SPK_VAMPIRE_GEAR", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\gearvampire.spk", true, true, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_OUSTERS_GEAR", "SPK_OUSTERS_GEAR", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\gearousters.spk", true, true, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_SLAYER_GEAR_SLOT", "SPK_SLAYER_GEAR_SLOT", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\gearslotslayer.spk", true, true, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_VAMPIRE_GEAR_SLOT", "SPK_VAMPIRE_GEAR_SLOT", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\gearslotvampire.spk", true, true, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_OUSTERS_GEAR_SLOT", "SPK_OUSTERS_GEAR_SLOT", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\gearslotousters.spk", true, true, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_SLAYER_QUICKITEM", "SPK_SLAYER_QUICKITEM", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\quickitemslayer.spk", true, true, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_OUSTERS_QUICKITEM", "SPK_OUSTERS_QUICKITEM", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\quickitemousters.spk", true, true, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_SHORTCUT_SLOT", "SPK_SHORTCUT_SLOT", LegacyUiSpriteScope::Hud, L"UI\\spk\\shortcutslot.spk", true, true, 1, LegacyUiRace::Shared);
        Add(r, "SPK_SLAYER_QUICKITEM_RENEWAL", "SPK_SLAYER_QUICKITEM", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\new\\qslotslayerrenewal\\00000.spk", false, false, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_OUSTERS_QUICKITEM_RENEWAL_00000", "SPK_OUSTERS_QUICKITEM", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\new\\quickitemoustersrenewal\\00000.spk", false, false, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_OUSTERS_QUICKITEM_RENEWAL_00016", "SPK_OUSTERS_QUICKITEM", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\new\\quickitemoustersrenewal\\00016.spk", false, false, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_ITEM", "SPK_ITEM", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\itemdescription.spk\\00000.spk", true, false, 1, LegacyUiRace::Shared, L"UI\\spk\\itemdescription.spki", false);
        AddIndexed(r, "ISPK_ITEM", "ISPK_ITEM", LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\newitem.ispk", L"UI\\spk\\newitem.ispki", true);

        Add(r, "SPK_SKILL_ICON", "SPK_SKILL_ICON", LegacyUiSpriteScope::Skill, L"UI\\spk\\skillminirenewal2.spk", true, true, 96, LegacyUiRace::Shared, L"UI\\spk\\skillminirenewal2.spki", true);
        Add(r, "SPK_SKILLVIEW_ETC", "SPK_SKILLVIEW_ETC", LegacyUiSpriteScope::Skill, L"UI\\spk\\skillwindow.spk", true, true, 36);
        Add(r, "SPK_RUNE_TAB", "SPK_RUNE_TAB", LegacyUiSpriteScope::Skill, L"UI\\spk\\runetab.spk", true, true, 31, LegacyUiRace::Shared, L"UI\\spk\\runetab.spki", true);
        Add(r, "SPK_SKILLVIEW_SLAYER_DOMAIN", "SPK_SKILLVIEW_SLAYER_DOMAIN", LegacyUiSpriteScope::Skill, L"UI\\spk\\skilletcslayer.spk", true, true, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_SKILLVIEW_VAMPIRE_DOMAIN", "SPK_SKILLVIEW_VAMPIRE_DOMAIN", LegacyUiSpriteScope::Skill, L"UI\\spk\\skilletcvampire.spk", true, true, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_SKILLVIEW_OUSTERS_DOMAIN", "SPK_SKILLVIEW_OUSTERS_DOMAIN", LegacyUiSpriteScope::Skill, L"UI\\spk\\skilletcousters.spk", true, true, 1, LegacyUiRace::Ousters);

        Add(r, "SPK_MINIMAP_SLAYER", "SPK_MINIMAP_SLAYER", LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\minimapslayer.spk", true, true, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_MINIMAP_VAMPIRE", "SPK_MINIMAP_VAMPIRE", LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\minimaprenewal.spk", true, true, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_MINIMAP_OUSTERS", "SPK_MINIMAP_OUSTERS", LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\minimapousters.spk", true, true, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_MINIMAP_RENEWAL", "SPK_MINIMAP_RENEWAL", LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\minimaprenewal.spk\\00000.spk", true, true, 1, LegacyUiRace::Shared, L"UI\\spk\\minimaprenewal.spki", false);
        Add(r, "SPK_WORLD_MAP", "SPK_WORLD_MAP", LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\worldmap.spk", true, true, 1);
        Add(r, "SPK_WORLD_MAP_ICON", "SPK_WORLD_MAP_ICON", LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\worldmapicon.spk", true, true, 1);
        Add(r, "SPK_GPS_SLAYER", "SPK_GPS_SLAYER", LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\gps_slayer.spk", true, false, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_GPS_VAMPIRE", "SPK_GPS_VAMPIRE", LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\gps_vampire.spk", true, false, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_GPS_OUSTERS", "SPK_GPS_OUSTERS", LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\gps_ousters.spk", true, false, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_GPSMAP", "SPK_GPSMAP", LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\gpsmap.spk", true, false, 1);
        Add(r, "SPK_GPS_FOR_ALL_RENEWAL", "SPK_GPSMAP", LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\new\\gpsforall_renewal\\gpsforall_renewal.spk", false, false, 1);

        Add(r, "SPK_SLAYER_SHOP_STORAGE", "SPK_SLAYER_SHOP_STORAGE", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\shop&storageslayer.spk", true, true, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_VAMPIRE_SHOP_STORAGE", "SPK_VAMPIRE_SHOP_STORAGE", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\shop&storagevampire.spk", true, true, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_OUSTERS_SHOP_STORAGE", "SPK_OUSTERS_SHOP_STORAGE", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\shop&storageousters.spk", true, true, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_SHOP_STORAGE_RENEWAL_00000", "SPK_SHOP_STORAGE_RENEWAL", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\new\\shopstoragerenewal\\00000.spk", false, false, 1);
        Add(r, "SPK_SHOP_STORAGE_RENEWAL_00016", "SPK_SHOP_STORAGE_RENEWAL", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\new\\shopstoragerenewal\\00016.spk", false, false, 1);
        Add(r, "SPK_SHOP_STORAGE_RENEWAL_00032", "SPK_SHOP_STORAGE_RENEWAL", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\new\\shopstoragerenewal\\00032.spk", false, false, 1);
        Add(r, "SPK_EXCHANGE_SLAYER", "SPK_EXCHANGE_SLAYER", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\exchangeslayer.spk", true, false, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_EXCHANGE_OUSTERS", "SPK_EXCHANGE_OUSTERS", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\exchangeousters.spk", true, false, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_PERSNALSHOP_SLAYER", "SPK_PERSNALSHOP_SLAYER", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\slayerpersnalshop.spk", true, false, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_PERSNALSHOP_VAMPIRE", "SPK_PERSNALSHOP_VAMPIRE", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\vampirepersnalshop.spk", true, false, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_PERSNALSHOP_OUSTERS", "SPK_PERSNALSHOP_OUSTERS", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\ousterspersnalshop.spk", true, false, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_PERSONALSHOP_RENEWAL", "SPK_PERSNALSHOP_RENEWAL", LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\new\\renewalpersonalshop\\00000.spk", false, false, 1);

        Add(r, "SPK_SLAYER_PARTY", "SPK_SLAYER_PARTY", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\partyslayer.spk", true, false, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_VAMPIRE_PARTY", "SPK_VAMPIRE_PARTY", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\partyrenewal.spk", true, false, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_OUSTERS_PARTY", "SPK_OUSTERS_PARTY", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\partyousters.spk", true, false, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_PARTY_RENEWAL_00000", "SPK_PARTY_RENEWAL", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\new\\partyrenewal\\00000.spk", false, false, 1);
        Add(r, "SPK_PARTY_RENEWAL_00016", "SPK_PARTY_RENEWAL", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\new\\partyrenewal\\00016.spk", false, false, 1);
        Add(r, "SPK_SLAYER_TEAM", "SPK_SLAYER_TEAM", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\teamslayer.spk", true, false, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_VAMPIRE_TEAM", "SPK_VAMPIRE_TEAM", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\teamvampire.spk", true, false, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_OUSTERS_TEAM", "SPK_OUSTERS_TEAM", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\teamousters.spk", true, false, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_SLAYER_TEAM_MEMBER", "SPK_SLAYER_TEAM_MEMBER", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\teammemberslayer.spk", true, false, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_VAMPIRE_TEAM_MEMBER", "SPK_VAMPIRE_TEAM_MEMBER", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\teammembervampire.spk", true, false, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_SLAYER_CHATTING", "SPK_CHATTING_SLAYER", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\ChattingPanelNew.spk", true, true, 57, LegacyUiRace::Slayer, std::nullopt, true, 9);
        Add(r, "SPK_VAMPIRE_CHATTING", "SPK_CHATTING_VAMPIRE", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\ChattingPanelNew.spk", true, true, 57, LegacyUiRace::Vampire, std::nullopt, true, 9);
        Add(r, "SPK_OUSTERS_CHATTING", "SPK_CHATTING_OUSTERS", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\ChattingPanelNew.spk", true, true, 57, LegacyUiRace::Ousters, std::nullopt, true, 9);
        AddCatalogOnly(r, "SPK_VAMPIRE_CHATTING_LEGACY_ALT", "SPK_CHATTING_VAMPIRE_ALT", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\chattingvampire.spk", LegacyUiRace::Vampire);
        Add(r, "SPK_RENEWAL_FRIEND", "SPK_RENEWAL_FRIEND", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\new\\renewalfriend\\00000.spk", false, false, 1);
        Add(r, "SPK_RENEWAL_FRIEND_INFO", "SPK_RENEWAL_FRIEND_INFO", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\new\\renewalfriendinfo\\00000.spk", false, false, 1);
        Add(r, "SPK_RENEWAL_FRIEND_POPUP_00000", "SPK_RENEWAL_FRIEND_POPUP", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\new\\renewalfriendpopup\\00000.spk", false, false, 1);
        Add(r, "SPK_RENEWAL_FRIEND_SCROLL_00000", "SPK_RENEWAL_FRIEND_SCROLL", LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\new\\renewalfriendscroll\\00000.spk", false, false, 1);

        Add(r, "SPK_QUEST_MODIFY", "SPK_QUEST_MODIFY", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\questmodify.spk", true, false, 1);
        Add(r, "SPK_SLAYER_QUESTMANAGER", "SPK_SLAYER_QUESTMAMAGER", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\QuestRenewalR3\\questmanager_slayer_r3.spk", true, false, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_VAMPIRE_QUESTMANAGER", "SPK_VAMPIRE_QUESTMAMAGER", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\QuestRenewalR3\\questmanager_vampire_r3.spk", true, false, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_OUSTERS_QUESTMANAGER", "SPK_OUSTERS_QUESTMAMAGER", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\QuestRenewalR3\\questmanager_ousters_r3.spk", true, false, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_PET_FACE", "SPK_PET_FACE", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petface.spk", true, false, 1);
        Add(r, "SPK_PET_BUTTON_SLAYER", "SPK_PET_BUTTON_SLAYER", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petbuttonslayer.spk", true, false, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_PET_BUTTON_VAMPIRE", "SPK_PET_BUTTON_VAMPIRE", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petbuttonslayer.spk", true, false, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_PET_BUTTON_OUSTERS", "SPK_PET_BUTTON_OUSTERS", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petbuttonslayer.spk", true, false, 1, LegacyUiRace::Ousters);
        Add(r, "SPK_MIXING_FORGE", "SPK_MIXING_FORGE", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\mixingforge.spk", true, false, 1);
        Add(r, "SPK_PET_MIXING_FORGE", "SPK_PET_MIXING_FORGE", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petmixingforge.spk", true, false, 1);
        Add(r, "SPK_LOTTERY_CARD", "SPK_LOTTERY_CARD", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\lotterycard.spk", true, false, 1);
        Add(r, "SPK_BULLETIN_BOARD", "SPK_BULLETIN_BOARD", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\bulletinboard.spk", true, false, 1);
        Add(r, "SPK_CTF_STATUS", "SPK_CTF_STATUS", LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\ctf.spk", true, false, 1);

        // Catalog-only legacy UI assets discovered from VS_UI_filepath.h.
        // They are present under GameData/data, but their frame indices and DX11 draw consumers
        // are not fully parity-mapped yet. Keep them out of eager runtime preload/audit so
        // catalog coverage does not create wrong placement or extra texture-upload cost.
        AddCatalogOnly(r, "SPK_OUSTERS_POWERJJANG", "SPK_OUSTERS_POWERJJANG", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Global, L"UI\\spk\\Powerjjang_Ousters.spk", LegacyUiRace::Ousters);
        AddCatalogOnly(r, "SPK_ANI_SYMBOL", "SPK_ANI_SYMBOL", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Global, L"UI\\spk\\Symbol.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_MONSTER_LEVEL", "SPK_MONSTER_LEVEL", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Global, L"UI\\spk\\monsterlevel.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_SLAYER_POWERJJANG", "SPK_SLAYER_POWERJJANG", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Global, L"UI\\spk\\Powerjjang_Slayer.spk", LegacyUiRace::Slayer);
                AddCatalogOnly(r, "SPK_VAMPIRE_POWERJJANG", "SPK_VAMPIRE_POWERJJANG", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Global, L"UI\\spk\\Powerjjang_Vampire.spk", LegacyUiRace::Vampire);
                AddCatalogOnly(r, "SPK_BLOOD_BRUST_OUSTERS", "SPK_BLOOD_BRUST_OUSTERS", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\bloodburstrenewal.spk\\00000.spk", LegacyUiRace::Ousters); //bloodburst renewal used
                AddCatalogOnly(r, "SPK_GRADE_OUSTERS", "SPK_GRADE_OUSTERS", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"Image\\GradeMarkOusters.spk", LegacyUiRace::Ousters);
                // Ousters GameMenu - now present in GameData, promote from catalog-only to runtime load with smoke decode
                Add(r, "SPK_OUSTERS_GAMEMENU", "SPK_OUSTERS_GAMEMENU", LegacyUiSpriteScope::Hud, L"UI\\spk\\GameMenuOusters.spk", true, true, 26, LegacyUiRace::Ousters, std::nullopt, true, 0, 0);
                AddCatalogOnly(r, "SPK_OUSTERS_SMS_MESSAGE", "SPK_OUSTERS_SMS_MESSAGE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\sms_ousters_main.spk", LegacyUiRace::Ousters);
                AddCatalogOnly(r, "SPK_OUSTERS_SYS_BUTTON", "SPK_OUSTERS_SYS_BUTTON", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\SysButtonOusters.spk", LegacyUiRace::Ousters);
                AddCatalogOnly(r, "SPK_COMBO_CNT", "SPK_COMBO_CNT", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\ComboCnt.spk", LegacyUiRace::Shared);
                AddCatalogOnly(r, "SPK_LEVELUP", "SPK_LEVELUP", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\LevelUp.spk", LegacyUiRace::Shared);
                AddCatalogOnly(r, "SPK_ORIGIN_MARK", "SPK_ORIGIN_MARK", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"Image\\OriginMark.spk", LegacyUiRace::Shared);
                AddCatalogOnly(r, "SPK_TIME_COUNT", "SPK_TIME_COUNT", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\TimeCount.spk", LegacyUiRace::Shared);
                AddCatalogOnly(r, "SPK_BLOOD_BRUST_SLAYER", "SPK_BLOOD_BRUST_SLAYER", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\bloodburstrenewal.spk\\00000.spk", LegacyUiRace::Slayer); //bloodburst renewal used
                AddCatalogOnly(r, "SPK_GRADE_SLAYER", "SPK_GRADE_SLAYER", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"Image\\GradeMarkSlayer.spk", LegacyUiRace::Slayer);
                // Slayer/Vampire GameMenu - promote from catalog-only to runtime load with smoke decode
                Add(r, "SPK_SLAYER_GAMEMENU", "SPK_SLAYER_GAMEMENU", LegacyUiSpriteScope::Hud, L"UI\\spk\\gamemenu.spk", true, true, 26, LegacyUiRace::Slayer, std::nullopt, true, 0, 0);
                AddCatalogOnly(r, "SPK_SLAYER_SMS_MESSAGE", "SPK_SLAYER_SMS_MESSAGE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\sms_slayer_main.spk", LegacyUiRace::Slayer);
                AddCatalogOnly(r, "SPK_SLAYER_SYS_BUTTON", "SPK_SLAYER_SYS_BUTTON", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\SysButtonSlayer.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_BLOOD_BRUST_VAMPIRE", "SPK_BLOOD_BRUST_VAMPIRE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\bloodburstrenewal.spk\\00000.spk", LegacyUiRace::Vampire); //bloodburst renewal used
        AddCatalogOnly(r, "SPK_GRADE_VAMPIRE", "SPK_GRADE_VAMPIRE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"Image\\GradeMarkVampire.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_OPTION_VAMPIRE", "SPK_OPTION_VAMPIRE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\OptionVampire.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_VAMPIRE_GAMEMENU", "SPK_VAMPIRE_GAMEMENU", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\gamemenu.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_VAMPIRE_SMS_MESSAGE", "SPK_VAMPIRE_SMS_MESSAGE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\sms_vampire_main.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_VAMPIRE_SYS_BUTTON", "SPK_VAMPIRE_SYS_BUTTON", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Hud, L"UI\\spk\\SysButtonVampire.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_OUSTERS_ADVANCEMENTGEAR", "SPK_OUSTERS_ADVANCEMENTGEAR", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\GearOustersAdvancement.spk", LegacyUiRace::Ousters);
        AddCatalogOnly(r, "SPK_ITEM_POWERJJANG", "SPK_ITEM_POWERJJANG", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\Powerjjang_Item.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_SLAYER_ADVANCEMENTGEAR", "SPK_SLAYER_ADVANCEMENTGEAR", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\GearSlayerAdvancement.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_VAMPIRE_ADVANCEMENTGEAR", "SPK_VAMPIRE_ADVANCEMENTGEAR", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\myinformation.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_MY_INFORMATION", "SPK_VAMPIRE_ADVANCEMENTGEAR", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::InventoryGear, L"UI\\spk\\myinformation.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "RPK_HOLYLAND_MINIMAP", "RPK_HOLYLAND_MINIMAP", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\HolylandMinimap.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_HORN", "SPK_HORN", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\PortalEtcOusters.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_HORN_MAP", "SPK_HORN_MAP", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\PortalMapOusters.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_MINIMAP", "SPK_MINIMAP", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\Minimap.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_SLAYER_PORTAL_ETC", "SPK_SLAYER_PORTAL_ETC", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\PortalEtcSlayer.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_SLAYER_PORTAL_MAP", "SPK_SLAYER_PORTAL_MAP", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::MapNavigation, L"UI\\spk\\PortalMapSlayer.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_PREMIUM_GIVE_ITEM_OUSTERS", "SPK_PREMIUM_GIVE_ITEM_OUSTERS", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\new\\premiumgiveitem_renewal\\00000.spk", LegacyUiRace::Ousters);
        AddCatalogOnly(r, "ISPK_CREATURE_3", "ISPK_CREATURE_3", LegacyUiSpriteKind::IndexedSpritePack, LegacyUiSpriteScope::QuestEventPet, L"Image\\creature3.ispk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "ISPK_CREATURE_4", "ISPK_CREATURE_4", LegacyUiSpriteKind::IndexedSpritePack, LegacyUiSpriteScope::QuestEventPet, L"Image\\creature4.ispk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "ISPK_CREATURE_5", "ISPK_CREATURE_5", LegacyUiSpriteKind::IndexedSpritePack, LegacyUiSpriteScope::QuestEventPet, L"Image\\creature5.ispk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "ISPK_CREATURE_6", "ISPK_CREATURE_6", LegacyUiSpriteKind::IndexedSpritePack, LegacyUiSpriteScope::QuestEventPet, L"Image\\creature6.ispk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "ISPK_CREATURE_7", "ISPK_CREATURE_7", LegacyUiSpriteKind::IndexedSpritePack, LegacyUiSpriteScope::QuestEventPet, L"Image\\creature7.ispk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_ELEVATOR", "SPK_ELEVATOR", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\Elevator.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_LEVEL_MARK", "SPK_LEVEL_MARK", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"Image\\LevelMarkPet.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_NPCFACE", "SPK_NPCFACE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\NpcFace.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_actiniumruffian", "SPK_PET_INFO_actiniumruffian", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petactiniumruffian.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_canine", "SPK_PET_INFO_canine", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petcanine.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_captain", "SPK_PET_INFO_captain", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petcaptain.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_CENTAURO", "SPK_PET_INFO_CENTAURO", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetCentauro.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_deadbody", "SPK_PET_INFO_deadbody", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petdeadbody.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_degula", "SPK_PET_INFO_degula", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petduracula.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_EML", "SPK_PET_INFO_EML", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetStirge.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_FIREELEMENTAL", "SPK_PET_INFO_FIREELEMENTAL", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetFireElemental.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_GARGOYLE", "SPK_PET_INFO_GARGOYLE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetGargoyle.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_HXZ", "SPK_PET_INFO_HXZ", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetPixie.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_INCUBUS", "SPK_PET_INFO_INCUBUS", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetIncubus.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_kid", "SPK_PET_INFO_kid", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petkid.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_LUPENG", "SPK_PET_INFO_LUPENG", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetLupeng.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_MASHIMARO", "SPK_PET_INFO_MASHIMARO", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetMashimaro.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_MILITARYBEAR", "SPK_PET_INFO_MILITARYBEAR", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetMilitaryBear.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_nepenthespot", "SPK_PET_INFO_nepenthespot", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petnepenthespot.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_PIXIE", "SPK_PET_INFO_PIXIE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetPixie.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_SIAMINGCAT", "SPK_PET_INFO_SIAMINGCAT", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petsiamingcat.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_soldier", "SPK_PET_INFO_soldier", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petsoldier.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_STIRGE", "SPK_PET_INFO_STIRGE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetStirge.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_SUCCUBUS", "SPK_PET_INFO_SUCCUBUS", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetSuccubus.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_TABBYCAT", "SPK_PET_INFO_TABBYCAT", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\pettabbycat.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_thoriumruffian", "SPK_PET_INFO_thoriumruffian", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petthoriumruffian.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_turningdead", "SPK_PET_INFO_turningdead", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petturningdead.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_WEALTHYCAT", "SPK_PET_INFO_WEALTHYCAT", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\petwealthycat.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_WOLFDOG", "SPK_PET_INFO_WOLFDOG", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetWolfDog.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_XKTK", "SPK_PET_INFO_XKTK", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetCentauro.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PET_INFO_YML", "SPK_PET_INFO_YML", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\PetWolfDog.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_SOUL_CHAIN", "SPK_SOUL_CHAIN", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\Trace.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_TUTORIAL_BOOK", "SPK_TUTORIAL_BOOK", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\TutorialBook.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_TUTORIAL_BOOKCASE", "SPK_TUTORIAL_BOOKCASE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\TutorialBookcase.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_TUTORIAL_COMPUTER", "SPK_TUTORIAL_COMPUTER", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\TutorialComputer.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_WEBBROWSER", "SPK_WEBBROWSER", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\WebBrowser.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_XMAS_CARD", "SPK_XMAS_CARD", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\XMasCard.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PREMIUM_GIVE_ITEM_SLAYER", "SPK_PREMIUM_GIVE_ITEM_SLAYER", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\new\\premiumgiveitem_renewal\\00000.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_PREMIUM_GIVE_ITEM_VAMPIRE", "SPK_PREMIUM_GIVE_ITEM_VAMPIRE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::QuestEventPet, L"UI\\spk\\new\\premiumgiveitem_renewal\\00000.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_TAXRATE", "SPK_TAXRATE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\TaxRate.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_SLAYER_BUY_DIALOG", "SPK_SLAYER_BUY_DIALOG", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::ShopTradeStorage, L"UI\\spk\\BuyDialogSlayer.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_OUSTERS_SKILL_ETC", "SPK_OUSTERS_SKILL_ETC", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Skill, L"UI\\spk\\SkillEtcOusters.spk", LegacyUiRace::Ousters);
        AddCatalogOnly(r, "SPK_SKILLVIEW_OUSTERS", "SPK_SKILLVIEW_OUSTERS", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Skill, L"UI\\spk\\skillwindow.spk", LegacyUiRace::Ousters);
        AddCatalogOnly(r, "SPK_SKILLVIEW_OUSTERS_GRADE", "SPK_SKILLVIEW_OUSTERS_GRADE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Skill, L"UI\\spk\\skillwindow.spk", LegacyUiRace::Ousters);
        AddCatalogOnly(r, "SPK_SKILL_MINI", "SPK_SKILL_MINI", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Skill, L"UI\\spk\\skillmini.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_SKILLVIEW_SLAYER", "SPK_SKILLVIEW_SLAYER", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Skill, L"UI\\spk\\skillwindow.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_SKILLVIEW_SLAYER_GRADE", "SPK_SKILLVIEW_SLAYER_GRADE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Skill, L"UI\\spk\\skillwindow.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_SLAYER_SKILL_ETC", "SPK_SLAYER_SKILL_ETC", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Skill, L"UI\\spk\\SkillEtcSlayer.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_SKILLVIEW_VAMPIRE", "SPK_SKILLVIEW_VAMPIRE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Skill, L"UI\\spk\\skillwindow.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_SKILLVIEW_VAMPIRE_GRADE", "SPK_SKILLVIEW_VAMPIRE_GRADE", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Skill, L"UI\\spk\\skillwindow.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_VAMPIRE_SKILL_ETC", "SPK_VAMPIRE_SKILL_ETC", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::Skill, L"UI\\spk\\SkillEtcVampire.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_OUSTERS_NAMING", "SPK_OUSTERS_NAMING", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\NamingOusters.spk", LegacyUiRace::Ousters);
        AddCatalogOnly(r, "SPK_OUSTERS_SMS_LIST", "SPK_OUSTERS_SMS_LIST", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\sms_ousters_list.spk", LegacyUiRace::Ousters);
        AddCatalogOnly(r, "SPK_OUSTERS_SMS_RECORD", "SPK_OUSTERS_SMS_RECORD", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\sms_ousters_record.spk", LegacyUiRace::Ousters);
        AddCatalogOnly(r, "SPK_OUSTERS_SMS_SCROLL", "SPK_OUSTERS_SMS_SCROLL", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\sms_ousters_scroll.spk", LegacyUiRace::Ousters);
        AddCatalogOnly(r, "SPK_FRIEND_COMMON", "SPK_FRIEND_COMMON", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\friendcommon.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_FRIEND_INFO", "SPK_FRIEND_INFO", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\friendinfo.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_FRIEND_SYSTEM", "SPK_FRIEND_SYSTEM", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\friendsystem.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_FRIEND_WAIT", "SPK_FRIEND_WAIT", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\friendwait.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_GUILD_MARK", "SPK_GUILD_MARK", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"Image\\GuildMark.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_MAILBOX", "SPK_MAILBOX", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\MailBox.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PARTY_CURSOR", "SPK_PARTY_CURSOR", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\PartyCursor.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PLAYER_POPUP", "SPK_PLAYER_POPUP", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\characterpopupmenu.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_SLAYER_NAMING", "SPK_SLAYER_NAMING", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\NamingSlayer.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_SLAYER_SMS_LIST", "SPK_SLAYER_SMS_LIST", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\sms_slayer_list.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_SLAYER_SMS_RECORD", "SPK_SLAYER_SMS_RECORD", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\sms_slayer_record.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_SLAYER_SMS_SCROLL", "SPK_SLAYER_SMS_SCROLL", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\sms_slayer_scroll.spk", LegacyUiRace::Slayer);
        AddCatalogOnly(r, "SPK_VAMPIRE_NAMING", "SPK_VAMPIRE_NAMING", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\NamingVampire.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_VAMPIRE_SMS_LIST", "SPK_VAMPIRE_SMS_LIST", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\sms_vampire_list.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_VAMPIRE_SMS_RECORD", "SPK_VAMPIRE_SMS_RECORD", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\sms_vampire_record.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_VAMPIRE_SMS_SCROLL", "SPK_VAMPIRE_SMS_SCROLL", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::SocialSystem, L"UI\\spk\\sms_vampire_scroll.spk", LegacyUiRace::Vampire);
        AddCatalogOnly(r, "SPK_MINE_PROGRESS", "SPK_MINE_PROGRESS", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\MineProgress.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_PROGRESS_CI", "SPK_PROGRESS_CI", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\ProgressCI.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_SERVER_SELECT", "SPK_SERVER_SELECT", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\ServerSelect.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_TITLE_OPTION_1024", "SPK_TITLE_OPTION_1024", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\OptionTitle1024.spk", LegacyUiRace::Shared);
        AddCatalogOnly(r, "SPK_TITLE_OPTION_SCROLL", "SPK_TITLE_OPTION_SCROLL", LegacyUiSpriteKind::SpritePack, LegacyUiSpriteScope::TitleLogin, L"UI\\spk\\OptionTitleScroll.spk", LegacyUiRace::Shared);

        return result;
    }

    const wchar_t* ToDisplayName(const LegacyUiSpriteKind kind) noexcept
    {
        switch (kind)
        {
        case LegacyUiSpriteKind::SpritePack:
            return L"SPK";
        case LegacyUiSpriteKind::IndexedSpritePack:
            return L"ISPK";
        case LegacyUiSpriteKind::FramePack:
            return L"CFPK";
        case LegacyUiSpriteKind::ResourcePack:
            return L"RPK";
        case LegacyUiSpriteKind::InterfaceText:
            return L"IFR";
        default:
            return L"Unknown";
        }
    }

    const wchar_t* ToDisplayName(const LegacyUiSpriteScope scope) noexcept
    {
        switch (scope)
        {
        case LegacyUiSpriteScope::Global:
            return L"Global";
        case LegacyUiSpriteScope::Widget:
            return L"Widget";
        case LegacyUiSpriteScope::TitleLogin:
            return L"TitleLogin";
        case LegacyUiSpriteScope::Character:
            return L"Character";
        case LegacyUiSpriteScope::Hud:
            return L"HUD";
        case LegacyUiSpriteScope::InventoryGear:
            return L"InventoryGear";
        case LegacyUiSpriteScope::Skill:
            return L"Skill";
        case LegacyUiSpriteScope::MapNavigation:
            return L"MapNavigation";
        case LegacyUiSpriteScope::ShopTradeStorage:
            return L"ShopTradeStorage";
        case LegacyUiSpriteScope::SocialSystem:
            return L"SocialSystem";
        case LegacyUiSpriteScope::QuestEventPet:
            return L"QuestEventPet";
        case LegacyUiSpriteScope::Text:
            return L"Text";
        default:
            return L"Unknown";
        }
    }

    const wchar_t* ToDisplayName(const LegacyUiRace race) noexcept
    {
        switch (race)
        {
        case LegacyUiRace::Slayer:
            return L"Slayer";
        case LegacyUiRace::Vampire:
            return L"Vampire";
        case LegacyUiRace::Ousters:
            return L"Ousters";
        case LegacyUiRace::Shared:
        default:
            return L"Shared";
        }
    }
}
