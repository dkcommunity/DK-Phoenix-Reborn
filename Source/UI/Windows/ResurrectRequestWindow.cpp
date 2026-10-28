#include "UI/Windows/ResurrectRequestWindow.h"

#include "Settings/InterfaceProfileLoader.h"
#include "Protocol/LegacyProtocol.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace phoenix::ui
{

ResurrectRequestWindow::ResurrectRequestWindow() noexcept
    : isOpen_(false)
    , race_(protocol::PcType::Slayer)
    , windowX_(2)
    , windowY_(0)
    , windowWidth_(200)
    , windowHeight_(50)
{
    // Initialize buttons
    for (auto& btn : buttons_)
    {
        btn.enabled = false;
        btn.imageIndex = -1;
        btn.delay = 0;
        btn.startTime = 0;
        btn.left = 0;
        btn.top = 0;
        btn.right = 0;
        btn.bottom = 0;
    }
}

void ResurrectRequestWindow::Open(const std::array<bool, 5>& enabledModes, protocol::PcType race) noexcept
{
    isOpen_ = true;
    race_ = race;
    clickedMode_.reset();

    // Set enabled modes
    for (size_t i = 0; i < buttons_.size(); ++i)
    {
        buttons_[i].enabled = enabledModes[i];
    }

    InitializeButtons();
    UpdateButtonRects();
    UpdateButtonImages();

    // Position window based on race
    windowY_ = GetRaceYOffset();
}

void ResurrectRequestWindow::Close() noexcept
{
    isOpen_ = false;
    levelUpSpk_.reset();
}

void ResurrectRequestWindow::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open({false, false, false, false, false}, race_);
}

void ResurrectRequestWindow::LoadFromConfig(const settings::ResurrectRequestProfile& profile) noexcept
{
    config_ = profile;
    windowX_ = config_.windowX;
    windowY_ = config_.windowY;
    windowWidth_ = config_.windowWidth;
    windowHeight_ = config_.windowHeight;

    // Update button positions from config
    // The config stores individual X/Y for each button
    // In Legacy, buttons are positioned dynamically in constructor
    // We'll use the config values as defaults
}

void ResurrectRequestWindow::ReloadConfig() noexcept
{
    // TODO: Reload from ini
}

void ResurrectRequestWindow::SetDelay(ResurrectMode mode, std::int32_t delayMs) noexcept
{
    size_t idx = static_cast<size_t>(mode);
    if (idx < buttons_.size())
    {
        buttons_[idx].delay = delayMs;
        buttons_[idx].startTime = static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }
}

void ResurrectRequestWindow::SetEnabled(ResurrectMode mode, bool enabled) noexcept
{
    size_t idx = static_cast<size_t>(mode);
    if (idx < buttons_.size())
    {
        buttons_[idx].enabled = enabled;
    }
}

bool ResurrectRequestWindow::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    // Check if click is on any enabled button
    for (size_t i = 0; i < buttons_.size(); ++i)
    {
        if (buttons_[i].enabled && buttons_[i].IsPointInside(x, y))
        {
            pushedIndex_ = static_cast<int>(i);
            return true;
        }
    }

    return false;
}

bool ResurrectRequestWindow::HandleMouseUp(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    if (rightButton)
        return false;

    if (pushedIndex_ != -1 && focusIndex_ != -1 && pushedIndex_ == focusIndex_)
    {
        if (buttons_[pushedIndex_].enabled && buttons_[pushedIndex_].delay <= 0)
        {
            OnButtonClicked(static_cast<ResurrectMode>(pushedIndex_));
        }
    }
    pushedIndex_ = -1;
    return false;
}

bool ResurrectRequestWindow::HandleMouseMove(int x, int y) noexcept
{
    if (!isOpen_)
        return false;

    // Update focus
    focusIndex_ = -1;
    for (size_t i = 0; i < buttons_.size(); ++i)
    {
        if (buttons_[i].enabled && buttons_[i].IsPointInside(x, y))
        {
            focusIndex_ = static_cast<int>(i);
            break;
        }
    }

    return false;
}

bool ResurrectRequestWindow::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    if (key == VK_ESCAPE)
    {
        Close();
        return true;
    }

    return false;
}

void ResurrectRequestWindow::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    UpdateDelays();

    if (levelUpSpk_)
    {
        // SPK doesn't need update for this window
    }
}

void ResurrectRequestWindow::Render() noexcept
{
    if (!isOpen_)
        return;

    if (!levelUpSpk_)
        return;

    // Draw guard background
    int guardFrame = 0;
    switch (race_)
    {
        case protocol::PcType::Slayer:
            guardFrame = config_.guardSlayerFrame;
            break;
        case protocol::PcType::Vampire:
            guardFrame = config_.guardVampireFrame;
            break;
        case protocol::PcType::Ousters:
            guardFrame = config_.guardOustersFrame;
            break;
    }

    levelUpSpk_->Draw(windowX_, windowY_, guardFrame);

    // Draw each enabled button
    for (size_t i = 0; i < buttons_.size(); ++i)
    {
        if (!buttons_[i].enabled || buttons_[i].imageIndex == -1)
            continue;

        // Draw button background (guard)
        int guardFrame = 0;
        switch (race_)
        {
            case protocol::PcType::Slayer:
                guardFrame = config_.guardSlayerFrame;
                break;
            case protocol::PcType::Vampire:
                guardFrame = config_.guardVampireFrame;
                break;
            case protocol::PcType::Ousters:
                guardFrame = config_.guardOustersFrame;
                break;
        }
        levelUpSpk_->Draw(buttons_[i].left, buttons_[i].top, guardFrame);

        // Draw button image
        if (focusIndex_ == static_cast<int>(i) && pushedIndex_ == static_cast<int>(i))
        {
            levelUpSpk_->Draw(buttons_[i].left + 10, buttons_[i].top + 10, buttons_[i].imageIndex);
        }
        else
        {
            levelUpSpk_->Draw(buttons_[i].left + 9, buttons_[i].top + 9, buttons_[i].imageIndex);
        }

        // Draw delay overlay (red fill from bottom up)
        if (buttons_[i].delay > 0)
        {
            UpdateDelays();
            if (buttons_[i].delay > 0)
            {
                // Draw red progress bar
                // This is a simplified version - legacy uses BltColorClipHeight
                // For now we just draw the button in red tint
            }
        }
    }
}

