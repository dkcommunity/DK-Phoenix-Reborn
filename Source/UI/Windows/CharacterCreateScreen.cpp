#include "UI/Windows/CharacterCreateScreen.h"

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

CharacterCreateScreen::CharacterCreateScreen() noexcept
    : isOpen_(false)
    , currentSlotIndex_(0)
    , animationAlpha_(31)
    , animationUp_(false)
    , rotationAngle_(0.0f)
    , focusedHelp_(0)
    , slotInfo_()
{
}

void CharacterCreateScreen::Open() noexcept
{
    isOpen_ = true;
    currentSlotIndex_ = 0;
    animationAlpha_ = config_.aniMarkX; // Using aniMarkX as start alpha
    animationUp_ = false;
    rotationAngle_ = 0.0f;
    focusedHelp_ = 0;
    
    slotInfo_ = CharacterCreateSlotInfo{};
    slotInfo_.race = Race::Slayer;
    slotInfo_.gender = Gender::Male;
    slotInfo_.faceSelection = FaceSelection::Left;
    slotInfo_.str = 10;
    slotInfo_.dex = 10;
    slotInfo_.int_ = 10;
    slotInfo_.bonusPoints = 0;
    slotInfo_.slayerClass = SlayerClassIcon::Blade;

    InitializeSPKs();
    InitializeButtons();
    PositionButtons();

    // Initialize name editor
    nameEditor_ = std::make_unique<LineEditor>();
    nameEditor_->SetByteLimit(config_.charNameByteLimit);
    nameEditor_->SetPosition(config_.charNameFocusX, config_.charNameFocusY);
}

void CharacterCreateScreen::Close() noexcept
{
    isOpen_ = false;
    imageSpk_.reset();
    imageHaysSpk_.reset();
    commonSpk_.reset();
    useGradeSpk_.reset();
    faceMakeSpk_.reset();
    slayerManIspk_.reset();
    slayerWomanIspk_.reset();
    vampireIspk_.reset();
    oustersIspk_.reset();
    advancementSlayerManIspk_.reset();
    advancementSlayerWomanIspk_.reset();
    advancementVampireManIspk_.reset();
    advancementVampireWomanIspk_.reset();
    advancementOustersIspk_.reset();
    buttonGroup_.reset();
    nameEditor_.reset();
}

void CharacterCreateScreen::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open();
}

void CharacterCreateScreen::LoadFromConfig(const settings::CharacterCreateProfile& profile) noexcept
{
    config_ = profile;
}

void CharacterCreateScreen::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

void CharacterCreateScreen::SetSlot(int slotIndex) noexcept
{
    if (slotIndex >= 0 && slotIndex < 3)
    {
        currentSlotIndex_ = slotIndex;
    }
}

void CharacterCreateScreen::InitializeSPKs() noexcept
{
    imageSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.imageSpkName.empty())
        imageSpk_->Open(config_.imageSpkName.c_str());

    imageHaysSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.imageHaysSpkName.empty())
        imageHaysSpk_->Open(config_.imageHaysSpkName.c_str());

    commonSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.commonSpkName.empty())
        commonSpk_->Open(config_.commonSpkName.c_str());

    useGradeSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.useGradeSpkName.empty())
        useGradeSpk_->Open(config_.useGradeSpkName.c_str());

    faceMakeSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.faceMakeSpkName.empty())
        faceMakeSpk_->Open(config_.faceMakeSpkName.c_str());

    slayerManIspk_ = std::make_unique<SpriteArchive>();
    if (!config_.slayerManIspkName.empty())
        slayerManIspk_->Open(config_.slayerManIspkName.c_str());

    slayerWomanIspk_ = std::make_unique<SpriteArchive>();
    if (!config_.slayerWomanIspkName.empty())
        slayerWomanIspk_->Open(config_.slayerWomanIspkName.c_str());

    vampireIspk_ = std::make_unique<SpriteArchive>();
    if (!config_.vampireIspkName.empty())
        vampireIspk_->Open(config_.vampireIspkName.c_str());

    oustersIspk_ = std::make_unique<SpriteArchive>();
    if (!config_.oustersIspkName.empty())
        oustersIspk_->Open(config_.oustersIspkName.c_str());

    advancementSlayerManIspk_ = std::make_unique<SpriteArchive>();
    if (!config_.advancementSlayerManIspkName.empty())
        advancementSlayerManIspk_->Open(config_.advancementSlayerManIspkName.c_str());

    advancementSlayerWomanIspk_ = std::make_unique<SpriteArchive>();
    if (!config_.advancementSlayerWomanIspkName.empty())
        advancementSlayerWomanIspk_->Open(config_.advancementSlayerWomanIspkName.c_str());

    advancementVampireManIspk_ = std::make_unique<SpriteArchive>();
    if (!config_.advancementVampireManIspkName.empty())
        advancementVampireManIspk_->Open(config_.advancementVampireManIspkName.c_str());

    advancementVampireWomanIspk_ = std::make_unique<SpriteArchive>();
    if (!config_.advancementVampireWomanIspkName.empty())
        advancementVampireWomanIspk_->Open(config_.advancementVampireWomanIspkName.c_str());

    advancementOustersIspk_ = std::make_unique<SpriteArchive>();
    if (!config_.advancementOustersIspkName.empty())
        advancementOustersIspk_->Open(config_.advancementOustersIspkName.c_str());
}

