#include "Parity/ParityReport.h"

#include <utility>

namespace phoenix::parity
{
    void ParityReport::Add(ParityFinding finding)
    {
        findings_.push_back(std::move(finding));
    }

    bool ParityReport::HasDifferences() const noexcept
    {
        for (const auto& finding : findings_)
        {
            if (finding.status == ParityStatus::Difference)
            {
                return true;
            }
        }

        return false;
    }

    bool ParityReport::HasBlockers() const noexcept
    {
        for (const auto& finding : findings_)
        {
            if (finding.status == ParityStatus::Blocked)
            {
                return true;
            }
        }

        return false;
    }

    const std::vector<ParityFinding>& ParityReport::Findings() const noexcept
    {
        return findings_;
    }
}
