#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <functional>

namespace phoenix::ui
{

// CharacterDelete config from interface.ini [CharacterDelete_1280x960]
struct CharacterDeleteProfile
{
    // Window
    std::int32_t windowX{ 0 };
    std::int32_t windowY{ 0 };
    std::int32_t windowWidth{ 324 };
    std::int32_t windowHeight{ 207 };
    bool topmost{ true };
    bool keyboardControl{ true };

    // SPK files
    std::string assembleBoxButtonSpkName{ "AssembleBoxButton.spk" };

    // Buttons
    std::int32_t okButtonX{ 163 };
    std::int32_t okButtonY{ 147 };
    std::int32_t cancelButtonX{ 229 };
    std::int32_t cancelButtonY{ 147 };
    std::int32_t okButtonFrame{ 0 };
    std::int32_t cancelButtonFrame{ 1 };

    // SSN fields
    std::int32_t ssnPart1X{ 55 };
    std::int32_t ssnPart1Y{ 108 };
    std::int32_t ssnPart1ByteLimit{ 7 };
    std::int32_t ssnPart2X{ 175 };
    std::int32_t ssnPart2Y{ 108 };
    std::int32_t ssnPart2ByteLimit{ 8 };

    // Colors
    std::uint8_t boxRgbR{ 0 };
    std::uint8_t boxRgbG{ 0 };
    std::uint8_t boxRgbB{ 0 };
    std::uint8_t boxRgbAlpha{ 0 };
    std::uint8_t lineRgbR{ 0 };
    std::uint8_t lineRgbG{ 0 };
    std::uint8_t lineRgbB{ 0 };

    // Text
    std::string titleString;
    std::string messageString;
    std::string okButtonString{ "Delete" };
    std::string cancelButtonString{ "Cancel" };
};

class CharacterDeleteScreen
{
public:
    CharacterDeleteScreen() noexcept;
    ~CharacterDeleteScreen() = default;

    // Lifecycle
    void Open(int slotIndex) noexcept;
    void Close() noexcept;
    void Toggle() noexcept;

    // Config
    void LoadFromConfig(const settings::CharacterDeleteProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // State
    bool IsOpen() const noexcept { return isOpen_; }
    int GetSlotIndex() const noexcept { return slotIndex_; }

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
    using ConfirmCallback = std::function<void(int slotIndex, const std::string& ssnPart1, const std::string& ssnPart2)>;
    using CancelCallback = std::function<void()>;
    void SetConfirmCallback(ConfirmCallback cb) noexcept { confirmCallback_ = std::move(cb); }
    void SetCancelCallback(CancelCallback cb) noexcept { cancelCallback_ = std::move(cb); }

private:
    // SPK loading
    void InitializeSPKs() noexcept;
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;

    // Input helpers
    bool IsPointInWindow(int x, int y) const noexcept;
    bool IsPointInSsnPart1(int x, int y) const noexcept;
    bool IsPointInSsnPart2(int x, int y) const noexcept;

    // Callbacks
    void OnOkClicked() noexcept;
    void OnCancelClicked() noexcept;

    // State
    bool isOpen_{ false };
    int slotIndex_{-1};
    settings::CharacterDeleteProfile config_;

    // SPK archives
    std::unique_ptr<SpriteArchive> assembleBoxButtonSpk_;

    // UI components
    std::unique_ptr<ButtonGroup> buttonGroup_;
    std::unique_ptr<LineEditor> ssnPart1Editor_;
    std::unique_ptr<LineEditor> ssnPart2Editor_;
    bool netmarbleMode_{ false };

    // Callbacks
    ConfirmCallback confirmCallback_;
    CancelCallback cancelCallback_;
};

} // namespace phoenix::ui