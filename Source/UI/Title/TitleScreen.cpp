#include "UI/Title/TitleScreen.h"

#include "Settings/InterfaceProfileLoader.h"
#include "Settings/SettingsTypes.h"
#include "UI/Layout/LoginTitleLayout.h"
#include "UI/Resources/LegacyUiSpriteResourceLoader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <Windows.h>

namespace phoenix::ui
{

TitleScreen::TitleScreen() noexcept
    : visible_(false)
    , state_(TitleScreenState::Idle)
    , autoLogin_(false)
    , netmarbleLogin_(false)
    , showCredit_(false)
    , animationTimer_(0)
    , creditScroll_(0)
    , aniArpha_(31)
    , aniArphaUp_(false)
{
}

void TitleScreen::LoadConfig(const settings::RuntimeInterfaceProfile& interfaceProfile) noexcept
{
    LoadTitleConfigFromProfile(interfaceProfile.ingameUi.title);
}

void TitleScreen::LoadFromIni(const std::filesystem::path& iniPath) noexcept
{
    iniPath_ = iniPath;
    if (std::filesystem::exists(iniPath_))
    {
        iniLastWrite_ = std::filesystem::last_write_time(iniPath_);
        LoadTitleConfigFromIni();
    }
}

void TitleScreen::ReloadConfig() noexcept
{
    LoadFromIni(iniPath_);
    if (config_.useDefaults)
    {
        InitializeButtons();
        PositionButtons();
        PositionEdits();
    }
}

void TitleScreen::Show() noexcept
{
    if (visible_)
        return;

    visible_ = true;
    state_ = TitleScreenState::Idle;

    // Initialize SPK archives
    titleSpk_ = std::make_unique<SpriteArchive>();
    titleNewSpk_ = std::make_unique<SpriteArchive>();
    titleMenuDefaultSpk_ = std::make_unique<SpriteArchive>();

    titleSpk_->Open("Title.spk");
    titleNewSpk_->Open("TitleNew.spk");
    titleMenuDefaultSpk_->Open("TitleMenuDefault.spk");

    // Initialize UI elements
    InitializeButtons();
    PositionButtons();
    PositionEdits();

    // Load edit controls
    if (!autoLogin_ && !netmarbleLogin_)
    {
        idEdit_ = std::make_unique<LineEditor>();
        passwordEdit_ = std::make_unique<LineEditor>();

        idEdit_->SetPosition(config_.idEditX, config_.idEditY);
        passwordEdit_->SetPosition(config_.pwEditX, config_.pwEditY);
        passwordEdit_->SetPasswordMode(true);
        passwordEdit_->SetByteLimit(10);

        // Restore saved ID if available
        if (!savedId_.empty())
        {
            idEdit_->AddString(savedId_.c_str());
            ChangeFocus();
        }
    }
}

void TitleScreen::Hide() noexcept
{
    visible_ = false;
    state_ = TitleScreenState::Idle;

    // Clean up
    titleSpk_.reset();
    titleNewSpk_.reset();
    titleMenuDefaultSpk_.reset();
    buttonGroup_.reset();
    idEdit_.reset();
    passwordEdit_.reset();
}

void TitleScreen::Update(float deltaTime) noexcept
{
    if (!visible_)
        return;

    // Hot-reload check
    if (std::filesystem::exists(iniPath_))
    {
        const auto writeTime = std::filesystem::last_write_time(iniPath_);
        if (writeTime != iniLastWrite_)
        {
            ReloadConfig();
        }
    }

    UpdateAnimation();

    if (showCredit_)
    {
        UpdateCreditScroll();
    }
}

void TitleScreen::Render() noexcept
{
    if (!visible_)
        return;

    DrawBackground();
    DrawTitleSprite();
    DrawButtons();
    DrawEdits();

    if (showCredit_)
    {
        DrawCredit();
    }

    DrawWaterLightEffect();
    DrawWarningText();
}

bool TitleScreen::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!visible_)
        return false;

    if (showCredit_)
    {
        if (rightButton || x < 0 || y < 0)
        {
            showCredit_ = false;
            creditWindow_.reset();
            return true;
        }
        return true;
    }

    if (dialog_)
    {
        return dialog_->HandleMouseDown(x, y, rightButton);
    }

    if (buttonGroup_)
    {
        return buttonGroup_->HandleMouseDown(x, y);
    }

    // Check edit focus
    if (idEdit_ && idEdit_->HitTest(x, y))
    {
        idEdit_->SetFocus(true);
        passwordEdit_->SetFocus(false);
        return true;
    }
    if (passwordEdit_ && passwordEdit_->HitTest(x, y))
    {
        passwordEdit_->SetFocus(false);
        idEdit_->SetFocus(true);
        return true;
    }

    return false;
}

