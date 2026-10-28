#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "Protocol/LegacyProtocol.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <array>

namespace phoenix::ui
{

// RemoveOption status (matching Legacy STATUS)
enum class RemoveOptionStatus : std::uint8_t
{
    SelectOption = 0,
    Question = 1,
    Count
};

// RemoveOption config from interface.ini [RemoveOption_1280x960]
struct RemoveOptionProfile
{
    std::int32_t windowX{ 0 };
    std::int32_t windowY{ 0 };
    std::int32_t windowWidth{ 340 };
    std::int32_t windowHeight{ 180 };
    bool topmost{ true };

    // Buttons
    std::int32_t okButtonX{ 140 };
    std::int32_t okButtonY{ 130 };
    std::int32_t cancelButtonX{ 250 };
    std::int32_t cancelButtonY{ 130 };

    // Option text area
    std::int32_t fieldY{ 70 };
    std::int32_t gapY{ 30 };

    // SPK
    std::string backSpkName{ "AssembleBoxButton.spk" };
    std::int32_t okButtonFrame{ 0 };
    std::int32_t cancelButtonFrame{ 1 };
};

class RemoveOptionWindow
{
public:
    explicit RemoveOptionWindow() noexcept;

    // State
    void Open(const MItem* pItem, const MItem* pCurItem) noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const settings::RemoveOptionProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Callbacks
    [[nodiscard]] bool HasSelectedOption() const noexcept { return selectedOption_ >= 0; }
    [[nodiscard]] int GetSelectedOption() const noexcept { return selectedOption_; }
    [[nodiscard]] const std::string& GetOptionText(int index) const noexcept;

private:
    // State
    bool isOpen_{ false };
    RemoveOptionStatus status_{ RemoveOptionStatus::SelectOption };
    int selectedOption_{ -1 };
    bool isPressed_{ false };
    bool isBokjory_{ false };

    // Items
    const MItem* mouseItem_{ nullptr };
    const MItem* curItem_{ nullptr };

    // Config
    settings::RemoveOptionProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // Option text (max 2)
    std::array<std::string, 2> optionText_{};

    // Button group
    std::unique_ptr<ButtonGroup> buttonGroup_;

    // SPK archive
    std::unique_ptr<SpriteArchive> backSpk_;

    // Window position
    std::int32_t windowX_{ 0 };
    std::int32_t windowY_{ 0 };
    std::int32_t windowWidth_{ 340 };
    std::int32_t windowHeight_{ 180 };

    // Helpers
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;
    void UpdateOptionText() noexcept;
    [[nodiscard]] bool IsPointInWindow(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInOption(int optionIndex, int x, int y) const noexcept;

    // Callbacks
    void OnOkClicked() noexcept;
    void OnCancelClicked() noexcept;
};

} // namespace phoenix::ui