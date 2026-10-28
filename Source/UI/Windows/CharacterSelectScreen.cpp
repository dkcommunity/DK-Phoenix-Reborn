#include "UI/Windows/CharacterSelectScreen.h"

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

CharacterSelectScreen::CharacterSelectScreen() noexcept
    : isOpen_(false)
    , selectedSlot_(CharacterSlot::Left)
    , focusSlot_(CharacterSlot::Left)
    , animationAlpha_(31)
    , animationUp_(false)
    , charUpdateTid_(0)
    , charIndex_(0)
    , focusedHelp_(0)
    , backgroundPicId_(-1)
{
    // Initialize default character slots
    for (int i = 0; i < 3; ++i)
    {
        characterSlots_[i].isSet = false;
    }
}

void CharacterSelectScreen::Open() noexcept
{
    isOpen_ = true;
    selectedSlot_ = CharacterSlot::Left;
    focusSlot_ = CharacterSlot::Left;
    animationAlpha_ = config_.aniArphaStart;
    animationUp_ = false;
    charUpdateTid_ = 0;
    charIndex_ = 0;
    focusedHelp_ = 0;
    backgroundPicId_ = -1;

    InitializeSPKs();
    InitializeButtons();
    PositionButtons();

    // Start animation timer (50ms)
    // gC_timer2.Add(ANI_MILLISEC, _Timer_CharUpdate);
}

void CharacterSelectScreen::Close() noexcept
{
    isOpen_ = false;
    imageSpk_.reset();
    imageHays0_.reset();
    imageHays1_.reset();
    imageHays2_.reset();
    useGradeSpk_.reset();
    netmarbleAgreementSpk_.reset();
    buttonGroup_.reset();
}

void CharacterSelectScreen::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open();
}

void CharacterSelectScreen::LoadFromConfig(const settings::CharacterSelectProfile& profile) noexcept
{
    config_ = profile;
    windowX_ = config_.windowX;
    windowY_ = config_.windowY;
    windowWidth_ = config_.windowWidth;
    windowHeight_ = config_.windowHeight;
    UpdateResolution();
}

void CharacterSelectScreen::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

void CharacterSelectScreen::SetCharacters(const std::array<CharacterSlotInfo, 3>& slots) noexcept
{
    characterSlots_ = slots;
    // Update selected slot to first valid character
    for (int i = 0; i < 3; ++i)
    {
        if (characterSlots_[i].isSet)
        {
            selectedSlot_ = static_cast<CharacterSlot>(i);
            focusSlot_ = selectedSlot_;
            break;
        }
    }
    AddCharInfoString(selectedSlot_);
}

const CharacterSlotInfo* CharacterSelectScreen::GetSelectedCharacter() const noexcept
{
    int idx = static_cast<int>(selectedSlot_);
    if (idx >= 0 && idx < 3 && characterSlots_[idx].isSet)
        return &characterSlots_[idx];
    return nullptr;
}

void CharacterSelectScreen::InitializeSPKs() noexcept
{
    imageSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.imageSpkName.empty())
        imageSpk_->Open(config_.imageSpkName.c_str());

    imageHays0_ = std::make_unique<SpriteArchive>();
    if (!config_.imageHays0SpkName.empty())
        imageHays0_->Open(config_.imageHays0SpkName.c_str());

    imageHays1_ = std::make_unique<SpriteArchive>();
    if (!config_.imageHays1SpkName.empty())
        imageHays1_->Open(config_.imageHays1SpkName.c_str());

    imageHays2_ = std::make_unique<SpriteArchive>();
    if (!config_.imageHays2SpkName.empty())
        imageHays2_->Open(config_.imageHays2SpkName.c_str());

    useGradeSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.useGradeSpkName.empty())
        useGradeSpk_->Open(config_.useGradeSpkName.c_str());

    netmarbleAgreementSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.netmarbleAgreementSpkName.empty())
        netmarbleAgreementSpk_->Open(config_.netmarbleAgreementSpkName.c_str());
}