void CharacterCreateScreen::InitializeButtons() noexcept
{
    buttonGroup_ = std::make_unique<ButtonGroup>();

    // Prev button (Back)
    buttonGroup_->AddButton(0, // BACK_ID
        config_.prevButtonX, config_.prevButtonY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnPrevClicked(); });

    // Next button (Game Start)
    buttonGroup_->AddButton(1, // NEXT_ID
        config_.nextButtonX, config_.nextButtonY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(1) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(1) : 16,
        [this]() { OnNextClicked(); });

    // Char Left Rotation
    buttonGroup_->AddButton(2, // CHAR_LEFT_ROTATION_ID
        config_.charLeftRotationButtonX, config_.charLeftRotationButtonY,
        imageSpk_ ? imageSpk_->GetWidth(0) : 16,
        imageSpk_ ? imageSpk_->GetHeight(0) : 16,
        [this]() { OnCharLeftRotationClicked(); });

    // Char Right Rotation
    buttonGroup_->AddButton(3, // CHAR_RIGHT_ROTATION_ID
        config_.charRightRotationButtonX, config_.charRightRotationButtonY,
        imageSpk_ ? imageSpk_->GetWidth(0) : 16,
        imageSpk_ ? imageSpk_->GetHeight(0) : 16,
        [this]() { OnCharRightRotationClicked(); });

    // Race: Slayer
    buttonGroup_->AddButton(4, // SLAYER_ID
        config_.selectRaceSlayerButtonX, config_.selectRaceSlayerButtonY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnSelectRaceSlayerClicked(); });

    // Race: Vampire
    buttonGroup_->AddButton(5, // VAMPIRE_ID
        config_.selectRaceVampireButtonX, config_.selectRaceVampireButtonY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnSelectRaceVampireClicked(); });

    // Race: Ousters
    buttonGroup_->AddButton(6, // OUSTERS_ID
        config_.selectRaceOustersButtonX, config_.selectRaceOustersButtonY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnSelectRaceOustersClicked(); });

    // Name Check
    buttonGroup_->AddButton(7, // CHECK_ID
        config_.nameCheckButtonX, config_.nameCheckButtonY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnNameCheckClicked(); });

    // Face Selection: Left
    buttonGroup_->AddButton(8,
        config_.selectLeftFaceRadioX, config_.selectLeftFaceRadioY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnSelectLeftFaceClicked(); });

    // Face Selection: Center
    buttonGroup_->AddButton(9,
        config_.selectCenterFaceRadioX, config_.selectCenterFaceRadioY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnSelectCenterFaceClicked(); });

    // Face Selection: Right
    buttonGroup_->AddButton(10,
        config_.selectRightFaceRadioX, config_.selectRightFaceRadioY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnSelectRightFaceClicked(); });

    // Male
    buttonGroup_->AddButton(11,
        config_.maleSelectButtonX, config_.maleSelectButtonY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnMaleSelected(); });

    // Female
    buttonGroup_->AddButton(12,
        config_.femaleSelectButtonX, config_.femaleSelectButtonY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnFemaleSelected(); });

    // Save
    buttonGroup_->AddButton(13,
        config_.saveButtonX, config_.saveButtonY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnSaveClicked(); });

    // Load
    buttonGroup_->AddButton(14,
        config_.loadButtonX, config_.loadButtonY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnLoadClicked(); });

    // Reset
    buttonGroup_->AddButton(15,
        config_.resetButtonX, config_.resetButtonY,
        imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
        imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
        [this]() { OnResetClicked(); });

    // Stet Plus buttons (Str, Dex, Int)
    for (int i = 0; i < 3; ++i)
    {
        buttonGroup_->AddButton(16 + i,
            config_.stetPlusButtonX[i], config_.stetPlusButtonY[i],
            imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
            imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
            [this, i]() { OnStetPlusClicked(i); });
    }

    // Stet Minus buttons (Str, Dex, Int)
    for (int i = 0; i < 3; ++i)
    {
        buttonGroup_->AddButton(19 + i,
            config_.stetMinusButtonX[i], config_.stetMinusButtonY[i],
            imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
            imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
            [this, i]() { OnStetMinusClicked(i); });
    }

    // Slayer class icons (5)
    for (int i = 0; i < 5; ++i)
    {
        buttonGroup_->AddButton(22 + i,
            config_.slayerClassIconX[i], config_.slayerClassIconY[i],
            imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16,
            imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16,
            [this, i]() { OnSlayerClassIconClicked(i); });
    }
}

