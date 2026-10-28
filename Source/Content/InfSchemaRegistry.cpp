#include "Content/InfSchemaRegistry.h"

namespace phoenix::content
{
    namespace
    {
        [[nodiscard]] InfSchemaEntry Binary(
            std::wstring logical,
            std::wstring key,
            std::wstring fallback,
            std::string parser,
            std::string consumer,
            const InfSchemaRequirement requirement = InfSchemaRequirement::Required)
        {
            return InfSchemaEntry{
                std::move(logical),
                std::move(key),
                std::move(fallback),
                InfSchemaFormat::BinaryTable,
                requirement,
                std::move(parser),
                std::move(consumer)
            };
        }

        [[nodiscard]] InfSchemaEntry Updater(std::wstring logical, std::wstring fallback)
        {
            return InfSchemaEntry{
                std::move(logical),
                L"",
                std::move(fallback),
                InfSchemaFormat::UpdaterOnly,
                InfSchemaRequirement::UpdaterOnly,
                "updater-key-value",
                "launcher/updater only"
            };
        }
    }

    std::vector<InfSchemaEntry> InfSchemaRegistry::BuildDefault()
    {
        std::vector<InfSchemaEntry> entries;
        entries.reserve(32);

        entries.push_back(InfSchemaEntry{ L"filedef", L"", L"Info\\filedef.inf", InfSchemaFormat::TextKeyValue, InfSchemaRequirement::Required, "MFileDef", "all runtime data path resolution" });
        entries.push_back(Binary(L"String", L"FILE_INFO_STRING", L"Info\\String.inf", "MStringArray", "GameStringService"));
        entries.push_back(Binary(L"Nickname", L"FILE_INFO_NICKNAME_STRING", L"Info\\Nickname.inf", "CTypeTable<MString>::LoadFromFile_NickNameString", "GameStringService"));
        entries.push_back(Binary(L"Item", L"FILE_INFO_ITEM", L"Info\\item.inf", "MItemTable", "item runtime/visuals"));
        entries.push_back(Binary(L"ItemOption", L"FILE_INFO_ITEMOPTION", L"Info\\ItemOption.inf", "MItemOptionTable", "item options"));
        entries.push_back(Binary(L"Creature", L"FILE_INFO_CREATURE", L"Info\\Creature.inf", "MCreatureTable", "creature runtime"));
        entries.push_back(Binary(L"CreatureSprite", L"FILE_INFO_CREATURESPRITE", L"Info\\CreatureSprite.inf", "MCreatureSpriteTable", "creature visuals"));
        entries.push_back(Binary(L"CreatureSpriteMapper", L"FILE_INFO_CREATURESPRITE_MAPPER", L"Info\\CreatureSpriteMapper.inf", "MCreatureSpriteMapper", "creature visual resolver"));
        entries.push_back(Binary(L"Zone", L"FILE_INFO_ZONE", L"Info\\Zone.inf", "MZoneTable", "world map resolver"));
        entries.push_back(Binary(L"Action", L"FILE_INFO_ACTION", L"Info\\Action.inf", "MActionInfoTable", "animation/action resolver", InfSchemaRequirement::Optional));
        entries.push_back(Binary(L"Skill", L"FILE_INFO_SKILL", L"Info\\Skill.inf", "MSkillTable", "skill labels/runtime"));
        entries.push_back(Binary(L"SkillInfo", L"FILE_INFO_SKILL_INFO", L"Info\\SkillInfo.inf", "MSkillInfoTable", "skill details/runtime"));
        entries.push_back(Binary(L"EffectStatus", L"FILE_INFO_EFFECTSTATUS", L"Info\\EffectStatus.inf", "MEffectStatusTable", "authoritative status effects"));
        entries.push_back(Binary(L"EffectSpriteType", L"FILE_INFO_EFFECTSPRITETYPE", L"Info\\EffectSpriteType.inf", "MEffectSpriteTypeTable", "visual effects"));
        entries.push_back(Binary(L"NPC", L"FILE_INFO_NPC", L"Info\\NPC.inf", "MNpcTable", "npc names/scripts"));
        entries.push_back(Binary(L"NPCScript", L"FILE_INFO_NPC_SCRIPT_INFO", L"Info\\NPCScript.inf", "MNpcScriptTable", "npc dialog"));
        entries.push_back(Binary(L"ShopTemplate", L"FILE_INFO_SHOPTEMPLATE", L"Info\\ShopTemplate.inf", "MShopTemplateTable", "shop runtime"));
        entries.push_back(Binary(L"InteractionObject", L"FILE_INFO_INTERACTIONOBJECT", L"Info\\InteractionObject.inf", "MInteractionObjectTable", "world interactions"));
        entries.push_back(Binary(L"Sound", L"FILE_INFO_SOUND", L"Info\\Sound.inf", "MSoundTable", "sound catalog"));
        entries.push_back(Binary(L"Music", L"FILE_INFO_MUSIC", L"Info\\music.inf", "MMusicTable", "music catalog"));

        entries.push_back(Updater(L"updateclient", L"Info\\updateclient.inf"));
        entries.push_back(Updater(L"version", L"Info\\version.inf"));
        entries.push_back(Updater(L"resolution", L"Info\\resolution.inf"));
        entries.push_back(Updater(L"updater_layout", L"Info\\updater_layout.inf"));
        entries.push_back(Updater(L"appendpatch", L"Info\\appendpatch.inf"));
        entries.push_back(Updater(L"crashreport", L"Info\\crashreport.inf"));
        entries.push_back(Updater(L"testserver", L"Info\\testserver.inf"));
        entries.push_back(Updater(L"netmarble", L"Info\\netmarble.inf"));
        entries.push_back(InfSchemaEntry{ L"sprite-header", L"", L"Image\\**\\header.inf", InfSchemaFormat::SpriteHeader, InfSchemaRequirement::SpriteHeader, "sprite pack header", "sprite archive metadata" });

        return entries;
    }

    InfSchemaRegistryResult InfSchemaRegistry::Resolve(
        const std::filesystem::path& dataRoot,
        const FileDefCatalogResult* fileDefCatalog) const
    {
        InfSchemaRegistryResult result;
        for (auto schema : BuildDefault())
        {
            std::filesystem::path path;
            if (!schema.fileDefKey.empty() && fileDefCatalog != nullptr && fileDefCatalog->Succeeded())
            {
                path = fileDefCatalog->Resolve(schema.fileDefKey, schema.fallbackRelativePath);
            }
            else
            {
                path = (dataRoot / std::filesystem::path(schema.fallbackRelativePath)).lexically_normal();
            }

            const auto isWildcard = schema.fallbackRelativePath.find(L"**") != std::wstring::npos;
            const auto exists = !isWildcard && std::filesystem::exists(path);
            if (!exists && schema.requirement == InfSchemaRequirement::Required)
            {
                ++result.requiredMissing;
            }
            if (schema.requirement == InfSchemaRequirement::UpdaterOnly)
            {
                ++result.updaterOnlyCount;
            }

            result.entries.push_back(InfSchemaResolvedEntry{ std::move(schema), std::move(path), exists });
        }
        return result;
    }
}
