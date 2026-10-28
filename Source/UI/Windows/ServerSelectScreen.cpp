#include "UI/Windows/ServerSelectScreen.h"

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

ServerSelectScreen::ServerSelectScreen() noexcept
    : isOpen_(false)
    , isWorldMode_(true)
    , focusIndex_(-1)
    , selectedIndex_(-1)
    , scrollOffset_(0)
    , animationAlpha_(31)
    , animationUp_(false)
    , isScrolling_(false)
    , scrollStartY_(0)
    , windowX_(0)
    , windowY_(0)
    , windowWidth_(0)
    , windowHeight_(0)
{
}

void ServerSelectScreen::Open() noexcept
{
    isOpen_ = true;
    isWorldMode_ = true;
    focusIndex_ = -1;
    selectedIndex_ = -1;
    scrollOffset_ = 0;
    animationAlpha_ = config_.aniArphaStart;
    animationUp_ = false;

    InitializeSPKs();
    UpdateResolution();
    InitializeButtons();
    PositionButtons();
}

void ServerSelectScreen::Close() noexcept
{
    isOpen_ = false;
    titleSpk_.reset();
    titleHaysSpk_.reset();
    imageHaysSpk_.reset();
    imageSpk_.reset();
    useGradeSpk_.reset();
    buttonGroup_.reset();
}

void ServerSelectScreen::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open();
}

void ServerSelectScreen::LoadFromConfig(const settings::ServerSelectProfile& profile) noexcept
{
    config_ = profile;
    windowX_ = config_.windowX;
    windowY_ = config_.windowY;
    windowWidth_ = config_.windowWidth;
    windowHeight_ = config_.windowHeight;
    UpdateResolution();
}

void ServerSelectScreen::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

void ServerSelectScreen::SetServerList(bool isWorldMode, const std::vector<ServerEntry>& servers, int defaultId) noexcept
{
    if (isWorldMode)
    {
        worldServers_ = servers;
        if (defaultId >= 0 && defaultId < static_cast<int>(worldServers_.size()))
        {
            selectedIndex_ = defaultId;
            focusIndex_ = defaultId;
        }
        else if (!worldServers_.empty())
        {
            selectedIndex_ = 0;
            focusIndex_ = 0;
        }
        else
        {
            selectedIndex_ = -1;
            focusIndex_ = -1;
        }
    }
    else
    {
        channelServers_ = servers;
        if (defaultId >= 0 && defaultId < static_cast<int>(channelServers_.size()))
        {
            selectedIndex_ = defaultId;
            focusIndex_ = defaultId;
        }
        else if (!channelServers_.empty())
        {
            selectedIndex_ = 0;
            focusIndex_ = 0;
        }
        else
        {
            selectedIndex_ = -1;
            focusIndex_ = -1;
        }
    }
}

void ServerSelectScreen::InitializeSPKs() noexcept
{
    titleSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.titleSpkName.empty())
        titleSpk_->Open(config_.titleSpkName.c_str());

    titleHaysSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.titleHaysSpkName.empty())
        titleHaysSpk_->Open(config_.titleHaysSpkName.c_str());

    imageHaysSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.imageHaysSpkName.empty())
        imageHaysSpk_->Open(config_.imageHaysSpkName.c_str());

    imageSpk_ = std::make_unique<SpriteArchive>();
    if (!config_.imageSpkName.empty())
        imageSpk_->Open(config_.imageSpkName.c_str());
}

void ServerSelectScreen::InitializeButtons() noexcept
{
    buttonGroup_ = std::make_unique<ButtonGroup>();

    // Previous/Back button
    buttonGroup_->AddButton(static_cast<int>(0), // BACK_ID
        config_.prevButtonX, config_.prevButtonY,
        imageSpk_ ? imageSpk_->GetWidth(config_.prevButtonFrame) : 16,
        imageSpk_ ? imageSpk_->GetHeight(config_.prevButtonFrame) : 16,
        [this]() { GoBack(); });

    // Next/Connect button
    buttonGroup_->AddButton(static_cast<int>(1), // NEXT_ID
        config_.nextButtonX, config_.nextButtonY,
        imageSpk_ ? imageSpk_->GetWidth(config_.nextButtonFrame) : 16,
        imageSpk_ ? imageSpk_->GetHeight(config_.nextButtonFrame) : 16,
        [this]() { GoNext(); });
}

void ServerSelectScreen::UpdateResolution() noexcept
{
    // Server select covers full screen
    windowWidth_ = 1280;
    windowHeight_ = 960;
    windowX_ = 0;
    windowY_ = 0;
}