void CharacterCreateScreen::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(0, config_.prevButtonX, config_.prevButtonY);
    buttonGroup_->SetPosition(1, config_.nextButtonX, config_.nextButtonY);
    buttonGroup_->SetPosition(2, config_.charLeftRotationButtonX, config_.charLeftRotationButtonY);
    buttonGroup_->SetPosition(3, config_.charRightRotationButtonX, config_.charRightRotationButtonY);
    buttonGroup_->SetPosition(4, config_.selectRaceSlayerButtonX, config_.selectRaceSlayerButtonY);
    buttonGroup_->SetPosition(5, config_.selectRaceVampireButtonX, config_.selectRaceVampireButtonY);
    buttonGroup_->SetPosition(6, config_.selectRaceOustersButtonX, config_.selectRaceOustersButtonY);
    buttonGroup_->SetPosition(7, config_.nameCheckButtonX, config_.nameCheckButtonY);
    buttonGroup_->SetPosition(8, config_.selectLeftFaceRadioX, config_.selectLeftFaceRadioY);
    buttonGroup_->SetPosition(9, config_.selectCenterFaceRadioX, config_.selectCenterFaceRadioY);
    buttonGroup_->SetPosition(10, config_.selectRightFaceRadioX, config_.selectRightFaceRadioY);
    buttonGroup_->SetPosition(11, config_.maleSelectButtonX, config_.maleSelectButtonY);
    buttonGroup_->SetPosition(12, config_.femaleSelectButtonX, config_.femaleSelectButtonY);
    buttonGroup_->SetPosition(13, config_.saveButtonX, config_.saveButtonY);
    buttonGroup_->SetPosition(14, config_.loadButtonX, config_.loadButtonY);
    buttonGroup_->SetPosition(15, config_.resetButtonX, config_.resetButtonY);

    for (int i = 0; i < 3; ++i)
    {
        buttonGroup_->SetPosition(16 + i, config_.stetPlusButtonX[i], config_.stetPlusButtonY[i]);
        buttonGroup_->SetPosition(19 + i, config_.stetMinusButtonX[i], config_.stetMinusButtonY[i]);
    }

    for (int i = 0; i < 5; ++i)
    {
        buttonGroup_->SetPosition(22 + i, config_.slayerClassIconX[i], config_.slayerClassIconY[i]);
    }
}

bool CharacterCreateScreen::IsPointInWindow(int x, int y) const noexcept
{
    return x >= config_.windowX && x < config_.windowX + config_.windowWidth &&
           y >= config_.windowY && y < config_.windowY + config_.windowHeight;
}

bool CharacterCreateScreen::IsPointInColorBox(int x, int y, bool isSkinColor) const noexcept
{
    if (isSkinColor)
    {
        return x >= config_.skinColorStartX && x < config_.skinColorStartX + 5 * 16 && // COLOR_LIST_X * COLOR_UNIT_X
               y >= config_.skinColorStartY && y < config_.skinColorStartY + 5 * 16;  // COLOR_LIST_Y * COLOR_UNIT_Y
    }
    else
    {
        return x >= config_.hairColorStartX && x < config_.hairColorStartX + 5 * 16 &&
               y >= config_.hairColorStartY && y < config_.hairColorStartY + 5 * 16;
    }
}