void CharacterSelectScreen::InitializeButtons() noexcept
{
    buttonGroup_ = std::make_unique<ButtonGroup>();

    // Back button (Previous)
    buttonGroup_->AddButton(static_cast<int>(CharManagerButtonId::Back),
        config_.prevButtonX, config_.prevButtonY,
        imageHays2_ ? imageHays2_->GetWidth(config_.buttonYellowNormalFrame) : 16,
        imageHays2_ ? imageHays2_->GetHeight(config_.buttonYellowNormalFrame) : 16,
        [this]() { OnBackClicked(); });

    // Next button (Game Start)
    buttonGroup_->AddButton(static_cast<int>(CharManagerButtonId::Next),
        config_.gameStartButtonX, config_.gameStartButtonY,
        imageHays2_ ? imageHays2_->GetWidth(config_.buttonGreenNormalFrame) : 16,
        imageHays2_ ? imageHays2_->GetHeight(config_.buttonGreenNormalFrame) : 16,
        [this]() { OnNextClicked(); });

    // Create character
    buttonGroup_->AddButton(static_cast<int>(CharManagerButtonId::Create),
        config_.charCreateButtonX, config_.charCreateButtonY,
        imageHays2_ ? imageHays2_->GetWidth(config_.buttonBlueNormalFrame) : 16,
        imageHays2_ ? imageHays2_->GetHeight(config_.buttonBlueNormalFrame) : 16,
        [this]() { OnCreateClicked(); });

    // Delete character
    buttonGroup_->AddButton(static_cast<int>(CharManagerButtonId::Delete),
        config_.charDeleteButtonX, config_.charDeleteButtonY,
        imageHays2_ ? imageHays2_->GetWidth(config_.buttonRedNormalFrame) : 16,
        imageHays2_ ? imageHays2_->GetHeight(config_.buttonRedNormalFrame) : 16,
        [this]() { OnDeleteClicked(); });

    // Character select buttons (3 slots)
    buttonGroup_->AddButton(static_cast<int>(CharManagerButtonId::CharSelectLeft),
        config_.selectBtnLeftX, config_.selectBtnLeftY,
        imageHays1_ ? imageHays1_->GetWidth(config_.buttonSelRoleNormalFrame) : 16,
        imageHays1_ ? imageHays1_->GetHeight(config_.buttonSelRoleNormalFrame) : 16,
        [this]() { OnCharSelectLeftClicked(); });

    buttonGroup_->AddButton(static_cast<int>(CharManagerButtonId::CharSelectCenter),
        config_.selectBtnCenterX, config_.selectBtnCenterY,
        imageHays1_ ? imageHays1_->GetWidth(config_.buttonSelRoleNormalFrame) : 16,
        imageHays1_ ? imageHays1_->GetHeight(config_.buttonSelRoleNormalFrame) : 16,
        [this]() { OnCharSelectCenterClicked(); });

    buttonGroup_->AddButton(static_cast<int>(CharManagerButtonId::CharSelectRight),
        config_.selectBtnRightX, config_.selectBtnRightY,
        imageHays1_ ? imageHays1_->GetWidth(config_.buttonSelRoleNormalFrame) : 16,
        imageHays1_ ? imageHays1_->GetHeight(config_.buttonSelRoleNormalFrame) : 16,
        [this]() { OnCharSelectRightClicked(); });
}

void CharacterSelectScreen::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(static_cast<int>(CharManagerButtonId::Back),
        config_.prevButtonX, config_.prevButtonY);
    buttonGroup_->SetPosition(static_cast<int>(CharManagerButtonId::Next),
        config_.gameStartButtonX, config_.gameStartButtonY);
    buttonGroup_->SetPosition(static_cast<int>(CharManagerButtonId::Create),
        config_.charCreateButtonX, config_.charCreateButtonY);
    buttonGroup_->SetPosition(static_cast<int>(CharManagerButtonId::Delete),
        config_.charDeleteButtonX, config_.charDeleteButtonY);
    buttonGroup_->SetPosition(static_cast<int>(CharManagerButtonId::CharSelectLeft),
        config_.selectBtnLeftX, config_.selectBtnLeftY);
    buttonGroup_->SetPosition(static_cast<int>(CharManagerButtonId::CharSelectCenter),
        config_.selectBtnCenterX, config_.selectBtnCenterY);
    buttonGroup_->SetPosition(static_cast<int>(CharManagerButtonId::CharSelectRight),
        config_.selectBtnRightX, config_.selectBtnRightY);
}

