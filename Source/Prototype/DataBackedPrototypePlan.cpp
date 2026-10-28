#include "Prototype/DataBackedPrototypePlan.h"

namespace phoenix::prototype
{
    DataBackedPrototypePlan DataBackedPrototypePlanFactory::CreateDefault(const std::filesystem::path& gameDataRoot)
    {
        return DataBackedPrototypePlan{
            L"acslayerman",
            gameDataRoot / L"Map" / L"adam_c.map",
            gameDataRoot / L"Sound" / L"advent_blade.wav",
            gameDataRoot / L"Music" / L"adam_normal.ogg",
            gameDataRoot / L"txt",
            gameDataRoot / L"UI" / L"spk" / L"new" / L"renewalgametitle" / L"00000.spk",
            gameDataRoot / L"UI" / L"spk" / L"new" / L"renewalgametitle" / L"00000.spki",
            gameDataRoot / L"UI" / L"spk" / L"TitleMenuDefault.spk",
            gameDataRoot / L"UI" / L"spk" / L"TitleMenuDefault.spki",
            gameDataRoot / L"UI" / L"spk" / L"new" / L"serverselectrenewal" / L"00000.spk",
            gameDataRoot / L"UI" / L"spk" / L"new" / L"serverselectrenewal" / L"00000.spki",
            gameDataRoot / L"UI" / L"spk" / L"new" / L"charmanagerrenewal",
            gameDataRoot / L"UI" / L"spk" / L"charcreaterenewal.spk",
        };
    }
}
