#include "UI/Windows/ImageNoticeWindow.h"

#include "Settings/InterfaceProfileLoader.h"
#include "Protocol/LegacyProtocol.h"
#include "UI/MinimalUIComponents.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace phoenix::ui
{

ImageNoticeWindow::ImageNoticeWindow() noexcept
    : isOpen_(false)
{
}

void ImageNoticeWindow::Open() noexcept
{
    isOpen_ = true;
}

void ImageNoticeWindow::Close() noexcept
{
    isOpen_ = false;
}

void ImageNoticeWindow::Toggle() noexcept
{
    if (isOpen_)
        Close();
    else
        Open();
}

void ImageNoticeWindow::LoadFromConfig(const settings::GameImageNoticeProfile& profile) noexcept
{
    config_ = profile;
    // Position will be set from config
}

void ImageNoticeWindow::ReloadConfig() noexcept
{
    // Reload from interface.ini
    // TODO: Implement ini reload
}

void ImageNoticeWindow::AddNotice(const std::string& name, std::uint32_t id, const std::string& giftName) noexcept
{
    Notice notice;
    notice.name = name;
    notice.id = id;
    notice.giftName = giftName;
    notice.createTime = std::chrono::steady_clock::now();
    notice.alpha = 32;  // Start with visible alpha

    notices_.push_back(std::move(notice));
}

void ImageNoticeWindow::RemoveNotice(std::size_t index) noexcept
{
    if (index < notices_.size())
    {
        notices_.erase(notices_.begin() + index);
    }
}

void ImageNoticeWindow::ClearNotices() noexcept
{
    notices_.clear();
}

bool ImageNoticeWindow::HandleMouseDown(int x, int y, bool rightButton) noexcept
{
    if (!isOpen_)
        return false;

    // Any click closes the notice
    if (rightButton)
    {
        Close();
        return true;
    }

    return true;
}

bool ImageNoticeWindow::HandleKeyDown(std::uint16_t key) noexcept
{
    if (!isOpen_)
        return false;

    switch (key)
    {
        case VK_ESCAPE:
            Close();
            return true;
    }

    return false;
}

void ImageNoticeWindow::Update(float deltaTime) noexcept
{
    if (!isOpen_)
        return;

    UpdateFadeOut();
    RemoveFadedNotices();
}

void ImageNoticeWindow::Render() noexcept
{
    if (!isOpen_)
        return;

    // Draw notices with alpha fade
    if (!notices_.empty() && imageSpk_)
    {
        int strX = config_.windowX;
        int strY = config_.windowY;

        for (const auto& notice : notices_)
        {
            if (notice.alpha >= 32)
            {
                // Full opacity
                imageSpk_->Draw(strX, strY, 0);
                strY += imageSpk_->GetHeight(0);
            }
            else
            {
                // Alpha blended
                imageSpk_->DrawAlpha(strX, strY, 0, notice.alpha);
                strY += (imageSpk_->GetHeight(0) * notice.alpha) / 32;
            }

            if (strY >= 600)
                break;
        }
    }
}

std::string ImageNoticeWindow::LoadGiftName(std::uint32_t id) const noexcept
{
    // TODO: Load from eventquest.inf
    return "Gift " + std::to_string(id);
}

void ImageNoticeWindow::UpdateFadeOut() noexcept
{
    auto now = std::chrono::steady_clock::now();

    for (auto& notice : notices_)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - notice.createTime).count();

        if (elapsed > config_.autoCloseTimeMs)
        {
            // Start fading out
            if (notice.alpha > config_.fadeOutSpeed)
                notice.alpha -= config_.fadeOutSpeed;
            else
                notice.alpha = 0;
        }
    }
}

void ImageNoticeWindow::RemoveFadedNotices() noexcept
{
    auto it = std::remove_if(notices_.begin(), notices_.end(),
        [](const Notice& n) { return n.alpha == 0; });
    notices_.erase(it, notices_.end());

    if (notices_.empty())
    {
        // Close window if no more notices
        isOpen_ = false;
    }
}

} // namespace phoenix::ui