void CharacterSelectScreen::UpdateResolution() noexcept
{
    windowWidth_ = 1280;
    windowHeight_ = 960;
    windowX_ = 0;
    windowY_ = 0;
}

bool CharacterSelectScreen::IsPointInWindow(int x, int y) const noexcept
{
    return x >= windowX_ && x < windowX_ + windowWidth_ &&
           y >= windowY_ && y < windowY_ + windowHeight_;
}

bool CharacterSelectScreen::IsPointInCharSelectButton(int x, int y, CharacterSlot slot) const noexcept
{
    int btnIndex = static_cast<int>(slot);
    int btnX = 0, btnY = 0, btnW = 0, btnH = 0;

    switch (slot)
    {
        case CharacterSlot::Left:
            btnX = config_.selectBtnLeftX;
            btnY = config_.selectBtnLeftY;
            btnW = imageHays1_ ? imageHays1_->GetWidth(config_.buttonSelRoleNormalFrame) : 64;
            btnH = imageHays1_ ? imageHays1_->GetHeight(config_.buttonSelRoleNormalFrame) : 64;
            break;
        case CharacterSlot::Center:
            btnX = config_.selectBtnCenterX;
            btnY = config_.selectBtnCenterY;
            btnW = imageHays1_ ? imageHays1_->GetWidth(config_.buttonSelRoleNormalFrame) : 64;
            btnH = imageHays1_ ? imageHays1_->GetHeight(config_.buttonSelRoleNormalFrame) : 64;
            break;
        case CharacterSlot::Right:
            btnX = config_.selectBtnRightX;
            btnY = config_.selectBtnRightY;
            btnW = imageHays1_ ? imageHays1_->GetWidth(config_.buttonSelRoleNormalFrame) : 64;
            btnH = imageHays1_ ? imageHays1_->GetHeight(config_.buttonSelRoleNormalFrame) : 64;
            break;
    }

    return x >= btnX && x < btnX + btnW &&
           y >= btnY && y < btnY + btnH;
}

void CharacterSelectScreen::UpdateAnimation(float deltaTime) noexcept
{
    // Legacy: 50ms timer, alpha 1-31 ping-pong
    static float timer = 0.0f;
    timer += deltaTime;
    if (timer >= 0.05f) // 50ms
    {
        timer = 0.0f;
        if (animationUp_)
        {
            animationAlpha_++;
            if (animationAlpha_ >= 31)
            {
                animationAlpha_ = 31;
                animationUp_ = false;
            }
        }
        else
        {
            animationAlpha_--;
            if (animationAlpha_ <= 1)
            {
                animationAlpha_ = 1;
                animationUp_ = true;
            }
        }
    }
}

void CharacterSelectScreen::UpdateCharacterAnimation() noexcept
{
    // Legacy: character animation frame cycling
    static float timer = 0.0f;
    timer += 1.0f / 60.0f; // approximate
    if (timer >= 0.1f) // ~10 FPS for character animation
    {
        timer = 0.0f;
        charIndex_ = (charIndex_ + 1) % 8; // 8 animation frames
    }
}