bool TitleScreen::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!visible_)
        return false;

    if (buttonGroup_)
    {
        return buttonGroup_->HandleMouseUp(x, y);
    }

    return false;
}

bool TitleScreen::HandleMouseMove(int x, int y) noexcept
{
    if (!visible_)
        return false;

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    return false;
}

bool TitleScreen::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!visible_)
        return false;

    if (showCredit_)
    {
        if (key == VK_ESCAPE)
        {
            showCredit_ = false;
            creditWindow_.reset();
            return true;
        }
        return true;
    }

    if (dialog_)
    {
        return dialog_->HandleKeyDown(key);
    }

    switch (key)
    {
        case VK_TAB:
            ChangeFocus();
            return true;
        case VK_ESCAPE:
            OnExitClicked();
            return true;
        case VK_RETURN:
            if (idEdit_ && idEdit_->HasFocus())
            {
                if (ReadyToSend())
                {
                    SendLoginToClient();
                }
                else
                {
                    ChangeFocus();
                }
            }
            else if (passwordEdit_ && passwordEdit_->HasFocus())
            {
                if (ReadyToSend())
                {
                    SendLoginToClient();
                }
            }
            return true;
    }

    return false;
}

bool TitleScreen::HandleChar(wchar_t ch) noexcept
{
    if (!visible_)
        return false;

    if (idEdit_ && idEdit_->HasFocus())
    {
        return idEdit_->AddChar(ch);
    }
    if (passwordEdit_ && passwordEdit_->HasFocus())
    {
        return passwordEdit_->AddChar(ch);
    }

    return false;
}

void TitleScreen::OnConnect() noexcept
{
    if (!autoLogin_ && !netmarbleLogin_)
    {
        if (ReadyToSend())
        {
            SendLoginToClient();
        }
        else
        {
            ChangeFocus();
        }
    }
    else
    {
        // Auto-login or Netmarble login
    }
}

void TitleScreen::OnNewAccount() noexcept
{
    // Open new account registration
}

void TitleScreen::OnOption() noexcept
{
    // Open options screen
}

void TitleScreen::OnCredit() noexcept
{
    showCredit_ = true;
    creditSpk_ = std::make_unique<SpriteArchive>();
    creditSpk_->Open("Credit.spk");
    creditScroll_ = 0;
    // creditWindow_ = std::make_unique<CreditWindow>(); // TODO: implement CreditWindow
}

void TitleScreen::OnExit() noexcept
{
    // Show exit confirmation dialog
    dialog_ = std::make_unique<DialogWindow>(DialogWindow::DialogType::ExitConfirm);
    dialog_->Show();
}

void TitleScreen::ReloadInterfaceConfig() noexcept
{
    LoadFromIni(iniPath_);
    InitializeButtons();
    PositionButtons();
    PositionEdits();
}

