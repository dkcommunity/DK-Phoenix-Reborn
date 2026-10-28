#pragma once

#include "Media/MediaTypes.h"

namespace phoenix::media
{
    class IMediaPlayer
    {
    public:
        virtual ~IMediaPlayer() = default;

        [[nodiscard]] virtual MediaBackend Backend() const noexcept = 0;
        virtual bool Open(const MediaRequest& request) = 0;
        virtual void Play() = 0;
        virtual void Stop() = 0;
        virtual void Resize(MediaViewport viewport) = 0;
    };
}