void ResurrectRequestWindow::OnButtonClicked(ResurrectMode mode) noexcept
{
    clickedMode_ = mode;
    Close();
}

void ResurrectRequestWindow::InitializeButtons() noexcept
{
    if (!levelUpSpk_)
    {
        levelUpSpk_ = std::make_unique<SpriteArchive>();
        levelUpSpk_->Open("LevelUp.spk");
    }

    // Button frame mapping based on Legacy
    // Buttons are: ResurrectScroll, Elixir, Eternity, SiegeAttacker, SkillFromOther
    // Images depend on race for SiegeAttacker and SkillFromOther
    // For ResurrectScroll, Elixir, Eternity: fixed images
    // For SiegeAttacker and SkillFromOther: race-dependent
}

void ResurrectRequestWindow::UpdateButtonRects() noexcept
{
    int posX = config_.resurrectScrollX;  // 17
    int posY = windowY_ + config_.resurrectScrollY;  // 0

    int btnWidth = GetButtonWidth();
    int btnHeight = GetButtonHeight();

    for (size_t i = 0; i < buttons_.size(); ++i)
    {
        buttons_[i].left = posX;
        buttons_[i].top = posY;
        buttons_[i].right = posX + btnWidth;
        buttons_[i].bottom = posY + btnHeight;
        posX += btnWidth + config_.buttonGap;  // 15
    }
}

void ResurrectRequestWindow::UpdateButtonImages() noexcept
{
    // Update image indices based on race and enabled state
    // Fixed images for first 3 buttons
    buttons_[0].imageIndex = config_.resurrectScrollFrame;  // ResurrectScroll
    buttons_[1].imageIndex = config_.elixirFrame;           // Elixir
    buttons_[2].imageIndex = config_.eternityFrame;         // Eternity

    // Race-dependent images for SiegeAttacker and SkillFromOther
    switch (race_)
    {
        case protocol::PcType::Slayer:
            buttons_[3].imageIndex = config_.siegeAttackerSlayerFrame;
            buttons_[4].imageIndex = config_.skillFromOtherSlayerFrame;
            break;
        case protocol::PcType::Vampire:
            buttons_[3].imageIndex = config_.siegeAttackerVampireFrame;
            buttons_[4].imageIndex = config_.skillFromOtherVampireFrame;
            break;
        case protocol::PcType::Ousters:
            buttons_[3].imageIndex = config_.siegeAttackerOustersFrame;
            buttons_[4].imageIndex = config_.skillFromOtherOustersFrame;
            break;
    }
}

std::int32_t ResurrectRequestWindow::GetRaceYOffset() const noexcept
{
    switch (race_)
    {
        case protocol::PcType::Slayer:
            return config_.slayerYOffset;
        case protocol::PcType::Vampire:
            return config_.vampireYOffset;
        case protocol::PcType::Ousters:
            return config_.oustersYOffset;
    }
    return config_.slayerYOffset;
}

std::int32_t ResurrectRequestWindow::GetButtonWidth() const noexcept
{
    if (levelUpSpk_)
    {
        // All buttons use guard frame size
        return levelUpSpk_->GetWidth(config_.guardSlayerFrame);
    }
    return 36;  // Default
}

std::int32_t ResurrectRequestWindow::GetButtonHeight() const noexcept
{
    if (levelUpSpk_)
    {
        return levelUpSpk_->GetHeight(config_.guardSlayerFrame);
    }
    return 36;  // Default
}

void ResurrectRequestWindow::UpdateDelays() noexcept
{
    auto now = static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());

    for (auto& btn : buttons_)
    {
        if (btn.delay > 0)
        {
            auto elapsed = now - btn.startTime;
            if (elapsed >= static_cast<std::uint32_t>(btn.delay))
            {
                btn.delay = 0;
            }
        }
    }
}

std::string ResurrectRequestWindow::GetTooltip(ResurrectMode mode) const noexcept
{
    switch (mode)
    {
        case ResurrectMode::ResurrectScroll:
            return "Resurrect by Scroll";
        case ResurrectMode::Elixir:
            return "Resurrect by Elixir";
        case ResurrectMode::Eternity:
            return "Resurrect by Eternity";
        case ResurrectMode::SiegeAttacker:
            return "Resurrect as Siege Attacker";
        case ResurrectMode::SkillFromOther:
            return "Resurrect by Skill from Other";
    }
    return "";
}

} // namespace phoenix::ui