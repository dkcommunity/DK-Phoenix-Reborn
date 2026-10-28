#include "Content/LegacyInfoCatalog.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <utility>

namespace phoenix::content
{
    namespace
    {
        constexpr std::int32_t kMaxLegacyTableCount = 100000;
        constexpr std::int32_t kMaxLegacyListCount = 10000;
        constexpr std::int32_t kSkillDomainOusters = 7;

        class LegacyInfoReader final
        {
        public:
            explicit LegacyInfoReader(std::vector<std::uint8_t> bytes)
                : bytes_(std::move(bytes))
            {
            }

            [[nodiscard]] bool U8(std::uint8_t& value) noexcept
            {
                if (!CanRead(1U))
                {
                    return false;
                }
                value = bytes_[offset_++];
                return true;
            }

            [[nodiscard]] bool U16(std::uint16_t& value) noexcept
            {
                if (!CanRead(2U))
                {
                    return false;
                }
                value = static_cast<std::uint16_t>(bytes_[offset_]) |
                    static_cast<std::uint16_t>(
                        static_cast<std::uint16_t>(bytes_[offset_ + 1U]) << 8U);
                offset_ += 2U;
                return true;
            }

            [[nodiscard]] bool U32(std::uint32_t& value) noexcept
            {
                if (!CanRead(4U))
                {
                    return false;
                }
                value = static_cast<std::uint32_t>(bytes_[offset_]) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 1U]) << 8U) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 2U]) << 16U) |
                    (static_cast<std::uint32_t>(bytes_[offset_ + 3U]) << 24U);
                offset_ += 4U;
                return true;
            }

            [[nodiscard]] bool I32(std::int32_t& value) noexcept
            {
                std::uint32_t raw = 0;
                if (!U32(raw))
                {
                    return false;
                }
                value = static_cast<std::int32_t>(raw);
                return true;
            }

            [[nodiscard]] bool String(std::string& value)
            {
                std::int32_t length = 0;
                if (!I32(length) || length < 0 || length > 65535)
                {
                    return false;
                }
                if (!CanRead(static_cast<std::size_t>(length)))
                {
                    return false;
                }
                value.assign(
                    reinterpret_cast<const char*>(bytes_.data() + offset_),
                    reinterpret_cast<const char*>(bytes_.data() + offset_ + static_cast<std::size_t>(length)));
                offset_ += static_cast<std::size_t>(length);
                return true;
            }

            [[nodiscard]] bool AtEnd() const noexcept
            {
                return offset_ == bytes_.size();
            }

        private:
            [[nodiscard]] bool CanRead(const std::size_t size) const noexcept
            {
                return offset_ + size <= bytes_.size();
            }

            std::vector<std::uint8_t> bytes_;
            std::size_t offset_{ 0 };
        };

        [[nodiscard]] std::vector<std::uint8_t> ReadAllBytes(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return {};
            }
            return std::vector<std::uint8_t>(
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>());
        }

        [[nodiscard]] bool ReadActionInfoList(LegacyInfoReader& reader, std::vector<std::uint16_t>& values)
        {
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > kMaxLegacyListCount)
            {
                return false;
            }

            values.clear();
            values.reserve(static_cast<std::size_t>(count));
            for (std::int32_t index = 0; index < count; ++index)
            {
                std::uint16_t value = 0;
                if (!reader.U16(value))
                {
                    return false;
                }
                values.push_back(value);
            }
            return true;
        }

        [[nodiscard]] bool ReadStringTable(LegacyInfoReader& reader, std::vector<std::string>& values)
        {
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > kMaxLegacyListCount)
            {
                return false;
            }

            values.clear();
            values.reserve(static_cast<std::size_t>(count));
            for (std::int32_t index = 0; index < count; ++index)
            {
                std::string value;
                if (!reader.String(value))
                {
                    return false;
                }
                values.push_back(std::move(value));
            }
            return true;
        }

        [[nodiscard]] bool ReadClientSkillInfo(
            LegacyInfoReader& reader,
            const std::uint16_t skillType,
            LegacySkillInfo& info)
        {
            info.skillType = skillType;
            std::uint8_t passive = 0;
            std::uint8_t active = 0;
            std::uint8_t hide = 0;
            if (!reader.String(info.name) ||
                !reader.String(info.hName) ||
                !reader.I32(info.level) ||
                !reader.I32(info.x) ||
                !reader.I32(info.y) ||
                !reader.U16(info.spriteId) ||
                !reader.I32(info.mp) ||
                !reader.U8(passive) ||
                !reader.I32(info.ymp) ||
                !reader.I32(info.zmp) ||
                !reader.U8(active) ||
                !reader.U8(info.skillStep) ||
                !ReadActionInfoList(reader, info.nextSkills) ||
                !ReadActionInfoList(reader, info.delayGroup) ||
                !reader.String(info.description) ||
                !reader.U8(hide))
            {
                return false;
            }

            info.passive = passive != 0;
            info.active = active != 0;
            info.hideInSkillTree = hide != 0;
            return true;
        }

        [[nodiscard]] bool ReadServerSkillInfo(LegacyInfoReader& reader, LegacySkillInfo& info)
        {
            std::string serverName;
            std::string serverHName;
            if (!reader.I32(info.learnLevel) ||
                !reader.I32(info.domainType) ||
                !reader.String(serverName) ||
                !reader.String(serverHName) ||
                !reader.I32(info.minDamage) ||
                !reader.I32(info.maxDamage) ||
                !reader.I32(info.minDelay) ||
                !reader.I32(info.maxDelay) ||
                !reader.I32(info.minDuration) ||
                !reader.I32(info.maxDuration) ||
                !reader.I32(info.mp) ||
                !reader.I32(info.minRange) ||
                !reader.I32(info.maxRange) ||
                !reader.I32(info.maxExp))
            {
                return false;
            }

            if (!serverName.empty() && serverName != "Empty Skill")
            {
                info.name = std::move(serverName);
                info.hName = std::move(serverHName);
            }

            if (info.domainType == kSkillDomainOusters)
            {
                std::int32_t count = 0;
                if (!reader.I32(info.skillPoint) ||
                    !reader.I32(info.levelUpPoint) ||
                    !reader.I32(count) ||
                    count < 0 ||
                    count > kMaxLegacyListCount)
                {
                    return false;
                }

                info.oustersSkillTypes.clear();
                info.oustersSkillTypes.reserve(static_cast<std::size_t>(count));
                for (std::int32_t index = 0; index < count; ++index)
                {
                    std::int32_t skillType = 0;
                    if (!reader.I32(skillType))
                    {
                        return false;
                    }
                    info.oustersSkillTypes.push_back(skillType);
                }

                if (!reader.I32(info.fire) ||
                    !reader.I32(info.water) ||
                    !reader.I32(info.earth) ||
                    !reader.I32(info.wind) ||
                    !reader.I32(info.sum) ||
                    !reader.I32(info.wristlet) ||
                    !reader.I32(info.stone1) ||
                    !reader.I32(info.stone2) ||
                    !reader.I32(info.stone3) ||
                    !reader.I32(info.stone4) ||
                    !reader.I32(info.elementalDomain) ||
                    !reader.U8(info.canDelete))
                {
                    return false;
                }
            }
            else
            {
                info.elementalDomain = -1;
            }

            info.hasServerInfo = true;
            return true;
        }

        [[nodiscard]] bool LoadSkillTable(
            const std::filesystem::path& path,
            LegacyInfoCatalogResult& result)
        {
            const auto bytes = ReadAllBytes(path);
            if (bytes.empty())
            {
                result.error = L"Skill.inf could not be opened: " + path.wstring();
                return false;
            }

            LegacyInfoReader reader(bytes);
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > kMaxLegacyTableCount)
            {
                result.error = L"Skill.inf has an invalid table count: " + path.wstring();
                return false;
            }

            for (std::int32_t index = 0; index < count; ++index)
            {
                LegacySkillInfo info;
                if (!ReadClientSkillInfo(reader, static_cast<std::uint16_t>(index), info))
                {
                    result.error = L"Skill.inf parse failed at index " + std::to_wstring(index) +
                        L": " + path.wstring();
                    return false;
                }
                result.skills[info.skillType] = std::move(info);
            }

            if (!reader.AtEnd())
            {
                result.warnings.push_back(L"Skill.inf has trailing bytes: " + path.wstring());
            }
            return true;
        }

        [[nodiscard]] bool LoadServerSkillTable(
            const std::filesystem::path& path,
            LegacyInfoCatalogResult& result)
        {
            const auto bytes = ReadAllBytes(path);
            if (bytes.empty())
            {
                result.error = L"SkillInfo.inf could not be opened: " + path.wstring();
                return false;
            }

            LegacyInfoReader reader(bytes);
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > kMaxLegacyTableCount)
            {
                result.error = L"SkillInfo.inf has an invalid table count: " + path.wstring();
                return false;
            }

            for (std::int32_t index = 0; index < count; ++index)
            {
                std::int32_t skillType = 0;
                if (!reader.I32(skillType) || skillType < 0 || skillType > 65535)
                {
                    result.error = L"SkillInfo.inf has an invalid skill type at record " + std::to_wstring(index) +
                        L": " + path.wstring();
                    return false;
                }

                auto& info = result.skills[static_cast<std::uint16_t>(skillType)];
                info.skillType = static_cast<std::uint16_t>(skillType);
                if (!ReadServerSkillInfo(reader, info))
                {
                    result.error = L"SkillInfo.inf parse failed for skill " + std::to_wstring(skillType) +
                        L": " + path.wstring();
                    return false;
                }
            }

            if (!reader.AtEnd())
            {
                result.warnings.push_back(L"SkillInfo.inf has trailing bytes: " + path.wstring());
            }
            return true;
        }

        [[nodiscard]] bool LoadNpcTable(
            const std::filesystem::path& path,
            LegacyInfoCatalogResult& result)
        {
            const auto bytes = ReadAllBytes(path);
            if (bytes.empty())
            {
                result.error = L"NPC.inf could not be opened: " + path.wstring();
                return false;
            }

            LegacyInfoReader reader(bytes);
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > kMaxLegacyTableCount)
            {
                result.error = L"NPC.inf has an invalid table count: " + path.wstring();
                return false;
            }

            for (std::int32_t index = 0; index < count; ++index)
            {
                LegacyNpcInfo npc;
                std::uint32_t id = 0;
                std::int32_t shopCount = 0;
                if (!reader.U32(id) ||
                    !reader.String(npc.name) ||
                    !reader.I32(shopCount) ||
                    shopCount < 0 ||
                    shopCount > kMaxLegacyListCount)
                {
                    result.error = L"NPC.inf parse failed at record " + std::to_wstring(index) +
                        L": " + path.wstring();
                    return false;
                }

                npc.id = id;
                npc.shopTemplateIds.reserve(static_cast<std::size_t>(shopCount));
                for (std::int32_t shopIndex = 0; shopIndex < shopCount; ++shopIndex)
                {
                    std::uint32_t shopTemplateId = 0;
                    if (!reader.U32(shopTemplateId))
                    {
                        result.error = L"NPC.inf shop template parse failed for NPC " + std::to_wstring(id) +
                            L": " + path.wstring();
                        return false;
                    }
                    npc.shopTemplateIds.push_back(shopTemplateId);
                }

                if (!reader.String(npc.description) ||
                    !reader.I32(npc.spriteId))
                {
                    result.error = L"NPC.inf parse failed for NPC " + std::to_wstring(id) +
                        L": " + path.wstring();
                    return false;
                }
                result.npcs[npc.id] = std::move(npc);
            }

            if (!reader.AtEnd())
            {
                result.warnings.push_back(L"NPC.inf has trailing bytes: " + path.wstring());
            }
            return true;
        }

        [[nodiscard]] bool LoadShopTemplateTable(
            const std::filesystem::path& path,
            LegacyInfoCatalogResult& result)
        {
            const auto bytes = ReadAllBytes(path);
            if (bytes.empty())
            {
                result.error = L"ShopTemplate.inf could not be opened: " + path.wstring();
                return false;
            }

            LegacyInfoReader reader(bytes);
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > kMaxLegacyTableCount)
            {
                result.error = L"ShopTemplate.inf has an invalid table count: " + path.wstring();
                return false;
            }

            for (std::int32_t index = 0; index < count; ++index)
            {
                LegacyShopTemplateInfo shop;
                if (!reader.U32(shop.id) ||
                    !reader.U8(shop.type) ||
                    !reader.I32(shop.itemClass) ||
                    !reader.U16(shop.minType) ||
                    !reader.U16(shop.maxType) ||
                    !reader.U8(shop.minOption) ||
                    !reader.U8(shop.maxOption))
                {
                    result.error = L"ShopTemplate.inf parse failed at record " + std::to_wstring(index) +
                        L": " + path.wstring();
                    return false;
                }
                result.shopTemplates[shop.id] = shop;
            }

            if (!reader.AtEnd())
            {
                result.warnings.push_back(L"ShopTemplate.inf has trailing bytes: " + path.wstring());
            }
            return true;
        }

        [[nodiscard]] bool LoadInteractionObjectTable(
            const std::filesystem::path& path,
            LegacyInfoCatalogResult& result)
        {
            const auto bytes = ReadAllBytes(path);
            if (bytes.empty())
            {
                result.error = L"InteractionObject.inf could not be opened: " + path.wstring();
                return false;
            }

            LegacyInfoReader reader(bytes);
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > kMaxLegacyTableCount)
            {
                result.error = L"InteractionObject.inf has an invalid table count: " + path.wstring();
                return false;
            }

            result.interactionObjects.clear();
            result.interactionObjects.reserve(static_cast<std::size_t>(count));
            for (std::int32_t index = 0; index < count; ++index)
            {
                LegacyInteractionObjectInfo info;
                info.index = static_cast<std::uint32_t>(index);
                if (!reader.U8(info.type) ||
                    !reader.U16(info.frameId) ||
                    !reader.I32(info.property) ||
                    !reader.U32(info.soundId))
                {
                    result.error = L"InteractionObject.inf parse failed at index " + std::to_wstring(index) +
                        L": " + path.wstring();
                    return false;
                }
                result.interactionObjects.push_back(info);
            }

            if (!reader.AtEnd())
            {
                result.warnings.push_back(L"InteractionObject.inf has trailing bytes: " + path.wstring());
            }
            return true;
        }

        [[nodiscard]] bool LoadNpcScriptTable(
            const std::filesystem::path& path,
            LegacyInfoCatalogResult& result)
        {
            const auto bytes = ReadAllBytes(path);
            if (bytes.empty())
            {
                result.error = L"NPCScript.inf could not be opened: " + path.wstring();
                return false;
            }

            LegacyInfoReader reader(bytes);
            std::int32_t count = 0;
            if (!reader.I32(count) || count < 0 || count > kMaxLegacyTableCount)
            {
                result.error = L"NPCScript.inf has an invalid table count: " + path.wstring();
                return false;
            }

            for (std::int32_t index = 0; index < count; ++index)
            {
                LegacyNpcScriptInfo script;
                if (!reader.U32(script.id) ||
                    !reader.String(script.ownerId) ||
                    !ReadStringTable(reader, script.subjects) ||
                    !ReadStringTable(reader, script.contents))
                {
                    result.error = L"NPCScript.inf parse failed at record " + std::to_wstring(index) +
                        L": " + path.wstring();
                    return false;
                }
                result.npcScripts[script.id] = std::move(script);
            }

            if (!reader.AtEnd())
            {
                result.warnings.push_back(L"NPCScript.inf has trailing bytes: " + path.wstring());
            }
            return true;
        }

        [[nodiscard]] bool LoadMonsterNameTable(
            const std::filesystem::path& path,
            LegacyInfoCatalogResult& result)
        {
            const auto bytes = ReadAllBytes(path);
            if (bytes.empty())
            {
                result.error = L"MonsterName.inf could not be opened: " + path.wstring();
                return false;
            }

            LegacyInfoReader reader(bytes);
            std::vector<std::string> firstNames;
            std::vector<std::string> middleNames;
            if (!ReadStringTable(reader, firstNames) ||
                !ReadStringTable(reader, middleNames) ||
                !ReadStringTable(reader, result.monsterLastNames))
            {
                result.error = L"MonsterName.inf parse failed: " + path.wstring();
                return false;
            }
            if (!reader.AtEnd())
            {
                result.warnings.push_back(L"MonsterName.inf has trailing bytes: " + path.wstring());
            }
            return true;
        }
    }

    const LegacySkillInfo* LegacyInfoCatalogResult::FindSkill(const std::uint16_t skillType) const noexcept
    {
        const auto it = skills.find(skillType);
        return it == skills.end() ? nullptr : &it->second;
    }

    const LegacyNpcInfo* LegacyInfoCatalogResult::FindNpc(const std::uint32_t npcId) const noexcept
    {
        const auto it = npcs.find(npcId);
        return it == npcs.end() ? nullptr : &it->second;
    }

    const LegacyShopTemplateInfo* LegacyInfoCatalogResult::FindShopTemplate(const std::uint32_t templateId) const noexcept
    {
        const auto it = shopTemplates.find(templateId);
        return it == shopTemplates.end() ? nullptr : &it->second;
    }

    const LegacyInteractionObjectInfo* LegacyInfoCatalogResult::FindInteractionObject(const std::uint32_t index) const noexcept
    {
        if (index >= interactionObjects.size())
        {
            return nullptr;
        }
        return &interactionObjects[index];
    }

    const LegacyNpcScriptInfo* LegacyInfoCatalogResult::FindNpcScript(const std::uint32_t scriptId) const noexcept
    {
        const auto it = npcScripts.find(scriptId);
        return it == npcScripts.end() ? nullptr : &it->second;
    }

    LegacyInfoCatalogResult LegacyInfoCatalog::Load(
        const std::filesystem::path& skillPath,
        const std::filesystem::path& skillInfoPath,
        const std::filesystem::path& npcPath,
        const std::filesystem::path& shopTemplatePath,
        const std::filesystem::path& interactionObjectPath,
        const std::filesystem::path& npcScriptPath,
        const std::filesystem::path& monsterNamePath) const
    {
        LegacyInfoCatalogResult result;
        if (!LoadSkillTable(skillPath, result) ||
            !LoadServerSkillTable(skillInfoPath, result) ||
            !LoadNpcTable(npcPath, result) ||
            !LoadShopTemplateTable(shopTemplatePath, result) ||
            !LoadInteractionObjectTable(interactionObjectPath, result) ||
            !LoadNpcScriptTable(npcScriptPath, result) ||
            !LoadMonsterNameTable(monsterNamePath, result))
        {
            return result;
        }
        return result;
    }
}
