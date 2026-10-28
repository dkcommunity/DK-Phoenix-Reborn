#pragma once

#include "Content/LegacyQuestCatalog.h"

#include "Diagnostics/Logger.h"
#include "Backends/DirectX11/DirectX11Renderer.h"
#include "Assets/LegacyAssetTypes.h"
#include "Assets/SpriteArchiveReader.h"
#include "Content/AudioCatalog.h"
#include "Content/CreatureInfoCatalog.h"
#include "Content/EffectInfoCatalog.h"
#include "Content/EffectStatusCatalog.h"
#include "Content/FileDefCatalog.h"
#include "Content/GameStringCatalog.h"
#include "Content/InfSchemaRegistry.h"
#include "Content/ItemInfoCatalog.h"
#include "Content/ItemOptionCatalog.h"
#include "Content/LegacyInfoCatalog.h"
#include "Content/ExperienceCatalog.h"
#include "Frontend/FrontendStateMachine.h"
#include "Frontend/FrontendRuntimeModels.h"
#include "Graphics/TextureTypes.h"
#include "Graphics/ImageData.h"
#include "Network/TcpClient.h"
#include "Protocol/LegacyProtocol.h"
#include "Runtime/GameRuntimeModels.h"
#include "Settings/InterfaceProfileLoader.h"
#include "Settings/SettingsTypes.h"
#include "UI/Windows/ClientUIEditor.h"
#include "UI/Windows/ChatPanel.h"
#include "UI/Layout/FrontendLayoutCatalog.h"
#include "UI/Resources/LegacyUiSpriteResourceLoader.h"
#include "UI/Resources/LegacyWindowTransparency.h"
#include "UI/Login/LoginInputModel.h"
#include "UI/Windows/ChatPanel.h"
#include "UI/Windows/QuestManager.h"
#include "UI/Windows/CharInfoWindow.h"
#include "UI/Windows/HpBarWindow.h"
#include "UI/Windows/ItemListWindow.h"
#include "UI/Windows/ImageNoticeWindow.h"
#include "UI/Windows/BulletinBoardWindow.h"
#include "UI/Windows/ResurrectRequestWindow.h"
#include "UI/Windows/MixingForgeWindow.h"
#include "UI/Windows/RemoveOptionWindow.h"
#include "UI/Windows/OustersSkillInfoWindow.h"
#include "UI/Windows/GenericBillingPromptWindow.h"
#include "UI/Windows/LegalDocumentsWindow.h"
#include "UI/Windows/LoginScreen.h"
#include "UI/Windows/ServerSelectScreen.h"
#include "UI/Windows/CharacterSelectScreen.h"
#include "UI/Windows/CharacterCreateScreen.h"
#include "UI/Windows/CharacterDeleteScreen.h"
#include "UI/Windows/OptionsScreen.h"
#include "UI/Windows/NewUserScreen.h"
#include "UI/Title/TitleScreen.h"
#include "Content/LegacyQuestCatalog.h"
#include "UI/Layout/FrontendLayoutCatalog.h"
#include "UI/Layout/LoginTitleLayout.h"
#include "UI/Text/DirectWriteTextRenderer.h"
#include "World/WorldAssetCache.h"
#include "World/WorldDrawListBuilder.h"
#include "World/LegacyMapReader.h"
#include "World/LegacyMapAnimationRenderer.h"
#include "World/ZoneInfoCatalog.h"

