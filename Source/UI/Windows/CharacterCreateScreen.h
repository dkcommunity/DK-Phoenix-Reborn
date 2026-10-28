#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"
#include "UI/Windows/CharacterSelectScreen.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <array>

namespace phoenix::ui
{

// CharacterCreate config from interface.ini [CharacterCreate_1280x960]
struct CharacterCreateProfile
{
    // Window (full screen)
    std::int32_t windowX{ 0 };
    std::int32_t windowY{ 0 };
    std::int32_t windowWidth{ 0 };
    std::int32_t windowHeight{ 0 };
    bool topmost{ true };
    bool keyboardControl{ true };

    // SPK files
    std::string imageSpkName{ "CharCreate.spk" };
    std::string imageHaysSpkName{ "CharCreateNew.spk" };
    std::string commonSpkName{ "Common.spk" };
    std::string useGradeSpkName{ "UseGrade.spk" };
    std::string faceMakeSpkName{ "FaceMake.spk" };
    std::string slayerManIspkName{ "SlayerMan.ispk" };
    std::string slayerWomanIspkName{ "SlayerWoman.ispk" };
    std::string vampireIspkName{ "Vampire.ispk" };
    std::string oustersIspkName{ "Ousters.ispk" };
    std::string advancementSlayerManIspkName{ "AdvancementSlayerMan.ispk" };
    std::string advancementSlayerWomanIspkName{ "AdvancementSlayerWoman.ispk" };
    std::string advancementVampireManIspkName{ "AdvancementVampireMan.ispk" };
    std::string advancementVampireWomanIspkName{ "AdvancementVampireWoman.ispk" };
    std::string advancementOustersIspkName{ "AdvancementOusters.ispk" };

    // CFPK files
    std::string slayerManCfpkName{ "SlayerMan.cfpk" };
    std::string slayerWomanCfpkName{ "SlayerWoman.cfpk" };
    std::string vampireCfpkName{ "Vampire.cfpk" };
    std::string oustersCfpkName{ "Ousters.cfpk" };
    std::string advancementOustersCfpkName{ "AdvancementOusters.cfpk" };
    std::string advancementVampireManCfpkName{ "AdvancementVampireMan.cfpk" };
    std::string advancementVampireWomanCfpkName{ "AdvancementVampireWoman.cfpk" };
    std::string advancementSlayerManCfpkName{ "AdvancementSlayerMan.cfpk" };
    std::string advancementSlayerWomanCfpkName{ "AdvancementSlayerWoman.cfpk" };

    // New char box
    std::int32_t newCharBoxX{ 0 };
    std::int32_t newCharBoxY{ 0 };

    // Buttons
    std::int32_t prevButtonX{ 0 };
    std::int32_t prevButtonY{ 0 };
    std::int32_t nextButtonX{ 0 };
    std::int32_t nextButtonY{ 0 };
    std::int32_t charLeftRotationButtonX{ 0 };
    std::int32_t charLeftRotationButtonY{ 0 };
    std::int32_t charRightRotationButtonX{ 0 };
    std::int32_t charRightRotationButtonY{ 0 };
    std::int32_t selectRaceSlayerButtonX{ 0 };
    std::int32_t selectRaceSlayerButtonY{ 0 };
    std::int32_t selectRaceVampireButtonX{ 0 };
    std::int32_t selectRaceVampireButtonY{ 0 };
    std::int32_t selectRaceOustersButtonX{ 0 };
    std::int32_t selectRaceOustersButtonY{ 0 };
    std::int32_t nameCheckButtonX{ 0 };
    std::int32_t nameCheckButtonY{ 0 };

    // Face selection
    std::int32_t selectLeftFaceRadioX{ 0 };
    std::int32_t selectLeftFaceRadioY{ 0 };
    std::int32_t selectCenterFaceRadioX{ 0 };
    std::int32_t selectCenterFaceRadioY{ 0 };
    std::int32_t selectRightFaceRadioX{ 0 };
    std::int32_t selectRightFaceRadioY{ 0 };
    std::int32_t selectLeftFaceX{ 0 };
    std::int32_t selectLeftFaceY{ 0 };
    std::int32_t selectCenterFaceX{ 0 };
    std::int32_t selectCenterFaceY{ 0 };
    std::int32_t selectRightFaceX{ 0 };
    std::int32_t selectRightFaceY{ 0 };