void ServerSelectScreen::PositionButtons() noexcept
{
    if (!buttonGroup_)
        return;

    buttonGroup_->SetPosition(0, config_.prevButtonX, config_.prevButtonY);
    buttonGroup_->SetPosition(1, config_.nextButtonX, config_.nextButtonY);
}

bool ServerSelectScreen::IsPointInWindow(int x, int y) const noexcept
{
    return x >= windowX_ && x < windowX_ + windowWidth_ &&
           y >= windowY_ && y < windowY_ + windowHeight_;
}

bool ServerSelectScreen::IsPointInWorldList(int x, int y, int& outIndex) const noexcept
{
    int startX = config_.selectWorldBarX;
    int startY = config_.selectWorldBarY;
    int intervalX = config_.selectWorldBarIntervalX;
    int intervalY = config_.selectWorldBarIntervalY;
    int showMax = config_.worldListShowMax;

    for (int i = 0; i < showMax; ++i)
    {
        int itemY = startY + i * intervalY;
        if (x >= startX && x < startX + intervalX &&
            y >= itemY && y < itemY + intervalY)
        {
            outIndex = i + scrollOffset_;
            return true;
        }
    }
    return false;
}

bool ServerSelectScreen::IsPointInChannelList(int x, int y, int& outIndex) const noexcept
{
    int startX = config_.selectChannelBarChannelX;
    int startY = config_.selectChannelBarChannelY;
    int intervalX = config_.selectChannelBarIntervalX;
    int intervalY = config_.selectChannelBarIntervalY;
    int showMax = config_.channelListShowMax;

    for (int i = 0; i < showMax; ++i)
    {
        int itemY = startY + i * intervalY;
        if (x >= startX && x < startX + intervalX &&
            y >= itemY && y < itemY + intervalY)
        {
            outIndex = i + scrollOffset_;
            return true;
        }
    }
    return false;
}

bool ServerSelectScreen::IsPointInScrollBar(int x, int y) const noexcept
{
    // TODO: Implement scroll bar hit test
    return false;
}

bool ServerSelectScreen::IsPointInPrevButton(int x, int y) const noexcept
{
    if (!imageSpk_)
        return false;

    int btnX = config_.prevButtonX;
    int btnY = config_.prevButtonY;
    int btnW = imageSpk_->GetWidth(config_.prevButtonFrame);
    int btnH = imageSpk_->GetHeight(config_.prevButtonFrame);

    return x >= btnX && x < btnX + btnW &&
           y >= btnY && y < btnY + btnH;
}

bool ServerSelectScreen::IsPointInNextButton(int x, int y) const noexcept
{
    if (!imageSpk_)
        return false;

    int btnX = config_.nextButtonX;
    int btnY = config_.nextButtonY;
    int btnW = imageSpk_->GetWidth(config_.nextButtonFrame);
    int btnH = imageSpk_->GetHeight(config_.nextButtonFrame);

    return x >= btnX && x < btnX + btnW &&
           y >= btnY && y < btnY + btnH;
}

void ServerSelectScreen::UpdateAnimation(float deltaTime) noexcept
{
    // Legacy: 50ms timer, alpha 1-31 ping-pong
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

void ServerSelectScreen::UpdateScroll(int y) noexcept
{
    if (isScrolling_)
    {
        // Legacy scroll calculation
        int scrollHeight = 200; // SCROLL_HEIGHT
        int scrollY = 100;      // SCROLL_Y
        int channelCount = static_cast<int>(channelServers_.size());

        if (channelCount > 12)
        {
            int scrollPos = (channelCount - 12) * 
                (std::min(scrollHeight, std::max(0, y - scrollY)) + scrollHeight / (channelCount - 12) / 2) / scrollHeight;
            scrollOffset_ = std::clamp(scrollPos, 0, channelCount - 12);
        }
    }
}

void ServerSelectScreen::EndScroll() noexcept
{
    isScrolling_ = false;
}

void ServerSelectScreen::SelectWorld(int index) noexcept
{
    if (index >= 0 && index < static_cast<int>(worldServers_.size()))
    {
        selectedIndex_ = index;
        focusIndex_ = index;
        isWorldMode_ = false; // Switch to channel select
        scrollOffset_ = 0;
    }
}

void ServerSelectScreen::SelectChannel(int index) noexcept
{
    if (index >= 0 && index < static_cast<int>(channelServers_.size()))
    {
        selectedIndex_ = index;
        focusIndex_ = index;
    }
}

void ServerSelectScreen::GoBack() noexcept
{
    if (isWorldMode_)
    {
        if (backCallback_)
            backCallback_();
    }
    else
    {
        isWorldMode_ = true;
        scrollOffset_ = 0;
    }
    Close();
}

void ServerSelectScreen::GoNext() noexcept
{
    if (isWorldMode_)
    {
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(worldServers_.size()))
        {
            if (worldServers_[selectedIndex_].status != ServerStatus::Closed)
            {
                if (selectCallback_)
                    selectCallback_(true, worldServers_[selectedIndex_].id);
            }
        }
    }
    else
    {
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(channelServers_.size()))
        {
            if (channelServers_[selectedIndex_].status != ServerStatus::VeryBad)
            {
                if (selectCallback_)
                    selectCallback_(false, channelServers_[selectedIndex_].id);
            }
        }
    }
    Close();
}

