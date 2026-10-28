#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "Protocol/LegacyProtocol.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <array>

namespace phoenix::ui
{

// Resurrect modes (matching Legacy RESURRECT_MODE enum)
enum class ResurrectMode : std::uint8_t
{
    ResurrectScroll = 0,
    Elixir = 1,
    Eternity = 2,
    SiegeAttacker = 3,
    SkillFromOther = 4,
    Count = 5
};

// Resurrect button data
struct ResurrectButton
{
    bool enabled{ false };
    int imageIndex{ -1 };      // Frame index in SPK
    std::int32_t delay{ 0 };   // Cooldown in ms
    std::uint32_t startTime{ 0 }; // timeGetTime() when delay started
    std::int32_t left{ 0 };
    std::int32_t top{ 0 };
    std::int32_t right{ 0 };
    std::int32_t bottom{ 0 };

    [[nodiscard]] bool IsPointInside(int x, int y) const noexcept
    {
        return x >= left && x < right && y >= top && y < bottom;
    }
};

// ResurrectRequest config from interface.ini [ResurrectRequest_1280x960]
struct ResurrectRequestProfile
{
    std::int32_t windowX{ 2 };
    std::int32_t windowY{ 0 };  // Set at runtime based on race
    std::int32_t windowWidth{ 200 };
    std::int32_t windowHeight{ 50 };

    // Button layout
    std::int32_t startX{ 17 };     // 2 + 15 + guard width
    std::int32_t startY{ 0 };      // Same as windowY
    std::int32_t buttonGap{ 15 };  // Gap between buttons

    // SPK frames (matching Legacy)
    std::int32_t guardSlayerFrame{ 0 };
    std::int32_t guardVampireFrame{ 1 };
    std::int32_t guardOustersFrame{ 2 };

    std::int32_t resurrectScrollFrame{ 3 };
    std::int32_t elixirFrame{ 4 };
    std::int32_t eternityFrame{ 5 };
    std::int32_t siegeAttackerSlayerFrame{ 6 };
    std::int32_t siegeAttackerVampireFrame{ 7 };
    std::int32_t siegeAttackerOustersFrame{ 8 };
    std::int32_t skillFromOtherSlayerFrame{ 9 };
    std::int32_t skillFromOtherVampireFrame{ 10 };
    std::int32_t skillFromOtherOustersFrame{ 11 };

    // Race-specific Y offsets
    std::int32_t slayerYOffset{ 218 };
    std::int32_t vampireYOffset{ 175 };
    std::int32_t oustersYOffset{ 218 };
};

class ResurrectRequestWindow
{
public:
    explicit ResurrectRequestWindow() noexcept;

    // State
    void Open(const std::array<bool, 5>& enabledModes, protocol::PcType race) noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const settings::ResurrectRequestProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // External API
    void SetDelay(ResurrectMode mode, std::int32_t delayMs) noexcept;
    void SetEnabled(ResurrectMode mode, bool enabled) noexcept;

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Callbacks
    [[nodiscard]] std::optional<ResurrectMode> GetClickedMode() const noexcept { return clickedMode_; }
    void ClearClickedMode() noexcept { clickedMode_.reset(); }

private:
    // State
    bool isOpen_{ false };
    protocol::PcType race_{ protocol::PcType::Slayer };

    // Config
    settings::ResurrectRequestProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // Buttons
    struct Button
    {
        bool enabled{ false };
        int imageIndex{ -1 };
        std::int32_t delay{ 0 };
        std::uint32_t startTime{ 0 };
        std::int32_t left{ 0 };
        std::int32_t top{ 0 };
        std::int32_t right{ 0 };
        std::int32_t bottom{ 0 };

        [[nodiscard]] bool IsPointInside(int x, int y) const noexcept
        {
            return x >= left && x < right && y >= top && y < bottom;
        }
    };
    std::array<Button, 5> buttons_;

    // Interaction state
    int focusIndex_{ -1 };
    int pushedIndex_{ -1 };
    std::optional<ResurrectMode> clickedMode_;

    // SPK archive
    std::unique_ptr<SpriteArchive> levelUpSpk_;

    // Window position
    std::int32_t windowX_{ 2 };
    std::int32_t windowY_{ 0 };
    std::int32_t windowWidth_{ 200 };
    std::int32_t windowHeight_{ 50 };

    // Helpers
    void InitializeButtons() noexcept;
    void UpdateButtonRects() noexcept;
    void UpdateButtonImages() noexcept;
    [[nodiscard]] std::int32_t GetRaceYOffset() const noexcept;
    [[nodiscard]] std::int32_t GetButtonWidth() const noexcept;
    [[nodiscard]] std::int32_t GetButtonHeight() const noexcept;
    void UpdateDelays() noexcept;
    [[nodiscard]] std::string GetTooltip(ResurrectMode mode) const noexcept;

    // Callbacks
    void OnButtonClicked(ResurrectMode mode) noexcept;
};

} // namespace phoenix::ui