bool CharacterCreateScreen::IsPointInSlayerClassIcon(int x, int y, int index) const noexcept
{
    if (index < 0 || index >= 5)
        return false;

    int btnW = imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 32;
    int btnH = imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 32;

    return x >= config_.slayerClassIconX[index] && x < config_.slayerClassIconX[index] + btnW &&
           y >= config_.slayerClassIconY[index] && y < config_.slayerClassIconY[index] + btnH;
}

bool CharacterCreateScreen::IsPointInStetButton(int x, int y, int index) const noexcept
{
    if (index < 0 || index >= 3)
        return false;

    int btnW = imageHaysSpk_ ? imageHaysSpk_->GetWidth(0) : 16;
    int btnH = imageHaysSpk_ ? imageHaysSpk_->GetHeight(0) : 16;

    return x >= config_.stetPlusButtonX[index] && x < config_.stetPlusButtonX[index] + btnW &&
           y >= config_.stetPlusButtonY[index] && y < config_.stetPlusButtonY[index] + btnH;
}

void CharacterCreateScreen::OnPrevClicked() noexcept
{
    if (backCallback_)
        backCallback_();
    Close();
}

void CharacterCreateScreen::OnNextClicked() noexcept
{
    if (nextCallback_)
        nextCallback_(slotInfo_);
    Close();
}

void CharacterCreateScreen::OnCharLeftRotationClicked() noexcept
{
    rotationAngle_ -= 45.0f; // 45 degrees per click
    UpdateCharacterModel();
}

void CharacterCreateScreen::OnCharRightRotationClicked() noexcept
{
    rotationAngle_ += 45.0f;
    UpdateCharacterModel();
}

void CharacterCreateScreen::OnSelectRaceSlayerClicked() noexcept
{
    slotInfo_.race = Race::Slayer;
    slotInfo_.gender = Gender::Male;
    slotInfo_.faceSelection = FaceSelection::Left;
    RandomizeCharacter();
}

void CharacterCreateScreen::OnSelectRaceVampireClicked() noexcept
{
    slotInfo_.race = Race::Vampire;
    slotInfo_.gender = Gender::Male;
    slotInfo_.faceSelection = FaceSelection::Left;
    RandomizeCharacter();
}

void CharacterCreateScreen::OnSelectRaceOustersClicked() noexcept
{
    slotInfo_.race = Race::Ousters;
    slotInfo_.gender = Gender::Male;
    slotInfo_.faceSelection = FaceSelection::Left;
    RandomizeCharacter();
}

void CharacterCreateScreen::OnNameCheckClicked() noexcept
{
    slotInfo_.name = nameEditor_->GetText();
    slotInfo_.nameChecked = !slotInfo_.name.empty();
}

void CharacterCreateScreen::OnSelectLeftFaceClicked() noexcept
{
    slotInfo_.faceSelection = FaceSelection::Left;
}

void CharacterCreateScreen::OnSelectCenterFaceClicked() noexcept
{
    slotInfo_.faceSelection = FaceSelection::Center;
}

void CharacterCreateScreen::OnSelectRightFaceClicked() noexcept
{
    slotInfo_.faceSelection = FaceSelection::Right;
}

void CharacterCreateScreen::OnMaleSelected() noexcept
{
    slotInfo_.gender = Gender::Male;
    UpdateCharacterModel();
}

void CharacterCreateScreen::OnFemaleSelected() noexcept
{
    slotInfo_.gender = Gender::Female;
    UpdateCharacterModel();
}

void CharacterCreateScreen::OnHairColorClicked(int x, int y) noexcept
{
    int relX = x - config_.hairColorStartX;
    int relY = y - config_.hairColorStartY;
    if (relX >= 0 && relY >= 0)
    {
        int col = relX / 16; // COLOR_UNIT_X
        int row = relY / 16; // COLOR_UNIT_Y
        if (col >= 0 && col < 5 && row >= 0 && row < 5)
        {
            slotInfo_.hairColor = row * 5 + col; // COLOR_LIST_X * row + col
        }
    }
}

