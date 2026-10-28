#pragma once

#include "Animation/LegacyCreatureAnimation.h"
#include "Protocol/LegacyProtocol.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::runtime
{
    enum class WorldMotionKind : std::uint8_t
    {
        None,
        Walk,
        FastMove,
        KnockBack
    };

    struct WorldMotionState
    {
        std::uint16_t serverX{ 0 };
        std::uint16_t serverY{ 0 };
        float visualX{ 0.0F };
        float visualY{ 0.0F };
        std::uint16_t targetX{ 0 };
        std::uint16_t targetY{ 0 };
        std::uint64_t lastTickMs{ 0 };
        std::int32_t pixelOffsetX{ 0 };
        std::int32_t pixelOffsetY{ 0 };
        std::uint8_t direction{ 0 };
        std::uint16_t moveCount{ 0 };
        std::uint16_t moveCountMax{ 0 };
        std::uint16_t nextMoveCount{ 1 };
        std::uint16_t moveRatio{ 1 };
        std::uint16_t moveTableIndex{ 0 };
        std::uint16_t moveTableCount{ 0 };
        WorldMotionKind kind{ WorldMotionKind::None };
        float specialStartX{ 0.0F };
        float specialStartY{ 0.0F };
        std::uint16_t specialStep{ 0 };
        std::uint16_t specialStepCount{ 0 };
        bool moving{ false };
    };

    struct PendingPlayerMove
    {
        std::uint16_t originX{ 0 };
        std::uint16_t originY{ 0 };
        std::uint8_t originDirection{ 0 };
        std::uint16_t targetX{ 0 };
        std::uint16_t targetY{ 0 };
        std::uint8_t direction{ 0 };
        std::uint64_t sentMs{ 0 };
    };

    struct StatusEffectRuntimeModel
    {
        std::uint16_t effectStatusId{ 0 };
        std::uint16_t actionInfo{ 0xFFFFU };
        std::uint16_t originalActionInfo{ 0xFFFFU };
        std::uint16_t effectSpriteType{ 0xFFFFU };
        std::uint16_t effectColor{ 0xFFFFU };
        std::uint8_t effectColorPart{ 0 };
        std::int32_t soundId{ -1 };
        std::uint32_t sourceObjectId{ 0 };
        std::uint64_t createdMs{ 0 };
        std::uint64_t expireMs{ 0 };
        bool serverOwned{ true };
        bool useEffectSprite{ true };
        bool attachGround{ false };
    };

    struct PlayerRuntimeModel
    {
        protocol::PcType pcType{ protocol::PcType::Slayer };
        std::string name;
        std::uint32_t objectId{ 0 };
        std::uint8_t sex{ 0 };
        std::uint16_t level{ 1 };
        std::uint16_t str{ 0 };
        std::uint16_t maxStr{ 0 };
        std::uint16_t basicStr{ 0 };
        std::uint16_t dex{ 0 };
        std::uint16_t maxDex{ 0 };
        std::uint16_t basicDex{ 0 };
        std::uint16_t intelligence{ 0 };
        std::uint16_t maxIntelligence{ 0 };
        std::uint16_t basicIntelligence{ 0 };
        std::uint32_t strExperienceRemaining{ 0 };
        std::uint32_t dexExperienceRemaining{ 0 };
        std::uint32_t intExperienceRemaining{ 0 };
        std::uint32_t experienceRemaining{ 0 };
        std::uint32_t advancementGoalExperience{ 0 };
        std::uint32_t hp{ 0 };
        std::uint32_t maxHp{ 0 };
        std::uint32_t mp{ 0 };
        std::uint32_t maxMp{ 0 };
        std::array<std::uint8_t, 6> domainLevels{};
        std::array<std::uint32_t, 6> domainExperience{};
        std::array<std::uint32_t, 6> domainGoalExperience{};
        std::uint32_t minimumDamage{ 0 };
        std::uint32_t maximumDamage{ 0 };
        std::uint32_t defense{ 0 };
        std::uint32_t protection{ 0 };
        std::uint32_t toHit{ 0 };
        std::uint32_t elementalFire{ 0 };
        std::uint32_t elementalWater{ 0 };
        std::uint32_t elementalEarth{ 0 };
        std::uint32_t elementalWind{ 0 };
        std::uint16_t bonus{ 0 };
        std::uint16_t skillBonus{ 0 };
        std::uint8_t hairStyle{ 0 };
        std::uint16_t hairColor{ 0 };
        std::uint16_t skinColor{ 0 };
        std::uint16_t masterEffectColor{ 0 };
        std::uint8_t born{ 0 };
        std::int32_t alignment{ 0 };
        std::uint8_t rank{ 0 };
        std::uint32_t rankExperienceRemaining{ 0 };
        std::uint8_t competence{ 0 };
        std::uint16_t guildId{ 0 };
        std::string guildName;
        std::uint8_t guildRank{ 0 };
        std::uint32_t unionId{ 0 };
        std::uint16_t advancementLevel{ 0 };
        std::uint8_t attackSpeed{ 0 };
        std::uint16_t creatureType{ 0 };
        bool master{ false };
        bool operatorMode{ false };
        bool invincible{ false };
        std::uint16_t zoneId{ 0 };
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
        std::uint8_t direction{ 0 };
        std::uint16_t verifiedX{ 0 };
        std::uint16_t verifiedY{ 0 };
        std::uint8_t verifiedDirection{ 0 };
                bool verifiedPositionKnown{ false };
                std::uint32_t fame{ 0 };  // 명성 (Fame)
                std::uint8_t premium{ 0 };
        std::uint8_t nonPk{ 0 };
        std::uint8_t serverStat{ 0 };
        std::uint32_t smsCharge{ 0 };
        protocol::NicknameRecord nickname;
        std::uint32_t guildUnionId{ 0 };
        std::uint8_t guildUnionUserType{ 0 };
        std::uint32_t bloodBibleOpenNum{ 0 };
        std::vector<std::uint16_t> bloodBibleSigns;
        std::int32_t powerjjang{ 0 };
        std::uint32_t realWearingInfo{ 0 };
        bool positionKnown{ false };
        bool dead{ false };
        WorldMotionState motion;
        animation::LegacyCreatureAnimation animation;
        bool pendingMove{ false };
        std::deque<PendingPlayerMove> pendingMoves;
        std::uint16_t pendingMoveOriginX{ 0 };
        std::uint16_t pendingMoveOriginY{ 0 };
        std::uint8_t pendingMoveOriginDirection{ 0 };
        std::uint16_t pendingMoveX{ 0 };
        std::uint16_t pendingMoveY{ 0 };
        std::uint8_t pendingMoveDirection{ 0 };
        std::uint64_t pendingMoveSentMs{ 0 };
        bool queuedMove{ false };
        std::int32_t queuedMoveDx{ 0 };
        std::int32_t queuedMoveDy{ 0 };
        std::uint8_t queuedMoveDirection{ 0 };
        std::uint32_t moveSendCount{ 0 };
        std::uint32_t moveAckCount{ 0 };
        std::uint32_t moveRejectCount{ 0 };
        std::map<std::uint16_t, StatusEffectRuntimeModel> statusEffects;
    };

    struct WorldObjectRuntimeModel
    {
        protocol::WorldObjectKind kind{ protocol::WorldObjectKind::Player };
        protocol::PcType pcType{ protocol::PcType::Slayer };
        std::uint32_t objectId{ 0 };
        std::string name;
        std::string halluName;
        std::uint16_t typeId{ 0 };
        std::uint16_t spriteType{ 0 };
        std::uint16_t mainColor{ 0 };
        std::uint16_t subColor{ 0 };
        std::uint32_t outlook{ 0 };
        std::array<std::uint8_t, 7> slayerWearTypes{};
        std::array<std::uint16_t, 10> slayerColors{};
        std::array<std::uint16_t, 8> vampireColors{};
        std::array<std::uint16_t, 6> oustersColors{};
        std::uint8_t hairStyle{ 0 };
        std::uint16_t hairColor{ 0 };
        std::uint16_t skinColor{ 0 };
        std::uint16_t masterEffectColor{ 0 };
        std::uint8_t coatType{ 0 };
        std::uint8_t armType{ 0 };
        std::uint8_t shape{ 0 };
        std::uint8_t sylphType{ 0 };
        std::uint8_t batType{ 0 };
        std::uint8_t wingSylphType{ 0 };
        std::uint16_t wingItemType{ 0 };
        std::uint16_t motorItemType{ 0 };
        std::uint8_t born{ 0 };
        std::uint8_t attackSpeed{ 0 };
        std::int32_t alignment{ 0 };
        std::uint8_t competence{ 0 };
        std::uint16_t guildId{ 0 };
        std::uint32_t unionId{ 0 };
        std::uint8_t advancementLevel{ 0 };
        std::int32_t contributePoint{ 0 };
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
        std::uint8_t direction{ 0 };
        std::uint8_t sex{ 0 };
        std::uint8_t rank{ 0 };
        std::uint8_t fromFlag{ 0 };
        std::uint32_t hp{ 0 };
        std::uint32_t maxHp{ 0 };
        WorldMotionState motion;
        animation::LegacyCreatureAnimation animation;
        std::optional<PendingPlayerMove> nextMove;
        std::deque<PendingPlayerMove> moveBuffer;
        bool dead{ false };
        bool isSummonedPet{ false };
        protocol::PetInfoRecord petInfo;
        std::map<std::uint16_t, StatusEffectRuntimeModel> statusEffects;
    };

    enum class ItemStorageKind : std::uint8_t
    {
        Unknown,
        Inventory,
        Gear,
        Extra,
        Motorcycle,
        Ground,
        Stash
    };

    struct ItemRuntimeRecord
    {
        ItemStorageKind storageKind{ ItemStorageKind::Unknown };
        std::uint32_t ownerId{ 0 };
        std::uint8_t storage{ 0 };
        std::uint32_t storageId{ 0 };
        std::uint32_t objectId{ 0 };
        std::uint8_t itemClass{ 0 };
        std::uint16_t itemType{ 0 };
        std::uint8_t itemNum{ 0 };
        std::uint32_t durability{ 0 };
        std::uint16_t silver{ 0 };
        std::uint32_t grade{ 0 };
        std::uint8_t enchantLevel{ 0 };
        std::uint16_t mainColor{ 0 };
        std::uint8_t slotOrX{ 0 };
        std::uint8_t y{ 0 };
        bool gearActive{ false };
        std::uint16_t dropFrameId{ 0xFFFFU };
        std::uint16_t inventoryFrameId{ 0xFFFFU };
        std::uint16_t gearFrameId{ 0xFFFFU };
        std::uint16_t addonMaleFrameId{ 0xFFFFU };
        std::uint16_t addonFemaleFrameId{ 0xFFFFU };
        std::uint16_t useFrameId{ 0xFFFFU };
        std::uint16_t descriptionFrameId{ 0U };
        std::uint8_t gridWidth{ 1 };
        std::uint8_t gridHeight{ 1 };
        std::uint32_t maxNumber{ 1 };
        std::uint32_t price{ 0 };
        std::uint16_t weight{ 0 };
        std::array<std::int32_t, 7> values{};
        std::uint8_t requireStr{ 0 };
        std::uint8_t requireDex{ 0 };
        std::uint8_t requireInt{ 0 };
        std::uint16_t requireSum{ 0 };
        std::uint8_t requireLevel{ 0 };
        std::uint8_t requireAdvancementLevel{ 0 };
        bool maleOnly{ false };
        bool femaleOnly{ false };
        std::uint32_t useActionInfo{ 0 };
        std::int32_t silverMax{ 0 };
        std::int32_t toHit{ 0 };
        std::int32_t criticalHit{ 0 };
        std::vector<std::uint16_t> defaultOptions;
        std::vector<std::uint16_t> options;
        std::vector<std::uint16_t> g2Options;
        std::vector<std::uint16_t> thirdOptions;
        std::vector<std::uint16_t> mixOptions;
        std::vector<protocol::SubItemRecord> subItems;
        std::size_t unknownOptionCount{ 0 };
        std::uint8_t thirdEnchantType{ 0 };
        std::uint8_t addedInfo1{ 0 };
        std::uint8_t addedInfo2{ 0 };
        std::uint8_t addedInfo3{ 0 };
        std::int32_t itemStyle{ 0 };
        std::int32_t elementalType{ -1 };
        std::uint16_t elemental{ 0 };
        std::uint8_t race{ 0 };
        std::uint8_t itemMoveControl{ 0 };
        std::uint8_t itemCanAdvance{ 0 };
        std::uint32_t dropItemNameTag{ 0 };
        std::int32_t normalItemGrade{ 0 };
        std::int32_t requiredTranscendenceLevel{ 0 };
        std::string displayName;
        std::string optionSummary;
    };

    enum class InventoryDragSourceKind : std::uint8_t
    {
        None,
        Inventory,
        Gear,
        Ground
    };

    enum class InventoryMouseOperation : std::uint8_t
    {
        None,
        PickUpInventory,
        PlaceInventory,
        CombineInventory,
        PickUpGear,
        PlaceGear,
        PickUpGround,
        DropZone,
        PlaceRankGem
    };

    struct InventoryMouseItemState
    {
        bool active{ false };
        bool waitingServer{ false };
        InventoryDragSourceKind source{ InventoryDragSourceKind::None };
        InventoryMouseOperation operation{ InventoryMouseOperation::None };
        ItemRuntimeRecord item;
        std::uint32_t sourceObjectId{ 0 };
        std::uint8_t sourceX{ 0 };
        std::uint8_t sourceY{ 0 };
        std::uint8_t sourceSlot{ 0 };
        std::uint64_t requestedMs{ 0 };
        std::string statusText;
    };

    struct GroundItemRuntimeModel
    {
        ItemRuntimeRecord item;
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
        std::uint64_t createdMs{ 0 };
        std::uint32_t dropPetObjectId{ 0 };
    };

    enum class WorldEffectAttachment : std::uint8_t
    {
        Object,
        Tile
    };

    struct EffectRuntimeModel
    {
        std::uint64_t runtimeId{ 0 };
        WorldEffectAttachment attachment{ WorldEffectAttachment::Object };
        std::uint32_t objectId{ 0 };
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
        std::uint16_t effectId{ 0 };
        std::uint64_t createdMs{ 0 };
        std::uint64_t expireMs{ 0 };
    };

    enum class ChatRuntimeKind : std::uint8_t
    {
        System,
        SystemOperator,
        SystemEvent,
        SystemTimer,
        SystemRanger,
        Local,
        Global,
        Whisper,
        Party,
        Guild,
        Union,
        Npc
    };

    struct ChatRuntimeLine
    {
        ChatRuntimeKind kind{ ChatRuntimeKind::System };
        std::uint32_t objectId{ 0 };
        std::uint32_t color{ 0xFFFFFFFFU };
        std::uint8_t race{ 0 };
        std::uint8_t systemType{ 0 };
        std::uint8_t channelCondition{ 0 };
        std::string senderName;
        std::string text;
        std::uint64_t createdMs{ 0 };
        std::uint64_t expireMs{ 0 };
    };

    struct LegacyMessageLine
    {
        std::string text;
        std::uint32_t color{ 0xFFFFFFFFU };
        std::uint64_t createdMs{ 0 };
    };

    struct LegacyInformationBoard
    {
        bool visible{ false };
        std::string score;
        std::array<std::string, 3> vampire;
        std::array<std::string, 3> slayer;
        std::array<std::string, 3> ousters;
    };

    struct FloatingDamageRuntimeModel
    {
        std::uint32_t objectId{ 0 };
        std::uint16_t damage{ 0 };
        std::uint64_t createdMs{ 0 };
        std::uint64_t expireMs{ 0 };
    };

    struct NpcDialogRuntimeModel
    {
        bool active{ false };
        std::uint32_t objectId{ 0 };
        std::uint32_t scriptId{ 0 };
        std::uint16_t npcId{ 0 };
        std::uint16_t creatureType{ 0 };
        std::string title;
        std::vector<std::string> options;
        std::vector<std::uint8_t> answerIds;
        std::vector<protocol::NpcScriptParameter> parameters;
        std::uint64_t createdMs{ 0 };
    };

    struct ShopRuntimeModel
    {
        bool active{ false };
        std::uint32_t objectId{ 0 };
        std::uint32_t npcId{ 0 };
        std::uint8_t rackType{ 0 };
        std::int32_t version{ 0 };
        std::array<std::int32_t, 3> rackVersions{};
        std::int16_t marketCondBuy{ 0 };
        std::int16_t marketCondSell{ 0 };
        std::uint8_t shopType{ 0 };
        bool mysterious{ false };
        std::array<protocol::ShopItemInfo, 20> rack{};
        std::string statusText;
        std::uint64_t updatedMs{ 0 };
    };

    struct TradeRuntimeModel
    {
        bool active{ false };
        std::uint32_t targetObjectId{ 0 };
        std::uint8_t prepareCode{ 0 };
        std::uint8_t finishCode{ 0 };
        std::uint8_t verifyCode{ 0 };
        std::uint8_t errorCode{ 0 };
        std::uint32_t myMoney{ 0 };
        std::uint32_t targetMoney{ 0 };
        std::map<std::uint32_t, ItemRuntimeRecord> targetItems;
        std::string statusText;
        std::uint64_t updatedMs{ 0 };
    };

    struct StashRuntimeModel
    {
        bool active{ false };
        std::uint8_t stashNum{ 0 };
        std::uint32_t gold{ 0 };
        std::uint32_t sellPrice{ 0 };
        std::map<std::uint16_t, ItemRuntimeRecord> itemsByRackSlot;
        std::string statusText;
        std::uint64_t updatedMs{ 0 };
    };

    struct InitialNpcSeed
    {
        std::string name;
        std::uint16_t npcId{ 0 };
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
        bool hidden{ false };
    };

    enum class WorldInteractionKind : std::uint8_t
    {
        None,
        GroundItem,
        Creature,
        Npc,
        InteractionObject,
        Player
    };

    struct WorldInteractionSelection
    {
        WorldInteractionKind kind{ WorldInteractionKind::None };
        std::uint32_t objectId{ 0 };
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
        std::string label;
    };

    struct WorldRuntimeModel
    {
        std::map<std::uint32_t, protocol::GQuestStatusInfo> quests;
        std::vector<std::uint16_t> questInventoryItemTypes;
        std::uint16_t zoneId{ 0 };
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
        protocol::GameTimeInfo time;
        std::uint8_t weather{ 0 };
        std::uint8_t weatherLevel{ 0 };
        std::uint8_t darkLevel{ 0 };
        std::uint8_t lightLevel{ 0 };
        std::string zoneName;
        std::string mapPath;
        std::uint16_t mapWidth{ 0 };
        std::uint16_t mapHeight{ 0 };
        std::size_t mapTileCount{ 0 };
        std::size_t mapObjectCount{ 0 };
        std::uint64_t mapSizeBytes{ 0 };
        std::size_t mapBytesConsumed{ 0 };
        std::size_t mapNonZeroPropertySectorCount{ 0 };
        std::size_t mapLitSectorCount{ 0 };
        bool mapLoaded{ false };
        std::size_t npcTypeCount{ 0 };
        std::size_t monsterTypeCount{ 0 };
        std::size_t npcInfoCount{ 0 };
        std::vector<std::uint16_t> npcSpriteTypes;
        std::vector<std::uint16_t> monsterSpriteTypes;
        std::size_t skillCatalogCount{ 0 };
        std::size_t npcCatalogCount{ 0 };
        std::size_t shopTemplateCatalogCount{ 0 };
        std::size_t interactionObjectCatalogCount{ 0 };
        std::size_t npcScriptCatalogCount{ 0 };
        std::vector<InitialNpcSeed> initialNpcSeeds;
        std::map<std::uint32_t, WorldObjectRuntimeModel> objects;
        std::map<std::uint32_t, WorldObjectRuntimeModel> fakeCreatures;
        std::uint32_t nextFakeCreatureId{ 0 };
        std::map<std::uint32_t, GroundItemRuntimeModel> groundItems;
        std::map<std::uint32_t, ItemRuntimeRecord> inventoryItems;
        std::map<std::uint32_t, ItemRuntimeRecord> gearItems;
        std::uint16_t currentGearSlotId{ 0 };
        bool currentGearSlotActive{ false };
        std::map<std::uint32_t, ItemRuntimeRecord> extraItems;
        std::map<std::uint32_t, ItemRuntimeRecord> motorcycleItems;
        InventoryMouseItemState mouseItem;
        std::map<std::uint8_t, protocol::GearToInventoryInfo> movedGearSlots;
        std::vector<EffectRuntimeModel> effects;
        std::size_t statusEffectPacketCount{ 0 };
        std::size_t unresolvedStatusEffectCount{ 0 };
        std::deque<ChatRuntimeLine> chatLines;
        std::vector<FloatingDamageRuntimeModel> floatingDamage;
        NpcDialogRuntimeModel npcDialog;
        ShopRuntimeModel shop;
        TradeRuntimeModel trade;
        StashRuntimeModel stash;
        bool hasMotorcycle{ false };
        std::uint8_t serverStat{ 0 };
        std::uint32_t smsCharge{ 0 };
        protocol::NicknameRecord nickname;
        std::uint8_t guildUnionUserType{ 0 };
        std::uint32_t bloodBibleOpenNum{ 0 };
        std::vector<std::uint16_t> bloodBibleSigns;
        std::int32_t powerjjang{ 0 };
        std::uint8_t skillInfoPcType{ 0 };
        std::size_t skillInfoCount{ 0 };
        protocol::SkillInfoSummary skillInfo;
        std::vector<std::uint8_t> skillInfoRaw;
        std::vector<std::uint32_t> availableRankBonusTypes;
        std::uint32_t selectedRankBonusType{ 0 };
        std::uint8_t rankBonusFailureCode{ 0 };
        std::uint32_t realWearingInfo{ 0 };
        protocol::PetInfoRecord petInfo;
        std::vector<std::uint8_t> petInfoRaw;
        protocol::ModifyInfoRecord lastUseModifyInfo;
        std::vector<std::uint8_t> lastUseModifyRaw;
        std::size_t useItemOkCount{ 0 };
        std::vector<std::uint8_t> weekItemListRaw;
        std::vector<std::uint8_t> friendListRaw;
        std::vector<std::uint8_t> friendWaitListRaw;
        std::size_t supportPacketCount{ 0 };
        std::size_t addPacketCount{ 0 };
        std::size_t movePacketCount{ 0 };
        std::size_t pendingObjectMovePacketCount{ 0 };
        std::size_t pendingObjectMoveApplyCount{ 0 };
        std::size_t pendingObjectMoveDropCount{ 0 };
        std::size_t deletePacketCount{ 0 };
        std::size_t groundItemPacketCount{ 0 };
        std::size_t inventoryPacketCount{ 0 };
        std::size_t effectPacketCount{ 0 };
        std::size_t chatPacketCount{ 0 };
        std::size_t damagePacketCount{ 0 };
        std::size_t malformedGameplayPacketCount{ 0 };
        std::size_t unsupportedPacketCount{ 0 };
        std::size_t interactionPacketCount{ 0 };
        std::size_t pickupRequestCount{ 0 };
        std::size_t attackRequestCount{ 0 };
        std::size_t npcTalkRequestCount{ 0 };
        std::size_t interpolatingObjectCount{ 0 };
        std::size_t systemMessageCount{ 0 };
        std::uint64_t nextEffectRuntimeId{ 1 };
        WorldInteractionSelection selectedInteraction;
        WorldInteractionSelection hoveredInteraction;
        std::map<std::uint32_t, protocol::MoveInfo> pendingObjectMoves;
        bool partyActive{ false };
        std::vector<std::string> partyMembers;
        std::deque<LegacyMessageLine> systemMessageRing;
        std::deque<LegacyMessageLine> gmMessageRing;
        std::deque<LegacyMessageLine> eventMessageRing;
        std::deque<LegacyMessageLine> worldChatMessageRing;
        std::deque<LegacyMessageLine> noticeMessageRing;
        std::string eventMessageCleaner;
        std::string previousSystemMessage;
        std::string rangerChatMessage;
        std::string timerDescription;
        std::uint64_t timerExpireMs{ 0 };
        LegacyInformationBoard informationBoard;
    };

    struct GameSessionModel
    {
        std::uint8_t worldId{ 0 };
        std::uint8_t serverGroupId{ 0 };
        std::uint8_t gameServerId{ 0 };
        std::uint8_t loginServerId{ 0 };
        std::uint32_t authKey{ 0 };
        bool updateInfoReceived{ false };
        bool readySent{ false };
        bool encryptionEnabled{ false };
        std::uint8_t encryptionCode{ 0 };
        PlayerRuntimeModel player;
        WorldRuntimeModel world;
        std::deque<std::string> systemMessages;
    };
}
