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

// OustersSkillInfo config from interface.ini [OustersSkillInfo_1280x960]
struct OustersSkillInfoProfile
{
    std::int32_t windowX{ 100 };
    std::int32_t windowY{ 100 };
    std::int32_t windowWidth{ 300 };
    std::int32_t windowHeight{ 350 };
    bool topmost{ true };

    // Buttons
    std::int32_t closeButtonX{ 255 };
    std::int32_t closeButtonY{ 310 };
    std::int32_t helpButtonX{ 230 };
    std::int32_t helpButtonY{ 310 };
    std::int32_t alphaButtonX{ 30 };
    std::int32_t alphaButtonY{ 310 };
    std::int32_t learnButtonX{ 160 };
    std::int32_t learnButtonY{ 307 };

    // Scroll bar
    std::int32_t scrollBarX{ 240 };
    std::int32_t scrollBarY{ 200 };
    std::int32_t scrollBarWidth{ 20 };
    std::int32_t scrollBarHeight{ 70 };

    // Text positions
    std::int32_t nameX{ 30 };
    std::int32_t nameY{ 35 };
    std::int32_t descX{ 40 };
    std::int32_t descY{ 140 };

    // SPK
    std::string backSpkName{ "Info.spk" };
    std::string assembleBoxButtonSpkName{ "AssembleBoxButton.spk" };

    // Frames
    std::int32_t closeButtonFrame{ 0 };
    std::int32_t helpButtonFrame{ 1 };
    std::int32_t alphaButtonFrame{ 2 };
    std::int32_t learnButtonFrame{ 3 };
    std::int32_t oustersSkillBackFrame{ 0 };
    std::int32_t oustersSkillBack2Frame{ 1 };
    std::int32_t oustersDownSkillButtonFrame{ 4 };
    std::int32_t oustersPlusButtonFrame{ 5 };
};

class OustersSkillInfoWindow
{
public:
    explicit OustersSkillInfoWindow() noexcept;

    // State
    void Open(std::uint32_t skillID, std::int32_t windowX, std::int32_t windowY, bool downSkill) noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const settings::OustersSkillInfoProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // Skill info
    void SetSkillID(std::uint32_t skillID) noexcept { skillID_ = skillID; }
    void SetDownSkill(bool downSkill) noexcept { downSkill_ = downSkill; }

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleMouseWheel(int delta) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

    // Callbacks
    [[nodiscard]] std::uint32_t GetSkillID() const noexcept { return skillID_; }

private:
    // State
    bool isOpen_{ false };
    std::uint32_t skillID_{ 0 };
    bool downSkill_{ false };

    // Config
    settings::OustersSkillInfoProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // UI Elements
    std::unique_ptr<ButtonGroup> buttonGroup_;
    std::unique_ptr<ScrollBar> scrollBar_;
    std::unique_ptr<SpriteArchive> backSpk_;
    std::unique_ptr<SpriteArchive> assembleBoxButtonSpk_;

    // Window position
    std::int32_t windowX_{ 100 };
    std::int32_t windowY_{ 100 };
    std::int32_t windowWidth_{ 300 };
    std::int32_t windowHeight_{ 350 };

    // Drag state
    bool isDragging_{ false };
    std::int32_t dragOffsetX_{ 0 };
    std::int32_t dragOffsetY_{ 0 };

    // Scroll state
    std::int32_t descYDistance_{ 5 };

    // Helpers
    void InitializeButtons() noexcept;
    void PositionButtons() noexcept;
    void InitializeScrollBar() noexcept;
    [[nodiscard]] bool IsPointInWindow(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInTitleBar(int x, int y) const noexcept;
    void UpdateDrag(int x, int y) noexcept;
    void EndDrag() noexcept;

    // Description rendering
    void ShowDesc(int x, int y) noexcept;
    void SetDescScrollPos(int pos) noexcept;

    // Skill info getters (to be connected to protocol)
    [[nodiscard]] bool IsSkillLearned() const noexcept;
    [[nodiscard]] bool CanLearnSkill() const noexcept;
    [[nodiscard]] bool CanDownSkill() const noexcept;
    [[nodiscard]] std::string GetSkillName() const noexcept;
    [[nodiscard]] std::string GetSkillDescription() const noexcept;
    [[nodiscard]] int GetSkillLevel() const noexcept;
    [[nodiscard]] int GetRequiredSkillPoints() const noexcept;
    [[nodiscard]] bool IsPassive() const noexcept;
    [[nodiscard]] bool IsDownSkill() const noexcept { return downSkill_; }

    // Callbacks
    void OnCloseClicked() noexcept;
    void OnHelpClicked() noexcept;
    void OnAlphaClicked() noexcept;
    void OnLearnClicked() noexcept;
};

} // namespace phoenix::ui