    // Race selection
    std::int32_t selectRaceSlayerRadioX{ 0 };
    std::int32_t selectRaceSlayerRadioY{ 0 };
    std::int32_t selectRaceVampireRadioX{ 0 };
    std::int32_t selectRaceVampireRadioY{ 0 };
    std::int32_t selectRaceOustersRadioX{ 0 };
    std::int32_t selectRaceOustersRadioY{ 0 };
    std::int32_t hideOustersSelectionButtonX{ 0 };
    std::int32_t hideOustersSelectionButtonY{ 0 };

    // Gender selection
    std::int32_t maleOrFemaleSelectBoxX{ 0 };
    std::int32_t maleOrFemaleSelectBoxY{ 0 };
    std::int32_t maleSelectRadioX{ 0 };
    std::int32_t maleSelectRadioY{ 0 };
    std::int32_t maleSelectButtonX{ 0 };
    std::int32_t maleSelectButtonY{ 0 };
    std::int32_t femaleSelectRadioX{ 0 };
    std::int32_t femaleSelectRadioY{ 0 };
    std::int32_t femaleSelectButtonX{ 0 };
    std::int32_t femaleSelectButtonY{ 0 };

    // Name input
    std::int32_t charNameFocusX{ 0 };
    std::int32_t charNameFocusY{ 0 };
    std::int32_t charNameByteLimit{ 11 };

    // Character view
    std::int32_t charViewX{ 0 };
    std::int32_t charViewY{ 0 };

    // Hair color
    std::int32_t hairColorBoxX{ 0 };
    std::int32_t hairColorBoxY{ 0 };
    std::int32_t hairColorStartX{ 0 };
    std::int32_t hairColorStartY{ 0 };

    // Skin color
    std::int32_t skinColorBoxX{ 0 };
    std::int32_t skinColorBoxY{ 0 };
    std::int32_t skinColorStartX{ 0 };
    std::int32_t skinColorStartY{ 0 };

    // OpSet box
    std::int32_t opsetBoxX{ 0 };
    std::int32_t opsetBoxY{ 0 };
    std::int32_t slayerClassIconStartX{ 0 };
    std::int32_t slayerClassIconStartY{ 0 };
    std::int32_t slayerClassIconGap{ 40 };
    std::int32_t slayerClassIconX[5]{ 0 };
    std::int32_t slayerClassIconY[5]{ 0 };
    std::int32_t slayerClassLabelX[5]{ 0 };
    std::int32_t slayerClassLabelY[5]{ 0 };
    bool slayerClassLabelPosEnabled[5]{ false };
    std::int32_t slayerClassLabelYOffset{ 3 };
    std::int32_t slayerCurrentClassTextYOffset{ -18 };

    // Save/Load/Reset buttons
    std::int32_t saveButtonX{ 0 };
    std::int32_t saveButtonY{ 0 };
    std::int32_t loadButtonX{ 0 };
    std::int32_t loadButtonY{ 0 };
    std::int32_t resetButtonX{ 0 };
    std::int32_t resetButtonY{ 0 };

    // Stat text positions
    std::int32_t strTextX{ 0 };
    std::int32_t strTextY{ 0 };
    std::int32_t dexTextX{ 0 };
    std::int32_t dexTextY{ 0 };
    std::int32_t intTextX{ 0 };
    std::int32_t intTextY{ 0 };
    std::int32_t hpTextX{ 0 };
    std::int32_t hpTextY{ 0 };
    std::int32_t mpTextX{ 0 };
    std::int32_t mpTextY{ 0 };
    std::int32_t defenseTextX{ 0 };
    std::int32_t defenseTextY{ 0 };
    std::int32_t protectionTextX{ 0 };
    std::int32_t protectionTextY{ 0 };
    std::int32_t tohitTextX{ 0 };
    std::int32_t tohitTextY{ 0 };
    std::int32_t damTextX{ 0 };
    std::int32_t damTextY{ 0 };