void TitleScreen::LoadTitleConfigFromProfile(const settings::GameTitleProfile& profile) noexcept
{
    config_.windowX = profile.windowX;
    config_.windowY = profile.windowY;
    config_.windowWidth = profile.windowWidth;
    config_.windowHeight = profile.windowHeight;
    config_.backgroundSprite = profile.backgroundSprite;
    config_.aniMarkX = profile.aniMarkX;
    config_.aniMarkY = profile.aniMarkY;
    config_.buttonLayoutX = profile.buttonLayoutX;
    config_.buttonLayoutY = profile.buttonLayoutY;
    config_.idEditX = profile.idEditX;
    config_.idEditY = profile.idEditY;
    config_.pwEditX = profile.pwEditX;
    config_.pwEditY = profile.pwEditY;
    config_.gameRunX = profile.gameRunX;
    config_.gameRunY = profile.gameRunY;
    config_.idCreateX = profile.idCreateX;
    config_.idCreateY = profile.idCreateY;
    config_.optionX = profile.optionX;
    config_.optionY = profile.optionY;
    config_.gameCreatersX = profile.gameCreatersX;
    config_.gameCreatersY = profile.gameCreatersY;
    config_.gameExitX = profile.gameExitX;
    config_.gameExitY = profile.gameExitY;
    config_.effectWaterLightX = profile.effectWaterLightX;
    config_.effectWaterLightY = profile.effectWaterLightY;
    config_.textImageX = profile.textImageX;
    config_.textImageY = profile.textImageY;
    config_.childX = profile.childX;
    config_.childY = profile.childY;
    config_.adultX = profile.adultX;
    config_.adultY = profile.adultY;
    config_.violenceX = profile.violenceX;
    config_.violenceY = profile.violenceY;
    config_.fearX = profile.fearX;
    config_.fearY = profile.fearY;
    config_.warningStringX = profile.warningStringX;
    config_.warningStringY = profile.warningStringY;
    config_.useDefaults = profile.useDefaults;
}

void TitleScreen::LoadTitleConfigFromIni() noexcept
{
    if (!std::filesystem::exists(iniPath_))
        return;

    // Parse ini for [GameTitle_1280x960] section
    std::ifstream file(iniPath_);
    std::string line;
    std::string currentSection;
    bool inTitleSection = false;

    while (std::getline(file, line))
    {
        // Trim
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());

        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;

        if (line[0] == '[' && line.back() == ']')
        {
            currentSection = line.substr(1, line.size() - 2);
            inTitleSection = (currentSection == "GameTitle_1280x960");
            continue;
        }

        if (!inTitleSection)
            continue;

        const auto eqPos = line.find('=');
        if (eqPos == std::string::npos)
            continue;

        const std::string key = line.substr(0, eqPos);
        const std::string value = line.substr(eqPos + 1);

        auto parseInt = [](const std::string& v) -> std::int32_t
        {
            try { return std::stoi(v); }
            catch (...) { return 0; }
        };

        auto parseBool = [](const std::string& v) -> bool
        {
            return v == "true" || v == "1" || v == "True" || v == "TRUE";
        };

        if (key == "WindowX") config_.windowX = parseInt(value);
        else if (key == "WindowY") config_.windowY = parseInt(value);
        else if (key == "WindowWidth") config_.windowWidth = parseInt(value);
        else if (key == "WindowHeight") config_.windowHeight = parseInt(value);
        else if (key == "BackgroundSprite") config_.backgroundSprite = parseInt(value);
        else if (key == "AniMarkX") config_.aniMarkX = parseInt(value);
        else if (key == "AniMarkY") config_.aniMarkY = parseInt(value);
        else if (key == "ButtonLayoutX") config_.buttonLayoutX = parseInt(value);
        else if (key == "ButtonLayoutY") config_.buttonLayoutY = parseInt(value);
        else if (key == "IDEditX") config_.idEditX = parseInt(value);
        else if (key == "IDEditY") config_.idEditY = parseInt(value);
        else if (key == "PWEditX") config_.pwEditX = parseInt(value);
        else if (key == "PWEditY") config_.pwEditY = parseInt(value);
        else if (key == "GameRunX") config_.gameRunX = parseInt(value);
        else if (key == "GameRunY") config_.gameRunY = parseInt(value);
        else if (key == "IDCreateX") config_.idCreateX = parseInt(value);
        else if (key == "IDCreateY") config_.idCreateY = parseInt(value);
        else if (key == "OptionX") config_.optionX = parseInt(value);
        else if (key == "OptionY") config_.optionY = parseInt(value);
        else if (key == "GameCreatersX") config_.gameCreatersX = parseInt(value);
        else if (key == "GameCreatersY") config_.gameCreatersY = parseInt(value);
        else if (key == "GameExitX") config_.gameExitX = parseInt(value);
        else if (key == "GameExitY") config_.gameExitY = parseInt(value);
        else if (key == "EffectWaterLightX") config_.effectWaterLightX = parseInt(value);
        else if (key == "EffectWaterLightY") config_.effectWaterLightY = parseInt(value);
        else if (key == "TextImageX") config_.textImageX = parseInt(value);
        else if (key == "TextImageY") config_.textImageY = parseInt(value);
        else if (key == "ChildX") config_.childX = parseInt(value);
        else if (key == "ChildY") config_.childY = parseInt(value);
        else if (key == "AdultX") config_.adultX = parseInt(value);
        else if (key == "AdultY") config_.adultY = parseInt(value);
        else if (key == "ViolenceX") config_.violenceX = parseInt(value);
        else if (key == "ViolenceY") config_.violenceY = parseInt(value);
        else if (key == "FearX") config_.fearX = parseInt(value);
        else if (key == "FearY") config_.fearY = parseInt(value);
        else if (key == "WarningStringX") config_.warningStringX = parseInt(value);
        else if (key == "WarningStringY") config_.warningStringY = parseInt(value);
        else if (key == "UseDefaults") config_.useDefaults = parseBool(value);
    }
}

