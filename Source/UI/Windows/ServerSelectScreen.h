#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace phoenix::ui
{

// Server status enum (matching Legacy STATUS_*)
enum class ServerStatus : std::uint8_t
{
    Open = 0,
    Normal = 1,
    Busy = 2,
    Full = 3,
    Closed = 4,
    VeryBad = 5
};

// Server entry structure
struct ServerEntry
{
    std::string name;
    int id{ -1 };
    ServerStatus status{ ServerStatus::Open };
    bool nonPk{ false };
    int slayerCount{ 0 };
    int vampireCount{ 0 };
    int oustersCount{ 0 };
};

// ServerSelect config from interface.ini [ServerSelect_1280x960]
struct ServerSelectProfile
{
    std::int32_t windowX{ 0 };
    std::int32_t windowY{ 0 };
    std::int32_t windowWidth{ 0 };
    std::int32_t windowHeight{ 0 };
    bool topmost{ true };
    bool keyboardControl{ true };

    // SPK files
    std::string titleSpkName{ "Title.spk" };
    std::string titleHaysSpkName{ "NewTitle.spk" };
    std::string imageHaysSpkName{ "NewServerSelect.spk" };
    std::string imageSpkName{ "ServerSelect.spk" };
    std::string useGradeSpkName{ "UseGrade.spk" };

    // World list
    std::int32_t worldListX{ 0 };
    std::int32_t worldListY{ 0 };
    std::int32_t selectWorldBarIntervalX{ 0 };
    std::int32_t selectWorldBarIntervalY{ 0 };
    std::int32_t selectWorldBarX{ 0 };
    std::int32_t selectWorldBarY{ 0 };
    std::int32_t selectWorldNameFontX{ 0 };
    std::int32_t selectWorldNameFontY{ 0 };
    std::int32_t selectWorldStatusFontX{ 0 };
    std::int32_t selectWorldStatusFontY{ 0 };
    std::int32_t worldListShowMax{ 12 };

    // Channel list
    std::int32_t channelListX{ 0 };
    std::int32_t channelListY{ 0 };
    std::int32_t selectChannelBarIntervalX{ 0 };
    std::int32_t selectChannelBarIntervalY{ 0 };
    std::int32_t selectChannelBarChannelX{ 0 };
    std::int32_t selectChannelBarChannelY{ 0 };
    std::int32_t selectChannelBarStatusX{ 0 };
    std::int32_t selectChannelBarStatusY{ 0 };
    std::int32_t selectChannelBarRaceInfoX{ 0 };
    std::int32_t selectChannelBarRaceInfoY{ 0 };
    std::int32_t selectChannelNameFontX{ 0 };
    std::int32_t selectChannelNameFontY{ 0 };
    std::int32_t selectChannelStatusFontX{ 0 };
    std::int32_t selectChannelStatusFontY{ 0 };
    std::int32_t channelListShowMax{ 12 };

    // Character bars
    std::int32_t charBarStartViewX{ 0 };
    std::int32_t charBarStartViewY{ 0 };
    std::int32_t charBarIntervalX{ 0 };
    std::int32_t charBarIntervalY{ 0 };

    // Buttons
    std::int32_t prevButtonX{ 0 };
    std::int32_t prevButtonY{ 0 };
    std::int32_t nextButtonX{ 0 };
    std::int32_t nextButtonY{ 0 };

    // Rating icons
    std::int32_t childX{ 0 };
    std::int32_t childY{ 0 };
    std::int32_t adultX{ 0 };
    std::int32_t adultY{ 0 };
    std::int32_t violenceX{ 0 };
    std::int32_t violenceY{ 0 };
    std::int32_t fearX{ 0 };
    std::int32_t fearY{ 0 };

    // Text bar
    std::int32_t textBarX{ 0 };
    std::int32_t textBarY{ 0 };

    // Animation mark
    std::int32_t aniMarkX{ 0 };
    std::int32_t aniMarkY{ 0 };

    // View race info
    std::int32_t viewRaceInfoImageX{ 0 };
    std::int32_t viewRaceInfoImageY{ 0 };

    // Text lines
    std::int32_t textLine1X{ 0 };
    std::int32_t textLine1Y{ 0 };
    std::int32_t textLine2X{ 0 };
    std::int32_t textLine2Y{ 0 };

    // Premium text
    std::int32_t premiumUseDayTextX{ 0 };
    std::int32_t premiumUseDayTextY{ 0 };

    // SPK frames
    std::int32_t prevScreenButtonFrame{ 0 };
    std::int32_t nextScreenButtonFrame{ 1 };
    std::int32_t scrollUpFrame{ 2 };
    std::int32_t scrollDownFrame{ 3 };
    std::int32_t blackBarFrame{ 4 };
    std::int32_t slayerBarFrame{ 5 };
    std::int32_t vampiresBarFrame{ 6 };
    std::int32_t oustersBarFrame{ 7 };
    std::int32_t slayerBoxFrame{ 8 };
    std::int32_t vampiresBoxFrame{ 9 };
    std::int32_t oustersBoxFrame{ 10 };
    std::int32_t chenelSelectBarChanelFrame{ 11 };
    std::int32_t chenelSelectBarStatusFrame{ 12 };
    std::int32_t chenelSelectBarRaceInfoFrame{ 13 };
    std::int32_t viewRaceInfoFrame{ 14 };
    std::int32_t textBarFrame{ 15 };
    std::int32_t aniMarkFrame{ 16 };
    std::int32_t childFrame{ 17 };
    std::int32_t adultFrame{ 18 };
    std::int32_t violenceFrame{ 19 };
    std::int32_t fearFrame{ 20 };
};

class ServerSelectScreen
{
public:
    explicit ServerSelectScreen() noexcept;

    // State
    void Open() noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const settings::ServerSelectProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // Server data
    void SetServerList(bool isWorldMode, const std::vector<ServerEntry>& servers, int defaultId = -1) noexcept;

    // Callbacks
    using SelectCallback = std::function<void(bool isWorld, int serverId)>;
    using BackCallback = std::function<void()>;

    void SetSelectCallback(SelectCallback cb) noexcept { selectCallback_ = std::move(cb); }
    void SetBackCallback(BackCallback cb) noexcept { backCallback_ = std::move(cb); }

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleMouseUp(int x, int y, bool rightButton) noexcept;
    bool HandleMouseMove(int x, int y) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

private:
    // State
    bool isOpen_{ false };
    bool isWorldMode_{ true };  // true = world select, false = channel select
    int focusIndex_{ -1 };
    int selectedIndex_{ -1 };
    int scrollOffset_{ 0 };
    int animationAlpha_{ 31 };
    bool animationUp_{ false };
    bool isScrolling_{ false };
    int scrollStartY_{ 0 };

    // Server data
    std::vector<ServerEntry> worldServers_;
    std::vector<ServerEntry> channelServers_;

    // Config
    settings::ServerSelectProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // UI Elements
    std::unique_ptr<ButtonGroup> buttonGroup_;
    std::unique_ptr<SpriteArchive> titleSpk_;
    std::unique_ptr<SpriteArchive> titleHaysSpk_;
    std::unique_ptr<SpriteArchive> imageHaysSpk_;
    std::unique_ptr<SpriteArchive> imageSpk_;
    std::unique_ptr<SpriteArchive> useGradeSpk_;

    // Window position
    std::int32_t windowX_{ 0 };
    std::int32_t windowY_{ 0 };
    std::int32_t windowWidth_{ 0 };
    std::int32_t windowHeight_{ 0 };

    // Callbacks
    SelectCallback selectCallback_;
    BackCallback backCallback_;

    // Helpers
    void InitializeSPKs() noexcept;
    void InitializeButtons() noexcept;
    void UpdateResolution() noexcept;
    void PositionButtons() noexcept;
    [[nodiscard]] bool IsPointInWindow(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInWorldList(int x, int y, int& outIndex) const noexcept;
    [[nodiscard]] bool IsPointInChannelList(int x, int y, int& outIndex) const noexcept;
    [[nodiscard]] bool IsPointInScrollBar(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInPrevButton(int x, int y) const noexcept;
    [[nodiscard]] bool IsPointInNextButton(int x, int y) const noexcept;
    void UpdateAnimation(float deltaTime) noexcept;
    void UpdateScroll(int y) noexcept;
    void EndScroll() noexcept;
    void SelectWorld(int index) noexcept;
    void SelectChannel(int index) noexcept;
    void GoBack() noexcept;
    void GoNext() noexcept;
    void UpdateScrollOffset() noexcept;

    // Render helpers
    void RenderBackground() noexcept;
    void RenderWorldList() noexcept;
    void RenderChannelList() noexcept;
    void RenderCharBars() noexcept;
    void RenderPopulationBars(int channelIndex, int baseY) noexcept;
    void RenderButtons() noexcept;
    void RenderTextLines() noexcept;
    void RenderPremiumText() noexcept;
    void RenderRatingIcons() noexcept;
    void RenderAnimationMark() noexcept;
    void RenderScrollBar() noexcept;
};

} // namespace phoenix::ui