    // Stet box
    std::int32_t stetBoxX{ 0 };
    std::int32_t stetBoxY{ 0 };
    std::int32_t bonusTextX{ 0 };
    std::int32_t bonusTextY{ 0 };
    std::int32_t bonusLabelTextX{ 0 };
    std::int32_t bonusLabelTextY{ 0 };
    std::int32_t stetPlusButtonX[6]{ 0 };
    std::int32_t stetPlusButtonY[6]{ 0 };
    std::int32_t stetMinusButtonX[6]{ 0 };
    std::int32_t stetMinusButtonY[6]{ 0 };

    // Effects
    std::int32_t effectFireLeft1X{ 0 };
    std::int32_t effectFireLeft1Y{ 0 };
    std::int32_t effectFireLeft2X{ 0 };
    std::int32_t effectFireLeft2Y{ 0 };
    std::int32_t effectFireRight1X{ 0 };
    std::int32_t effectFireRight1Y{ 0 };
    std::int32_t effectFireRight2X{ 0 };
    std::int32_t effectFireRight2Y{ 0 };
    std::int32_t effectFireCenterX{ 0 };
    std::int32_t effectFireCenterY{ 0 };

    // Rating
    std::int32_t childX{ 0 };
    std::int32_t childY{ 0 };
    std::int32_t adultX{ 0 };
    std::int32_t adultY{ 0 };
    std::int32_t violenceX{ 0 };
    std::int32_t violenceY{ 0 };
    std::int32_t fearX{ 0 };
    std::int32_t fearY{ 0 };

    // Text
    std::int32_t textBarX{ 0 };
    std::int32_t textBarY{ 0 };
    std::int32_t aniMarkX{ 0 };
    std::int32_t aniMarkY{ 0 };
    std::int32_t textLine1X{ 0 };
    std::int32_t textLine1Y{ 0 };
    std::int32_t textLine2X{ 0 };
    std::int32_t textLine2Y{ 0 };
    std::int32_t textLine3X{ 0 };
    std::int32_t textLine3Y{ 0 };
    std::int32_t textLine4X{ 0 };
    std::int32_t textLine4Y{ 0 };

    // Premium
    std::int32_t premiumUseDayTextX{ 0 };
    std::int32_t premiumUseDayTextY{ 0 };

    // Warning
    std::int32_t warningStringX{ 0 };
    std::int32_t warningStringY{ 0 };
};

// Character slot info for character creation
struct CharacterCreateSlotInfo
{
    Race race{ Race::Slayer };
    Gender gender{ Gender::Male };
    FaceSelection faceSelection{ FaceSelection::Left };
    std::int32_t hairColor{ 0 };
    std::int32_t skinColor{ 0 };
    std::int32_t str{ 10 };
    std::int32_t dex{ 10 };
    std::int32_t int_{ 10 };
    std::int32_t hp{ 0 };
    std::int32_t mp{ 0 };
    std::int32_t defense{ 0 };
    std::int32_t protection{ 0 };
    std::int32_t tohit{ 0 };
    std::int32_t dam{ 0 };
    std::int32_t bonusPoints{ 0 };
    SlayerClassIcon slayerClass{ SlayerClassIcon::Blade };
    std::string name;
    bool nameChecked{ false };
};

class CharacterCreateScreen
{
public:
    CharacterCreateScreen() noexcept;
    ~CharacterCreateScreen() = default;

    // Lifecycle
    void Open() noexcept;
    void Close() noexcept;
    void Toggle() noexcept;