void CharacterCreateScreen::OnSkinColorClicked(int x, int y) noexcept
{
    int relX = x - config_.skinColorStartX;
    int relY = y - config_.skinColorStartY;
    if (relX >= 0 && relY >= 0)
    {
        int col = relX / 16;
        int row = relY / 16;
        if (col >= 0 && col < 5 && row >= 0 && row < 5)
        {
            slotInfo_.skinColor = row * 5 + col;
        }
    }
}

void CharacterCreateScreen::OnSlayerClassIconClicked(int index) noexcept
{
    if (index >= 0 && index < 5)
    {
        slotInfo_.slayerClass = static_cast<SlayerClassIcon>(index);
    }
}

void CharacterCreateScreen::OnSaveClicked() noexcept
{
    // TODO: Save character template to file
}

void CharacterCreateScreen::OnLoadClicked() noexcept
{
    // TODO: Load character template from file
}

void CharacterCreateScreen::OnResetClicked() noexcept
{
    RandomizeCharacter();
}

void CharacterCreateScreen::OnStetPlusClicked(int index) noexcept
{
    if (slotInfo_.bonusPoints > 0)
    {
        switch (index)
        {
            case 0: slotInfo_.str++; break;
            case 1: slotInfo_.dex++; break;
            case 2: slotInfo_.int_++; break;
        }
        slotInfo_.bonusPoints--;
    }
}

void CharacterCreateScreen::OnStetMinusClicked(int index) noexcept
{
    switch (index)
    {
        case 0: if (slotInfo_.str > 10) { slotInfo_.str--; slotInfo_.bonusPoints++; } break;
        case 1: if (slotInfo_.dex > 10) { slotInfo_.dex--; slotInfo_.bonusPoints++; } break;
        case 2: if (slotInfo_.int_ > 10) { slotInfo_.int_--; slotInfo_.bonusPoints++; } break;
    }
}

void CharacterCreateScreen::UpdateAnimation(float deltaTime) noexcept
{
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

void CharacterCreateScreen::UpdateCharacterRotation(float deltaTime) noexcept
{
    rotationAngle_ += deltaTime * 10.0f; // Slow auto-rotation
    if (rotationAngle_ >= 360.0f)
        rotationAngle_ -= 360.0f;
}

void CharacterCreateScreen::RollDice() noexcept
{
    // Legacy RollDice - randomize stats within bounds
    slotInfo_.str = 10 + (rand() % 5);
    slotInfo_.dex = 10 + (rand() % 5);
    slotInfo_.int_ = 10 + (rand() % 5);
    slotInfo_.bonusPoints = 0;
}

void CharacterCreateScreen::RandomizeCharacter() noexcept
{
    // Reset to defaults
    slotInfo_.str = 10;
    slotInfo_.dex = 10;
    slotInfo_.int_ = 10;
    slotInfo_.bonusPoints = 0;
    slotInfo_.hairColor = rand() % 25;
    if (slotInfo_.race != Race::Ousters)
        slotInfo_.skinColor = rand() % 25;
    else
        slotInfo_.skinColor = 0; // Ousters don't have skin color
    slotInfo_.faceSelection = static_cast<FaceSelection>(rand() % 3);
    RollDice();
    UpdateCharacterModel();
}

void CharacterCreateScreen::UpdateCharacterModel() noexcept
{
    // Legacy: Update ISPK/CFPK character model based on race, gender, face, etc.
    // This is a placeholder for the actual 3D model rendering
}

bool CharacterCreateScreen::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    // Check color boxes (hair/skin)
    if (slotInfo_.race != Race::Vampire && IsPointInColorBox(x, y, false)) // Hair color
    {
        OnHairColorClicked(x, y);
        return true;
    }

    if (slotInfo_.race != Race::Ousters && IsPointInColorBox(x, y, true)) // Skin color
    {
        OnSkinColorClicked(x, y);
        return true;
    }

    // Check slayer class icons
    if (slotInfo_.race == Race::Slayer)
    {
        for (int i = 0; i < 5; ++i)
        {
            if (IsPointInSlayerClassIcon(x, y, i))
            {
                OnSlayerClassIconClicked(i);
                return true;
            }
        }
    }

    // Check name editor
    if (x >= config_.charNameFocusX && x < config_.charNameFocusX + 130 &&
        y >= config_.charNameFocusY && y < config_.charNameFocusY + 23)
    {
        // Focus name editor
        return true;
    }

    // Check buttons
    if (buttonGroup_)
    {
        if (buttonGroup_->HandleMouseDown(x, y))
            return true;
    }

    return false;
}

