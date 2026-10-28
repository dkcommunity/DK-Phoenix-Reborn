#include "Render/RenderQueue.h"

#include <algorithm>
#include <utility>

namespace phoenix::render
{
    void RenderQueue::Clear() noexcept
    {
        commands_.clear();
    }

    void RenderQueue::Submit(RenderCommand command)
    {
        commands_.push_back(std::move(command));
    }

    void RenderQueue::Sort()
    {
        std::stable_sort(
            commands_.begin(),
            commands_.end(),
            [](const RenderCommand& left, const RenderCommand& right)
            {
                return left.sortKey < right.sortKey;
            });
    }

    const std::vector<RenderCommand>& RenderQueue::Commands() const noexcept
    {
        return commands_;
    }

    bool RenderQueue::Empty() const noexcept
    {
        return commands_.empty();
    }
}
