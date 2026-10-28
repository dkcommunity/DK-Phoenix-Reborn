#pragma once

#include "Media/MediaPlayer.h"

#include <optional>

namespace phoenix::backends::mediafoundation
{
    class MediaFoundationPlayer final : public media::IMediaPlayer
    {
    public:
        [[nodiscard]] media::MediaBackend Backend() const noexcept override;
        bool Open(const media::MediaRequest& request) override;
        void Play() override;
        void Stop() override;
        void Resize(media::MediaViewport viewport) override;

        [[nodiscard]] bool IsPlaying() const noexcept;

    private:
        std::optional<media::MediaRequest> request_;
        media::MediaViewport viewport_;
        bool playing_{ false };
    };
}
