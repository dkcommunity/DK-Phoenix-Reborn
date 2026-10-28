#include "Migration/RpkMigrationPolicy.h"

#include <system_error>

namespace phoenix::migration
{
    std::vector<RpkPasswordCandidate> RpkMigrationPolicy::KnownPasswords()
    {
        return {
            RpkPasswordCandidate{ "1x700", "Active legacy RPK_PASSWORD; verified for title.rpk." },
            RpkPasswordCandidate{ "cjdgorkr", "Commented old legacy password; keep for older RPK checks." },
        };
    }

    RpkMigrationTarget RpkMigrationPolicy::TitleTextTarget(const std::filesystem::path& gameDataRoot)
    {
        return RpkMigrationTarget{
            gameDataRoot / L"UI" / L"txt" / L"title.rpk",
            gameDataRoot / L"txt",
            {
                "back.txt", "bonus.txt", "character.txt", "Check.txt", "create.txt", "create_window.txt",
                "Damage.txt", "Defence.txt", "Dex.txt", "FaceType.txt", "Gender.txt", "HairColor.txt",
                "HP.txt", "Int.txt", "Load.txt", "MP.txt", "name.txt", "next.txt", "Protection.txt",
                "Reroll.txt", "Save.txt", "server.txt", "SkinColor.txt", "Str.txt", "Tohit.txt",
                "Tribe.txt", "world.txt", "X.txt",
            }
        };
    }

    bool RpkMigrationPolicy::ShouldPreferExtractedText(const std::filesystem::path& extractedDirectory)
    {
        std::error_code ec;
        return std::filesystem::exists(extractedDirectory, ec) &&
            std::filesystem::is_directory(extractedDirectory, ec) &&
            std::filesystem::exists(extractedDirectory / L"create.txt", ec) &&
            std::filesystem::exists(extractedDirectory / L"character.txt", ec);
    }
}
