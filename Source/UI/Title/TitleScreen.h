#pragma once

#include "UI/MinimalUIComponents.h"
#include "Settings/InterfaceProfileLoader.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <functional>
#include <Windows.h>

namespace phoenix::ui
{

// Title screen button roles (matching Legacy C_VS_UI_TITLE)
enum class TitleButtonRole : std::uint8_t
{
    Connect = 0,
    NewAccount = 1,
    Option = 2,
    Credit = 3,
    Exit = 4,
    Count
};

// Title screen state
enum class TitleScreenState : std::uint8_t
{
    Idle,
    ShowingCredit,
    ShowingDialog
};

// Title screen config from interface.ini [Title_1280x960] section
struct TitleConfig
{
    // Window rect (centered by default)
    std::int32_t windowX{ 0 };
    std::int32_t windowY{ 0 };
    std::int32_t windowWidth{ 1280 };
    std::int32_t windowHeight{ 960 };

    // Background
    std::int32_t backgroundSprite{ 0 };

    // Animation mark
    std::int32_t aniMarkX{ 0 };
    std::int32_t aniMarkY{ 0 };

    // Button layout
    std::int32_t buttonLayoutX{ 0 };
    std::int32_t buttonLayoutY{ 0 };

    // Edit controls
    std::int32_t idEditX{ 0 };
    std::int32_t idEditY{ 0 };
    std::int32_t pwEditX{ 0 };
    std::int32_t pwEditY{ 0 };

    // Buttons
    std::int32_t gameRunX{ 0 };
    std::int32_t gameRunY{ 0 };
    std::int32_t idCreateX{ 0 };
    std::int32_t idCreateY{ 0 };
    std::int32_t optionX{ 0 };
    std::int32_t optionY{ 0 };
    std::int32_t gameCreatersX{ 0 };
    std::int32_t gameCreatersY{ 0 };
    std::int32_t gameExitX{ 0 };
    std::int32_t gameExitY{ 0 };

    // Effect
    std::int32_t effectWaterLightX{ 0 };
    std::int32_t effectWaterLightY{ 0 };

    // Text image
    std::int32_t textImageX{ 0 };
    std::int32_t textImageY{ 0 };

    // Rating icons
    std::int32_t childX{ 0 };
    std::int32_t childY{ 0 };
    std::int32_t adultX{ 0 };
    std::int32_t adultY{ 0 };
    std::int32_t violenceX{ 0 };
    std::int32_t violenceY{ 0 };
    std::int32_t fearX{ 0 };
    std::int32_t fearY{ 0 };

    // Warning string
    std::int32_t warningStringX{ 0 };
    std::int32_t warningStringY{ 0 };

    // Defaults from Legacy (fallback percentages)
    bool useDefaults{ true };
};

class TitleScreen final
{
public:
    explicit TitleScreen() noexcept;

    // Config
    void LoadConfig(const settings::RuntimeInterfaceProfile& interfaceProfile) noexcept;
    void LoadFromIni(const std::filesystem::path& iniPath) noexcept;
    void ReloadConfig() noexcept;

    // State
    void Show() noexcept;
    void Hide() noexcept;
    [[nodiscard]] bool IsVisible() const noexcept { return visible_; }

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;
    bool HandleChar(wchar_t ch) noexcept;

    // Button callbacks
    void OnConnect() noexcept;
    void OnNewAccount() noexcept;
    void OnOption() noexcept;
    void OnCredit() noexcept;
    void OnExit() noexcept;

    // Hot-reload
    void ReloadInterfaceConfig() noexcept;

private:
    // Legacy-style member variables (mapped from C_VS_UI_TITLE)
    bool visible_{ false };
    TitleScreenState state_{ TitleScreenState::Idle };

    // SPK archives (Title, NewTitle, TitleMenuDefault)
    std::unique_ptr<SpriteArchive> titleSpk_;
    std::unique_ptr<SpriteArchive> titleNewSpk_;
    std::unique_ptr<SpriteArchive> titleMenuDefaultSpk_;

    // Button group
    std::unique_ptr<ButtonGroup> buttonGroup_;

    // Edit controls
    std::unique_ptr<LineEditor> idEdit_;
    std::unique_ptr<LineEditor> passwordEdit_;

    // Config
    TitleConfig config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // UI state
    bool autoLogin_{ false };
    bool netmarbleLogin_{ false };
    bool showCredit_{ false };
    std::uint32_t animationTimer_{ 0 };
    std::uint32_t creditScroll_{ 0 };
    std::unique_ptr<SpriteArchive> creditSpk_;
    std::string savedId_;

    // Animation
    std::uint8_t aniArpha_{ 31 };
    bool aniArphaUp_{ false };

    // Child windows
    std::unique_ptr<DialogWindow> dialog_;
    std::unique_ptr<SpriteArchive> creditWindow_; // TODO: implement proper CreditWindow

    // Helpers
    void LoadTitleConfigFromProfile(const settings::GameTitleProfile& profile) noexcept;
    void LoadTitleConfigFromIni() noexcept;
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;
    void PositionEdits() noexcept;
    void UpdateAnimation() noexcept;
    void UpdateCreditScroll() noexcept;
    void DrawBackground() noexcept;
    void DrawTitleSprite() noexcept;
    void DrawButtons() noexcept;
    void DrawEdits() noexcept;
    void DrawCredit() noexcept;
    void DrawWaterLightEffect() noexcept;
    void DrawWarningText() noexcept;

    // Callbacks
    void OnConnectClicked() noexcept;
    void OnNewAccountClicked() noexcept;
    void OnOptionClicked() noexcept;
    void OnCreditClicked() noexcept;
    void OnExitClicked() noexcept;

    // Input focus
    void ChangeFocus() noexcept;
    [[nodiscard]] bool ReadyToSend() const noexcept;
    void SendLoginToClient() noexcept;

    // Button hit test
    [[nodiscard]] bool IsPointInButton(TitleButtonRole role, int x, int y) const noexcept;
};

} // namespace phoenix::ui