#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "Protocol/LegacyProtocol.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace phoenix::ui
{

// BulletinBoard config from interface.ini [BulletinBoard_1280x960]
struct BulletinBoardProfile
{
    std::int32_t windowX{ 0 };
    std::int32_t windowY{ 0 };
    std::int32_t windowWidth{ 400 };
    std::int32_t windowHeight{ 200 };
    bool topmost{ true };
    bool keyboardControl{ true };
    bool draggable{ true };

    // Buttons
    std::int32_t okButtonX{ 169 };
    std::int32_t okButtonY{ 116 };
    std::int32_t cancelButtonX{ 181 };
    std::int32_t cancelButtonY{ 116 };

    // Text fields
    std::int32_t fromX{ 111 };
    std::int32_t fromY{ 88 };
    std::int32_t msgX{ 47 };
    std::int32_t msgY{ 28 };
    std::int32_t msgWidth{ 115 };
    std::int32_t msgHeight{ 80 };
    std::int32_t msgByteLimit{ 50 };
    std::int32_t fromByteLimit{ 14 };

    // SPK
    std::string backSpkName{ "BulletinBoard.spk" };
    std::int32_t mainWindowFrame{ 0 };
    std::int32_t buttonOkFrame{ 0 };
    std::int32_t buttonCancelFrame{ 1 };
};

class BulletinBoardWindow
{
public:
    explicit BulletinBoardWindow() noexcept;

    // State
    void Open() noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const settings::BulletinBoardProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // Set the item that triggered this window
    void SetItem(std::uint32_t itemID) noexcept;
    [[nodiscard]] std::uint32_t GetItemID() const noexcept { return itemID_; }

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;
    bool HandleChar(wchar_t ch) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Callbacks
    void OnOkClicked() noexcept;
    void OnCancelClicked() noexcept;

private:
    // State
    bool isOpen_{ false };
    bool draggable_{ true };
    bool isDragging_{ false };
    std::int32_t dragOffsetX_{ 0 };
    std::int32_t dragOffsetY_{ 0 };

    // Config
    settings::BulletinBoardProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // Item reference
    std::uint32_t itemID_{ 0 };

    // Text fields
    std::unique_ptr<LineEditor> msgEdit_;
    std::unique_ptr<LineEditor> fromEdit_;

    // Button group
    std::unique_ptr<ButtonGroup> buttonGroup_;

    // SPK archive
    std::unique_ptr<SpriteArchive> backSpk_;

    // Window position
    std::int32_t windowX_{ 0 };
    std::int32_t windowY_{ 0 };
    std::int32_t windowWidth_{ 400 };
    std::int32_t windowHeight_{ 200 };

    // Helpers
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;
    void PositionEdits() noexcept;
    [[nodiscard]] bool IsPointInWindow(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInMsgEdit(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInFromEdit(int x, int y) const noexcept;
    void UpdateDrag(int x, int y) noexcept;
    void EndDrag() noexcept;
};

} // namespace phoenix::ui