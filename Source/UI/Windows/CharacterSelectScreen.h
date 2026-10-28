#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace phoenix::ui
{

// Race enum (matching Legacy RACE_*)
enum class Race : std::uint8_t
{
    Slayer = 0,
    Vampire = 1,
    Ousters = 2,
    Count = 3
};

// Gender enum
enum class Gender : std::uint8_t
{
    Male = 0,
    Female = 1
};

// Face selection
enum class FaceSelection : std::uint8_t
{
    Left = 0,
    Center = 1,
    Right = 2
};

// Character stat types
enum class StatType : std::uint8_t
{
    Str = 0,
    Dex = 1,
    Int = 2,
    Max = 3
};

// Slayer class icons
enum class SlayerClassIcon : std::uint8_t
{
    Blade = 0,
    Sword = 1,
    Gun = 2,
    Heal = 3,
    Enchant = 4,
    Max = 5
};

// Stet (stat) buttons
enum class StetButton : std::uint8_t
{
    StrPlus = 0,
    StrMinus = 1,
    DexPlus = 2,
    DexMinus = 3,
    IntPlus = 4,
    IntMinus = 5,
    Max = 6
};

// Character slot enum
enum class CharacterSlot : std::uint8_t
{
    Left = 0,
    Center = 1,
    Right = 2,
    Count = 3
};

// Character grade names for Slayer
inline const char* SLAYER_GRADE_NAMES[20] = {
    "Private", "Sergeant", "Feacel", "Lieutenant", "General",
    "Fore General", "Major General", "Cloel General", "Marshal",
    "Initiate Slayer", "Vanguard Overseer", "Tactical Archon",
    "Elite Operative", "Cyber Commander", "Black General",
    "Supreme War General", "High-Tech Marshal", "Slayer Overlord", "Omega Dominus"
};

// Character grade names for Vampire
inline const char* VAMPIRE_GRADE_NAMES[20] = {
    "Ritter", "Reichsritter", "Baronet", "Preyher", "Graf",
    "Markgraf", "Pfalzgraf", "Furst", "Herzog", "Landesher",
    "Blood Acolyte", "Night Ritter", "Crimson Butcher",
    "Veil Predator", "Blood Tyrant", "Dread Markgraf",
    "Palatine Of Night", "Crimson Furst", "Sanguine Herzog", "Nocturnal Landesher"
};

// Character slot info (matching Legacy S_SLOT / MAN_INFO)
struct CharacterSlotInfo
{
    bool isSet{ false };
    Race race{ Race::Slayer };
    std::string name;
    int level{ 0 };
    int strPure{ 0 };
    int dexPure{ 0 };
    int intPure{ 0 };
    int domainSword{ 0 };
    int domainBlade{ 0 };
    int domainGun{ 0 };
    int domainHeal{ 0 };
    int domainEnchant{ 0 };
    int fame{ 0 };
    int grade{ 0 };
    int advancementLevel{ -1 };
    int m_AdvancementLevel{ -1 };

    // Additional fields
    int silverHp{ 0 };
    int hp{ 0 };
    int mp{ 0 };
    int hpMax{ 0 };
    int mpMax{ 0 };
    int damage{ 0 };
    int defense{ 0 };
    int protection{ 0 };
    int toHit{ 0 };
    int STR_CUR{ 0 };
    int DEX_CUR{ 0 };
    int INT_CUR{ 0 };
    int STR_MAX{ 0 };
    int DEX_MAX{ 0 };
    int INT_MAX_VAL{ 0 };
    int STR_EXP_REMAIN{ 0 };
    int DEX_EXP_REMAIN{ 0 };
    int INT_EXP_REMAIN{ 0 };
    int EXP_REMAIN{ 0 };
    int CHANGE_VAMPIRE{ 0 };
    int DOMAIN_SWORD{ 0 };
    int DOMAIN_BLADE{ 0 };
    int DOMAIN_GUN{ 0 };
    int DOMAIN_HEAL{ 0 };
    int DOMAIN_ENCHANT{ 0 };
    int FAME{ 0 };
    int GRADE{ 0 };
    int GRADE_EXP_REMAIN{ 0 };
    int GUILD_ID{ 0 };
    int GUILD_GRADE{ 0 };
    int WS{ 0 };
    int WeaponSpeed{ 0 };
    int bonusPoint{ 0 };
    int skillPoint{ 0 };
    int AttackBloodBurstPoint{ 0 };
    int DefenseBloodBurstPoint{ 0 };
    int PartyBloodBurstPoint{ 0 };
    int MagicRemainSec{ 0 };
    int MagicRemainTick{ 0 };
    int alignment{ 0 };
    int alignmentNum{ 0 };
    int ElementalFire{ 0 };
    int ElementalWater{ 0 };
    int ElementalEarth{ 0 };
    int ElementalWind{ 0 };
    int SMS_Charge{ 0 };
    int Powerjjang_Point{ 0 };
    std::string sz_name;
    std::string sz_guild_name;
    std::string STATUS;
};

// Character info strings for display
struct CharacterInfoStrings
{
    int slotIndex{ -1 };
    std::string strName;
    std::string strLevel;
    std::string strStrength;
    std::string strDex;
    std::string strInt;
    std::string strSwordLevel;
    std::string strBladeLevel;
    std::string strGunLevel;
    std::string strHealLevel;
    std::string strEnchantLevel;
    std::string strFame;
    std::string strGrade;
};

// Button IDs for character manager
enum class CharManagerButtonId : std::uint8_t
{
    Back = 0,
    Next = 1,
    Delete = 2,
    Create = 3,
    CharSelectLeft = 4,
    CharSelectCenter = 5,
    CharSelectRight = 6
};

class CharacterSelectScreen
{
public:
    explicit CharacterSelectScreen() noexcept;

    // State
    void Open() noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const settings::CharacterSelectProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // Character data
    void SetCharacters(const std::array<CharacterSlotInfo, 3>& slots) noexcept;
    [[nodiscard]] const CharacterSlotInfo* GetSelectedCharacter() const noexcept;
    [[nodiscard]] CharacterSlot GetSelectedSlot() const noexcept { return selectedSlot_; }

    // Callbacks
    using BackCallback = std::function<void()>;
    using StartCallback = std::function<void(CharacterSlot slot)>;
    using CreateCallback = std::function<void(CharacterSlot slot)>;
    using DeleteCallback = std::function<void(CharacterSlot slot)>;

    void SetBackCallback(BackCallback cb) noexcept { backCallback_ = std::move(cb); }
    void SetStartCallback(StartCallback cb) noexcept { startCallback_ = std::move(cb); }
    void SetCreateCallback(CreateCallback cb) noexcept { createCallback_ = std::move(cb); }
    void SetDeleteCallback(DeleteCallback cb) noexcept { deleteCallback_ = std::move(cb); }

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

private:
    // State
    bool isOpen_{ false };
    CharacterSlot selectedSlot_{ CharacterSlot::Left };
    CharacterSlot focusSlot_{ CharacterSlot::Left };
    int animationAlpha_{ 31 };
    bool animationUp_{ false };
    int charUpdateTid_{ 0 };
    int charIndex_{ 0 };
    int focusedHelp_{ 0 };
    int backgroundPicId_{ -1 };

    // Character data
    std::array<CharacterSlotInfo, 3> characterSlots_;
    std::vector<CharacterInfoStrings> charInfoStrings_;

    // Config
    settings::CharacterSelectProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // UI Elements
    std::unique_ptr<ButtonGroup> buttonGroup_;
    std::unique_ptr<SpriteArchive> imageSpk_;
    std::unique_ptr<SpriteArchive> imageHays0_;
    std::unique_ptr<SpriteArchive> imageHays1_;
    std::unique_ptr<SpriteArchive> imageHays2_;
    std::unique_ptr<SpriteArchive> useGradeSpk_;
    std::unique_ptr<SpriteArchive> netmarbleAgreementSpk_;

    // Window position
    std::int32_t windowX_{ 0 };
    std::int32_t windowY_{ 0 };
    std::int32_t windowWidth_{ 1280 };
    std::int32_t windowHeight_{ 960 };

    // Callbacks
    BackCallback backCallback_;
    StartCallback startCallback_;
    CreateCallback createCallback_;
    DeleteCallback deleteCallback_;

    // Helpers
    void InitializeSPKs() noexcept;
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;
    void UpdateResolution() noexcept;
    [[nodiscard]] bool IsPointInWindow(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInCharSelectButton(int x, int y, CharacterSlot slot) const noexcept;
    void UpdateAnimation(float deltaTime) noexcept;
    void UpdateCharacterAnimation() noexcept;
    void AddCharInfoString(CharacterSlot slot) noexcept;
    void DeleteCharInfoString() noexcept;
    void DrawCharInfoString() noexcept;
    [[nodiscard]] bool SelectSlot(int n) noexcept;

    // Callbacks
    void OnBackClicked() noexcept;
    void OnNextClicked() noexcept;
    void OnDeleteClicked() noexcept;
    void OnCreateClicked() noexcept;
    void OnCharSelectLeftClicked() noexcept;
    void OnCharSelectCenterClicked() noexcept;
    void OnCharSelectRightClicked() noexcept;
};

} // namespace phoenix::ui