#include <Windows.h>
#include <atomic>
#include <array>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace phoenix::platform
{
    enum class ServerSelectButtonRole
    {
        Prev,
        Next
    };

    enum class OptionTab
    {
        Graphics,
        Sound,
        Game,
        Advanced,
        Advanced2
    };

    enum class LoginSocketPurpose
    {
        None,
        Login,
        RegisterPlayer,
        ReconnectLogin
    };

    class Application final
    {
    public:
        explicit Application(HINSTANCE instance);

        int Run(int showCommand);
        LRESULT HandleMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

    private:
        enum class FrameTextureLoadKind : std::uint8_t;
        struct FramePerfSample;

        struct WorldEffectTexture
        {
            graphics::TextureDescriptor texture;
            std::int32_t originX{ 0 };
            std::int32_t originY{ 0 };
        };

        struct WorldCreatureTexture
        {
            struct Frame
            {
                std::uint32_t x{ 0 };
                std::uint32_t y{ 0 };
                std::uint32_t width{ 0 };
                std::uint32_t height{ 0 };
                std::int32_t originX{ 0 };
                std::int32_t originY{ 0 };
            };

            graphics::TextureDescriptor texture;
            std::int32_t originX{ 0 };
            std::int32_t originY{ 0 };
            std::uint64_t key{ 0 };
            std::vector<Frame> frames;
        };

        struct WorldHitRegion
        {
            ui::UiRect rect;
            runtime::WorldInteractionSelection selection;
        };

        struct PendingWorldCreatureTextureRequest;
        struct PendingWorldAssetDecode;

        struct MouseMoveStep
        {
            std::int32_t dx{ 0 };
            std::int32_t dy{ 0 };
            std::uint8_t direction{ 0 };
        };

        enum class WorldLoadingStage
        {
            Idle,
            Connecting,
            Map,
            Tiles,
            WorldObjects,
            Creatures,
            ItemsEffects,
            Ready
        };

        enum class NewIdFocus
        {
            Id,
            Password
        };

        enum class NewIdResult
        {
            None,
            Success,
            Failed,
            AlreadyExists
        };

        struct NewIdRegistrationState
        {
            bool open{ false };
            bool waiting{ false };
            NewIdFocus focus{ NewIdFocus::Id };
            NewIdResult result{ NewIdResult::None };
            std::string id;
            std::string password;
        };

        HWND CreateMainWindow();
        bool InitializeRuntime(HWND window);
        void RenderFrame();
        void ResetFramePerfCounters() noexcept;
        void AccumulateFramePerfMs(std::uint32_t& target, std::chrono::steady_clock::time_point start) noexcept;
        void RecordFrameTextureLoad(FrameTextureLoadKind kind, std::chrono::steady_clock::time_point start, bool uploaded) noexcept;
#if defined(PHOENIX_ENABLE_DIAGNOSTICS)
        void PushFramePerfSample(FramePerfSample sample);
        void LogFramePerfSpike(const FramePerfSample& sample);
        [[nodiscard]] std::wstring FormatFramePerfSample(const FramePerfSample& sample) const;
        void BeginEnterWorldDiagnostics();
        void LogEnterWorldDiagnostic(std::wstring_view category, std::wstring_view function, std::wstring detail = {}, double durationMs = 0.0);
        void RecordEnterWorldFrame(const FramePerfSample& sample);
        void FinishEnterWorldDiagnostics();
        void MarkEnterWorldMilestone(std::wstring_view name);
#else
        void PushFramePerfSample(FramePerfSample) noexcept {}
        void LogFramePerfSpike(const FramePerfSample&) noexcept {}
        [[nodiscard]] std::wstring FormatFramePerfSample(const FramePerfSample&) const { return {}; }
        void BeginEnterWorldDiagnostics() noexcept {}
        void LogEnterWorldDiagnostic(std::wstring_view, std::wstring_view, std::wstring = {}, double = 0.0) noexcept {}
        void RecordEnterWorldFrame(const FramePerfSample&) noexcept {}
        void FinishEnterWorldDiagnostics() noexcept {}
        void MarkEnterWorldMilestone(std::wstring_view) noexcept {}
#endif
        [[nodiscard]] bool ValidateWorldInput(std::int32_t x, std::int32_t y, std::wstring_view eventName);
        [[nodiscard]] const wchar_t* WorldLoadingStageName(WorldLoadingStage stage) const noexcept;
        void Resize(std::uint32_t width, std::uint32_t height);
        void HandleMouseDown(std::int32_t x, std::int32_t y, bool rightButton = false);
        void HandleMouseMove(std::int32_t x, std::int32_t y) noexcept;
        void HandleMouseUp(std::int32_t x, std::int32_t y);
        void HandleMouseWheel(std::int32_t x, std::int32_t y, std::int16_t delta) noexcept;
        void HandleChar(wchar_t ch);
        void HandleImeComposition(LPARAM flags);
        void PositionChatImeWindow() noexcept;
        void HandleKeyDown(WPARAM key);
        void SubmitLogin();
        void PollNetworkEvents();
        void StartLoginNetwork();
        void SendSelectedWorld(std::uint8_t worldId);
        void SendSelectedServer();
        void SendSelectedCharacter();
        void StartGameNetwork(const protocol::ReconnectInfo& reconnect);
        void StartLoginReconnect(const protocol::LoginReconnectInfo& reconnect);
        void HandleLoginPacket(protocol::PacketId packetId, const std::vector<std::uint8_t>& body);
        void HandleGamePacket(protocol::PacketId packetId, const std::vector<std::uint8_t>& body);
        void UpsertWorldObject(const protocol::WorldObjectInfo& info);
        void ApplyWorldMove(const protocol::MoveInfo& move);
        void ApplyPlayerPosition(std::uint16_t x, std::uint16_t y, std::uint8_t direction, bool smooth);
        void ApplyPlayerMoveOk(std::uint16_t x, std::uint16_t y, std::uint8_t direction);
        void ApplyPlayerMoveError(std::uint16_t x, std::uint16_t y);
        void ApplyRemoteWorldMove(
            runtime::WorldObjectRuntimeModel& object,
            const protocol::MoveInfo& move,
            bool receivedFromServer = true);
        bool ApplyFastWorldMove(std::uint32_t objectId, std::uint16_t targetX, std::uint16_t targetY);
        bool CreateFakeFastMove(std::uint32_t sourceObjectId, std::uint16_t targetX, std::uint16_t targetY);
        bool ApplyKnockBackWorldMove(
            std::uint32_t objectId,
            std::uint16_t originX,
            std::uint16_t originY,
            std::uint16_t targetX,
            std::uint16_t targetY);
        void StartMotion(
            runtime::WorldMotionState& motion,
            animation::LegacyCreatureAnimation& creatureAnimation,
            std::uint16_t creatureType,
            std::uint16_t targetX,
            std::uint16_t targetY,
            std::uint8_t direction,
            std::uint64_t nowMs,
            bool smooth);
        void StartSpecialMotion(
            runtime::WorldMotionState& motion,
            animation::LegacyCreatureAnimation& creatureAnimation,
            std::uint16_t creatureType,
            std::uint16_t targetX,
            std::uint16_t targetY,
            std::uint8_t direction,
            runtime::WorldMotionKind kind,
            std::uint64_t nowMs);
        [[nodiscard]] bool AdvanceMotion(
            runtime::WorldMotionState& motion,
            animation::LegacyCreatureAnimation& creatureAnimation,
            std::uint64_t nowMs);
        [[nodiscard]] std::uint16_t ResolvePlayerCreatureType() const noexcept;
        [[nodiscard]] std::uint16_t ResolveMotionCreatureType(const runtime::WorldObjectRuntimeModel& object) const noexcept;
        [[nodiscard]] std::uint8_t ResolveLegacyInitialCreatureDirection(std::uint16_t creatureType);
        void UpdateWorldMotion();
        void ConfigureCreatureAnimation(runtime::WorldObjectRuntimeModel& object);
        void ConfigurePlayerAnimation();
        void RequestCreatureAnimation(
            std::uint32_t objectId,
            animation::CreatureAnimationState state,
            std::optional<std::uint8_t> direction = std::nullopt);
        void LogCreatureAnimationIssue(
            const runtime::WorldObjectRuntimeModel& object,
            std::uint32_t action,
            std::uint32_t direction,
            std::wstring_view field);
        void SyncPlayerPendingMoveFields();
        void ClearPlayerPendingMoves();
        void PromoteInitialNpcSeedsToWorldObjects();
        void RemoveMatchingInitialNpcSeedObject(const protocol::WorldObjectInfo& info);
        void HideMatchingInitialNpcSeed(const protocol::WorldObjectInfo& info);
        void DeleteWorldObject(std::uint32_t objectId);
        [[nodiscard]] runtime::ItemRuntimeRecord BuildItemRuntimeRecord(const protocol::RawItemRecord& item);
        [[nodiscard]] runtime::ItemRuntimeRecord BuildItemRuntimeRecord(const protocol::ShopItemInfo& item);
        void UpsertGroundItem(const protocol::ZoneItemInfo& info);
        void UpsertInventoryItem(const protocol::InventoryItemInfo& info);
        void DeleteGroundItem(std::uint32_t objectId);
        void DeleteInventoryItem(std::uint32_t objectId);
        void AddObjectEffect(const protocol::ObjectEffectInfo& info);
        void RemoveObjectEffects(const protocol::ObjectEffectInfo& info);
        void AddTileEffect(const protocol::TileEffectInfo& info);
        void RemoveTileEffect(const protocol::TileEffectInfo& info);
        void MarkCreatureDied(std::uint32_t objectId);
        void InvalidateLocalPlayerCreatureVisual();
        void AddFloatingDamage(const protocol::DamageInfo& info);
        void PushChatLine(
            runtime::ChatRuntimeKind kind,
            std::uint32_t objectId,
            std::uint32_t color,
            std::string text,
            std::string senderName = {},
            std::uint8_t race = 0,
            std::uint8_t systemType = 0,
            std::uint8_t channelCondition = 0);
        void PushSystemMessage(std::string message);
        void HandleLegacySystemMessage(const protocol::SystemMessage& message);
        void ParseLegacyInformationBoard(std::string_view message);
        void RotateLegacyMessageRings();
        void LoadLegacyChatHelp();
        void AddLegacyMessage(
            std::deque<runtime::LegacyMessageLine>& ring,
            std::string message,
            std::uint32_t color);
        [[nodiscard]] std::string GameStringOr(std::uint32_t index, std::string fallback) const;
        [[nodiscard]] bool IsMasterName(std::string_view name) const;
        [[nodiscard]] bool IsPlayerMaster() const;
        [[nodiscard]] bool IsPlayerOperator() const noexcept;
        [[nodiscard]] bool PlayerHasEffectStatus(std::uint16_t effectStatusId) const noexcept;
        [[nodiscard]] bool PlayerHasActionInfo(std::uint16_t actionInfo) const noexcept;
        [[nodiscard]] std::string ApplyLegacyChatMask(std::string text, int percent) const;
        [[nodiscard]] std::uint8_t PlayerRace() const noexcept;
        [[nodiscard]] std::uint8_t ObjectRace(const runtime::WorldObjectRuntimeModel& object) const noexcept;
        [[nodiscard]] bool IsPlayerChatObject(const runtime::WorldObjectRuntimeModel& object) const noexcept;
        void SendPlayerMove(std::int32_t dx, std::int32_t dy, std::uint8_t direction);
        void TrySendQueuedPlayerMove();
        void UpdateMouseMove();
        void StartMouseMoveTo(std::uint16_t targetX, std::uint16_t targetY);
        [[nodiscard]] bool RebuildMouseMovePath();
        [[nodiscard]] std::optional<std::pair<std::uint16_t, std::uint16_t>> ScreenToWorldTile(std::int32_t x, std::int32_t y) const;
        [[nodiscard]] std::optional<std::pair<std::uint16_t, std::uint16_t>> ResolveWalkableMouseTarget(std::uint16_t targetX, std::uint16_t targetY) const;
        [[nodiscard]] bool CanAttemptPlayerMove(std::uint16_t targetX, std::uint16_t targetY) const;
        void SendChatMessage();
        [[nodiscard]] std::pair<std::int32_t, std::int32_t> ScreenToReferenceUiPoint(
            std::int32_t x,
            std::int32_t y) const noexcept;
        [[nodiscard]] ui::UiRect ReferenceClientBounds() const noexcept;
        bool HandleChatPanelClick(std::int32_t x, std::int32_t y, bool rightButton);
        void DrawLegacyChatPanel();
        bool HandleGameHudClick(std::int32_t x, std::int32_t y, bool rightButton);
        void HandleGameWorldClick(std::int32_t x, std::int32_t y, bool rightButton);
        [[nodiscard]] runtime::WorldInteractionSelection PickWorldInteraction(std::int32_t x, std::int32_t y) const;
        [[nodiscard]] std::optional<std::pair<std::uint8_t, std::uint8_t>> FindFirstInventorySlot(
            const runtime::ItemRuntimeRecord& item) const;
        void SendPickupGroundItem(std::uint32_t objectId);
        void SendAttackTarget(std::uint32_t objectId);
        void SendNpcTalk(std::uint32_t objectId);
        void SendCGUseBonusPoint(std::uint8_t which);
        bool UseHotbarSkill(std::size_t hotbarIndex);
        [[nodiscard]] std::vector<const protocol::SkillInfoSummary::Entry*> LearnedSkillEntries() const;
        [[nodiscard]] std::string SkillTooltipText(const protocol::SkillInfoSummary::Entry& entry) const;
        [[nodiscard]] ui::UiRect LegacySkillWindowReferenceRect();
        [[nodiscard]] std::vector<const content::LegacySkillInfo*> VisibleLegacySkillWindowEntries() const;
        [[nodiscard]] std::vector<const content::LegacySkillInfo*> LegacyAdvancementSkillEntries() const;
        [[nodiscard]] std::uint16_t LegacySkillDomainLevel(std::uint8_t domain) const noexcept;
        [[nodiscard]] const protocol::SkillInfoSummary::Entry* FindLearnedSkill(std::uint16_t skillType) const noexcept;
        void OpenLegacySkillWindow();
        void CloseLegacySkillWindow() noexcept;
        bool HandleLegacySkillWindowClick(std::int32_t x, std::int32_t y, bool rightButton);
        void ApplyLearnSkillReady(const protocol::LearnSkillReady& info);
        void ApplyLearnSkillOk(const protocol::LearnSkillResult& info);
        void ApplyLearnSkillFailed(const protocol::LearnSkillFailure& info);
        void ApplyRankGemData(const protocol::RankGemDataList& info);
        void DrawGameTooltipOverlay();
        void OpenNpcDialogFromStaticAsk(const protocol::NpcDialogInfo& info);
        void OpenNpcDialogFromDynamicAsk(const protocol::NpcDialogInfo& info);
        void OpenNpcDialogFromVariableAsk(const protocol::NpcDialogInfo& info);
        void HandleNpcSay(const protocol::NpcDialogInfo& info);
        void HandleNpcResponse(const protocol::NpcDialogInfo& info);
        void SendNpcAskAnswer(std::size_t optionIndex);
        [[nodiscard]] std::string ResolveNpcScriptText(
            const std::string& text,
            const std::vector<protocol::NpcScriptParameter>& parameters) const;
        void HandleShopVersion(const protocol::ShopVersionInfo& info);
        void HandleShopList(const protocol::ShopListInfo& info);
        void HandleShopBuyOk(const protocol::ShopBuyOkInfo& info);
        void HandleShopBuyFail(const protocol::ShopBuyFailInfo& info);
        void HandleShopSellOk(const protocol::ShopSellOkInfo& info);
        void HandleShopSellFail(std::uint32_t objectId);
        void HandleShopSold(const protocol::ShopRackDeltaInfo& info);
        void HandleShopBought(const protocol::ShopRackDeltaInfo& info);
        void HandleShopMarketCondition(const protocol::ShopMarketConditionInfo& info);
        void RequestShopRack(std::uint32_t objectId, std::uint8_t rackType);
        void UpdatePingProbe();
        void UpdateLegacyVerifyTime();
        void HandleItemToItemVerify(const protocol::ItemToItemVerify& verify);
        void UpdateWorldTransientState();
        [[nodiscard]] std::uint64_t NowMilliseconds() const noexcept;
        bool PrepareWorldRuntimeAfterUpdate();
        void DrainWorldRuntimePreparation();
        void FinishWorldRuntimePreparation();
        void StopWorldRuntimePreparation();
        void BeginWorldLoadingStage(
            WorldLoadingStage stage,
            std::string message,
            float progress,
            std::wstring_view caller);
        void LogEnterWorldRuntimeSnapshot(bool force = false);
        void SelectRandomProgressEventLoadingSprite();
        void BeginWorldAssetWarmup();
        void UpdateWorldLoadingWarmup();
        [[nodiscard]] bool WarmupWorldTiles();
        [[nodiscard]] bool WarmupWorldObjects();
        [[nodiscard]] bool WarmupWorldCreatures();
        [[nodiscard]] bool WarmupWorldItemsAndEffects();
        void PrefetchSectorDelta(const phoenix::world::LegacyVisibleSectorBounds& previousBounds, const phoenix::world::LegacyVisibleSectorBounds& currentBounds);
        bool SendGameReadyIfNeeded();
        bool EnsureFileDefCatalogLoaded();
        bool EnsureInfSchemaRegistryLoaded();
        bool EnsureGameStringCatalogLoaded();
        bool EnsureAudioCatalogLoaded();
        [[nodiscard]] std::filesystem::path ResolveFileDefPath(std::wstring_view key, std::wstring_view fallbackRelativePath) const;
        [[nodiscard]] std::filesystem::path ResolveExistingFileDefPath(
            std::wstring_view key,
            std::wstring_view fallbackRelativePath) const;
        bool EnsureZoneCatalogLoaded();
        bool EnsureCreatureInfoCatalogLoaded();
        bool EnsureItemInfoCatalogLoaded();
        bool EnsureItemOptionCatalogLoaded();
        bool EnsureLegacyInfoCatalogLoaded();
        bool EnsureExperienceCatalogLoaded();
        bool EnsureLegacyUiSpriteCatalogLoaded();
        bool EnsureEffectInfoCatalogLoaded();
        bool EnsureEffectStatusCatalogLoaded();
        bool EnsureEffectFramePacksLoaded();
        bool EnsureMapAnimationFramesLoaded();
        void StartRuntimeCatalogPreload();
        void WaitRuntimeCatalogPreload();
        void StopRuntimeCatalogPreload();
        void UpsertStatusEffect(std::uint32_t objectId, std::uint16_t effectStatusId, std::uint32_t durationSeconds);
        void RemoveStatusEffects(std::uint32_t objectId, const std::vector<std::uint16_t>& effectStatusIds);
        void ClearWorldRuntimeAssets();
        [[nodiscard]] std::filesystem::path ResolveZoneMapPath(const world::ZoneInfo& zone) const;
        [[nodiscard]] std::optional<graphics::TextureDescriptor> UploadWorldTileTexture(std::uint32_t spriteId);
        [[nodiscard]] std::optional<graphics::TextureDescriptor> UploadWorldImageObjectTexture(
            const world::LegacyMapImageObject& object);
        [[nodiscard]] std::optional<graphics::TextureDescriptor> UploadWorldMapAnimationTexture(
            const world::LegacyMapResolvedFrame& frame);
        [[nodiscard]] std::optional<WorldCreatureTexture> UploadWorldCreatureTexture(
            const runtime::WorldObjectRuntimeModel& object,
            bool localPlayer);
        [[nodiscard]] std::optional<WorldCreatureTexture> UploadWorldCreatureShadowTexture(
            const runtime::WorldObjectRuntimeModel& object,
            bool localPlayer);
        [[nodiscard]] std::optional<graphics::TextureDescriptor> UploadWorldItemDropTexture(
            const runtime::ItemRuntimeRecord& item);
        [[nodiscard]] std::optional<WorldEffectTexture> UploadWorldEffectTexture(
            const runtime::EffectRuntimeModel& effect);
        void ResetWorldFrameTextureBudget();
        [[nodiscard]] bool TryReserveWorldTextureUploadSlot();
        void FailNetwork(std::wstring message);
        [[nodiscard]] std::optional<std::pair<std::string, std::uint16_t>> ResolveLoginEndpoint() const;
        void OpenNewIdRegistration();
        void CloseNewIdRegistration(bool stopSocket);
        void SubmitNewIdRegistration();
        void StartNewIdRegistrationNetwork();
        void CompleteNewIdRegistration(bool success, bool alreadyExists);
        [[nodiscard]] bool ValidateNewIdRegistrationInput() const noexcept;
        [[nodiscard]] std::array<std::uint8_t, 6> ReadPrimaryMacAddress() const noexcept;
        void ApplyResolution(std::uint32_t width, std::uint32_t height);
        void ApplyWindowMode();
        void ApplyGraphRuntimeOptions();
        void ApplyAudioRuntimeOptions(bool previewEffects);
        void ApplyAdvancedRuntimeOptions();
        void ApplyAdvanced2RuntimeOptions();
        void ApplyFrameLimitPacing();
        void CloseRuntimeAudio() noexcept;
        void StopMciAlias(const std::wstring& alias, bool& open) noexcept;
        [[nodiscard]] bool ShouldSkipIntroMedia() const noexcept;
        [[nodiscard]] std::filesystem::path IntroMediaPath() const;
        bool StartIntroMedia();
        void ResizeIntroMedia() noexcept;
        void UpdateIntroMedia();
        void FinishIntroMedia();
        void OpenLoginAfterStartupMedia();
        bool StartBgm();
        bool PlaySfxPreview();
        void LoadUserSettings();
        void SaveUserSettings() const;
        void ReloadInterfaceConfig();
        [[nodiscard]] std::filesystem::path UserSettingsPath() const;
        void ConfigureLoginLayout(bool chooseBackground);
        bool LoadLoginSpriteTextures();
        bool LoadOptionSpriteTextures();
        bool LoadNewIdCreateSpriteTextures();
        bool LoadHotkeySettingSpriteTextures();
        bool LoadProgressEventSpriteTextures();
        bool LoadCharacterSelectSpriteTextures();
        bool LoadCharacterCreateSpriteTextures();
        bool LoadGameUiSpriteTextures();
        [[nodiscard]] bool LoadLegacyGameUiSpriteTextures(std::string_view key, std::uint16_t maxFrames = 32);
        void StartFrontendPreload();
        void StopFrontendPreload();
        void DrainPreloadedTextures();
        void StartWorldAssetDecodeWorker();
        void StopWorldAssetDecodeWorker();
        void DrainWorldAssetDecodeUploads();
        void QueueWorldAssetDecode(PendingWorldAssetDecode request);
        void StartWorldCreatureTextureWorker();
        void StopWorldCreatureTextureWorker();
        void DrainWorldCreatureTextureUploads();
        void QueueWorldCreatureTextureBuild(PendingWorldCreatureTextureRequest request);
        [[nodiscard]] bool HasWorldCreatureTextureQueueCapacity();
        void ClearWorldCreatureTextureBuildQueue();
        void DrawLoginFrame();
        void DrawNewIdRegistrationOverlay();
        void UpdateLoginTextTextures();
        void DrawLoginTextDx11();
        void DrawLoginCaretOverlay();
        void DrawOptionsFrame();
        void DrawPerformanceOverlay();
        void DrawBackgroundLoadingOverlay();
        void DrawVisualAdjustmentOverlay();
        void DrawGameFrame();
        void DrawGameHudOverlay();
        void DrawGameInventoryOverlay();
        void DrawGameGearOverlay();
        void DrawGameQuestManagerOverlay();
        void DrawGameSkillWindowOverlay();
        void DrawGameRuneWindowOverlay();
        void DrawGameMenuOverlay();
        void DrawGameMinimapOverlay(const ui::UiRect& rect);
        [[nodiscard]] bool LoadActiveZoneMinimapTexture();
        void DrawLegacyMousePointerOverlay();
        [[nodiscard]] bool DrawGameUiSprite(std::uint32_t spriteId, const ui::UiRect& rect);
        [[nodiscard]] bool DrawLegacyGameUiSprite(std::string_view key, std::uint16_t frameIndex, const ui::UiRect& rect, float opacity = 1.0F);
        [[nodiscard]] const graphics::TextureDescriptor* FindLegacyGameUiSpriteTexture(std::string_view key, std::uint16_t frameIndex);
        [[nodiscard]] bool DrawLegacyGameUiSpriteAt(std::string_view key, std::uint16_t frameIndex, std::int32_t referenceX, std::int32_t referenceY, float opacity = 1.0F);
        [[nodiscard]] bool DrawLegacyGameUiSpriteClipWidth(std::string_view key, std::uint16_t frameIndex, const ui::UiRect& rect, float widthRatio);
        [[nodiscard]] bool DrawLegacyGameUiSpriteClip(
            std::string_view key,
            std::uint16_t frameIndex,
            const ui::UiRect& destinationRect,
            const ui::UiRect& sourceRect,
            float opacity = 1.0F);
        [[nodiscard]] bool DrawLegacySimpleInformationHud();
        [[nodiscard]] bool DrawLegacyHpMpBarHud();
        [[nodiscard]] bool DrawLegacyQuickSlotHud();
        [[nodiscard]] bool DrawLegacyInventoryWindow();
        struct LegacyInventoryLayout
        {
            std::int32_t x{ 10 };
            std::int32_t y{ 128 };
            std::int32_t gridStartX{ 13 };
            std::int32_t gridStartY{ 25 };
            std::int32_t moneyButtonX{ 16 };
            std::int32_t moneyButtonY{ 214 };
            std::int32_t descButtonX{ 214 };
            std::int32_t descButtonY{ 214 };
            std::int32_t alphaButtonX{ 240 };
            std::int32_t alphaButtonY{ 214 };
            std::int32_t helpButtonX{ 266 };
            std::int32_t helpButtonY{ 214 };
            std::int32_t closeButtonX{ 292 };
            std::int32_t closeButtonY{ 214 };
        };
        [[nodiscard]] LegacyInventoryLayout LegacyInventoryLayoutForCurrentRace() const noexcept;
        [[nodiscard]] std::string_view LegacyAssembleButtonKey() const noexcept;
        [[nodiscard]] ui::UiRect LegacyInventoryGridScreenRect() const noexcept;
        [[nodiscard]] std::optional<ui::UiRect> LegacyInventoryWindowScreenRect();
        [[nodiscard]] std::optional<std::pair<std::uint8_t, std::uint8_t>> LegacyInventoryCellFromScreen(
            std::int32_t x,
            std::int32_t y,
            const runtime::ItemRuntimeRecord* heldItem = nullptr) const noexcept;
        [[nodiscard]] const runtime::ItemRuntimeRecord* FindInventoryItemAtCell(std::uint8_t cellX, std::uint8_t cellY) const noexcept;
        [[nodiscard]] std::optional<ui::UiRect> LegacyInventoryItemScreenRect(const runtime::ItemRuntimeRecord& item) const noexcept;
        [[nodiscard]] bool DrawLegacyInventoryButton(
            std::string_view buttonKey,
            std::uint16_t frameIndex,
            std::int32_t referenceX,
            std::int32_t referenceY,
            bool disabled);
        [[nodiscard]] std::string_view LegacyHudMainKey() const noexcept;
        [[nodiscard]] std::string_view LegacyHpBarKey() const noexcept;
        [[nodiscard]] std::string_view LegacyEffectStatusKey() const noexcept;
        [[nodiscard]] std::string_view LegacyInventoryKey() const noexcept;
        [[nodiscard]] std::string_view LegacyInventorySubKey() const noexcept;
        [[nodiscard]] std::string_view LegacyGearKey() const noexcept;
        [[nodiscard]] std::string_view LegacyGearSlotKey() const noexcept;
        [[nodiscard]] std::string_view LegacyQuestManagerKey() const noexcept;
        [[nodiscard]] std::string_view LegacyQuickItemKey() const noexcept;
        [[nodiscard]] std::string_view LegacyMinimapKey() const noexcept;
        [[nodiscard]] std::string_view LegacyChattingKey() const noexcept;
        [[nodiscard]] std::string_view LegacySkillDomainKey() const noexcept;
        [[nodiscard]] std::string_view LegacyGameMenuKey() const noexcept;
        [[nodiscard]] std::optional<ui::UiRect> LegacyGameMenuReferenceRect();
        void ClosePopupWindowsForLegacyGameMenu() noexcept;
        void OpenLegacyGameMenu() noexcept;
        void RequestLegacySessionEnd(bool quitApplication);
        void UpdateLegacySessionEnd();
        [[nodiscard]] bool IsPlayerInLegacySafeArea() const noexcept;
        void DrawServerSelectFrame();
        void UpdateServerSelectTextTextures();
        void DrawServerSelectTextDx11();
        void DrawCharacterSelectFrame();
        void DrawCharacterCreateFrame();
        void DrawAsciiText(
            const ui::UiRect& rect,
            const std::string& text,
            std::uint32_t color = RGB(255, 255, 255),
            std::uint8_t alpha = 255);
        void DrawUnicodeText(
            const ui::UiRect& rect,
            const std::string& text,
            std::uint32_t color = RGB(255, 255, 255));
        void DrawRect(const ui::UiRect& rect);
        void DrawColorRect(const ui::UiRect& rect, std::uint32_t rgba);
        void DrawRectOutline(const ui::UiRect& rect, std::int32_t thickness = 1);
        void DrawUiLayoutEditorOverlay();
        void DrawUiLayoutEditorElementList(const std::vector<ui::ClientUIEditorEntry>& entries);
        [[nodiscard]] bool TrySelectUiLayoutEditorListItem(std::int32_t x, std::int32_t y, const std::vector<ui::ClientUIEditorEntry>& entries);
        [[nodiscard]] ui::UiRect LayoutRect(const std::string& key, ui::UiRect fallback) const;
        [[nodiscard]] std::vector<ui::ClientUIEditorEntry> ActiveEditableLayoutRects() const;
        [[nodiscard]] std::vector<ui::ClientUIEditorEntry> AllEditableLayoutRects() const;
        [[nodiscard]] ui::FrontendLayout BuildServerSelectLayout() const;
        [[nodiscard]] ui::UiRect ScaleUiRect(ui::UiRect rect) const noexcept;
        [[nodiscard]] ui::UiRect CharacterSelectSlotNameRect(std::uint32_t slot) const;
        [[nodiscard]] ui::UiRect CharacterSelectSlotLevelRect(std::uint32_t slot) const;
        [[nodiscard]] graphics::TextureDescriptor TextureForLoginBinding(const ui::LoginTitleSpriteBinding& binding) const noexcept;
        void PersistResolution(std::uint32_t width, std::uint32_t height) const;
        [[nodiscard]] std::uint32_t SelectLoginBackgroundSpriteId(const settings::InterfaceResolutionProfile& profile);
        [[nodiscard]] std::filesystem::path LoginBackgroundCursorPath() const;
        [[nodiscard]] std::uint32_t ReadLoginBackgroundCursor(std::uint32_t width, std::uint32_t height) const;
        void WriteLoginBackgroundCursor(std::uint32_t width, std::uint32_t height, std::uint32_t nextPosition) const;
        [[nodiscard]] const settings::InterfaceResolutionProfile* CurrentInterfaceProfile() const noexcept;
        void WriteLoginSnapshot() const;
        void ReloadIngameUiIfChanged();
        void ReloadHpBarConfig() noexcept;
        void EnableDpiAwareness();

        enum class PreloadTarget
        {
            CharacterSelect,
            CharacterCreate
        };

        struct PreloadedSprite
        {
            PreloadTarget target{ PreloadTarget::CharacterSelect };
            std::uint32_t spriteId{ 0 };
            std::string key;
            graphics::ImageData image;
            std::uint32_t decodeMilliseconds{ 0 };
        };

        enum class WorldCreatureTextureTarget : std::uint8_t
        {
            Body,
            Shadow
        };

        enum class WorldCreatureResourceState : std::uint8_t
        {
            Queued,
            Loading,
            Prepared,
            Ready,
            Failed
        };

        static constexpr std::size_t MaxWorldCreaturePendingTextureBuilds = 128U;

        using WorldCreatureResourceKey = std::pair<std::uint64_t, WorldCreatureTextureTarget>;

        struct WorldCreatureLayerSpritePack
        {
            std::uint32_t body{ 0 };
            std::filesystem::path packPath;
            std::filesystem::path indexPath;
            std::wstring cacheName;
            std::wstring extension;
            assets::LegacyAssetFormat format{ assets::LegacyAssetFormat::IndexedSpritePack };
        };

        struct PendingWorldCreatureTextureRequest
        {
            std::uint64_t generation{ 0 };
            WorldCreatureTextureTarget target{ WorldCreatureTextureTarget::Body };
            std::uint64_t key{ 0 };
            std::filesystem::path spritePackDirectory;
            std::filesystem::path spritePackIndex;
            std::wstring spritePackCacheName;
            std::wstring spritePackExtension;
            assets::LegacyAssetFormat spritePackFormat{ assets::LegacyAssetFormat::IndexedSpritePack };
            std::shared_ptr<const std::vector<assets::LegacyFrameSequence>> frameSequences;
            std::vector<std::pair<std::uint32_t, assets::LegacyIndexedColorMapping>> bodyDrawOrder;
            std::vector<WorldCreatureLayerSpritePack> layerSpritePacks;
            std::uint32_t action{ 0 };
            std::uint32_t direction{ 0 };
            std::size_t frameIndex{ 0 };
            std::size_t frameCount{ 0 };
            bool buildAnimationSequence{ false };
            bool layeredSpritePacks{ false };
            bool reduceAsShadow{ false };
            std::uint64_t requestId{ 0 };
            std::uint32_t entityId{ 0 };
            std::uint32_t spriteId{ 0 };
            std::uint64_t requestTimeMs{ 0 };
        };

        struct PreparedWorldCreatureTexture
        {
            std::uint64_t generation{ 0 };
            WorldCreatureTextureTarget target{ WorldCreatureTextureTarget::Body };
            std::uint64_t key{ 0 };
            graphics::ImageData image;
            std::int32_t originX{ 0 };
            std::int32_t originY{ 0 };
            std::vector<WorldCreatureTexture::Frame> frames;
            std::uint32_t composeMilliseconds{ 0 };
            std::uint64_t requestId{ 0 };
            std::uint32_t entityId{ 0 };
            std::uint32_t spriteId{ 0 };
            std::uint64_t requestTimeMs{ 0 };
        };

        struct PendingWorldMapLoad
        {
            struct PreparedSprite
            {
                bool tile{ false };
                std::uint32_t key{ 0 };
                graphics::ImageData image;
            };
            std::uint16_t zoneId{ 0 };
            std::filesystem::path mapPath;
            world::LegacyMapReadResult result;
            std::vector<PreparedSprite> preparedSprites;
            std::uint32_t loadMilliseconds{ 0 };
        };

        struct PendingWorldAssetDecode
        {
            std::uint64_t generation{ 0 };
            bool tile{ false };
            std::uint32_t key{ 0 };
            std::uint32_t spriteId{ 0 };
            std::filesystem::path packPath;
            std::filesystem::path indexPath;
            std::filesystem::path palettePath;
            std::uint16_t paletteId{ 0xFFFFU };
            bool alphaPixels{ false };
            assets::LegacyAssetFormat format{ assets::LegacyAssetFormat::SpritePack };
        };

        struct PreparedWorldAssetDecode
        {
            std::uint64_t generation{ 0 };
            bool tile{ false };
            std::uint32_t key{ 0 };
            graphics::ImageData image;
        };

        enum class FrameTextureLoadKind : std::uint8_t
        {
            Tile,
            Object,
            Creature,
            CreatureShadow,
            Item,
            Effect,
            Frontend,
            UiSprite,
            Text,
            Solid,
            Progress,
            Count
        };

        struct FramePerfCounters
        {
            std::uint32_t preloadMs{ 0 };
            std::uint32_t introMs{ 0 };
            std::uint32_t networkMs{ 0 };
            std::uint32_t runtimePrepMs{ 0 };
            std::uint32_t clearMs{ 0 };
            std::uint32_t sceneDrawMs{ 0 };
            std::uint32_t modalMs{ 0 };
            std::uint32_t overlayMs{ 0 };
            std::uint32_t presentMs{ 0 };
            std::uint32_t gameMotionMs{ 0 };
            std::uint32_t gameLoadingWarmupMs{ 0 };
            std::uint32_t gameDrawListMs{ 0 };
            std::uint32_t gameTileMs{ 0 };
            std::uint32_t gameLayerMs{ 0 };
            std::uint32_t gameHudMs{ 0 };
            std::array<std::uint32_t, static_cast<std::size_t>(FrameTextureLoadKind::Count)> textureLoadMs{};
            std::array<std::uint32_t, static_cast<std::size_t>(FrameTextureLoadKind::Count)> textureLoadCount{};
            std::array<std::uint32_t, static_cast<std::size_t>(FrameTextureLoadKind::Count)> textureUploadCount{};
        };

        struct FramePerfSample
        {
            std::uint64_t frameIndex{ 0 };
            std::uint64_t timeMs{ 0 };
            std::uint32_t totalMs{ 0 };
            std::uint32_t fps{ 0 };
            frontend::FrontendState state{ frontend::FrontendState::Login };
            std::uint32_t drawCalls{ 0 };
            std::uint32_t quads{ 0 };
            std::uint32_t pendingDecode{ 0 };
            std::uint32_t worldUploads{ 0 };
            std::uint32_t worldDeferredUploads{ 0 };
            std::uint32_t visibleTiles{ 0 };
            std::uint32_t visibleObjects{ 0 };
            std::uint32_t visibleCreatures{ 0 };
            std::uint32_t visibleEffects{ 0 };
            std::uint32_t culledTiles{ 0 };
            std::uint32_t culledObjects{ 0 };
            std::uint32_t liveObjects{ 0 };
            std::uint32_t groundItems{ 0 };
            std::uint32_t activeEffects{ 0 };
            FramePerfCounters counters;
        };

        struct ScopedFrameTextureLoad
        {
            ScopedFrameTextureLoad(Application& owner, FrameTextureLoadKind kind) noexcept;
            ScopedFrameTextureLoad(const ScopedFrameTextureLoad&) = delete;
            ScopedFrameTextureLoad& operator=(const ScopedFrameTextureLoad&) = delete;
            ~ScopedFrameTextureLoad();

            void MarkUploaded() noexcept;

            Application& owner;
            FrameTextureLoadKind kind{ FrameTextureLoadKind::Tile };
            std::chrono::steady_clock::time_point start;
            bool uploaded{ false };
        };

        enum class EnterWorldProfileKind : std::uint8_t
        {
            Cpu,
            DiskRead,
            Decode,
            GpuCreate,
            LockWait,
            NetworkWait
        };

        struct EnterWorldProfileMetric
        {
            std::uint64_t count{ 0 };
            double inclusiveMs{ 0.0 };
            double exclusiveMs{ 0.0 };
            double cpuMs{ 0.0 };
            double mainThreadMs{ 0.0 };
            double workerThreadMs{ 0.0 };
            double blockedMs{ 0.0 };
            double maxMs{ 0.0 };
            std::uint64_t bytes{ 0 };
            std::uint64_t failures{ 0 };
        };

        class ScopedEnterWorldProfile final
        {
        public:
            ScopedEnterWorldProfile(
                Application& owner,
                std::wstring_view name,
                EnterWorldProfileKind kind = EnterWorldProfileKind::Cpu,
                std::uint64_t bytes = 0) noexcept;
            ~ScopedEnterWorldProfile();
            ScopedEnterWorldProfile(const ScopedEnterWorldProfile&) = delete;
            ScopedEnterWorldProfile& operator=(const ScopedEnterWorldProfile&) = delete;
            void MarkFailed() noexcept { failed_ = true; }

        private:
            Application& owner_;
            std::wstring name_;
            EnterWorldProfileKind kind_{ EnterWorldProfileKind::Cpu };
            std::chrono::steady_clock::time_point start_;
            std::uint64_t startCpu100ns_{ 0 };
            std::uint64_t startReadBytes_{ 0 };
            std::uint64_t bytes_{ 0 };
            double childWallMs_{ 0.0 };
            ScopedEnterWorldProfile* parent_{ nullptr };
            bool active_{ false };
            bool failed_{ false };
            static thread_local ScopedEnterWorldProfile* current_;
        };

#if defined(PHOENIX_ENABLE_DIAGNOSTICS)
        void RecordEnterWorldProfile(
            std::wstring_view name,
            EnterWorldProfileKind kind,
            double inclusiveMs,
            double exclusiveMs,
            double cpuMs,
            bool mainThread,
            std::uint64_t bytes,
            bool failed);
        void WriteEnterWorldProfileSummary();
        void RecordVisibilityFrame(const FramePerfSample& sample);
        void WriteVisibilitySpikeSummary();
        void ConfigureVisibilityRouteDiagnostics();
        void RecordVisibilityRouteMove(std::int32_t dx, std::int32_t dy, std::uint8_t direction);
        void UpdateVisibilityRouteReplay();
        void SaveVisibilityRoute();
        void ConfigureAnimationClockDiagnostics();
        void CaptureAnimationClockTrace(bool renderPhase, std::uint64_t rawDeltaMs);
        void WriteAnimationClockTraceSummary();
        void ApplyDebugAnimationStallIfRequested();
        void ConfigureDrawOrderDiagnostics();
        void CaptureWorldDrawOrder(const world::WorldDrawList& drawList);
        void CaptureMapResourceIdentity();
#else
        void RecordEnterWorldProfile(std::wstring_view, EnterWorldProfileKind, double, double, double, bool, std::uint64_t, bool) noexcept {}
        void WriteEnterWorldProfileSummary() noexcept {}
        void RecordVisibilityFrame(const FramePerfSample&) noexcept {}
        void WriteVisibilitySpikeSummary() noexcept {}
        void ConfigureVisibilityRouteDiagnostics() noexcept {}
        void RecordVisibilityRouteMove(std::int32_t, std::int32_t, std::uint8_t) noexcept {}
        void UpdateVisibilityRouteReplay() noexcept {}
        void SaveVisibilityRoute() noexcept {}
        void ConfigureAnimationClockDiagnostics() noexcept {}
        void CaptureAnimationClockTrace(bool, std::uint64_t) noexcept {}
        void WriteAnimationClockTraceSummary() noexcept {}
        void ApplyDebugAnimationStallIfRequested() noexcept {}
        void ConfigureDrawOrderDiagnostics() noexcept {}
        void CaptureWorldDrawOrder(const world::WorldDrawList&) noexcept {}
        void CaptureMapResourceIdentity() noexcept {}
#endif

        HINSTANCE instance_{ nullptr };
        HWND window_{ nullptr };
        settings::DisplaySettings display_;
        frontend::FrontendStateMachine frontend_;
        frontend::ServerSelectModel serverSelect_;
        frontend::CharacterSelectModel characterSelect_;
        frontend::CharacterCreateModel characterCreate_;
        ui::LoginInputModel loginInput_;
        backends::directx11::DirectX11Renderer renderer_;
        ui::text::DirectWriteTextRenderer textRenderer_;
        graphics::TextureDescriptor whiteTexture_;
        std::map<std::uint32_t, graphics::TextureDescriptor> renewalTextures_;
        std::map<std::uint32_t, graphics::TextureDescriptor> titleMenuTextures_;
        std::map<std::uint32_t, graphics::TextureDescriptor> newIdCreateTextures_;
        std::map<std::uint32_t, graphics::TextureDescriptor> optionTextures_;
        std::map<std::uint32_t, graphics::TextureDescriptor> hotkeySettingTextures_;
        std::map<std::uint32_t, graphics::TextureDescriptor> progressEventTextures_;
        std::vector<std::uint32_t> progressEventLoadingSpriteIds_;
        std::optional<std::uint32_t> selectedProgressEventLoadingSpriteId_;
        std::map<std::uint32_t, graphics::TextureDescriptor> serverSelectTextures_;
        std::map<std::uint32_t, graphics::TextureDescriptor> characterSelectTextures_;
        std::map<std::uint32_t, graphics::TextureDescriptor> characterCreateTextures_;
        std::map<std::uint32_t, graphics::TextureDescriptor> characterCreatePreviewTextures_;
        std::map<std::uint32_t, graphics::TextureDescriptor> gameUiTextures_;
        std::map<std::string, std::vector<graphics::TextureDescriptor>> legacyGameUiTextures_;
        std::set<std::string> legacyGameUiTextureCompleteKeys_;
        std::set<std::string> legacyGameUiTextureMissingKeys_;
        std::map<std::uint32_t, graphics::TextureDescriptor> worldTileTextures_;
        std::map<std::uint32_t, graphics::TextureDescriptor> worldImageObjectTextures_;
        std::map<std::uint64_t, WorldCreatureTexture> worldCreatureTextures_;
        std::map<std::uint64_t, WorldCreatureTexture> worldCreatureShadowTextures_;
        std::map<std::uint32_t, std::uint64_t> worldCreatureLastBodyTextureKey_;
        std::map<std::uint32_t, std::uint64_t> worldCreatureLastShadowTextureKey_;
        std::map<std::uint64_t, graphics::TextureDescriptor> worldItemDropTextures_;
        std::map<std::uint64_t, WorldEffectTexture> worldEffectTextures_;
        std::map<std::uint32_t, graphics::TextureDescriptor> solidColorTextures_;
        std::map<std::string, graphics::TextureDescriptor> asciiTextTextures_;
        std::map<std::wstring, std::shared_ptr<const std::vector<assets::LegacyFrameSequence>>> worldCreatureFrameSequences_;
        std::vector<WorldHitRegion> worldHitRegions_;
        graphics::TextureDescriptor usernameTextTexture_;
        graphics::TextureDescriptor passwordTextTexture_;
        graphics::TextureDescriptor serverNameTextTexture_;
        graphics::TextureDescriptor serverStatusTextTexture_;
        graphics::TextureDescriptor channelNameTextTexture_;
        graphics::TextureDescriptor channelStatusTextTexture_;
        graphics::TextureDescriptor serverPrevTextTexture_;
        graphics::TextureDescriptor serverNextTextTexture_;
        std::string cachedUsernameText_;
        std::string cachedPasswordText_;
        std::string cachedServerNameText_;
        std::string cachedServerStatusText_;
        std::string cachedChannelNameText_;
        std::string cachedChannelStatusText_;
        std::string cachedServerPrevText_;
        std::string cachedServerNextText_;
        settings::RuntimeInterfaceProfile interfaceProfile_;
        std::uint32_t selectedLoginBackgroundSpriteId_{ 3 };
        std::filesystem::path interfacePath_;
        std::filesystem::file_time_type interfaceLastWrite_{};
        std::optional<ui::LoginTitleSpriteRole> hoveredLoginRole_;
        std::optional<ui::LoginTitleSpriteRole> pressedLoginRole_;
        std::optional<ServerSelectButtonRole> hoveredServerSelectButton_;
        std::optional<ServerSelectButtonRole> pressedServerSelectButton_;
        ui::ClientUIEditor uiEditor_;
        OptionTab selectedOptionTab_{ OptionTab::Graphics };
        std::uint32_t optionPendingWidth_{ 1920 };
        std::uint32_t optionPendingHeight_{ 1080 };
        std::uint32_t optionGraphFlags_{ 0x000001DFU };
        std::uint32_t optionGameFlags_{ 0x000001FFU };
        std::uint32_t optionAdvancedFlags_{ 0x0000003FU };
        std::uint32_t optionAdvanced2Flags_{ 0x00000024U };
        std::uint32_t optionFrameLimit_{ 60 };
        ui::LegacyWindowTransparency optionTransparency_{ true };
        float optionMusicVolume_{ 0.8F };
        float optionEffectsVolume_{ 0.8F };
        float optionGamma_{ 1.0F };
        float optionBrightness_{ 0.5F };
        bool optionFullscreen_{ false };
        bool optionLinearFiltering_{ true };
        bool optionMusicEnabled_{ true };
        bool optionEffectsEnabled_{ true };
        std::uint32_t chatTextColor_{ RGB(198, 195, 198) };
        bool chatWhite_{ false };
        bool chatFilteringCurse_{ true };
        bool chatShowBeginnerHelp_{ true };
        bool chatPopupByWhisper_{ true };
        bool chatDoNotShowWar_{ false };
        bool chatDoNotShowLair_{ false };
        bool chatDoNotShowHolyLand_{ false };
        bool running_{ false };
        bool mouseDown_{ false };
        bool leftMouseDown_{ false };
        bool mouseDownConsumedByUi_{ false };
        bool chatInputActive_{ false };
        std::string chatImeComposition_;
        bool chatAssetLoadLogged_{ false };
        ui::ChatPanel chatPanel_;
        std::unique_ptr<ui::TitleScreen> titleScreen_;
        std::unique_ptr<ui::ItemListWindow> itemListWindow_;
        std::unique_ptr<ui::ImageNoticeWindow> imageNoticeWindow_;
        std::unique_ptr<ui::BulletinBoardWindow> bulletinBoardWindow_;
        std::unique_ptr<ui::ResurrectRequestWindow> resurrectRequestWindow_;
        std::unique_ptr<ui::MixingForgeWindow> mixingForgeWindow_;
        std::unique_ptr<ui::RemoveOptionWindow> removeOptionWindow_;
        std::unique_ptr<ui::OustersSkillInfoWindow> oustersSkillInfoWindow_;
        std::unique_ptr<ui::GenericBillingPromptWindow> genericBillingPromptWindow_;
        std::unique_ptr<ui::LegalDocumentsWindow> legalDocumentsWindow_;
        std::unique_ptr<ui::LoginScreen> loginScreen_;
        std::unique_ptr<ui::ServerSelectScreen> serverSelectScreen_;
        std::unique_ptr<ui::CharacterSelectScreen> characterSelectScreen_;
        std::unique_ptr<ui::CharacterCreateScreen> characterCreateScreen_;
        std::unique_ptr<ui::CharacterDeleteScreen> characterDeleteScreen_;
        std::unique_ptr<ui::OptionsScreen> optionsScreen_;
        std::unique_ptr<ui::NewUserScreen> newUserScreen_;
        std::unique_ptr<ui::CharInfoWindow> charInfoWindow_;
        std::unique_ptr<ui::HpBarWindow> hpBarWindow_;
                ui::UiRect hpBarScreenRect_{ 0, 0, 0, 0 };
                bool hpBarDragging_{ false };
                std::int32_t hpBarUserPosX_{ -1 };   // reference coords, -1 = default position
                std::int32_t hpBarUserPosY_{ -1 };
                std::int32_t hpBarDragGrabX_{ 0 };   // grab offset (mouse - barPos) captured at drag start
                std::int32_t hpBarDragGrabY_{ 0 };
        content::LegacyQuestCatalog questCatalog_;
        ui::QuestManagerState questManagerState_;
        bool inventoryOverlayOpen_{ false };
                                bool gearWindowOpen_{ false };
                                std::int32_t gearHoverSlot_{ -1 };
                                bool gearDragging_{ false };
                                std::int32_t gearDragOffsetX_{ 0 };
                                std::int32_t gearDragOffsetY_{ 0 };
                                ui::LegacyWindowTransparency inventoryTransparency_{ true };
                                bool skillWindowOpen_{ false };
                                std::uint8_t skillWindowPrimaryTab_{ 0 };
                                std::uint8_t skillWindowPage_{ 0 };
                                std::uint8_t skillWindowDomain_{ 0 };
                                std::int32_t skillWindowScrollRow_{ 0 };
                                std::optional<std::uint16_t> skillWindowFocusedSkill_;
                                std::optional<std::uint16_t> skillWindowPendingLearnSkill_;
                                std::uint8_t skillWindowPendingLearnDomain_{ 0 };
                                bool skillWindowLearnConfirmationOpen_{ false };
                                std::optional<std::uint32_t> skillWindowPendingRemoveRankGemSlot_;
                                std::array<bool, 8> skillWindowDomainReady_{};
                                std::array<std::optional<protocol::RankGemDataEntry>, 56> skillWindowRankGemSlots_{};
                                bool skillWindowRankGemRequestPending_{ false };
                                bool skillWindowPositionInitialized_{ false };
                                bool skillWindowDragging_{ false };
                                std::int32_t skillWindowReferenceX_{ 0 };
                                std::int32_t skillWindowReferenceY_{ 0 };
                                std::int32_t skillWindowDragOffsetX_{ 0 };
                                std::int32_t skillWindowDragOffsetY_{ 0 };
                                bool runeWindowOpen_{ false };
                                bool gameMenuOpen_{ false };
                                bool petInfoWindowOpen_{ false };
                                bool guildWindowOpen_{ false };
                                bool guildListWindowOpen_{ false };
                                bool waitingGuildWindowOpen_{ false };
                                bool guildMemberWindowOpen_{ false };
                                bool friendWindowOpen_{ false };
                                bool friendWaitWindowOpen_{ false };
                                bool partyWindowOpen_{ false };
                                bool tmallWindowOpen_{ false };
                                bool personalShopWindowOpen_{ false };
                                bool webMarketWindowOpen_{ false };
                                bool helpWindowOpen_{ false };
                                bool legacySessionEndPending_{ false };
        bool legacySessionEndQuitsApplication_{ false };
        std::chrono::steady_clock::time_point legacySessionEndDeadline_{};
        bool minimapOverlayOpen_{ true };
                bool mouseMoveActive_{ false };
                bool uiEditorListScrollbarDragging_{ false };
                bool optionResolutionDropdownOpen_{ false };
                bool optionHotkeyWindowOpen_{ false };
                bool fullscreenActive_{ false };
                bool highPrecisionTimerActive_{ false };
                bool bgmMciOpen_{ false };
                bool sfxMciOpen_{ false };
                bool introMciOpen_{ false };
                bool introMediaPlaying_{ false };
                bool optionTexturesAttempted_{ false };
                bool newIdCreateTexturesAttempted_{ false };
                bool hotkeySettingTexturesAttempted_{ false };
                bool progressEventTexturesAttempted_{ false };
                bool characterSelectTexturesAttempted_{ false };
                bool characterCreateTexturesAttempted_{ false };
                bool gameUiTexturesAttempted_{ false };
                // HP Bar Window
                LONG_PTR windowedStyle_{ WS_OVERLAPPEDWINDOW };
        RECT windowedRect_{ 0, 0, 0, 0 };
        std::size_t uiEditorListScrollOffset_{ 0 };
        std::int32_t uiEditorListScrollbarDragOffsetY_{ 0 };
        std::uint32_t measuredFps_{ 0 };
        std::uint32_t fpsFrameCounter_{ 0 };
        std::uint32_t lastFrameDurationMs_{ 0 };
        std::uint32_t worstFrameDurationMs_{ 0 };
        std::uint32_t worldFrameTextureUploadAttempts_{ 0 };
        std::uint32_t worldFrameDeferredTextureUploads_{ 0 };
        std::optional<std::uint32_t> measuredPingMs_;
        std::uint64_t pendingPingSentMs_{ 0 };
        std::uint64_t nextPingProbeMs_{ 0 };
        std::uint64_t nextLegacyVerifyTimeMs_{ 0 };
        std::uint64_t lastFrameStallLogMs_{ 0 };
        std::uint64_t frameSequence_{ 0 };
        std::uint64_t localPlayerVisualRevision_{ 0 };
        FramePerfCounters framePerfCounters_;
        std::deque<FramePerfSample> framePerfHistory_;
        std::uint16_t mouseMoveTargetX_{ 0 };
        std::uint16_t mouseMoveTargetY_{ 0 };
        std::uint16_t previousMouseMoveTargetX_{ 0xFFFFU };
        std::uint16_t previousMouseMoveTargetY_{ 0xFFFFU };
        std::optional<std::string> loginHostOverride_;
        std::optional<std::uint16_t> loginPortOverride_;
        std::optional<std::uint8_t> gameServerIdOverride_;
        LoginSocketPurpose loginSocketPurpose_{ LoginSocketPurpose::None };
        std::uint32_t loginReconnectAuthKey_{ 0 };
        NewIdRegistrationState newIdRegistration_;
        network::TcpClient loginClient_;
        network::TcpClient gameClient_;
        std::array<std::uint8_t, 6> clientMac_{};
        runtime::GameSessionModel gameSession_;
        bool loginDisconnectExpected_{ false };
        bool gameDisconnectExpected_{ false };
        std::chrono::steady_clock::time_point fpsSampleStart_;
        std::chrono::steady_clock::time_point frameLimitTimestamp_;
        std::chrono::steady_clock::time_point loadingPulseStart_;
        std::chrono::steady_clock::time_point introMediaStartedAt_;
        std::chrono::steady_clock::time_point worldFrameTextureUploadStart_;
        std::thread preloadThread_;
        std::mutex preloadMutex_;
        std::deque<PreloadedSprite> preloadQueue_;
        std::set<std::string> preloadQueuedKeys_;
        std::set<std::string> preloadUploadedKeys_;
        std::thread runtimeCatalogPreloadThread_;
        std::recursive_mutex runtimeCatalogMutex_;
        std::atomic_bool runtimeCatalogPreloadStarted_{ false };
        std::atomic_bool runtimeCatalogPreloadCompleted_{ false };
        std::thread worldCreatureTextureThread_;
        std::mutex worldCreatureTextureMutex_;
        std::condition_variable worldCreatureTextureCv_;
        std::deque<PendingWorldCreatureTextureRequest> worldCreatureTextureRequests_;
        std::deque<PreparedWorldCreatureTexture> worldCreatureTexturePrepared_;
        std::map<WorldCreatureResourceKey, WorldCreatureResourceState> worldCreatureResourceStates_;
        std::set<WorldCreatureResourceKey> worldCreatureBootstrapKeys_;
        bool worldCreatureBootstrapRequestActive_{ false };
        std::thread worldAssetDecodeThread_;
        std::mutex worldAssetDecodeMutex_;
        std::condition_variable worldAssetDecodeCv_;
        std::deque<PendingWorldAssetDecode> worldAssetDecodeRequests_;
        std::deque<PreparedWorldAssetDecode> worldAssetDecodePrepared_;
                std::set<std::uint64_t> worldAssetDecodeActiveKeys_;
                std::unordered_map<std::uint64_t, std::uint8_t> worldAssetUploadAttempts_;
                std::atomic_bool worldAssetDecodeStopRequested_{ false };
                std::atomic_bool worldAssetDecodeWorkerStarted_{ false };
        std::uint64_t worldAssetDecodeGeneration_{ 0 };
        std::uint64_t worldCreatureTextureBuildGeneration_{ 0 };
        std::set<std::uint32_t> worldTileTextureMisses_;
        std::set<std::uint32_t> worldImageObjectTextureMisses_;
        std::set<std::uint64_t> worldCreatureTextureMisses_;
        std::set<std::uint64_t> worldItemDropTextureMisses_;
        std::set<std::uint64_t> worldEffectTextureMisses_;
        std::set<std::wstring> loggedCreatureAnimationIssues_;
        std::set<std::uint32_t> creatureAnimationInvalidObjects_;
        struct CreatureAnimationVisualLogState
        {
            std::uint8_t state{ 0xFFU };
            std::uint8_t action{ 0xFFU };
            std::uint8_t direction{ 0xFFU };
            std::uint16_t frame{ 0U };
        };
        std::map<std::uint32_t, CreatureAnimationVisualLogState> creatureAnimationVisualLogStates_;
        std::set<std::uint16_t> loggedUnsupportedGamePacketIds_;
        std::map<std::uint64_t, graphics::ImageData> worldDecodedSpriteCache_;
        world::WorldAssetCacheStats worldAssetCacheStats_;
        std::uint32_t worldVisibleTileCount_{ 0 };
        std::uint32_t worldVisibleObjectCount_{ 0 };
        std::uint32_t worldVisibleGroundItemCount_{ 0 };
        std::uint32_t worldVisibleCreatureCount_{ 0 };
        std::uint32_t worldVisibleEffectCount_{ 0 };
        std::uint32_t worldCulledTileCount_{ 0 };
        std::uint32_t worldCulledObjectCount_{ 0 };
        std::optional<content::FileDefCatalogResult> fileDefCatalog_;
        std::optional<content::InfSchemaRegistryResult> infSchemaRegistry_;
        std::optional<content::GameStringCatalogResult> gameStringCatalog_;
        std::optional<content::AudioCatalogResult> audioCatalog_;
        std::optional<world::ZoneInfoCatalogResult> zoneInfoCatalog_;
        std::optional<content::CreatureInfoCatalogResult> creatureInfoCatalog_;
        std::optional<content::ItemInfoCatalogResult> itemInfoCatalog_;
        std::optional<content::ItemOptionCatalogResult> itemOptionCatalog_;
        std::optional<content::LegacyInfoCatalogResult> legacyInfoCatalog_;
        std::optional<content::ExperienceCatalogResult> experienceCatalog_;
        std::optional<ui::LegacyUiSpriteResourceReport> legacyUiSpriteResourceReport_;
        std::optional<content::EffectInfoCatalogResult> effectInfoCatalog_;
        std::optional<content::EffectStatusCatalogResult> effectStatusCatalog_;
        std::optional<std::vector<assets::LegacyEffectFrameSequence>> effectFrameSequences_;
        std::optional<std::vector<assets::LegacyEffectFrameSequence>> normalEffectFrameSequences_;
        std::optional<std::vector<assets::LegacyEffectFrameSequence>> shadowEffectFrameSequences_;
        std::optional<std::vector<assets::LegacyEffectFrameSequence>> screenEffectFrameSequences_;
        world::LegacyMapAnimationRenderer mapAnimationRenderer_;
        bool mapAnimationFramesAttempted_{ false };
        std::optional<world::ZoneInfo> activeZoneInfo_;
        std::optional<std::uint16_t> activeMinimapSpriteId_;
        std::optional<graphics::TextureDescriptor> activeMinimapTexture_;
        std::set<std::uint16_t> failedMinimapSpriteIds_;
        std::optional<world::LegacyMap> activeWorldMap_;
        world::LegacyTileSpriteIndex worldTileLogicalIndex_;
        std::uint32_t worldPhysicalTileSpriteCount_{ 0U };
        // Streaming prefetch icin ONCEKI frame'in genisletilmis sektor bounds'u.
        // Sahibi: Application (DrawGameFrame gunceller, harita yuklemesi resetler).
        // maxX/maxY == -1 => gecerli onceki kare yok (yeni zone / ilk kare).
        world::LegacyVisibleSectorBounds worldPrefetchPreviousBounds_{};
        std::thread worldLoadThread_;
        std::mutex worldLoadMutex_;
        std::optional<PendingWorldMapLoad> pendingWorldMapLoad_;
        WorldLoadingStage worldLoadingStage_{ WorldLoadingStage::Idle };
                std::string worldLoadingMessage_{ "Loading..." };
                float worldLoadingProgress_{ 0.0F };
                bool worldLoadingWarmupActive_{ false };
                std::vector<std::uint32_t> worldWarmupTileSpriteIds_;
                std::vector<std::size_t> worldWarmupObjectIndices_;
                std::size_t worldWarmupCursor_{ 0 };
                std::size_t worldWarmupSubCursor_{ 0 };
                std::uint32_t worldWarmupFrames_{ 0 };
        std::atomic_bool preloadStopRequested_{ false };
        std::atomic_bool preloadStarted_{ false };
        std::atomic_bool worldCreatureTextureStopRequested_{ false };
        std::atomic_bool worldCreatureTextureWorkerStarted_{ false };
        std::atomic_bool worldLoadStopRequested_{ false };
        std::atomic_bool worldLoadInProgress_{ false };
        std::uint64_t nextSystemMessageShiftMs_{ 0 };
        std::uint64_t nextGmMessageShiftMs_{ 0 };
        std::uint64_t nextNoticeMessageShiftMs_{ 0 };
        std::uint64_t nextWorldChatMessageShiftMs_{ 0 };
        std::deque<MouseMoveStep> mouseMovePath_;
        std::array<std::uint16_t, 10> skillHotbarSlots_{};
        std::int32_t cursorX_{ 0 };
        std::int32_t cursorY_{ 0 };
        std::uint32_t playerGold_{ 0 };
        phoenix::diagnostics::Logger* logger_{ nullptr };
        struct EnterWorldEntityTrace
        {
            std::uint32_t entityId{ 0 };
            std::uint32_t spriteId{ 0 };
            std::string type;
            std::uint64_t packetMs{ 0 };
            std::uint64_t createMs{ 0 };
            std::uint64_t resourceReadyMs{ 0 };
            std::uint64_t firstDrawMs{ 0 };
            bool cacheHit{ false };
        };
        bool enterWorldDiagnosticActive_{ false };
        bool enterWorldReady_{ false };
        bool enterWorldInputEnabled_{ false };
        bool enterWorldLoadingVisible_{ false };
        std::uint32_t enterWorldStableFrames_{ 0 };
        std::uint64_t enterWorldStartMs_{ 0 };
        std::uint64_t enterWorldStageStartMs_{ 0 };
        std::uint64_t enterWorldFirstUpdateMs_{ 0 };
        std::uint64_t enterWorldFirstVisibleMs_{ 0 };
        std::uint32_t enterWorldPostLoadingFrames_{ 0 };
        std::uint64_t nextCreatureDiagnosticRequestId_{ 1 };
        std::map<std::uint32_t, EnterWorldEntityTrace> enterWorldEntityTraces_;
        std::map<std::wstring, std::uint32_t> enterWorldResourceRequests_;
        std::uint64_t enterWorldCacheHits_{ 0 };
        std::uint64_t enterWorldCacheMisses_{ 0 };
        std::uint64_t enterWorldMainThreadResourceMs_{ 0 };
        std::wstring enterWorldCorrelationId_;
        bool enterWorldDetailedLoggingEnabled_{ true };
        std::uint32_t enterWorldMainThreadId_{ 0 };
        std::mutex enterWorldProfileMutex_;
        std::map<std::wstring, EnterWorldProfileMetric> enterWorldProfileMetrics_;
        std::map<std::wstring, std::uint64_t> enterWorldMilestones_;
        std::vector<double> enterWorldFrameSamplesMs_;
        std::array<std::uint64_t, 8> enterWorldStageCallCounts_{};
        std::uint64_t enterWorldLastRuntimeSnapshotMs_{ 0 };
        std::uint64_t enterWorldLastPrivateBytes_{ 0 };
        std::uint64_t enterWorldLastWorkingSetBytes_{ 0 };
        std::uint64_t enterWorldCreatureQueueAttempts_{ 0 };
        std::uint64_t enterWorldCreatureQueueAccepted_{ 0 };
        std::uint64_t enterWorldCreatureQueueDuplicates_{ 0 };
        std::uint64_t enterWorldCreatureQueueDeferred_{ 0 };
        std::atomic_uint64_t enterWorldCreatureDecodeCompleted_{ 0 };
        std::atomic_uint64_t enterWorldCreatureDecodeFailed_{ 0 };
        std::uint64_t enterWorldCreatureUploadCompleted_{ 0 };
        std::uint64_t enterWorldCreatureUploadFailed_{ 0 };
        // Map, tile, world object, creature body, creature shadow, item, effect, atlas/SPK.
        std::array<std::uint64_t, 8> enterWorldResourceAttemptCounts_{};

        struct VisibilityFrameSample
        {
            FramePerfSample frame;
            std::uint16_t cameraX{ 0 };
            std::uint16_t cameraY{ 0 };
            std::int32_t viewLeft{ 0 };
            std::int32_t viewTop{ 0 };
            std::int32_t viewWidth{ 0 };
            std::int32_t viewHeight{ 0 };
            bool cameraCellChanged{ false };
            std::uint32_t newlyVisible{ 0 };
            std::uint32_t newlyCreated{ 0 };
            std::uint32_t spawnPackets{ 0 };
            std::uint32_t firstSpriteRequests{ 0 };
            std::uint32_t animationInitializations{ 0 };
            std::uint32_t visibilityOnlyEntries{ 0 };
            std::uint32_t readyFirstDraws{ 0 };
            std::uint64_t diskReadBytes{ 0 };
            std::uint64_t layerSortUs{ 0 };
            std::uint64_t mainThreadAllocationBytes{ 0 };
            double lockWaitMs{ 0.0 };
            double collisionMs{ 0.0 };
            double pathfindingMs{ 0.0 };
            std::size_t scannedWorldObjects{ 0 };
            std::size_t scannedStaticObjects{ 0 };
        };

        struct VisibilitySpikeCapture
        {
            std::uint64_t spikeFrame{ 0 };
            std::uint64_t spikeTimeMs{ 0 };
            std::uint64_t captureUntilMs{ 0 };
            std::vector<VisibilityFrameSample> frames;
            bool emitted{ false };
        };

        struct VisibilityRouteMove
        {
            std::uint64_t offsetMs{ 0 };
            std::int32_t dx{ 0 };
            std::int32_t dy{ 0 };
            std::uint8_t direction{ 0 };
        };

        std::deque<VisibilityFrameSample> visibilityRecentFrames_;
        std::vector<VisibilitySpikeCapture> visibilitySpikeCaptures_;
        std::set<std::uint32_t> previousVisibleEntityIds_;
        std::set<std::uint32_t> visibilityNewlyVisibleEntityIds_;
        std::set<std::uint32_t> visibilityFirstRequestEntityIdsThisFrame_;
        std::map<std::uint32_t, std::uint64_t> entityCreatedFrame_;
        std::set<std::uint64_t> visibilityRequestedResourceKeys_;
        std::uint16_t visibilityCameraX_{ 0 };
        std::uint16_t visibilityCameraY_{ 0 };
        std::uint16_t visibilityPreviousCameraX_{ 0 };
        std::uint16_t visibilityPreviousCameraY_{ 0 };
        bool visibilityCameraKnown_{ false };
        ui::UiRect visibilityViewRect_{};
        std::uint32_t visibilityNewlyVisibleThisFrame_{ 0 };
        std::uint32_t visibilityNewlyCreatedThisFrame_{ 0 };
        std::uint32_t visibilitySpawnPacketsThisFrame_{ 0 };
        std::uint32_t visibilityFirstSpriteRequestsThisFrame_{ 0 };
        std::uint32_t visibilityAnimationInitializationsThisFrame_{ 0 };
        std::uint32_t visibilityOnlyEntriesThisFrame_{ 0 };
        std::uint32_t visibilityReadyFirstDrawsThisFrame_{ 0 };
        std::uint64_t visibilityLayerSortUsThisFrame_{ 0 };
        std::uint64_t visibilityFrameReadBytesStart_{ 0 };
        std::uint64_t visibilityFramePrivateBytesStart_{ 0 };
        double visibilityLockWaitMsThisFrame_{ 0.0 };
        double visibilityCollisionMsThisFrame_{ 0.0 };
        double visibilityPathfindingMsThisFrame_{ 0.0 };
        std::size_t visibilityScannedWorldObjectsThisFrame_{ 0 };
        std::size_t visibilityScannedStaticObjectsThisFrame_{ 0 };
        bool visibilityDiagnosticsConfigured_{ false };
        bool visibilityDiagnosticsEnabled_{ false };
        enum class VisibilityRouteMode : std::uint8_t { Disabled, Record, Replay };
        VisibilityRouteMode visibilityRouteMode_{ VisibilityRouteMode::Disabled };
        std::vector<VisibilityRouteMove> visibilityRouteMoves_;
        std::size_t visibilityRouteReplayCursor_{ 0 };
        std::uint64_t visibilityRouteStartMs_{ 0 };
        std::filesystem::path visibilityRoutePath_;
        bool drawOrderCaptureEnabled_{ false };
        bool drawOrderCapturePending_{ false };
        std::uint64_t drawOrderCaptureSequence_{ 0 };

        struct AnimationClockTraceSample
        {
            std::uint64_t simulationTick{ 0 };
            std::uint64_t renderFrame{ 0 };
            std::uint64_t timestampMs{ 0 };
            std::uint64_t rawDeltaMs{ 0 };
            std::uint64_t clampedDeltaMs{ 0 };
            std::uint64_t accumulatorMs{ 0 };
            std::uint64_t catchUpSteps{ 0 };
            float beforeX{ 0.0F };
            float beforeY{ 0.0F };
            float afterX{ 0.0F };
            float afterY{ 0.0F };
            float renderX{ 0.0F };
            float renderY{ 0.0F };
            float distanceMoved{ 0.0F };
            std::uint16_t movementSpeed{ 0 };
            std::uint8_t direction{ 0 };
            std::uint8_t animationState{ 0 };
            std::uint8_t animationAction{ 0 };
            std::uint16_t animationFrame{ 0 };
            std::uint16_t movementFrame{ 0 };
            std::uint16_t movementFrameCount{ 0 };
            std::uint64_t animationLastUpdateMs{ 0 };
            bool movementActive{ false };
            bool animationUpdateCalled{ false };
            bool visible{ false };
            bool resourceReady{ false };
            bool firstVisible{ false };
            bool reVisible{ false };
            bool renderPhase{ false };
        };

        std::map<std::uint32_t, std::deque<AnimationClockTraceSample>> animationClockTraceRings_;
        std::set<std::uint32_t> animationClockTrackedCreatureIds_;
        std::set<std::uint32_t> animationClockEverVisibleIds_;
        std::map<std::uint32_t, std::pair<float, float>> animationClockBeforePositions_;
        std::map<std::uint32_t, std::uint64_t> animationClockBeforeTicks_;
        std::uint64_t animationSimulationTick_{ 0 };
        std::uint64_t animationLastUpdateCallMs_{ 0 };
        std::uint64_t animationCurrentRawDeltaMs_{ 0 };
        bool animationClockDiagnosticsConfigured_{ false };
#if defined(_DEBUG)
        std::uint32_t animationDebugStallMs_{ 0 };
        bool animationDebugStallInjected_{ false };
#endif
    };
}
