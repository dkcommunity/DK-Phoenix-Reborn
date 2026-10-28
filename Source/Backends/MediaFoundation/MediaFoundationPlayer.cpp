#include "Backends/MediaFoundation/MediaFoundationPlayer.h"

#include <filesystem>

namespace phoenix::backends::mediafoundation
{
    media::MediaBackend MediaFoundationPlayer::Backend() const noexcept
    {
        return media::MediaBackend::WindowsMediaFoundation;
    }

    bool MediaFoundationPlayer::Open(const media::MediaRequest& request)
    {
        std::error_code ec;
        if (request.path.empty() || !std::filesystem::exists(request.path, ec))
        {
            request_.reset();
            return false;
        }

        request_ = request;
        return true;
    }

    void MediaFoundationPlayer::Play()
    {
        playing_ = request_.has_value();
    }

    void MediaFoundationPlayer::Stop()
    {
        playing_ = false;
    }

    void MediaFoundationPlayer::Resize(media::MediaViewport viewport)
    {
        viewport_ = viewport;
    }

    bool MediaFoundationPlayer::IsPlaying() const noexcept
    {
        return playing_;
    }
}