void ServerSelectScreen::UpdateScrollOffset() noexcept
{
    // Keep selected index visible
    if (focusIndex_ < scrollOffset_)
        scrollOffset_ = focusIndex_;
    else if (focusIndex_ >= scrollOffset_ + config_.worldListShowMax)
        scrollOffset_ = focusIndex_ - config_.worldListShowMax + 1;

    scrollOffset_ = std::max(0, std::min(scrollOffset_, 
        static_cast<int>(isWorldMode_ ? worldServers_.size() : channelServers_.size()) - config_.worldListShowMax));
}

bool ServerSelectScreen::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    // Check world list
    int index = -1;
    if (isWorldMode_ && IsPointInWorldList(x, y, index))
    {
        focusIndex_ = index;
        selectedIndex_ = index;
        return true;
    }

    // Check channel list
    if (!isWorldMode_ && IsPointInChannelList(x, y, index))
    {
        focusIndex_ = index;
        selectedIndex_ = index;
        return true;
    }

    // Check scroll bar
    if (IsPointInScrollBar(x, y))
    {
        isScrolling_ = true;
        scrollStartY_ = y;
        return true;
    }

    // Check prev button
    if (IsPointInPrevButton(x, y))
    {
        GoBack();
        return true;
    }

    // Check next button
    if (IsPointInNextButton(x, y))
    {
        GoNext();
        return true;
    }

    // Check button group
    if (buttonGroup_)
    {
        if (buttonGroup_->HandleMouseDown(x, y))
            return true;
    }

    return false;
}

bool ServerSelectScreen::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    EndScroll();

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseUp(x, y);
    }

    return false;
}

bool ServerSelectScreen::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    if (isScrolling_)
    {
        UpdateScroll(y);
        return true;
    }

    if (buttonGroup_)
    {
        buttonGroup_->HandleMouseMove(x, y);
    }

    return false;
}

bool ServerSelectScreen::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    if (key == VK_ESCAPE)
    {
        GoBack();
        return true;
    }
    if (key == VK_RETURN)
    {
        GoNext();
        return true;
    }

    if (isWorldMode_)
    {
        switch (key)
        {
            case VK_UP:
                if (focusIndex_ > 0)
                {
                    focusIndex_--;
                    UpdateScrollOffset();
                }
                return true;
            case VK_DOWN:
                if (focusIndex_ < static_cast<int>(worldServers_.size()) - 1)
                {
                    focusIndex_++;
                    UpdateScrollOffset();
                }
                return true;
            case VK_LEFT:
                // Stay in world mode
                return true;
            case VK_RIGHT:
                // Switch to channel mode
                if (!worldServers_.empty() && selectedIndex_ >= 0)
                {
                    isWorldMode_ = false;
                    scrollOffset_ = 0;
                    focusIndex_ = 0;
                }
                return true;
        }
    }
    else
    {
        switch (key)
        {
            case VK_UP:
                if (focusIndex_ > 0)
                {
                    focusIndex_--;
                    UpdateScrollOffset();
                }
                return true;
            case VK_DOWN:
                if (focusIndex_ < static_cast<int>(channelServers_.size()) - 1)
                {
                    focusIndex_++;
                    UpdateScrollOffset();
                }
                return true;
            case VK_LEFT:
                isWorldMode_ = true;
                scrollOffset_ = 0;
                focusIndex_ = selectedIndex_;
                return true;
            case VK_RIGHT:
                // Stay in channel mode
                return true;
        }
    }

    return false;
}

void ServerSelectScreen::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    UpdateAnimation(deltaTime);

    if (buttonGroup_)
    {
        buttonGroup_->Update(deltaTime);
    }
}

