#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <array>

namespace phoenix::ui
{

// Legal document types
enum class LegalDocumentType : std::uint8_t
{
    EULA = 0,
    PrivacyPolicy = 1,
    TermsOfService = 2,
    Count
};

// LegalDocuments config from interface.ini [LegalDocuments_1280x960]
struct LegalDocumentsProfile
{
    std::int32_t windowX{ 440 };
    std::int32_t windowY{ 330 };
    std::int32_t windowWidth{ 400 };
    std::int32_t windowHeight{ 300 };
    bool topmost{ true };
    bool keyboardControl{ true };

    // Buttons
    std::int32_t okButtonX{ 250 };
    std::int32_t okButtonY{ 240 };
    std::int32_t cancelButtonX{ 325 };
    std::int32_t cancelButtonY{ 240 };

    // Checkbox
    std::int32_t checkBoxX{ 30 };
    std::int32_t checkBoxY{ 204 };

    // Tabs
    std::int32_t tabEULAX{ 30 };
    std::int32_t tabPrivacyX{ 130 };
    std::int32_t tabTermsX{ 230 };

    // SPK
    std::string backSpkName{ "AssembleBox.spk" };
    std::string checkBoxSpkName{ "Option.spk" };
    std::int32_t okButtonFrame{ 0 };
    std::int32_t cancelButtonFrame{ 1 };
    std::int32_t checkBoxFrame{ 0 };
    std::int32_t checkBoxCheckFrame{ 1 };
};

class LegalDocumentsWindow
{
public:
    explicit LegalDocumentsWindow() noexcept;

    // State
    void Open() noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const settings::LegalDocumentsProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // Set document texts (from string table or external files)
    void SetDocumentText(LegalDocumentType type, const std::string& text) noexcept;

    // Callback for when user accepts
    using AcceptCallback = std::function<void(LegalDocumentType acceptedType)>;
    void SetAcceptCallback(AcceptCallback cb) noexcept { acceptCallback_ = std::move(cb); }

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;
    bool HandleChar(wchar_t ch) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Current state
    [[nodiscard]] LegalDocumentType GetCurrentTab() const noexcept { return currentTab_; }
    [[nodiscard]] bool IsCheckBoxChecked() const noexcept { return checkBoxChecked_; }

private:
    // State
    bool isOpen_{ false };
    LegalDocumentType currentTab_{ LegalDocumentType::EULA };
    bool checkBoxChecked_{ false };
    bool isDragging_{ false };
    std::int32_t dragOffsetX_{ 0 };
    std::int32_t dragOffsetY_{ 0 };

    // Config
    settings::LegalDocumentsProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // Document texts
    std::array<std::string, static_cast<size_t>(LegalDocumentType::Count)> documentTexts_;

    // Callback
    std::function<void(LegalDocumentType)> acceptCallback_;

    // UI Elements
    std::unique_ptr<ButtonGroup> buttonGroup_;
    std::unique_ptr<LineEditor> checkBox_;
    std::unique_ptr<SpriteArchive> backSpk_;
    std::unique_ptr<SpriteArchive> checkBoxSpk_;

    // Window position
    std::int32_t windowX_{ 440 };
    std::int32_t windowY_{ 330 };
    std::int32_t windowWidth_{ 400 };
    std::int32_t windowHeight_{ 300 };

    // Helpers
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;
    void InitializeCheckBox() noexcept;
    [[nodiscard]] bool IsPointInWindow(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInTab(int x, int y, LegalDocumentType tab) const noexcept;
    [[nodiscard]] bool IsPointInCheckBox(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInOkButton(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInCancelButton(int x, int y) const noexcept;
    void UpdateDrag(int x, int y) noexcept;
    void EndDrag() noexcept;
    [[nodiscard]] std::string GetCurrentDocumentText() const noexcept;

    // Callbacks
    void OnOkClicked() noexcept;
    void OnCancelClicked() noexcept;
    void OnCheckBoxToggled() noexcept;
    void OnTabClicked(LegalDocumentType tab) noexcept;
};

} // namespace phoenix::ui