void CharacterSelectScreen::AddCharInfoString(CharacterSlot slot) noexcept
{
    int idx = static_cast<int>(slot);
    if (idx < 0 || idx >= 3)
        return;

    CharacterInfoStrings info;
    info.slotIndex = idx;

    const auto& slotInfo = characterSlots_[idx];
    if (!slotInfo.isSet)
        return;

    // Build name string
    info.strName = "Name: " + slotInfo.name;

    // Build level string
    info.strLevel = "Level: " + std::to_string(slotInfo.level);

    // Build strength string
    info.strStrength = "STR: " + std::to_string(slotInfo.strPure);

    // Build dexterity string
    info.strDex = "DEX: " + std::to_string(slotInfo.dexPure);

    // Build intelligence string
    info.strInt = "INT: " + std::to_string(slotInfo.intPure);

    // Build sword level
    info.strSwordLevel = "Sword: " + std::to_string(slotInfo.domainSword);

    // Build blade level
    info.strBladeLevel = "Blade: " + std::to_string(slotInfo.domainBlade);

    // Build gun level
    info.strGunLevel = "Gun: " + std::to_string(slotInfo.domainGun);

    // Build heal level
    info.strHealLevel = "Heal: " + std::to_string(slotInfo.domainHeal);

    // Build enchant level
    info.strEnchantLevel = "Enchant: " + std::to_string(slotInfo.domainEnchant);

    // Build fame
    info.strFame = "Fame: " + std::to_string(slotInfo.fame);

    // Build grade string
    if (slotInfo.grade > 0 && slotInfo.grade <= 100)
    {
        const char** gradeNames = (slotInfo.race == Race::Slayer) ? SLAYER_GRADE_NAMES : VAMPIRE_GRADE_NAMES;
        int gradeIndex = (slotInfo.grade - 1) / 5;
        if (gradeIndex >= 0 && gradeIndex < 20)
        {
            info.strGrade = std::string("Grade: ") + gradeNames[gradeIndex] + " [" + std::to_string(slotInfo.grade) + "]";
        }
        else
        {
            info.strGrade = "Grade: " + std::to_string(slotInfo.grade);
        }
    }
    else
    {
        info.strGrade = "Grade: -";
    }

    // Update or add
    auto it = std::find_if(charInfoStrings_.begin(), charInfoStrings_.end(),
        [idx](const CharacterInfoStrings& s) { return s.slotIndex == idx; });

    if (it != charInfoStrings_.end())
    {
        *it = info;
    }
    else
    {
        charInfoStrings_.push_back(info);
    }
}

void CharacterSelectScreen::DeleteCharInfoString() noexcept
{
    charInfoStrings_.clear();
}

void CharacterSelectScreen::DrawCharInfoString() noexcept
{
    // Legacy DrawCharInfoString - render selected character info
    if (selectedSlot_ == CharacterSlot::Left ||
        selectedSlot_ == CharacterSlot::Center ||
        selectedSlot_ == CharacterSlot::Right)
    {
        int idx = static_cast<int>(selectedSlot_);
        if (idx < static_cast<int>(charInfoStrings_.size()))
        {
            const auto& info = charInfoStrings_[idx];
            // Rendering handled in Render()
        }
    }
}

bool CharacterSelectScreen::SelectSlot(int n) noexcept
{
    if (n >= 0 && n < 3)
    {
        if (characterSlots_[n].isSet)
        {
            selectedSlot_ = static_cast<CharacterSlot>(n);
            focusSlot_ = selectedSlot_;
            AddCharInfoString(selectedSlot_);
            return true;
        }
    }
    return false;
}

bool CharacterSelectScreen::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    // Check character select buttons
    for (int i = 0; i < 3; ++i)
    {
        CharacterSlot slot = static_cast<CharacterSlot>(i);
        if (IsPointInCharSelectButton(x, y, slot))
        {
            if (characterSlots_[i].isSet)
            {
                SelectSlot(i);
            }
            else
            {
                // Empty slot - create character
                OnCreateClicked();
            }
            return true;
        }
    }

    // Check main buttons
    if (buttonGroup_)
    {
        if (buttonGroup_->HandleMouseDown(x, y))
            return true;
    }

    return false;
}

bool CharacterSelectScreen::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    return false;
}

bool CharacterSelectScreen::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    return false;
}