void ServerSelectScreen::Render() noexcept
{
    if (!isOpen_)
        return;

    RenderBackground();

    if (isWorldMode_)
        RenderWorldList();
    else
        RenderChannelList();

    RenderCharBars();
    RenderPopulationBars(-1, 0); // Placeholder
    RenderButtons();
    RenderTextLines();
    RenderPremiumText();
    RenderRatingIcons();
    RenderAnimationMark();
    RenderScrollBar();
}

void ServerSelectScreen::RenderBackground() noexcept
{
    if (!titleSpk_ || !titleHaysSpk_)
        return;

    // Legacy: resolution-dependent background
    if (windowWidth_ > 800) // High resolution
    {
        if (windowWidth_ <= 1024)
        {
            titleHaysSpk_->Draw(
                (windowWidth_ - titleHaysSpk_->GetWidth(config_.titleFrame1024Low)) / 2,
                (windowHeight_ - titleHaysSpk_->GetHeight(config_.titleFrame1024Low)) / 2,
                config_.titleFrame1024Low);
        }
        else
        {
            if (windowHeight_ < 960)
            {
                titleHaysSpk_->Draw(
                    (windowWidth_ - titleHaysSpk_->GetWidth(config_.titleFrame1024Low)) / 2,
                    (windowHeight_ - titleHaysSpk_->GetHeight(config_.titleFrame1024Low)) / 2,
                    config_.titleFrame1024Low);
            }
            else if (windowHeight_ < 1024)
            {
                titleHaysSpk_->Draw(
                    (windowWidth_ - titleHaysSpk_->GetWidth(config_.titleFrame1024Mid)) / 2,
                    (windowHeight_ - titleHaysSpk_->GetHeight(config_.titleFrame1024Mid)) / 2,
                    config_.titleFrame1024Mid);
            }
            else
            {
                titleHaysSpk_->Draw(
                    (windowWidth_ - titleHaysSpk_->GetWidth(config_.titleFrame1024High)) / 2,
                    (windowHeight_ - titleHaysSpk_->GetHeight(config_.titleFrame1024High)) / 2,
                    config_.titleFrame1024High);
            }
        }
    }
    else
    {
        titleHaysSpk_->Draw(
            (windowWidth_ - titleHaysSpk_->GetWidth(config_.titleFrameNormal)) / 2,
            (windowHeight_ - titleHaysSpk_->GetHeight(config_.titleFrameNormal)) / 2,
            config_.titleFrameNormal);
    }

    // World and Channel titles
    if (imageHaysSpk_)
    {
        imageHaysSpk_->Draw(config_.worldListX, config_.worldListY, config_.worldTitleFrame);
        imageHaysSpk_->Draw(config_.channelListX, config_.channelListY, config_.serverTitleFrame);
    }

    if (buttonGroup_)
        buttonGroup_->Render();
}

void ServerSelectScreen::RenderWorldList() noexcept
{
    // Legacy ServerNameViewWorld logic
    if (!imageHaysSpk_)
        return;

    int showMax = config_.worldListShowMax;
    int baseFontX = config_.selectWorldNameFontX;
    int baseFontY = config_.selectWorldNameFontY;
    int baseStatusX = config_.selectWorldStatusFontX;
    int baseStatusY = config_.selectWorldStatusFontY;
    int intervalY = config_.selectWorldBarIntervalY;

    for (int i = 0; i < showMax && (i + scrollOffset_) < static_cast<int>(worldServers_.size()); ++i)
    {
        int idx = i + scrollOffset_;
        const auto& server = worldServers_[idx];

        // Status string
        std::string statusStr;
        switch (server.status)
        {
            case ServerStatus::Open: statusStr = "Open"; break;
            case ServerStatus::Closed: statusStr = "Closed"; break;
            default: statusStr = "Unknown"; break;
        }

        // Render name and status (placeholder - actual rendering uses DirectWrite)
        // g_PrintColorStr(...)

        // Selection highlight
        if (idx == focusIndex_ && isWorldMode_)
        {
            imageHaysSpk_->Draw(config_.selectWorldBarX,
                config_.selectWorldBarY + i * intervalY,
                config_.worldSelectBarFrame);
        }
        else if (idx == selectedIndex_)
        {
            // Clicked index highlight
            // g_PrintColorStr(..., RGB(128,255,0));
        }
    }
}