bool CharacterCreateScreen::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    return false;
}

bool CharacterCreateScreen::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    return false;
}

bool CharacterCreateScreen::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    if (key == VK_ESCAPE)
    {
        OnPrevClicked();
        return true;
    }

    if (key == VK_RETURN)
    {
        OnNextClicked();
        return true;
    }

    return false;
}

bool CharacterCreateScreen::HandleChar(char c) noexcept
{
    if (!isOpen_)
        return false;

    // Legacy character filtering
    static const char ignoreChars[] = {
        '~', '`', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '_',
        '+', '=', '\\', '|', '[', ']', '{', '}', ';', ':', '"', '\'', ',', '<', '.', '>',
        '/', '?', ' '
    };

    for (char ignore : ignoreChars)
    {
        if (c == ignore)
            return false;
    }

    if (nameEditor_)
    {
        nameEditor_->InsertChar(c);
        return true;
    }

    return false;
}

void CharacterCreateScreen::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    UpdateAnimation(deltaTime);
    UpdateCharacterRotation(deltaTime);

    if (buttonGroup_)
    {
        buttonGroup_->Update(deltaTime);
    }

    if (nameEditor_)
    {
        nameEditor_->Update(deltaTime);
    }
}

void CharacterCreateScreen::Render() noexcept
{
    if (!isOpen_)
        return;

    // Render background
    if (imageHaysSpk_)
    {
        int bgFrame = 0; // Default
        if (config_.windowWidth > 800)
        {
            if (config_.windowWidth <= 1024)
                bgFrame = 1;
            else if (config_.windowHeight < 960)
                bgFrame = 2;
            else
                bgFrame = 3;
        }

        imageHaysSpk_->Draw(
            (config_.windowWidth - imageHaysSpk_->GetWidth(bgFrame)) / 2,
            (config_.windowHeight - imageHaysSpk_->GetHeight(bgFrame)) / 2,
            bgFrame);
    }

    // Render character model (placeholder - would use ISPK/CFPK)
    if (slotInfo_.race == Race::Slayer)
    {
        if (slotInfo_.gender == Gender::Male && slayerManIspk_)
        {
            slayerManIspk_->Draw(config_.charViewX, config_.charViewY, 0);
        }
        else if (slotInfo_.gender == Gender::Female && slayerWomanIspk_)
        {
            slayerWomanIspk_->Draw(config_.charViewX, config_.charViewY, 0);
        }
    }
    else if (slotInfo_.race == Race::Vampire && vampireIspk_)
    {
        vampireIspk_->Draw(config_.charViewX, config_.charViewY, 0);
    }
    else if (slotInfo_.race == Race::Ousters && oustersIspk_)
    {
        oustersIspk_->Draw(config_.charViewX, config_.charViewY, 0);
    }

    // Render hair/skin color boxes
    if (imageHaysSpk_)
    {
        // Hair color box
        imageHaysSpk_->Draw(config_.hairColorBoxX, config_.hairColorBoxY, 0);
        // Skin color box
        if (slotInfo_.race != Race::Ousters)
        {
            imageHaysSpk_->Draw(config_.skinColorBoxX, config_.skinColorBoxY, 0);
        }
    }

    // Render OpSet box (Slayer class icons)
    if (slotInfo_.race == Race::Slayer && imageHaysSpk_)
    {
        imageHaysSpk_->Draw(config_.opsetBoxX, config_.opsetBoxY, 0);

        // Render class icons
        static const int classIconFrames[5] = { 0, 1, 2, 3, 4 }; // BTN_BLAZE_NORMAL, BTN_SWORD_NORMAL, etc.
        for (int i = 0; i < 5; ++i)
        {
            int frame = classIconFrames[i];
            if (slotInfo_.slayerClass == static_cast<SlayerClassIcon>(i))
            {
                frame += 5; // Pressed state
            }
            imageHaysSpk_->Draw(config_.slayerClassIconX[i], config_.slayerClassIconY[i], frame);

            // Render label if enabled
            if (config_.slayerClassLabelPosEnabled[i])
            {
                // g_PrintColorStr(config_.slayerClassLabelX[i], config_.slayerClassLabelY[i], classNames[i], ...);
            }
        }

        // Current class text
        if (config_.slayerCurrentClassTextPosEnabled)
        {
            // g_PrintColorStr(config_.slayerCurrentClassTextX, config_.slayerCurrentClassTextY, ...);
        }
    }

    // Render face selection
    if (imageSpk_)
    {
        for (int i = 0; i < 3; ++i)
        {
            // Radio buttons
            int radioFrame = (slotInfo_.faceSelection == static_cast<FaceSelection>(i)) ? 1 : 0;
            imageSpk_->Draw(
                (i == 0) ? config_.selectLeftFaceRadioX : (i == 1) ? config_.selectCenterFaceRadioX : config_.selectRightFaceRadioX,
                (i == 0) ? config_.selectLeftFaceRadioY : (i == 1) ? config_.selectCenterFaceRadioY : config_.selectRightFaceRadioY,
                radioFrame);

            // Face preview
            int faceFrame = slotInfo_.gender == Gender::Female
                ? 0 // CHAR_FACE_SLAYER_FEMALE_SELECT_1 + i
                : 0; // CHAR_FACE_SLAYER_MALE_SELECT_1 + i
            imageSpk_->Draw(
                (i == 0) ? config_.selectLeftFaceX : (i == 1) ? config_.selectCenterFaceX : config_.selectRightFaceX,
                (i == 0) ? config_.selectLeftFaceY : (i == 1) ? config_.selectCenterFaceY : config_.selectRightFaceY,
                faceFrame);
        }
    }

    // Render race selection
    if (imageHaysSpk_)
    {
        // Radio buttons
        for (int i = 0; i < 3; ++i)
        {
            int radioX = (i == 0) ? config_.selectRaceSlayerRadioX : (i == 1) ? config_.selectRaceVampireRadioX : config_.selectRaceOustersRadioX;
            int radioY = (i == 0) ? config_.selectRaceSlayerRadioY : (i == 1) ? config_.selectRaceVampireRadioY : config_.selectRaceOustersRadioY;
            int radioFrame = (slotInfo_.race == static_cast<Race>(i)) ? 1 : 0;
            imageHaysSpk_->Draw(radioX, radioY, radioFrame);
        }

        // Race buttons
        static const int raceButtonFrames[3] = { 0, 1, 2 }; // BUTTON_SLAYER_NORMAL, BUTTON_VAMPIRE_NORMAL, BUTTON_OUTER_NORMAL
        for (int i = 0; i < 3; ++i)
        {
            int btnX = (i == 0) ? config_.selectRaceSlayerButtonX : (i == 1) ? config_.selectRaceVampireButtonX : config_.selectRaceOustersButtonX;
            int btnY = (i == 0) ? config_.selectRaceSlayerButtonY : (i == 1) ? config_.selectRaceVampireButtonY : config_.selectRaceOustersButtonY;
            int frame = raceButtonFrames[i];
            if (slotInfo_.race == static_cast<Race>(i))
                frame += 3; // Pressed variant
            imageHaysSpk_->Draw(btnX, btnY, frame);
        }
    }

    // Render gender selection
    if (imageHaysSpk_)
    {
        // Male
        int maleRadioFrame = (slotInfo_.gender == Gender::Male) ? 1 : 0;
        imageHaysSpk_->Draw(config_.maleSelectRadioX, config_.maleSelectRadioY, maleRadioFrame);
        imageHaysSpk_->Draw(config_.maleSelectButtonX, config_.maleSelectButtonY, 0);

        // Female
        int femaleRadioFrame = (slotInfo_.gender == Gender::Female) ? 1 : 0;
        imageHaysSpk_->Draw(config_.femaleSelectRadioX, config_.femaleSelectRadioY, femaleRadioFrame);
        imageHaysSpk_->Draw(config_.femaleSelectButtonX, config_.femaleSelectButtonY, 0);
    }

    // Render name input
    if (nameEditor_)
    {
        nameEditor_->Render();
    }

    // Render stat text
    // g_PrintColorStr(config_.strTextX, config_.strTextY, "STR: " + std::to_string(slotInfo_.str), ...);
    // g_PrintColorStr(config_.dexTextX, config_.dexTextY, "DEX: " + std::to_string(slotInfo_.dex), ...);
    // g_PrintColorStr(config_.intTextX, config_.intTextY, "INT: " + std::to_string(slotInfo_.int_), ...);
    // g_PrintColorStr(config_.hpTextX, config_.hpTextY, "HP: " + std::to_string(slotInfo_.hp), ...);
    // g_PrintColorStr(config_.mpTextX, config_.mpTextY, "MP: " + std::to_string(slotInfo_.mp), ...);
    // g_PrintColorStr(config_.defenseTextX, config_.defenseTextY, "DEF: " + std::to_string(slotInfo_.defense), ...);
    // g_PrintColorStr(config_.protectionTextX, config_.protectionTextY, "PROT: " + std::to_string(slotInfo_.protection), ...);
    // g_PrintColorStr(config_.tohitTextX, config_.tohitTextY, "HIT: " + std::to_string(slotInfo_.tohit), ...);
    // g_PrintColorStr(config_.damTextX, config_.damTextY, "DAM: " + std::to_string(slotInfo_.dam), ...);
    // g_PrintColorStr(config_.bonusTextX, config_.bonusTextY, "BONUS: " + std::to_string(slotInfo_.bonusPoints), ...);

    // Render effects
    if (imageSpk_)
    {
        imageSpk_->Draw(config_.effectFireLeft1X, config_.effectFireLeft1Y, 0);
        imageSpk_->Draw(config_.effectFireLeft2X, config_.effectFireLeft2Y, 0);
        imageSpk_->Draw(config_.effectFireRight1X, config_.effectFireRight1Y, 0);
        imageSpk_->Draw(config_.effectFireRight2X, config_.effectFireRight2Y, 0);
        imageSpk_->Draw(config_.effectFireCenterX, config_.effectFireCenterY, 0);
    }

    // Render rating icons
    if (imageHaysSpk_)
    {
        imageHaysSpk_->Draw(config_.childX, config_.childY, 0);
        imageHaysSpk_->Draw(config_.adultX, config_.adultY, 1);
        imageHaysSpk_->Draw(config_.violenceX, config_.violenceY, 2);
        imageHaysSpk_->Draw(config_.fearX, config_.fearY, 3);
    }

    // Render text bar and lines
    if (imageHaysSpk_)
    {
        imageHaysSpk_->Draw(config_.textBarX, config_.textBarY, 0);
    }
    // g_PrintColorStr(config_.textLine1X, config_.textLine1Y, textLine1String, ...);
    // g_PrintColorStr(config_.textLine2X, config_.textLine2Y, textLine2String, ...);
    // g_PrintColorStr(config_.textLine3X, config_.textLine3Y, textLine3String, ...);
    // g_PrintColorStr(config_.textLine4X, config_.textLine4Y, textLine4String, ...);

    // Render animation mark
    if (imageHaysSpk_)
    {
        imageHaysSpk_->DrawAlpha(config_.aniMarkX, config_.aniMarkY, 0, animationAlpha_);
    }

    // Render premium text
    // char szBuffer[256];
    // sprintf(szBuffer, "Premium: %d days", premiumDays);
    // g_PrintColorStr(config_.premiumUseDayTextX - len, config_.premiumUseDayTextY, szBuffer, ...);

    // Render warning string
    // g_PrintColorStr(config_.warningStringX, config_.warningStringY, "Child Guard...", ...);

    // Render buttons
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }

    // Render Save/Load/Reset buttons
    if (imageHaysSpk_)
    {
        imageHaysSpk_->Draw(config_.saveButtonX, config_.saveButtonY, 0);
        imageHaysSpk_->Draw(config_.loadButtonX, config_.loadButtonY, 0);
        imageHaysSpk_->Draw(config_.resetButtonX, config_.resetButtonY, 0);
    }

    // Render stet buttons
    for (int i = 0; i < 3; ++i)
    {
        if (imageHaysSpk_)
        {
            imageHaysSpk_->Draw(config_.stetPlusButtonX[i], config_.stetPlusButtonY[i], 0);
            imageHaysSpk_->Draw(config_.stetMinusButtonX[i], config_.stetMinusButtonY[i], 0);
        }
    }
}

} // namespace phoenix::ui