bool CharacterSelectScreen::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    if (key == VK_ESCAPE)
    {
        OnBackClicked();
        return true;
    }
    if (key == VK_RETURN)
    {
        OnNextClicked();
        return true;
    }

    switch (key)
    {
        case VK_LEFT:
            if (!SelectSlot(static_cast<int>(selectedSlot_) - 1))
            {
                if (!SelectSlot(static_cast<int>(selectedSlot_) - 2))
                    SelectSlot(0);
            }
            return true;
        case VK_RIGHT:
            if (!SelectSlot(static_cast<int>(selectedSlot_) + 1))
            {
                if (!SelectSlot(static_cast<int>(selectedSlot_) + 2))
                    SelectSlot(2);
            }
            return true;
        case VK_UP:
            // Not used in character select
            return true;
        case VK_DOWN:
            // Not used in character select
            return true;
    }

    return false;
}

void CharacterSelectScreen::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    UpdateAnimation(deltaTime);
    UpdateCharacterAnimation();

    if (buttonGroup_)
    {
        buttonGroup_->Update(deltaTime);
    }
}

void CharacterSelectScreen::Render() noexcept
{
    if (!isOpen_)
        return;

    // Render background
    if (imageHays0_)
    {
        // Determine background based on selected slot
        int bgFrame = config_.none800Frame; // default
        if (windowWidth_ > 800)
        {
            if (windowWidth_ <= 1024)
                bgFrame = config_.none1024Frame;
            else if (windowHeight_ < 960)
                bgFrame = config_.none1280_960Frame;
            else
                bgFrame = config_.none1280_1024Frame;
        }

        if (selectedSlot_ != CharacterSlot::Left &&
            selectedSlot_ != CharacterSlot::Center &&
            selectedSlot_ != CharacterSlot::Right)
        {
            // No character selected - show empty slot background
            imageHays0_->Draw(
                (windowWidth_ - imageHays0_->GetWidth(bgFrame)) / 2,
                (windowHeight_ - imageHays0_->GetHeight(bgFrame)) / 2,
                bgFrame);
        }
        else
        {
            // Character selected - show race-specific background
            const auto& slotInfo = characterSlots_[static_cast<int>(selectedSlot_)];
            if (slotInfo.isSet)
            {
                int raceFrame = 0;
                if (windowWidth_ > 800)
                {
                    if (windowWidth_ <= 1024)
                    {
                        raceFrame = (slotInfo.race == Race::Slayer) ? config_.slayer1024Frame :
                                    (slotInfo.race == Race::Vampire) ? config_.slayerCharInfo1024Frame : config_.ousters1024Frame;
                    }
                    else if (windowHeight_ < 960)
                    {
                        raceFrame = (slotInfo.race == Race::Slayer) ? config_.slayer1280_960Frame :
                                    (slotInfo.race == Race::Vampire) ? config_.slayerCharInfo1280_960Frame : config_.ousters1280_960Frame;
                    }
                    else
                    {
                        raceFrame = (slotInfo.race == Race::Slayer) ? config_.slayer1280_1024Frame :
                                    (slotInfo.race == Race::Vampire) ? config_.slayerCharInfo1280_1024Frame : config_.ousters1280_1024Frame;
                    }
                }
                else
                {
                    raceFrame = (slotInfo.race == Race::Slayer) ? config_.none800Frame :
                                (slotInfo.race == Race::Vampire) ? config_.none800Frame : config_.none800Frame;
                }

                imageHays0_->Draw(
                    (windowWidth_ - imageHays0_->GetWidth(raceFrame)) / 2,
                    (windowHeight_ - imageHays0_->GetHeight(raceFrame)) / 2,
                    raceFrame);
            }
        }
    }

    // Render panel and list info
    if (imageHays1_)
    {
        imageHays1_->Draw(config_.panelX, config_.panelY, config_.panalNoneFrame);
        imageHays1_->DrawAlpha(config_.panelX, config_.panelY, config_.panalFillFrame, 20);
        imageHays1_->Draw(config_.listInfoX, config_.listInfoY, config_.listInfomationNoneFrame);
        imageHays1_->DrawAlpha(config_.listInfoX, config_.listInfoY, config_.listInfomationNormalFrame, 20);

        // Locked slots
        imageHays1_->Draw(config_.lockedSlot0X, config_.lockedSlot0Y, config_.buttonSelRoleEnableFrame);
        imageHays1_->Draw(config_.lockedSlot1X, config_.lockedSlot1Y, config_.buttonSelRoleEnableFrame);
    }

    // Render character info text
    if (selectedSlot_ != CharacterSlot::Left &&
        selectedSlot_ != CharacterSlot::Center &&
        selectedSlot_ != CharacterSlot::Right)
    {
        // No character selected
    }
    else
    {
        int idx = static_cast<int>(selectedSlot_);
        if (idx < static_cast<int>(charInfoStrings_.size()))
        {
            const auto& info = charInfoStrings_[idx];
            const auto& slotInfo = characterSlots_[idx];
            int px = config_.charInfoX;
            int py = config_.charInfoY;

            if (slotInfo.race == Race::Slayer)
            {
                px = config_.slayerCharInfoX;
                py = config_.slayerCharInfoY;
            }

            // Render character info strings (placeholder - actual rendering uses DirectWrite)
            // g_PrintColorStr(px+21, py+12, info.strName.c_str(), ...);
            // g_PrintColorStr(px+21, py+65, info.strStrength.c_str(), ...);
            // g_PrintColorStr(px+21, py+94, info.strDex.c_str(), ...);
            // g_PrintColorStr(px+21, py+123, info.strInt.c_str(), ...);
            // g_PrintColorStr(px+21, py+187, info.strFame.c_str(), ...);
            // g_PrintColorStr(px+21, py+217, info.strGrade.c_str(), ...);
        }
    }

    // Render character slots with names and levels
    for (int i = 0; i < 3; ++i)
    {
        if (characterSlots_[i].isSet)
        {
            const auto& slot = characterSlots_[i];
            int nameX = config_.panelX + 203;
            int nameY = config_.panelY + 119 + i * 80;
            // g_PrintColorStrShadow(nameX, nameY, slot.name.c_str(), ...);

            int levelX = config_.panelX + 94;
            int levelY = config_.panelY + 91 + i * 80;
            std::string levelStr = (slot.advancementLevel < 0)
                ? "Domain Level: " + std::to_string(slot.level)
                : "Adv Level: " + std::to_string(slot.advancementLevel);
            // g_PrintColorStrShadow(levelX, levelY, levelStr.c_str(), ...);
        }
    }

    // Render character models with effects (placeholder)
    // g_pTopView->DrawTitleEffect(...)

    // Render character select buttons with race icons
    for (int i = 0; i < 3; ++i)
    {
        CharacterSlot slot = static_cast<CharacterSlot>(i);
        int btnX = 0, btnY = 0;

        switch (slot)
        {
            case CharacterSlot::Left:
                btnX = config_.selectBtnLeftX;
                btnY = config_.selectBtnLeftY;
                break;
            case CharacterSlot::Center:
                btnX = config_.selectBtnCenterX;
                btnY = config_.selectBtnCenterY;
                break;
            case CharacterSlot::Right:
                btnX = config_.selectBtnRightX;
                btnY = config_.selectBtnRightY;
                break;
        }

        if (imageHays1_)
        {
            // Base button
            imageHays1_->Draw(btnX, btnY, config_.buttonSelRoleNormalFrame);

            if (characterSlots_[i].isSet)
            {
                // Race icon overlay
                int raceIconFrame = 0;
                switch (characterSlots_[i].race)
                {
                    case Race::Slayer: raceIconFrame = config_.raceIconSlayerActiveFrame; break;
                    case Race::Vampire: raceIconFrame = config_.raceIconVampireActiveFrame; break;
                    case Race::Ousters: raceIconFrame = config_.raceIconOustersActiveFrame; break;
                }
                imageHays1_->Draw(btnX, btnY, raceIconFrame + 3);

                // Selected highlight
                if (selectedSlot_ == slot)
                {
                    imageHays1_->Draw(btnX, btnY, config_.buttonSelRoleNormalFrame + 1);
                    if (characterSlots_[i].isSet)
                        imageHays1_->Draw(btnX, btnY, raceIconFrame);
                }
            }
            else
            {
                // Empty slot - show "Create" or disabled state
                // Not set
            }
        }
    }

    // Render rating icons
    if (imageHays1_)
    {
        imageHays1_->Draw(config_.childX, config_.childY, config_.childFrame);
        imageHays1_->Draw(config_.adultX, config_.adultY, config_.adultFrame);
        imageHays1_->Draw(config_.violenceX, config_.violenceY, config_.violenceFrame);
        imageHays1_->Draw(config_.fearX, config_.fearY, config_.fearFrame);
    }

    // Render animation mark
    if (imageHays1_)
    {
        imageHays1_->DrawAlpha(config_.aniMarkX, config_.aniMarkY, config_.aniMarkFrame, animationAlpha_);
    }

    // Render text lines
    // g_PrintColorStr(config_.textLine1X, config_.textLine1Y, config_.textLine1String.c_str(), ...);
    // g_PrintColorStr(config_.textLine2X, config_.textLine2Y, config_.textLine2String.c_str(), ...);
    // g_PrintColorStr(config_.textLine3X, config_.textLine3Y, config_.textLine3String.c_str(), ...);

    // Render premium text
    // char szBuffer[256];
    // Build premium string...
    // g_PrintColorStr(config_.premiumUseDayTextX - szBufferLen, config_.premiumUseDayTextY, szBuffer, ...);

    // Render warning string
    // g_PrintColorStr(config_.warningStringX, config_.warningStringY, "Child Guard...", ...);

    // Render buttons
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }

    // Render button names/labels
    // Handled by button group callbacks
}