void TitleScreen::InitializeButtons() noexcept
{
    buttonGroup_ = std::make_unique<ButtonGroup>();

    // CONNECT (Game Run)
    buttonGroup_->AddButton(static_cast<int>(TitleButtonRole::Connect),
        config_.gameRunX, config_.gameRunY,
        titleMenuDefaultSpk_->GetWidth(0), titleMenuDefaultSpk_->GetHeight(0),
        [this]() { OnConnectClicked(); });

    // NEW ACCOUNT (ID Create)
    buttonGroup_->AddButton(static_cast<int>(TitleButtonRole::NewAccount),
        config_.idCreateX, config_.idCreateY,
        titleMenuDefaultSpk_->GetWidth(1), titleMenuDefaultSpk_->GetHeight(1),
        [this]() { OnNewAccountClicked(); });

    // OPTION
    buttonGroup_->AddButton(static_cast<int>(TitleButtonRole::Option),
        config_.optionX, config_.optionY,
        titleMenuDefaultSpk_->GetWidth(2), titleMenuDefaultSpk_->GetHeight(2),
        [this]() { OnOptionClicked(); });

    // CREDIT
    buttonGroup_->AddButton(static_cast<int>(TitleButtonRole::Credit),
        config_.gameCreatersX, config_.gameCreatersY,
        titleMenuDefaultSpk_->GetWidth(3), titleMenuDefaultSpk_->GetHeight(3),
        [this]() { OnCreditClicked(); });

    // EXIT
    buttonGroup_->AddButton(static_cast<int>(TitleButtonRole::Exit),
        config_.gameExitX, config_.gameExitY,
        titleMenuDefaultSpk_->GetWidth(4), titleMenuDefaultSpk_->GetHeight(4),
        [this]() { OnExitClicked(); });
}

void TitleScreen::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(static_cast<int>(TitleButtonRole::Connect), config_.gameRunX, config_.gameRunY);
    buttonGroup_->SetPosition(static_cast<int>(TitleButtonRole::NewAccount), config_.idCreateX, config_.idCreateY);
    buttonGroup_->SetPosition(static_cast<int>(TitleButtonRole::Option), config_.optionX, config_.optionY);
    buttonGroup_->SetPosition(static_cast<int>(TitleButtonRole::Credit), config_.gameCreatersX, config_.gameCreatersY);
    buttonGroup_->SetPosition(static_cast<int>(TitleButtonRole::Exit), config_.gameExitX, config_.gameExitY);
}

void TitleScreen::PositionEdits() noexcept
{
    if (idEdit_)
    {
        idEdit_->SetPosition(config_.idEditX, config_.idEditY);
    }
    if (passwordEdit_)
    {
        passwordEdit_->SetPosition(config_.pwEditX, config_.pwEditY);
    }
}

