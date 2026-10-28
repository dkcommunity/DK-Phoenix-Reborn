#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::protocol
{
    enum class PacketId : std::uint16_t
    {
        CGAttack = 0,
        CGAddGearToMouse = 1,
        CGAddInventoryToMouse = 2,
        CGAddMouseToGear = 3,
        CGAddMouseToInventory = 4,
        CGAddMouseToZone = 6,
        CGAddZoneToMouse = 8,
        CGAddZoneToInventory = 9,
        CGConnect = 10,
        CGLearnSkill = 12,
        CGLogout = 13,
        CGMove = 14,
        CGReady = 15,
        CGSay = 19,
        CGSkillToInventory = 20,
        CGCastingSkill = 37,
        CGUseItemFromInventory = 269,
        CGUseItemFromGear = 377,
        CGResurrect = 282,
        CGUseItemFromGQuestInventory = 444,
        CGSelectGuild = 315,
        CGRequestGuildMemberList = 328,
        CGRequestGuildList = 446,
        CGRequestStoreInfo = 470,
        CGRequestWebMarket = 485,
        CGRequestTmallList = 537,
        CGSelectQuest = 381,
        CGFailQuest = 399,
        CGGQuestAccept = 436,
        CGGQuestCancel = 437,
        CGNPCTalk = 40,
        CGNPCAskAnswer = 138,
        CGTradePrepare = 207,
        CGTradeAddItem = 208,
        CGTradeRemoveItem = 209,
        CGTradeMoney = 210,
        CGTradeFinish = 211,
        CGStashList = 200,
        CGAddItemToItem = 349,
        CGShopRequestList = 139,
        CGShopRequestBuy = 140,
        CGShopRequestSell = 150,
        CGVerifyTime = 273,
        CGUseMessageItemFromInventory = 368,
        CGRequestRankGemData = 541,
        GCSendRankGemData = 542,
        CGUseRankGemFromInventory = 543,
        CGRemoveRankGem = 544,
        CLReconnectLogin = 250,
        CLLogin = 27,
        CLRegisterPlayer = 30,
        CLSelectPC = 31,
        CGGlobalChat = 32,
        GCGlobalChat = 33,
        GCSystemMessage = 34,
        GCReconnectLogin = 251,
        GCUseOK = 35,
        GCNPCAsk = 41,
        GCNPCSay = 42,
        GCCannotAdd = 64,
        GCShopVersion = 141,
        GCShopList = 142,
        GCShopBuyOK = 143,
        GCShopBuyFail = 144,
        GCShopSold = 145,
        GCShopSellFail = 151,
        GCShopSellOK = 152,
        GCShopBought = 153,
        GCShopListMysterious = 165,
        GCShopMarketCondition = 166,
        GCTradePrepare = 212,
        GCTradeAddItem = 213,
        GCTradeRemoveItem = 214,
        GCTradeMoney = 215,
        GCTradeFinish = 216,
        GCTradeError = 217,
        GCTradeVerify = 218,
        CGSkillToObject = 219,
        CGSkillToSelf = 220,
        CGSkillToTile = 221,
        GCStashList = 201,
        GCStashSell = 205,
        GCAddNewItemToZone = 49,
        GCAddMonster = 47,
        GCAddMonsterCorpse = 48,
        GCAddNPC = 50,
        GCAddSlayer = 51,
        GCAddSlayerCorpse = 52,
        GCAddVampire = 53,
        GCAddVampireCorpse = 54,
        GCAttack = 63,
        GCChangeDarkLight = 67,
        GCChangeWeather = 68,
        GCCreatureDied = 69,
        GCDeleteObject = 70,
        GCDeleteAndPickUpOK = 71,
        GCGetDamage = 73,
        GCLearnSkillFailed = 77,
        GCLearnSkillOK = 78,
        GCLearnSkillReady = 79,
        GCModifyInformation = 81,
        GCMove = 82,
        GCMoveError = 83,
        GCMoveOK = 84,
        GCSay = 89,
        GCSetPosition = 90,
        GCUpdateInfo = 109,
        LCLoginError = 116,
        LCLoginOK = 117,
        LCPCList = 118,
        LCReconnect = 120,
        LCRegisterPlayerError = 121,
        LCRegisterPlayerOK = 122,
        GCBloodDrainOK3 = 128,
        GCAddGearToInventory = 129,
        GCDropItemToZone = 133,
        GCCreateItem = 137,
        GCKnocksTargetBackOK1 = 146,
        GCKnocksTargetBackOK2 = 147,
        GCKnocksTargetBackOK4 = 148,
        GCKnocksTargetBackOK5 = 149,
        GCNPCResponse = 262,
        GCNPCAskDynamic = 266,
        GCNPCSayDynamic = 267,
        GCRemoveEffect = 154,
        GCAddVampireFromBurrowing = 155,
        GCAddMonsterFromBurrowing = 156,
        GCAddBurrowingCreature = 157,
        GCUnburrowOK = 159,
        GCAddEffect = 161,
        CGPhoneSay = 170,
        CGWhisper = 171,
        GCPhoneSay = 176,
        GCWhisper = 177,
        GCWhisperFailed = 178,
        GCSkillInfo = 181,
        GCAddEffectToTile = 182,
        GCStatusCurrentHP = 185,
        GCRealWearingInfo = 189,
        GCRemoveFromGear = 190,
        GCDeleteEffectFromTile = 197,
        GCDeleteInventoryItem = 238,
        GCKnockBack = 206,
        GCAddWolf = 222,
        GCAddBat = 223,
        GCAddMonsterFromTransformation = 224,
        GCAddVampireFromTransformation = 225,
        GCFastMove = 229,
        GCUntransformOK = 231,
        GCEnterVampirePortal = 274,
        GCFakeMove = 355,
        GCPartyInvite = 278,
        GCPartyLeave = 279,
        GCPartyJoined = 280,
        GCPartyError = 281,
        GCAddItemToItemVerify = 350,
        CGGuildChat = 345,
        GCGuildChat = 346,
        CGSkillToNamed = 360,
        CGSelectRankBonus = 363,
        GCSelectRankBonusOK = 364,
        GCSelectRankBonusFailed = 365,
        GCRankBonusInfo = 366,
        GCAddOusters = 390,
        GCPetInfo = 408,
        GCSelectQuestID = 380,
        GCQuestStatus = 382,
        GCMonsterKillQuestInfo = 383,
        GCGQuestStatusInfo = 434,
        GCGQuestStatusModify = 435,
        GCGQuestInventory = 439,
        CGPartySay = 415,
        GCPartySay = 416,
        CGWithdrawPet = 419,
        CGDepositPet = 420,
        GCNPCInfo = 370,
        GCNPCAskVariable = 371,
        CGChangeGearSlot = 487,
        GCChangeGearSlot = 488,
        GCUpdateWeekItemList = 495,
        GCFriendList = 511,
        GCFriendWaitList = 512,
        CGRangerSay = 463,
        RCSay = 290,
        CRWhisper = 292,
        LCServerList = 248,
        CLGetWorldList = 300,
        LCWorldList = 301,
        CLSelectWorld = 302,
        CLSelectServer = 303,
        CGRequestPing = 545
    };

    enum class PcType : std::uint8_t
    {
        Slayer = 0,
        Vampire = 1,
        Ousters = 2
    };

    struct PacketFrame
    {
        PacketId id{ PacketId::CLLogin };
        std::vector<std::uint8_t> body;
    };

    struct GQuestMissionInfo
    {
        std::uint8_t condition{ 0 };
        std::uint16_t index{ 0 };
        std::uint8_t status{ 0 };
        std::string stringArgument;
        std::uint32_t numericArgument{ 0 };
    };

    struct GQuestStatusInfo
    {
        std::uint32_t questId{ 0 };
        std::uint8_t status{ 0 };
        std::vector<GQuestMissionInfo> missions;
    };

    struct GQuestStatusList
    {
        std::vector<GQuestStatusInfo> quests;
    };

    struct GQuestStatusModify
    {
        std::uint8_t type{ 0 };
        GQuestStatusInfo quest;
    };

    struct GQuestInventory
    {
        std::vector<std::uint16_t> itemTypes;
    };

    struct LoginOk
    {
        bool isAdult{ false };
        bool underFifteen{ false };
        bool family{ false };
        std::uint8_t stat{ 0 };
        std::uint16_t serverId{ 0 };
        std::uint16_t serverIdA{ 0 };
        std::uint16_t serverIdB{ 0 };
        std::uint16_t serverIdC{ 0 };
    };

    struct LoginError
    {
        std::uint8_t errorId{ 0 };
        std::wstring message;
    };

    struct RegisterPlayerOk
    {
        std::string groupName;
        bool isAdult{ false };
    };

    struct RegisterPlayerError
    {
        std::uint8_t errorId{ 0 };
    };

    struct WorldInfo
    {
        std::uint8_t id{ 0 };
        std::string name;
        std::uint8_t status{ 0 };
    };

    struct WorldList
    {
        std::uint8_t currentWorldId{ 0 };
        std::vector<WorldInfo> worlds;
    };

    struct ServerGroupInfo
    {
        std::uint8_t id{ 0 };
        std::string name;
        std::uint8_t status{ 0 };
        bool nonPk{ false };
        std::uint16_t slayerCount{ 0 };
        std::uint16_t vampireCount{ 0 };
        std::uint16_t oustersCount{ 0 };
    };

    struct ServerList
    {
        std::uint8_t currentServerGroupId{ 0 };
        std::vector<ServerGroupInfo> servers;
    };

    struct CharacterInfo
    {
        bool occupied{ false };
        std::uint8_t slot{ 0 };
        PcType pcType{ PcType::Slayer };
        std::string name;
        std::uint16_t level{ 1 };
    };

    struct CharacterList
    {
        std::array<CharacterInfo, 3> slots;
    };

    struct ReconnectInfo
    {
        std::string gameServerIp;
        std::uint32_t gameServerPort{ 0 };
        std::uint32_t authKey{ 0 };
    };

    struct LoginReconnectInfo
    {
        std::string loginServerIp;
        std::uint32_t loginServerPort{ 0 };
        std::uint32_t authKey{ 0 };
    };

    struct AttributeTriplet
    {
        std::uint16_t current{ 0 };
        std::uint16_t maximum{ 0 };
        std::uint16_t basic{ 0 };
    };

    struct GameTimeInfo
    {
        std::uint16_t year{ 0 };
        std::uint8_t month{ 0 };
        std::uint8_t day{ 0 };
        std::uint8_t hour{ 0 };
        std::uint8_t minute{ 0 };
        std::uint8_t second{ 0 };
    };

    struct SubItemRecord
    {
        std::uint32_t objectId{ 0 };
        std::uint8_t itemClass{ 0 };
        std::uint16_t itemType{ 0 };
        std::uint8_t itemNum{ 0 };
        std::uint8_t slotId{ 0 };
    };

    struct RawItemRecord
    {
        std::uint32_t objectId{ 0 };
        std::uint8_t itemClass{ 0 };
        std::uint16_t itemType{ 0 };
        std::uint32_t durability{ 0 };
        std::uint16_t silver{ 0 };
        std::uint32_t grade{ 0 };
        std::uint8_t enchantLevel{ 0 };
        std::uint8_t itemNum{ 0 };
        std::uint16_t mainColor{ 0 };
        std::vector<std::uint16_t> options;
        std::vector<std::uint16_t> g2Options;
        std::vector<std::uint16_t> thirdOptions;
        std::vector<std::uint16_t> mixOptions;
        std::vector<SubItemRecord> subItems;
        std::uint8_t thirdEnchantType{ 0 };
        std::uint8_t addedInfo1{ 0 };
        std::uint8_t addedInfo2{ 0 };
        std::uint8_t addedInfo3{ 0 };
        std::uint8_t slotOrX{ 0 };
        std::uint8_t y{ 0 };
        bool gearActive{ false };
        std::vector<std::uint8_t> raw;
    };

    struct EffectRecord
    {
        std::uint16_t effectId{ 0 };
        std::uint32_t remainingTime{ 0 };
    };

    struct NicknameRecord
    {
        std::uint16_t id{ 0 };
        std::uint8_t type{ 0 };
        std::uint16_t index{ 0 };
        std::string text;
    };

    struct PetInfoRecord
    {
        bool present{ false };
        std::uint8_t petType{ 0 };
        std::uint16_t creatureType{ 0 };
        std::uint8_t level{ 0 };
        std::uint32_t experience{ 0 };
        std::uint32_t hp{ 0 };
        std::uint8_t attr{ 0 };
        std::uint8_t attrLevel{ 0 };
        std::uint16_t option{ 0 };
        std::uint16_t option2{ 0 };
        std::uint16_t option3{ 0 };
        std::uint16_t foodType{ 0 };
        bool canGamble{ false };
        bool canCutHead{ false };
        bool canAttack{ false };
        bool summonInfo{ false };
        std::string nickname;
        std::uint32_t itemObjectId{ 0 };
        std::int32_t magicRemainSec{ 0 };
        std::int32_t expertPetChangerRemainSec{ 0 };
        std::uint16_t enchantSkillType{ 0 };
        std::vector<std::uint16_t> options;
        std::uint8_t functionPetfoodType{ 0 };
        std::vector<std::uint16_t> mixOptions;
        std::uint32_t summonObjectId{ 0 };
        std::vector<std::uint8_t> raw;
    };

    enum class WorldObjectKind : std::uint8_t
    {
        Player,
        Monster,
        Npc
    };

    struct WorldObjectInfo
    {
        WorldObjectKind kind{ WorldObjectKind::Player };
        PcType pcType{ PcType::Slayer };
        std::uint32_t objectId{ 0 };
        std::string name;
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
        std::uint32_t currentHp{ 0 };
        std::uint32_t maxHp{ 0 };
        bool dead{ false };
        std::vector<EffectRecord> effects;
        PetInfoRecord petInfo;
        NicknameRecord nickname;
        std::vector<std::uint8_t> raw;
    };

    struct MoveInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
        std::uint8_t direction{ 0 };
    };

    struct FastMoveInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint8_t fromX{ 0 };
        std::uint8_t fromY{ 0 };
        std::uint8_t toX{ 0 };
        std::uint8_t toY{ 0 };
        std::uint16_t skillType{ 0 };
    };

    struct KnockBackInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint16_t originX{ 0 };
        std::uint16_t originY{ 0 };
        std::uint16_t targetX{ 0 };
        std::uint16_t targetY{ 0 };
    };

    struct ObjectPositionInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
    };

    struct DeleteObjectInfo
    {
        std::uint32_t objectId{ 0 };
    };

    struct StatusCurrentHP
    {
        std::uint32_t objectId{ 0 };
        std::uint32_t currentHp{ 0 };
    };

    struct SystemMessage
    {
        std::string message;
        std::uint32_t color{ 0 };
        std::uint8_t type{ 0 };
    };

    struct ChatMessage
    {
        std::uint32_t objectId{ 0 };
        std::uint32_t color{ 0xFFFFFFFFU };
        std::uint8_t race{ 0 };
        std::uint8_t guildChatType{ 0 };
        std::string senderName;
        std::string guildName;
        std::string message;
        bool global{ false };
    };

    struct PartyMemberInfo
    {
        std::string name;
        std::uint8_t sex{ 0 };
        std::uint8_t hairStyle{ 0 };
        std::uint32_t ip{ 0 };
    };

    struct PartyJoinedInfo
    {
        std::vector<PartyMemberInfo> members;
    };

    struct PartyLeaveInfo
    {
        std::string expeller;
        std::string expellee;
    };

    struct RankBonusInfo
    {
        std::vector<std::uint32_t> rankBonusTypes;
    };

    struct RankBonusSelectionResult
    {
        std::uint32_t rankBonusType{ 0 };
        std::uint8_t failureCode{ 0 };
    };

    struct NpcScriptParameter
    {
        std::string name;
        std::string value;
    };

    struct NpcDialogInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint32_t scriptId{ 0 };
        std::uint16_t npcId{ 0 };
        std::uint8_t subjectId{ 0 };
        std::string subject;
        std::string message;
        std::vector<std::string> contents;
        std::vector<std::uint8_t> answerIds;
        std::vector<NpcScriptParameter> parameters;
        std::uint8_t responseCode{ 0 };
        std::uint32_t responseParameter{ 0 };
        bool hasResponseParameter{ false };
        std::vector<std::uint8_t> raw;
    };

    struct ShopItemInfo
    {
        bool present{ false };
        std::uint8_t rackIndex{ 0 };
        std::uint32_t objectId{ 0 };
        std::uint8_t itemClass{ 0 };
        std::uint16_t itemType{ 0 };
        std::vector<std::uint16_t> options;
        std::uint32_t durability{ 0 };
        std::uint16_t silver{ 0 };
        std::uint32_t grade{ 0 };
        std::uint8_t enchantLevel{ 0 };
        std::uint8_t itemNum{ 0 };
        std::uint32_t price{ 0 };
        bool mysterious{ false };
    };

    struct ShopVersionInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint32_t npcId{ 0 };
        std::array<std::int32_t, 3> versions{};
        std::int16_t marketCondSell{ 0 };
    };

    struct ShopListInfo
    {
        std::uint32_t objectId{ 0 };
        std::int32_t version{ 0 };
        std::uint8_t rackType{ 0 };
        std::int16_t marketCondBuy{ 0 };
        std::int16_t marketCondSell{ 0 };
        std::uint8_t shopType{ 0 };
        bool mysterious{ false };
        std::array<ShopItemInfo, 20> rack{};
        std::vector<std::uint8_t> raw;
    };

    struct ShopBuyOkInfo
    {
        std::uint32_t objectId{ 0 };
        std::int32_t version{ 0 };
        ShopItemInfo item;
        std::uint32_t price{ 0 };
    };

    struct ShopBuyFailInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint8_t code{ 0 };
        std::uint32_t amount{ 0 };
    };

    struct ShopSellOkInfo
    {
        std::uint32_t objectId{ 0 };
        std::int32_t version{ 0 };
        std::uint32_t itemObjectId{ 0 };
        std::uint32_t price{ 0 };
    };

    struct ShopRackDeltaInfo
    {
        std::uint32_t objectId{ 0 };
        std::int32_t version{ 0 };
        std::uint8_t rackType{ 0 };
        std::uint8_t rackIndex{ 0 };
        ShopItemInfo item;
    };

    struct ShopMarketConditionInfo
    {
        std::uint32_t objectId{ 0 };
        std::int16_t marketCondBuy{ 0 };
        std::int16_t marketCondSell{ 0 };
    };

    struct ModifyShortData
    {
        std::uint8_t type{ 0 };
        std::uint16_t value{ 0 };
    };

    struct ModifyLongData
    {
        std::uint8_t type{ 0 };
        std::uint32_t value{ 0 };
    };

    struct ModifyInfoRecord
    {
        std::vector<ModifyShortData> shorts;
        std::vector<ModifyLongData> longs;
        std::vector<std::uint8_t> raw;
    };

    struct KnockTargetBackInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint32_t targetObjectId{ 0 };
        std::uint16_t skillType{ 0 };
        std::uint8_t direction{ 0 };
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
        std::uint8_t bullet{ 0 };
        bool success{ true };
        ModifyInfoRecord modify;
    };

    struct TradeCodeInfo
    {
        std::uint32_t targetObjectId{ 0 };
        std::uint8_t code{ 0 };
    };

    struct TradeMoneyInfo
    {
        std::uint32_t targetObjectId{ 0 };
        std::uint32_t amount{ 0 };
        std::uint8_t code{ 0 };
    };

    struct TradeItemInfo
    {
        std::uint32_t targetObjectId{ 0 };
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
        RawItemRecord item;
        std::uint8_t cashItem{ 0 };
    };

    struct TradeRemoveItemInfo
    {
        std::uint32_t targetObjectId{ 0 };
        std::uint32_t itemObjectId{ 0 };
    };

    struct TradeVerifyInfo
    {
        std::uint8_t code{ 0 };
    };

    struct StashItemInfo
    {
        std::uint8_t rack{ 0 };
        std::uint8_t index{ 0 };
        RawItemRecord item;
        std::uint8_t cashItem{ 0 };
    };

    struct StashListInfo
    {
        std::uint8_t stashNum{ 0 };
        std::uint8_t totalItemCount{ 0 };
        std::uint32_t gold{ 0 };
        std::vector<StashItemInfo> items;
        std::vector<std::uint8_t> raw;
    };

    struct ZoneItemInfo
    {
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
        RawItemRecord item;
        std::uint32_t dropPetObjectId{ 0 };
    };

    struct InventoryItemInfo
    {
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
        RawItemRecord item;
        std::uint32_t inventoryObjectId{ 0 };
    };

    struct GearToInventoryInfo
    {
        std::uint8_t slotId{ 0 };
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
    };

    struct CannotAddInfo
    {
        std::uint32_t objectId{ 0 };
        std::int32_t reason{ 0 };
        bool hasReason{ false };
    };

    struct RemoveFromGearInfo
    {
        std::uint8_t slotId{ 0 };
    };

    struct ChangeGearSlotInfo
    {
        std::uint16_t requestedGearSlotId{ 0 };
        std::uint16_t activeGearSlotId{ 0 };
        bool active{ false };
        std::vector<RawItemRecord> gearItems;
    };

    struct ObjectEffectInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint16_t effectId{ 0 };
        std::uint32_t duration{ 0 };
        std::vector<std::uint16_t> effectIds;
    };

    struct TileEffectInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
        std::uint16_t effectId{ 0 };
        std::uint32_t duration{ 0 };
    };

    struct DamageInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint16_t damage{ 0 };
    };

    struct CreatureAttackInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
        std::uint8_t direction{ 0 };
    };

    struct BloodDrainInfo
    {
        std::uint32_t objectId{ 0 };
        std::uint32_t targetObjectId{ 0 };
    };

    struct ChangeDarkLightInfo
    {
        std::uint8_t darkLevel{ 0 };
        std::uint8_t lightLevel{ 0 };
    };

    struct ChangeWeatherInfo
    {
        std::uint8_t weather{ 0 };
        std::uint8_t weatherLevel{ 0 };
    };

    struct SkillInfoSummary
    {
        struct Entry
        {
            std::uint16_t skillType{ 0 };
            std::uint32_t experience{ 0 };
            std::uint16_t experienceLevel{ 0 };
            std::uint32_t interval{ 0 };
            std::uint32_t castingTime{ 0 };
            bool enabled{ true };
        };

        struct Group
        {
            bool learnNewSkill{ false };
            std::uint8_t domainType{ 0 };
            std::vector<Entry> entries;
        };

        std::uint8_t pcType{ 0 };
        std::uint8_t groupCount{ 0 };
        std::size_t skillCount{ 0 };
        std::vector<Group> groups;
        std::vector<std::uint8_t> raw;
    };

    struct LearnSkillResult
    {
        std::uint16_t skillType{ 0 };
        std::uint8_t domainType{ 0 };
    };

    struct LearnSkillFailure
    {
        std::uint16_t skillType{ 0 };
        std::uint8_t description{ 0 };
    };

    struct LearnSkillReady
    {
        std::uint8_t domainType{ 0 };
    };

    struct RankGemDataEntry
    {
        std::uint32_t slot{ 0 };
        std::uint32_t type{ 0 };
        std::uint32_t grade{ 0 };
    };

    struct RankGemDataList
    {
        std::vector<RankGemDataEntry> entries;
    };

    struct RealWearingInfo
    {
        std::uint32_t info{ 0 };
    };

    struct NpcRuntimeInfo
    {
        std::string name;
        std::uint16_t npcId{ 0 };
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
    };

    struct UpdateInfo
    {
        char pcType{ '0' };
        PcType normalizedPcType{ PcType::Slayer };
        std::uint32_t objectId{ 0 };
        std::string name;
        std::uint8_t sex{ 0 };
        std::uint16_t level{ 1 };
        AttributeTriplet str;
        AttributeTriplet dex;
        AttributeTriplet intelligence;
        std::uint32_t hp{ 0 };
        std::uint32_t maxHp{ 0 };
        std::uint32_t mp{ 0 };
        std::uint32_t maxMp{ 0 };
        std::uint32_t gold{ 0 };
        bool goldKnown{ false };
        std::array<std::uint8_t, 6> domainLevels{};
        std::array<std::uint32_t, 6> domainExperience{};
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
        std::uint32_t experienceRemaining{ 0 };
        std::uint32_t advancementGoalExperience{ 0 };
        std::uint32_t strExperienceRemaining{ 0 };
        std::uint32_t dexExperienceRemaining{ 0 };
        std::uint32_t intExperienceRemaining{ 0 };
        std::uint8_t competence{ 0 };
        std::uint16_t guildId{ 0 };
        std::string guildName;
        std::uint8_t guildRank{ 0 };
        std::uint32_t unionId{ 0 };
        std::uint16_t advancementLevel{ 0 };
        std::uint8_t attackSpeed{ 0 };
        std::uint16_t zoneId{ 0 };
        std::uint16_t zoneX{ 0 };
        std::uint16_t zoneY{ 0 };
        bool zoneKnown{ false };
        GameTimeInfo time;
        std::uint8_t weather{ 0 };
        std::uint8_t weatherLevel{ 0 };
        std::uint8_t darkLevel{ 0 };
        std::uint8_t lightLevel{ 0 };
        std::vector<RawItemRecord> inventoryItems;
        std::vector<RawItemRecord> gearItems;
        std::uint16_t currentGearSlotId{ 0 };
        bool currentGearSlotActive{ false };
        std::vector<RawItemRecord> extraItems;
        std::vector<EffectRecord> effects;
        PetInfoRecord petInfo;
        bool hasMotorcycle{ false };
        std::vector<RawItemRecord> motorcycleItems;
        std::vector<std::uint16_t> npcTypes;
        std::vector<std::uint16_t> monsterTypes;
        std::vector<NpcRuntimeInfo> npcInfos;
        std::uint8_t serverStat{ 0 };
        std::uint8_t premium{ 0 };
        std::uint32_t smsCharge{ 0 };
        NicknameRecord nickname;
        std::uint8_t nonPk{ 0 };
        std::uint32_t guildUnionId{ 0 };
        std::uint8_t guildUnionUserType{ 0 };
        std::uint32_t bloodBibleOpenNum{ 0 };
        std::vector<std::uint16_t> bloodBibleSigns;
        std::int32_t powerjjang{ 0 };
        std::size_t bytesConsumed{ 0 };
        bool strictAlignment{ false };
    };

    struct SetPosition
    {
        std::uint8_t x{ 0 };
        std::uint8_t y{ 0 };
        std::uint8_t direction{ 0 };
    };

    struct ItemToItemVerify
    {
        std::uint8_t code{ 0 };
        std::uint32_t parameter{ 0 };
    };

    [[nodiscard]] const char* PacketName(PacketId id) noexcept;
    [[nodiscard]] std::uint32_t LegacyReportedPacketSize(PacketId id, const std::vector<std::uint8_t>& body) noexcept;
    [[nodiscard]] std::uint32_t LegacyIncomingWireBodySize(PacketId id, std::uint32_t headerBodySize) noexcept;
    [[nodiscard]] std::optional<std::uint32_t> ProbeLegacyIncomingRequiredBodySize(
        PacketId id,
        const std::vector<std::uint8_t>& body) noexcept;
    [[nodiscard]] std::vector<std::uint8_t> BuildPacket(PacketId id, const std::vector<std::uint8_t>& body);

    [[nodiscard]] std::vector<std::uint8_t> EncodeCLLogin(
        const std::string& username,
        const std::string& password,
        const std::array<std::uint8_t, 6>& macAddress,
        std::uint8_t loginMode,
        std::uint32_t realIpAddress);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCLRegisterPlayer(
        const std::string& username,
        const std::string& password);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCLGetWorldList();
    [[nodiscard]] std::vector<std::uint8_t> EncodeCLSelectWorld(std::uint8_t worldId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCLSelectServer(std::uint8_t serverGroupId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCLSelectPC(const std::string& pcName, PcType pcType);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCLReconnectLogin(std::uint32_t authKey, std::uint8_t loginMode);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGConnect(
        std::uint32_t authKey,
        PcType pcType,
        const std::string& pcName,
        const std::array<std::uint8_t, 6>& macAddress);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGLogout();
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGResurrect();
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGMove(std::uint8_t x, std::uint8_t y, std::uint8_t direction);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGReady();
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGUseBonusPoint(std::uint8_t which);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGSay(std::uint32_t color, const std::string& message);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGGlobalChat(std::uint32_t color, const std::string& message);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGWhisper(
        const std::string& name,
        std::uint32_t color,
        const std::string& message);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGPartySay(std::uint32_t color, const std::string& message);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGGuildChat(
        std::uint8_t type,
        std::uint32_t color,
        const std::string& message);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGRangerSay(const std::string& message);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGSelectRankBonus(std::uint32_t rankBonusType);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGWithdrawPet();
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGDepositPet();
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGSelectGuild(std::uint32_t guildId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGRequestGuildMemberList(std::uint32_t guildId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGRequestGuildList(std::uint8_t guildType);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGRequestStoreInfo(std::uint32_t ownerObjectId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGRequestWebMarket(std::uint8_t requestCode);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGRequestTmallList(std::uint8_t kind);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGAddZoneToInventory(
        std::uint32_t objectId,
        std::uint8_t zoneX,
        std::uint8_t zoneY,
        std::uint8_t inventoryX,
        std::uint8_t inventoryY);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGAddZoneToMouse(
        std::uint32_t objectId,
        std::uint8_t zoneX,
        std::uint8_t zoneY);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGAddInventoryToMouse(
        std::uint32_t objectId,
        std::uint32_t inventoryItemObjectId,
        std::uint8_t inventoryX,
        std::uint8_t inventoryY);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGAddMouseToInventory(
        std::uint32_t objectId,
        std::uint32_t inventoryItemObjectId,
        std::uint8_t inventoryX,
        std::uint8_t inventoryY);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGAddMouseToZone(std::uint32_t objectId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGAddGearToMouse(
        std::uint32_t objectId,
        std::uint8_t slotId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGAddMouseToGear(
        std::uint16_t gearSlotId,
        std::uint32_t objectId,
        std::uint8_t slotId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGChangeGearSlot(
        std::uint16_t gearSlotId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGAddItemToItem(
        std::uint32_t objectId,
        std::uint8_t inventoryX,
        std::uint8_t inventoryY);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGCastingSkill(std::uint16_t skillType);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGLearnSkill(
        std::uint16_t skillType,
        std::uint8_t domainType);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGRequestRankGemData();
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGUseRankGemFromInventory(
        std::uint32_t objectId,
        std::uint32_t slotId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGRemoveRankGem(std::uint32_t slotId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGSkillToSelf(
        std::uint16_t skillType,
        std::uint16_t effectId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGSkillToObject(
        std::uint16_t skillType,
        std::uint16_t effectId,
        std::uint32_t targetObjectId,
        std::uint8_t actionStep);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGSkillToTile(
        std::uint16_t skillType,
        std::uint16_t effectId,
        std::uint8_t x,
        std::uint8_t y);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGSkillToInventory(
        std::uint16_t skillType,
        std::uint32_t objectId,
        std::uint32_t inventoryItemObjectId,
        std::uint8_t x,
        std::uint8_t y,
        std::uint8_t targetX,
        std::uint8_t targetY);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGSkillToNamed(
        std::uint16_t skillType,
        std::uint16_t effectId,
        const std::string& targetName);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGAttack(
        std::uint32_t objectId,
        std::uint8_t x,
        std::uint8_t y,
        std::uint8_t direction,
        std::uint8_t actionStep);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGNPCTalk(std::uint32_t objectId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGNPCAskAnswer(
        std::uint32_t objectId,
        std::uint32_t scriptId,
        std::uint8_t answerId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGShopRequestList(
        std::uint32_t objectId,
        std::uint8_t rackType);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGShopRequestBuy(
        std::uint32_t objectId,
        std::uint8_t rackType,
        std::uint8_t rackIndex,
        std::uint8_t itemNum,
        std::uint8_t inventoryX,
        std::uint8_t inventoryY);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGShopRequestSell(
        std::uint32_t objectId,
        std::uint32_t itemObjectId,
        std::uint8_t opCode);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGUseItemFromInventory(
        std::uint32_t objectId,
        std::uint32_t inventoryItemObjectId,
        std::uint8_t inventoryX,
        std::uint8_t inventoryY);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGUseItemFromGear(
        std::uint32_t objectId,
        std::uint8_t part);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGUseItemFromGQuestInventory(
        std::uint8_t index);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGGQuestAccept(std::uint32_t questId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGGQuestCancel(std::uint32_t questId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGTradePrepare(
        std::uint32_t targetObjectId,
        std::uint8_t code);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGTradeAddItem(
        std::uint32_t targetObjectId,
        std::uint32_t itemObjectId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGTradeRemoveItem(
        std::uint32_t targetObjectId,
        std::uint32_t itemObjectId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGTradeMoney(
        std::uint32_t targetObjectId,
        std::uint32_t amount,
        std::uint8_t code);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGTradeFinish(
        std::uint32_t targetObjectId,
        std::uint8_t code);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGStashList(std::uint32_t objectId);
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGVerifyTime();
    [[nodiscard]] std::vector<std::uint8_t> EncodeCGRequestPing();

    [[nodiscard]] std::optional<LoginOk> DecodeLCLoginOK(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<LoginError> DecodeLCLoginError(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<RegisterPlayerOk> DecodeLCRegisterPlayerOK(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<RegisterPlayerError> DecodeLCRegisterPlayerError(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldList> DecodeLCWorldList(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ServerList> DecodeLCServerList(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<CharacterList> DecodeLCPCList(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ReconnectInfo> DecodeLCReconnect(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<LoginReconnectInfo> DecodeGCReconnectLogin(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<UpdateInfo> DecodeGCUpdateInfo(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ModifyInfoRecord> DecodeGCModifyInformation(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<GQuestStatusList> DecodeGCGQuestStatusInfo(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<GQuestStatusModify> DecodeGCGQuestStatusModify(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<GQuestInventory> DecodeGCGQuestInventory(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<SetPosition> DecodeGCSetPosition(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<MoveInfo> DecodeGCMove(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<SetPosition> DecodeGCMoveOK(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<SetPosition> DecodeGCMoveError(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<FastMoveInfo> DecodeGCFastMove(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<KnockBackInfo> DecodeGCKnockBack(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ObjectPositionInfo> DecodeGCFakeMove(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ObjectPositionInfo> DecodeGCEnterVampirePortal(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<KnockTargetBackInfo> DecodeGCKnocksTargetBackOK1(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<KnockTargetBackInfo> DecodeGCKnocksTargetBackOK2(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<KnockTargetBackInfo> DecodeGCKnocksTargetBackOK4(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<KnockTargetBackInfo> DecodeGCKnocksTargetBackOK5(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<DeleteObjectInfo> DecodeGCDeleteObject(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddSlayer(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddVampire(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddOusters(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddMonster(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddMonsterCorpse(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddVampireCorpse(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddMonsterFromBurrowing(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddVampireFromBurrowing(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddMonsterFromTransformation(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddVampireFromTransformation(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddWolf(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddBat(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<WorldObjectInfo> DecodeGCAddNPC(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<StatusCurrentHP> DecodeGCStatusCurrentHP(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<SystemMessage> DecodeGCSystemMessage(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ChatMessage> DecodeGCSay(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ChatMessage> DecodeGCGlobalChat(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ChatMessage> DecodeGCWhisper(const std::vector<std::uint8_t>& body);
    [[nodiscard]] bool DecodeGCWhisperFailed(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ChatMessage> DecodeGCPartySay(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ChatMessage> DecodeGCGuildChat(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<PartyJoinedInfo> DecodeGCPartyJoined(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<PartyLeaveInfo> DecodeGCPartyLeave(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<RankBonusInfo> DecodeGCRankBonusInfo(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<RankBonusSelectionResult> DecodeGCSelectRankBonusOK(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<RankBonusSelectionResult> DecodeGCSelectRankBonusFailed(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<NpcDialogInfo> DecodeGCNPCAsk(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<NpcDialogInfo> DecodeGCNPCSay(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<NpcDialogInfo> DecodeGCNPCAskDynamic(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<NpcDialogInfo> DecodeGCNPCSayDynamic(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<NpcDialogInfo> DecodeGCNPCAskVariable(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<NpcDialogInfo> DecodeGCNPCResponse(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<std::vector<NpcRuntimeInfo>> DecodeGCNPCInfo(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ShopVersionInfo> DecodeGCShopVersion(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ShopListInfo> DecodeGCShopList(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ShopListInfo> DecodeGCShopListMysterious(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ShopBuyOkInfo> DecodeGCShopBuyOK(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ShopBuyFailInfo> DecodeGCShopBuyFail(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ShopSellOkInfo> DecodeGCShopSellOK(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<std::uint32_t> DecodeGCShopSellFail(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ShopRackDeltaInfo> DecodeGCShopSold(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ShopRackDeltaInfo> DecodeGCShopBought(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ShopMarketConditionInfo> DecodeGCShopMarketCondition(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ModifyInfoRecord> DecodeGCUseOK(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<TradeCodeInfo> DecodeGCTradePrepare(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<TradeItemInfo> DecodeGCTradeAddItem(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<TradeRemoveItemInfo> DecodeGCTradeRemoveItem(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<TradeMoneyInfo> DecodeGCTradeMoney(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<TradeCodeInfo> DecodeGCTradeFinish(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<TradeCodeInfo> DecodeGCTradeError(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<TradeVerifyInfo> DecodeGCTradeVerify(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<StashListInfo> DecodeGCStashList(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<std::uint32_t> DecodeGCStashSell(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ZoneItemInfo> DecodeGCAddNewItemToZone(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ZoneItemInfo> DecodeGCDropItemToZone(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<DeleteObjectInfo> DecodeGCDeleteAndPickUpOK(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<InventoryItemInfo> DecodeGCCreateItem(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<GearToInventoryInfo> DecodeGCAddGearToInventory(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<CannotAddInfo> DecodeGCCannotAdd(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<RemoveFromGearInfo> DecodeGCRemoveFromGear(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ChangeGearSlotInfo> DecodeGCChangeGearSlot(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<DeleteObjectInfo> DecodeGCDeleteInventoryItem(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ItemToItemVerify> DecodeGCAddItemToItemVerify(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ObjectEffectInfo> DecodeGCAddEffect(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ObjectEffectInfo> DecodeGCRemoveEffect(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<TileEffectInfo> DecodeGCAddEffectToTile(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<TileEffectInfo> DecodeGCDeleteEffectFromTile(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<DeleteObjectInfo> DecodeGCCreatureDied(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<DamageInfo> DecodeGCGetDamage(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<CreatureAttackInfo> DecodeGCAttack(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<BloodDrainInfo> DecodeGCBloodDrainOK3(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ChangeDarkLightInfo> DecodeGCChangeDarkLight(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<ChangeWeatherInfo> DecodeGCChangeWeather(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<SkillInfoSummary> DecodeGCSkillInfo(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<LearnSkillFailure> DecodeGCLearnSkillFailed(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<LearnSkillResult> DecodeGCLearnSkillOK(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<LearnSkillReady> DecodeGCLearnSkillReady(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<RankGemDataList> DecodeGCSendRankGemData(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<RealWearingInfo> DecodeGCRealWearingInfo(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<PetInfoRecord> DecodeGCPetInfo(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::uint8_t CalculateDefaultEncryptCode(std::uint16_t zoneId, std::uint8_t serverId) noexcept;
    [[nodiscard]] bool ApplyLegacyIncomingEncryption(PacketId id, std::vector<std::uint8_t>& body, std::uint8_t code);
    [[nodiscard]] bool ApplyLegacyOutgoingEncryption(PacketId id, std::vector<std::uint8_t>& body, std::uint8_t code);
}
