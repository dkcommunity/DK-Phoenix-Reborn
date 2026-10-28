#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace phoenix::content
{
    struct LegacySkillInfo
    {
        std::uint16_t skillType{ 0 };
        std::string name;
        std::string hName;
        std::string description;
        std::int32_t level{ 0 };
        std::int32_t x{ 0 };
        std::int32_t y{ 0 };
        std::uint16_t spriteId{ 0 };
        std::int32_t mp{ 0 };
        std::int32_t ymp{ 0 };
        std::int32_t zmp{ 0 };
        bool passive{ false };
        bool active{ false };
        std::uint8_t skillStep{ 0 };
        std::vector<std::uint16_t> nextSkills;
        std::vector<std::uint16_t> delayGroup;
        bool hideInSkillTree{ false };

        bool hasServerInfo{ false };
        std::int32_t learnLevel{ -1 };
        std::int32_t domainType{ -1 };
        std::int32_t minDamage{ 0 };
        std::int32_t maxDamage{ 0 };
        std::int32_t minDelay{ 0 };
        std::int32_t maxDelay{ 0 };
        std::int32_t minDuration{ 0 };
        std::int32_t maxDuration{ 0 };
        std::int32_t minRange{ 0 };
        std::int32_t maxRange{ 0 };
        std::int32_t maxExp{ 0 };
        std::int32_t skillPoint{ 0 };
        std::int32_t levelUpPoint{ 0 };
        std::vector<std::int32_t> oustersSkillTypes;
        std::int32_t fire{ 0 };
        std::int32_t water{ 0 };
        std::int32_t earth{ 0 };
        std::int32_t wind{ 0 };
        std::int32_t sum{ 0 };
        std::int32_t wristlet{ 0 };
        std::int32_t stone1{ 0 };
        std::int32_t stone2{ 0 };
        std::int32_t stone3{ 0 };
        std::int32_t stone4{ 0 };
        std::int32_t elementalDomain{ -1 };
        std::uint8_t canDelete{ 0 };
    };

    struct LegacyShopTemplateInfo
    {
        std::uint32_t id{ 0 };
        std::uint8_t type{ 0 };
        std::int32_t itemClass{ 0 };
        std::uint16_t minType{ 0 };
        std::uint16_t maxType{ 0 };
        std::uint8_t minOption{ 0 };
        std::uint8_t maxOption{ 0 };
    };

    struct LegacyNpcInfo
    {
        std::uint32_t id{ 0 };
        std::string name;
        std::vector<std::uint32_t> shopTemplateIds;
        std::string description;
        std::int32_t spriteId{ -1 };
    };

    struct LegacyInteractionObjectInfo
    {
        std::uint32_t index{ 0 };
        std::uint8_t type{ 0 };
        std::uint16_t frameId{ 0 };
        std::int32_t property{ 0 };
        std::uint32_t soundId{ 0 };
    };

    struct LegacyNpcScriptInfo
    {
        std::uint32_t id{ 0 };
        std::string ownerId;
        std::vector<std::string> subjects;
        std::vector<std::string> contents;
    };

    struct LegacyInfoCatalogResult
    {
        std::map<std::uint16_t, LegacySkillInfo> skills;
        std::map<std::uint32_t, LegacyNpcInfo> npcs;
        std::map<std::uint32_t, LegacyShopTemplateInfo> shopTemplates;
        std::vector<LegacyInteractionObjectInfo> interactionObjects;
        std::map<std::uint32_t, LegacyNpcScriptInfo> npcScripts;
        std::vector<std::string> monsterLastNames;
        std::wstring error;
        std::vector<std::wstring> warnings;

        [[nodiscard]] bool Succeeded() const noexcept
        {
            return error.empty();
        }

        [[nodiscard]] const LegacySkillInfo* FindSkill(std::uint16_t skillType) const noexcept;
        [[nodiscard]] const LegacyNpcInfo* FindNpc(std::uint32_t npcId) const noexcept;
        [[nodiscard]] const LegacyShopTemplateInfo* FindShopTemplate(std::uint32_t templateId) const noexcept;
        [[nodiscard]] const LegacyInteractionObjectInfo* FindInteractionObject(std::uint32_t index) const noexcept;
        [[nodiscard]] const LegacyNpcScriptInfo* FindNpcScript(std::uint32_t scriptId) const noexcept;
    };

    class LegacyInfoCatalog final
    {
    public:
        [[nodiscard]] LegacyInfoCatalogResult Load(
            const std::filesystem::path& skillPath,
            const std::filesystem::path& skillInfoPath,
            const std::filesystem::path& npcPath,
            const std::filesystem::path& shopTemplatePath,
            const std::filesystem::path& interactionObjectPath,
            const std::filesystem::path& npcScriptPath,
            const std::filesystem::path& monsterNamePath) const;
    };
}