void TitleScreen::UpdateAnimation() noexcept
{
    if (aniArphaUp_)
    {
        if (aniArpha_ >= 31)
        {
            aniArphaUp_ = false;
        }
        else
        {
            aniArpha_++;
        }
    }
    else
    {
        if (aniArpha_ <= 0)
        {
            aniArphaUp_ = true;
        }
        else
        {
            aniArpha_--;
        }
    }
}

void TitleScreen::UpdateCreditScroll() noexcept
{
    creditScroll_ += 2;
    if (creditScroll_ > 2000) // Reset after full scroll
    {
        creditScroll_ = 0;
    }
}

void TitleScreen::DrawBackground() noexcept
{
    // Draw title background sprite
    if (titleSpk_)
    {
        const auto id = config_.backgroundSprite;
        const auto w = titleSpk_->GetWidth(id);
        const auto h = titleSpk_->GetHeight(id);
        const int x = (1280 - w) / 2;
        const int y = (960 - h) / 2;
        titleSpk_->Draw(x, y, id);
    }
}

void TitleScreen::DrawTitleSprite() noexcept
{
    // Draw title_new_spk (main title graphic)
    if (titleNewSpk_)
    {
        const auto id = 0; // Default frame
        const auto w = titleNewSpk_->GetWidth(id);
        const auto h = titleNewSpk_->GetHeight(id);
        const int x = (1280 - w) / 2;
        const int y = (960 - h) / 2;
        titleNewSpk_->Draw(x, y, id);
    }
}

void TitleScreen::DrawButtons() noexcept
{
    if (buttonGroup_)
    {
        buttonGroup_->Render();
    }
}

void TitleScreen::DrawEdits() noexcept
{
    if (idEdit_)
    {
        idEdit_->Render();
    }
    if (passwordEdit_)
    {
        passwordEdit_->Render();
    }
}

void TitleScreen::DrawCredit() noexcept
{
    if (!creditSpk_ || !showCredit_)
        return;

    // Draw credit background
    const int scrollY = creditScroll_;
    creditSpk_->Draw(0, -scrollY, 0);
}

void TitleScreen::DrawWaterLightEffect() noexcept
{
    // Draw water light effect at configured position
    if (config_.effectWaterLightX > 0 || config_.effectWaterLightY > 0)
    {
        // Legacy effect sprite
    }
}

void TitleScreen::DrawWarningText() noexcept
{
    if (config_.warningStringX > 0 || config_.warningStringY > 0)
    {
        // Draw warning text from game string table
    }
}

void TitleScreen::OnConnectClicked() noexcept
{
    OnConnect();
}

void TitleScreen::OnNewAccountClicked() noexcept
{
    OnNewAccount();
}

void TitleScreen::OnOptionClicked() noexcept
{
    OnOption();
}

void TitleScreen::OnCreditClicked() noexcept
{
    OnCredit();
}

void TitleScreen::OnExitClicked() noexcept
{
    OnExit();
}

void TitleScreen::ChangeFocus() noexcept
{
    if (idEdit_ && idEdit_->HasFocus())
    {
        passwordEdit_->SetFocus(true);
        idEdit_->SetFocus(false);
    }
    else
    {
        idEdit_->SetFocus(true);
        passwordEdit_->SetFocus(false);
    }
}

bool TitleScreen::ReadyToSend() const noexcept
{
    return idEdit_ && idEdit_->Size() > 0 &&
           passwordEdit_ && passwordEdit_->Size() > 0;
}

void TitleScreen::SendLoginToClient() noexcept
{
    if (!ReadyToSend())
        return;

    // Save ID for next session
    if (idEdit_)
    {
        savedId_ = idEdit_->GetString();
    }

    // Send login packet
    // Legacy: SendLoginToClient() -> EncodeCLLogin -> send
}

bool TitleScreen::IsPointInButton(TitleButtonRole role, int x, int y) const noexcept
{
    if (!buttonGroup_)
        return false;

    return buttonGroup_->HitTest(static_cast<int>(role), x, y);
}

} // namespace phoenix::ui