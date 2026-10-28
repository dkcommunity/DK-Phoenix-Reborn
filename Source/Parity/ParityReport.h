#pragma once

#include <string>
#include <vector>

namespace phoenix::parity
{
    enum class ParityStatus
    {
        Match,
        Difference,
        Blocked
    };

    struct ParityFinding
    {
        ParityStatus status{ ParityStatus::Blocked };
        std::wstring area;
        std::wstring detail;
    };

    class ParityReport final
    {
    public:
        void Add(ParityFinding finding);
        [[nodiscard]] bool HasDifferences() const noexcept;
        [[nodiscard]] bool HasBlockers() const noexcept;
        [[nodiscard]] const std::vector<ParityFinding>& Findings() const noexcept;

    private:
        std::vector<ParityFinding> findings_;
    };
}
