#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace phoenix::content
{
    enum class LegacyUiSpriteKind
    {
        SpritePack,
        IndexedSpritePack,
        FramePack,
        ResourcePack,
        InterfaceText
    };

    enum class LegacyUiSpriteScope
    {
        Global,
        Widget,
        TitleLogin,
        Character,
        Hud,
        InventoryGear,
        Skill,
        MapNavigation,
        ShopTradeStorage,
        SocialSystem,
        QuestEventPet,
        Text
    };

    enum class LegacyUiRace
    {
        Shared,
        Slayer,
        Vampire,
        Ousters
    };

    struct LegacyUiSpriteRecord
    {
        std::string key;
        std::string legacyDefine;
        LegacyUiSpriteKind kind{ LegacyUiSpriteKind::SpritePack };
        LegacyUiSpriteScope scope{ LegacyUiSpriteScope::Global };
        LegacyUiRace race{ LegacyUiRace::Shared };
        std::filesystem::path relativePath;
        std::optional<std::filesystem::path> relativeIndexPath;
        bool required{ false };
        bool indexRequired{ true };
        bool smokeDecode{ false };
        std::uint16_t smokeFrameIndex{ 0 };
        std::uint16_t splitBaseFrameIndex{ 0 };
        std::uint16_t expectedMinimumFrames{ 0 };
        bool catalogOnly{ false };
    };

    struct LegacyUiSpriteCatalogResult
    {
        std::filesystem::path dataRoot;
        std::vector<LegacyUiSpriteRecord> records;

        [[nodiscard]] const LegacyUiSpriteRecord* Find(std::string_view key) const noexcept;
        [[nodiscard]] std::vector<const LegacyUiSpriteRecord*> ByScope(LegacyUiSpriteScope scope) const;
        [[nodiscard]] std::vector<const LegacyUiSpriteRecord*> GlobalForRace(LegacyUiRace race) const;
        [[nodiscard]] std::filesystem::path ResolvePath(const LegacyUiSpriteRecord& record) const;
        [[nodiscard]] std::filesystem::path ResolveIndexPath(const LegacyUiSpriteRecord& record) const;
    };

    class LegacyUiSpriteCatalog final
    {
    public:
        [[nodiscard]] LegacyUiSpriteCatalogResult Build(const std::filesystem::path& dataRoot) const;
    };

    [[nodiscard]] const wchar_t* ToDisplayName(LegacyUiSpriteKind kind) noexcept;
    [[nodiscard]] const wchar_t* ToDisplayName(LegacyUiSpriteScope scope) noexcept;
    [[nodiscard]] const wchar_t* ToDisplayName(LegacyUiRace race) noexcept;
}
