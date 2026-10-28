#pragma once

#include "Render/RenderTypes.h"

#include <vector>

namespace phoenix::render
{
    class RenderQueue final
    {
    public:
        void Clear() noexcept;
        void Submit(RenderCommand command);
        void Sort();

        [[nodiscard]] const std::vector<RenderCommand>& Commands() const noexcept;
        [[nodiscard]] bool Empty() const noexcept;

    private:
        std::vector<RenderCommand> commands_;
    };
}
