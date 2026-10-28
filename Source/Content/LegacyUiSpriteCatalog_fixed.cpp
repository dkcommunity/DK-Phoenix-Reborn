Add(r, "SPK_SLAYER_MAIN", "SPK_SLAYER_MAIN", LegacyUiSpriteScope::Hud, L"UI\\spk\\simpleinformation.spk", true, true, 32, LegacyUiRace::Slayer);
        Add(r, "SPK_VAMPIRE_MAIN", "SPK_VAMPIRE_MAIN", LegacyUiSpriteScope::Hud, L"UI\\spk\\simpleinformation.spk", true, true, 32, LegacyUiRace::Vampire);
        Add(r, "SPK_OUSTERS_MAIN", "SPK_OUSTERS_MAIN", LegacyUiSpriteScope::Hud, L"UI\\spk\\simpleinformation.spk", true, true, 32, LegacyUiRace::Ousters);
        // HPBAR: monolithic hpbar.spk (new location: UI\spk\new\hpbar\hpbar.spk)
        // Single file with .spki index, frames 0-49 directly accessible
        Add(r, "SPK_HPBAR", "SPK_HPBAR", LegacyUiSpriteScope::Hud, L"UI\\spk\\new\\hpbar\\hpbar.spk", true, true, 50, LegacyUiRace::Shared, L"UI\\spk\\new\\hpbar\\hpbar.spki", false, 0, 50);
        Add(r, "SPK_EFFECT_STATUS_SLAYER", "SPK_EFFECT_STATUS_SLAYER", LegacyUiSpriteScope::Hud, L"UI\\spk\\effectstatusslayer.spk", true, false, 1, LegacyUiRace::Slayer);
        Add(r, "SPK_EFFECT_STATUS_VAMPIRE", "SPK_EFFECT_STATUS_VAMPIRE", LegacyUiSpriteScope::Hud, L"UI\\spk\\effectstatusvampire.spk", true, false, 1, LegacyUiRace::Vampire);
        Add(r, "SPK_EFFECT_STATUS_OUSTERS", "SPK_EFFECT_STATUS_OUSTERS", LegacyUiSpriteScope::Hud, L"UI\\spk\\effectstatusousters.spk", true, false, 1, LegacyUiRace::Ousters);