void ServerSelectScreen::RenderChannelList() noexcept
{
    // Legacy ServerNameView logic
    if (!imageHaysSpk_)
        return;

    int showMax = config_.channelListShowMax;
    int baseFontX = config_.selectChannelNameFontX;
    int baseFontY = config_.selectChannelNameFontY;
    int baseStatusX = config_.selectChannelStatusFontX;
    int baseStatusY = config_.selectChannelStatusFontY;
    int intervalY = config_.selectChannelBarIntervalY;

    for (int i = 0; i < showMax && (i + scrollOffset_) < static_cast<int>(channelServers_.size()); ++i)
    {
        int idx = i + scrollOffset_;
        const auto& server = channelServers_[idx];

        // Status string
        std::string statusStr;
        switch (server.status)
        {
            case ServerStatus::Open: statusStr = "Very Good"; break;
            case ServerStatus::Normal: statusStr = "Good"; break;
            case ServerStatus::Busy: statusStr = "Normal"; break;
            case ServerStatus::Full: statusStr = "Bad"; break;
            case ServerStatus::VeryBad: statusStr = "Very Bad"; break;
            case ServerStatus::Closed: statusStr = "Closed"; break;
        }

        // Render name and status (placeholder)

        // Selection highlight
        if (idx == focusIndex_ && !isWorldMode_)
        {
            // Channel select bar (3 parts: channel, status, raceinfo)
            imageHaysSpk_->Draw(config_.selectChannelBarChannelX,
                config_.selectChannelBarChannelY + i * intervalY,
                config_.channelSelectBarChannelFrame);
            imageHaysSpk_->Draw(config_.selectChannelBarStatusX,
                config_.selectChannelBarStatusY + i * intervalY,
                config_.channelSelectBarStatusFrame);
            imageHaysSpk_->Draw(config_.selectChannelBarRaceInfoX,
                config_.selectChannelBarRaceInfoY + i * intervalY,
                config_.channelSelectBarRaceInfoFrame);
        }
    }
}

void ServerSelectScreen::RenderCharBars() noexcept
{
    // Legacy CharBoxView - render Slayer/Vampire/Ousters boxes
    if (!imageSpk_)
        return;

    // if (!isWorldMode_) // Only in channel mode
    // {
    //     imageSpk_->Draw(config_.charBarStartViewX, config_.charBarStartViewY, config_.slayerBoxFrame);
    //     imageSpk_->Draw(config_.charBarStartViewX, config_.charBarStartViewY + config_.charBarIntervalY, config_.vampiresBoxFrame);
    //     imageSpk_->Draw(config_.charBarStartViewX, config_.charBarStartViewY + config_.charBarIntervalY * 2, config_.oustersBoxFrame);
    // }
}

void ServerSelectScreen::RenderPopulationBars(int channelIndex, int baseY) noexcept
{
    // Legacy ChannelCharBarView / CharBarView
    // Render 10 black bars + colored bars for each race
    if (!imageHaysSpk_)
        return;

    if (channelIndex < 0 || channelIndex >= static_cast<int>(channelServers_.size()))
        return;

    // Black bars (10 each for Slayer, Vampire, Ousters)
    // Colored bars based on population
}

void ServerSelectScreen::RenderButtons() noexcept
{
    if (buttonGroup_)
        buttonGroup_->Render();
}

void ServerSelectScreen::RenderTextLines() noexcept
{
    // Legacy: g_PrintColorStr for text lines
}

void ServerSelectScreen::RenderPremiumText() noexcept
{
    // Legacy: Premium days text
}

void ServerSelectScreen::RenderRatingIcons() noexcept
{
    if (!imageHaysSpk_)
        return;

    // Child/Adult/Violence/Fear icons
    // imageHaysSpk_->Draw(config_.childX, config_.childY, config_.childFrame);
    // imageHaysSpk_->Draw(config_.adultX, config_.adultY, config_.adultFrame);
    // imageHaysSpk_->Draw(config_.violenceX, config_.violenceY, config_.violenceFrame);
    // imageHaysSpk_->Draw(config_.fearX, config_.fearY, config_.fearFrame);
}

void ServerSelectScreen::RenderAnimationMark() noexcept
{
    if (!imageHaysSpk_)
        return;

    // Animation mark with alpha pulsing
    // imageHaysSpk_->DrawAlpha(config_.aniMarkX, config_.aniMarkY, config_.aniMarkFrame, animationAlpha_);
}

void ServerSelectScreen::RenderScrollBar() noexcept
{
    // Legacy scroll bar rendering
}

} // namespace phoenix::ui