void CharacterSelectScreen::OnBackClicked() noexcept
{
    if (backCallback_)
        backCallback_();
    Close();
}

void CharacterSelectScreen::OnNextClicked() noexcept
{
    if (selectedSlot_ != CharacterSlot::Left &&
        selectedSlot_ != CharacterSlot::Center &&
        selectedSlot_ != CharacterSlot::Right)
        return;

    int idx = static_cast<int>(selectedSlot_);
    if (characterSlots_[idx].isSet && startCallback_)
    {
        startCallback_(selectedSlot_);
    }
    Close();
}

void CharacterSelectScreen::OnDeleteClicked() noexcept
{
    if (selectedSlot_ != CharacterSlot::Left &&
        selectedSlot_ != CharacterSlot::Center &&
        selectedSlot_ != CharacterSlot::Right)
        return;

    int idx = static_cast<int>(selectedSlot_);
    if (characterSlots_[idx].isSet && deleteCallback_)
    {
        deleteCallback_(selectedSlot_);
    }
    Close();
}

void CharacterSelectScreen::OnCreateClicked() noexcept
{
    if (selectedSlot_ != CharacterSlot::Left &&
        selectedSlot_ != CharacterSlot::Center &&
        selectedSlot_ != CharacterSlot::Right)
        return;

    int idx = static_cast<int>(selectedSlot_);
    if (!characterSlots_[idx].isSet && createCallback_)
    {
        createCallback_(selectedSlot_);
    }
    Close();
}

void CharacterSelectScreen::OnCharSelectLeftClicked() noexcept
{
    if (characterSlots_[0].isSet)
    {
        SelectSlot(0);
    }
    else
    {
        OnCreateClicked();
    }
}

void CharacterSelectScreen::OnCharSelectCenterClicked() noexcept
{
    if (characterSlots_[1].isSet)
    {
        SelectSlot(1);
    }
    else
    {
        OnCreateClicked();
    }
}

void CharacterSelectScreen::OnCharSelectRightClicked() noexcept
{
    if (characterSlots_[2].isSet)
    {
        SelectSlot(2);
    }
    else
    {
        OnCreateClicked();
    }
}

} // namespace phoenix::ui