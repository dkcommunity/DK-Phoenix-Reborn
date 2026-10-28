#pragma once

#include "Settings/InterfaceProfileLoader.h"
#include "UI/MinimalUIComponents.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>

namespace phoenix::ui
{

// ImageNotice config from interface.ini [ImageNotice_1280x960]
struct ImageNoticeProfile
{
    std::int32_t windowX{ 0 };
    std::int32_t windowY{ 200 };
    std::int32_t windowWidth{ 400 };
    std::int32_t windowHeight{ 100 };
    bool topmost{ false };
    std::int32_t autoCloseTimeMs{ 10000 };  // 10 seconds
    std::int32_t fadeOutSpeed{ 1 };  // alpha decrement per frame
    std::int32_t maxDisplayLines{ 10 };
    std::int32_t lineHeight{ 32 };
    std::int32_t textOffsetX{ 66 };
    std::int32_t nameOffsetY{ 10 };
    std::int32_t giftOffsetY{ 30 };
    std::int32_t msgOffsetY{ 50 };
};

// Notice entry
struct ImageNoticeEntry
{
    std::string name;
    std::string giftName;
    std::uint32_t id{ 0 };
    std::chrono::steady_clock::time_point createTime;
    std::uint8_t alpha{ 32 };  // 0-32 fade alpha
};

class ImageNoticeWindow
{
public:
    explicit ImageNoticeWindow() noexcept;

    // State
    void Open() noexcept;
    void Close() noexcept;
    void Toggle() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    // Config
    void LoadFromConfig(const settings::GameImageNoticeProfile& profile) noexcept;
    void ReloadConfig() noexcept;

    // Notice management
    void AddNotice(const std::string& name, std::uint32_t id, const std::string& giftName = "") noexcept;
    void RemoveNotice(std::size_t index) noexcept;
    void ClearNotices() noexcept;
    [[nodiscard]] std::size_t GetNoticeCount() const noexcept { return notices_.size(); }

    // Input
    bool HandleMouseDown(int x, int y, bool rightButton) noexcept;
    bool HandleKeyDown(std::uint16_t key) noexcept;

    // Update/Render
    void Update(float deltaTime) noexcept;
    void Render() noexcept;

private:
    // State
    bool isOpen_{ false };

    // Config
    settings::GameImageNoticeProfile config_;
    std::filesystem::path iniPath_;
    std::filesystem::file_time_type iniLastWrite_;

    // Notices
    struct Notice
    {
        std::string name;
        std::string giftName;
        std::uint32_t id{ 0 };
        std::chrono::steady_clock::time_point createTime;
        std::uint8_t alpha{ 32 };  // 0-32 fade alpha
    };
    std::vector<Notice> notices_;

    // SPK archive
    std::unique_ptr<SpriteArchive> imageSpk_;

    // Helpers
    std::string LoadGiftName(std::uint32_t id) const noexcept;
    void UpdateFadeOut() noexcept;
    void RemoveFadedNotices() noexcept;

    // Callbacks
    void OnEscapePressed() noexcept;
};

} // namespace phoenix::ui