    // Config
    void LoadFromConfig(const settings::CharacterCreateProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // State
    bool IsOpen() const noexcept { return isOpen_; }

    // Character slot management
    void SetSlot(int slotIndex) noexcept;
    CharacterCreateSlotInfo& GetCurrentSlot() noexcept { return slotInfo_; }

    // Input handling
    bool HandleMouseDown(int x, int y, bool rightButton = false) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton = false) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;
    bool HandleChar(char c) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Callbacks
    using BackCallback = std::function<void()>;
    using NextCallback = std::function<void(const CharacterCreateSlotInfo&)>;
    void SetBackCallback(BackCallback cb) noexcept { backCallback_ = std::move(cb); }
    void SetNextCallback(NextCallback cb) noexcept { nextCallback_ = std::move(cb); }

private:
    // SPK loading
    void InitializeSPKs() noexcept;
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;

    // Input helpers
    bool IsPointInWindow(int x, int y) const noexcept;
    bool IsPointInColorBox(int x, int y, bool isSkinColor) const noexcept;
    bool IsPointInSlayerClassIcon(int x, int y, int index) const noexcept;
    bool IsPointInStetButton(int x, int y, int index) const noexcept;

    // Callbacks
    void OnPrevClicked() noexcept;
    void OnNextClicked() noexcept;
    void OnCharLeftRotationClicked() noexcept;
    void OnCharRightRotationClicked() noexcept;
    void OnSelectRaceSlayerClicked() noexcept;
    void OnSelectRaceVampireClicked() noexcept;
    void OnSelectRaceOustersClicked() noexcept;
    void OnNameCheckClicked() noexcept;
    void OnSelectLeftFaceClicked() noexcept;
    void OnSelectCenterFaceClicked() noexcept;
    void OnSelectRightFaceClicked() noexcept;
    void OnMaleSelected() noexcept;
    void OnFemaleSelected() noexcept;
    void OnHairColorClicked(int x, int y) noexcept;
    void OnSkinColorClicked(int x, int y) noexcept;
    void OnSlayerClassIconClicked(int index) noexcept;
    void OnSaveClicked() noexcept;
    void OnLoadClicked() noexcept;
    void OnResetClicked() noexcept;
    void OnStetPlusClicked(int index) noexcept;
    void OnStetMinusClicked(int index) noexcept;

    // Animation
    void UpdateAnimation(float deltaTime) noexcept;
    void UpdateCharacterRotation(float deltaTime) noexcept;

    // Roll dice / random stats
    void RollDice() noexcept;
    void RandomizeCharacter() noexcept;

    // Character model
    void UpdateCharacterModel() noexcept;

    // State
    bool isOpen_{ false };
    settings::CharacterCreateProfile config_;

    // Current slot
    int currentSlotIndex_{ 0 };
    CharacterCreateSlotInfo slotInfo_;

    // Animation
    std::int32_t animationAlpha_{ 31 };
    bool animationUp_{ false };
    float rotationAngle_{ 0.0f };

    // Focus/help
    std::int32_t focusedHelp_{ 0 };

    // SPK archives
    std::unique_ptr<SpriteArchive> imageSpk_;
    std::unique_ptr<SpriteArchive> imageHaysSpk_;
    std::unique_ptr<SpriteArchive> commonSpk_;
    std::unique_ptr<SpriteArchive> useGradeSpk_;
    std::unique_ptr<SpriteArchive> faceMakeSpk_;
    std::unique_ptr<SpriteArchive> slayerManIspk_;
    std::unique_ptr<SpriteArchive> slayerWomanIspk_;
    std::unique_ptr<SpriteArchive> vampireIspk_;
    std::unique_ptr<SpriteArchive> oustersIspk_;
    std::unique_ptr<SpriteArchive> advancementSlayerManIspk_;
    std::unique_ptr<SpriteArchive> advancementSlayerWomanIspk_;
    std::unique_ptr<SpriteArchive> advancementVampireManIspk_;
    std::unique_ptr<SpriteArchive> advancementVampireWomanIspk_;
    std::unique_ptr<SpriteArchive> advancementOustersIspk_;

    // UI components
    std::unique_ptr<ButtonGroup> buttonGroup_;
    std::unique_ptr<LineEditor> nameEditor_;

    // Callbacks
    BackCallback backCallback_;
    NextCallback nextCallback_;
};

} // namespace phoenix::ui