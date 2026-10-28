#pragma once

#include "Settings/DisplayResolutionCatalog.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace phoenix::settings
{
    struct InterfaceResolutionProfile
    {
        DisplayResolution resolution;
        DisplayResolution referenceCanvas{ 1280, 960 };
        std::wstring scalePolicy{ L"UniformFit" };
        std::wstring backgroundPolicy{ L"Fit" };
        std::vector<std::uint32_t> loginBackgroundSprites;
        bool swapChainOnly{ true };
        bool recreateRendererState{ false };
    };

    struct InterfaceProfileValidation
    {
        bool valid{ false };
        std::vector<std::wstring> errors;
    };

    // ── Ingame UI profiles ──

    // ── GameHpBar_1280x960 ────────────────────────────────────────────────
        // hpbar.spk sprite haritası (50 frame — Docs/HPBAR_LEGACY_PARITY_PLAN.md):
        //   sprite 0     : Level-Rank bar (orta üst şerit)
        //   sprite 1-6   : Minimize/Expand butonları (sol kenar üstünde durur)
        //   sprite 7     : Sol kenar çerçevesi  (BÜYÜK mod)
        //   sprite 8     : HP dolgu kırmızı     (BÜYÜK mod)
        //   sprite 9     : Sağ kenar çerçevesi  (BÜYÜK mod)
        //   sprite 10    : MP dolgu mavi        (BÜYÜK mod)
        //   sprite 11    : Gri bar arka planı   (BÜYÜK mod)
        //   sprite 12    : Sol kenar çerçevesi  (KÜÇÜK mod)
        //   sprite 13    : HP dolgu kırmızı     (KÜÇÜK mod)
        //   sprite 14    : Sağ kenar çerçevesi  (KÜÇÜK mod)
        //   sprite 15    : MP dolgu mavi        (KÜÇÜK mod)
        //   sprite 16    : Gri bar arka planı   (KÜÇÜK mod)
        //   sprite 17-20 : Ousters MP sarı/yeşil varyantları (2. MP barı)
        //   sprite 21-38 : Height (dikey) mod sprite seti
        //   sprite 39-42 : Slayer orta parça  (40=erkek portre, 41=dişi portre, 42=arka plan)
        //   sprite 43-46 : Vampire orta parça (44=erkek portre, 45=dişi portre, 46=arka plan)
        //   sprite 47-49 : Ousters orta parça (48=portre, 49=arka plan)
        //
        // Not: Tüm *OffsetX/Y değerleri bar ana pozisyonuna (PositionX/Y) GÖRELİDİR.
        struct GameHpBarProfile
        {
            // ── Genel pencere ──
            std::int32_t positionX{ 246 }, positionY{ 855 };          // Bar ana pozisyonu (tüm sprite'ların referans noktası)
            bool defaultWidthMode{true}, defaultSmallMode{false}, draggable{true};
            std::int32_t panelWidth{535}, panelHeight{43};            // Drag hitbox boyutu (sprite 7+orta+9 toplamı)

            // ── Büyük mod sprite offset'leri ──
            std::int32_t leftEdgeOffsetX{0}, leftEdgeOffsetY{0};      // sprite 7  — Sol kenar (büyük)
            std::int32_t centerOffsetX{1}, centerOffsetY{13};         // sprite 40/41/44/45/48 — Irk portresi (X: sol kenarın SAĞINA göreli)
            std::int32_t rightEdgeGapX{1}, rightEdgeOffsetY{0};       // sprite 9  — Sağ kenar (büyük) (X: portrenin sağına göreli boşluk)
            std::int32_t greyBarOffsetX{320}, greyBarOffsetY{17};     // sprite 11 — Gri bar arka planı (büyük)
            std::int32_t hpFillOffsetX{26}, hpFillOffsetY{19};        // sprite 8  — HP dolgu kırmızı (büyük)
            std::int32_t mpFillOffsetX{320}, mpFillOffsetY{19};       // sprite 10 — MP dolgu mavi (büyük)

            // ── Küçük mod sprite offset'leri ──
            std::int32_t smallLeftEdgeOffsetX{0}, smallLeftEdgeOffsetY{0};      // sprite 12 — Sol kenar (küçük)
            std::int32_t smallHpFillOffsetX{26}, smallHpFillOffsetY{19};        // sprite 13 — HP dolgu (küçük)
            std::int32_t smallRightEdgeOffsetX{1}, smallRightEdgeOffsetY{0};    // sprite 14 — Sağ kenar (küçük)
            std::int32_t smallMpFillOffsetX{320}, smallMpFillOffsetY{19};       // sprite 15 — MP dolgu (küçük)
            std::int32_t smallGreyBarOffsetX{320}, smallGreyBarOffsetY{17};     // sprite 16 — Gri bar (küçük)
            std::int32_t smallCenterOffsetX{1}, smallCenterOffsetY{13};         // sprite 40/41/44/45/48 — Irk portresi (küçük mod)

            // ── Butonlar + Level bar ──
            std::int32_t buttonOffsetX{10}, buttonOffsetY{15};        // sprite 1-6 — Minimize/Expand buton grubu başlangıcı
            std::int32_t levelBarOffsetX{236}, levelBarOffsetY{19};   // sprite 0  — Level-Rank bar

            // ── Metin offset'leri (sprite değil, ASCII text) ──
            std::int32_t textOffsetY{19};                             // Tüm metinlerin ortak Y'i
            std::int32_t hpTextOffsetX{26}, mpTextOffsetX{318};       // HP / MP metin X
            std::int32_t levelLabelOffsetX{236};                      // Level metin X
        };

    struct GameChatProfile
    {
        std::int32_t originX{}, originY{}, panelWidth{}, panelHeight{};
        bool visible{true};
        std::int32_t inputHeight{20}, lineHeight{14};
        std::int32_t historyLeft{}, historyRight{}, historyBottomOffset{};
        std::int32_t inputLeft{}, inputTextLeft{}, inputTextBottomOffset{};
        std::int32_t dragHandleWidth{8};
        std::int32_t channelButtonLeft{}, channelButtonTop{}, channelButtonGap{};
        std::int32_t channelSelectBarChannelFrame{};
        std::int32_t channelSelectBarStatusFrame{};
        std::int32_t channelSelectBarRaceInfoFrame{};
    };

    struct GameCharInfoProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{320}, windowHeight{480};
        bool draggable{false};
    };

    struct GameGearProfile
    {
        std::int32_t windowOffsetX{ 100 };
        std::int32_t windowY{ 128 };
        bool draggable{ false };
        std::int32_t statsPanelX{ 357 };
        std::int32_t statsBaseY{ 50 };
        std::int32_t statsLeftX{ 7 };
        std::int32_t statsMidX{ 69 };
        std::int32_t expBarX{ 37 };
        std::int32_t expBarY{ 43 };
        std::int32_t expBarFillX{ 38 };
        std::int32_t expBarFillY{ 44 };
        std::int32_t expBarW{ 100 };
        std::int32_t expBarH{ 6 };
        std::int32_t expPctTextX{ 117 };
        std::int32_t expPctTextY{ 44 };
        std::int32_t pointBtnX{ 199 };
        std::int32_t pointBtnSpacing{ 15 };
        std::int32_t closeBtnX{ 560 };
        std::int32_t closeBtnY{ 8 };
        std::int32_t helpBtnX{ 558 };
        std::int32_t helpBtnY{ 88 };
    };

    struct GameQuestProfile
    {
        std::int32_t listX{}, listY{37}, detailX{}, detailY{197};
        std::int32_t missionX{}, missionY{357};
        std::int32_t itemX{482}, itemY{360};
        std::int32_t iconX{}, iconY{};
    };

    struct GameTitleProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        std::int32_t backgroundSprite{};
        std::int32_t aniMarkX{}, aniMarkY{};
        std::int32_t buttonLayoutX{}, buttonLayoutY{};
        std::int32_t idEditX{}, idEditY{}, pwEditX{}, pwEditY{};
        std::int32_t idCreateX{}, idCreateY{};
        std::int32_t gameRunX{}, gameRunY{};
        std::int32_t gameExitX{}, gameExitY{};
        std::int32_t optionX{}, optionY{};
        std::int32_t gameCreatersX{}, gameCreatersY{};
        std::int32_t textImageX{}, textImageY{};
        std::int32_t warningStringX{}, warningStringY{};
        std::int32_t violenceX{}, violenceY{};
        std::int32_t adultX{}, adultY{};
        std::int32_t childX{}, childY{};
        std::int32_t fearX{}, fearY{};
        std::int32_t effectWaterLightX{}, effectWaterLightY{};
        std::int32_t titleFrameNormal{};
        std::int32_t titleFrame1024Low{}, titleFrame1024Mid{}, titleFrame1024High{};
        std::int32_t serverTitleFrame{}, worldTitleFrame{}, worldSelectBarFrame{};
        std::int32_t messageBoxFrame{};
        bool useDefault{ false };
        bool useDefaults{ false };
        bool slayerCurrentClassTextPosEnabled{ false };
        std::string slayerInventorySpkName;
        std::string assembleBoxButtonSpkName;
        std::int32_t aniArphaStart{};
    };

    struct GameItemListProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool draggable{ false };
        std::int32_t itemListX{}, itemListY{}, itemListWidth{}, itemListHeight{};
        std::int32_t visibleLines{}, lineGap{};
        std::int32_t okButtonX{}, okButtonY{}, cancelButtonX{}, cancelButtonY{};
        std::int32_t okButtonNormalFrame{}, okButtonHighlightedFrame{}, okButtonPressedFrame{};
        std::int32_t cancelButtonNormalFrame{}, cancelButtonHighlightedFrame{}, cancelButtonPressedFrame{};
        std::string okButtonLabel, cancelButtonLabel;
        std::int32_t scrollBarX{}, scrollBarY{}, scrollBarWidth{}, scrollBarHeight{};
        std::int32_t scrollBarMaxPos{};
        bool scrollBarHorizontal{ false };
        std::int32_t nextButtonFrame{}, prevButtonFrame{};
    };

    struct GameImageNoticeProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool topmost{ false };
        std::int32_t textOffsetX{}, nameOffsetY{}, msgOffsetY{}, giftOffsetY{};
        std::int32_t lineHeight{}, maxDisplayLines{}, autoCloseTimeMs{};
        float fadeOutSpeed{ 0.0f };
    };

    // ── Other window profiles ──

    struct BulletinBoardProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool topmost{ false }, keyboardControl{ false }, draggable{ false };
        std::int32_t okButtonX{}, okButtonY{}, cancelButtonX{}, cancelButtonY{};
        std::int32_t buttonOkFrame{}, buttonCancelFrame{}, mainWindowFrame{};
        std::int32_t okButtonFrame{}, cancelButtonFrame{};
        std::string backSpkName;
        std::int32_t fromX{}, fromY{}, fromByteLimit{};
        std::int32_t msgX{}, msgY{}, msgWidth{}, msgHeight{}, msgByteLimit{};
        std::int32_t ssnPart1X{}, ssnPart1Y{}, ssnPart1ByteLimit{};
        std::int32_t ssnPart2X{}, ssnPart2Y{}, ssnPart2ByteLimit{};
    };

    struct ResurrectRequestProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        std::int32_t listX{}, listY{}, listWidth{}, listHeight{};
        std::int32_t buttonGap{}, elixirFrame{};
        std::int32_t scrollX{}, scrollY{};
        std::int32_t resurrectScrollX{}, resurrectScrollY{};
        std::int32_t resurrectScrollFrame{};
        std::int32_t itemTextOffsetX{}, itemTextOffsetY{};
        std::int32_t okButtonX{}, okButtonY{}, cancelButtonX{}, cancelButtonY{};
        std::int32_t descX{}, descY{}, descWidth{}, descHeight{};
        std::int32_t eternityFrame{};
        std::int32_t guardOustersFrame{}, guardSlayerFrame{}, guardVampireFrame{};
        std::int32_t oustersYOffset{};
        std::int32_t siegeAttackerOustersFrame{}, siegeAttackerSlayerFrame{}, siegeAttackerVampireFrame{};
        std::int32_t skillFromOtherOustersFrame{}, skillFromOtherSlayerFrame{}, skillFromOtherVampireFrame{};
        std::int32_t slayerYOffset{}, vampireYOffset{};
    };

    struct MixingForgeProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        std::string backSpkName;
        std::int32_t buttonCancelFrame{}, buttonOkFrame{};
        std::int32_t cancelButtonX{}, cancelButtonY{};
        std::int32_t gridX{}, gridY{};
        std::int32_t inventoryWindowFrame{}, mainWindowFrame{};
        std::int32_t okButtonX{}, okButtonY{};
        std::int32_t slayerGridX{}, slayerGridY{};
        std::int32_t slot1X{}, slot1Y{}, slot2X{}, slot2Y{};
        std::string slayerInventorySpkName;
        std::int32_t slayerInventoryX{}, slayerInventoryY{};
    };

    struct RemoveOptionProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool topmost{ false };
        std::string backSpkName;
        std::int32_t cancelButtonFrame{}, cancelButtonX{}, cancelButtonY{};
        std::int32_t fieldY{}, gapY{};
        std::int32_t okButtonFrame{}, okButtonX{}, okButtonY{};
    };

    struct OustersSkillInfoProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool topmost{ false };
        std::string assembleBoxButtonSpkName;
        std::string backSpkName;
        std::int32_t closeButtonFrame{}, closeButtonX{}, closeButtonY{};
        std::int32_t helpButtonFrame{}, helpButtonX{}, helpButtonY{};
        std::int32_t learnButtonFrame{}, learnButtonX{}, learnButtonY{};
        std::int32_t oustersDownSkillButtonFrame{};
        std::int32_t oustersSkillBack2Frame{}, oustersSkillBackFrame{};
        std::int32_t scrollBarHeight{}, scrollBarWidth{}, scrollBarX{}, scrollBarY{};
    };

    struct CharacterCreateProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool keyboardControl{ false }, topmost{ false };
        std::string advancementOustersIspkName, advancementSlayerManIspkName;
        std::string advancementSlayerWomanIspkName, advancementVampireManIspkName, advancementVampireWomanIspkName;
        std::int32_t aniMarkX{};
        std::int32_t charLeftRotationButtonX{}, charLeftRotationButtonY{};
        std::int32_t charNameByteLimit{};
        std::int32_t charNameFocusX{}, charNameFocusY{};
        std::int32_t charRightRotationButtonX{}, charRightRotationButtonY{};
        std::string commonSpkName;
        std::string faceMakeSpkName;
        std::int32_t femaleSelectButtonX{}, femaleSelectButtonY{};
        std::string imageHaysSpkName, imageSpkName;
        std::int32_t loadButtonX{}, loadButtonY{};
        std::int32_t maleSelectButtonX{}, maleSelectButtonY{};
        std::int32_t nameCheckButtonX{}, nameCheckButtonY{};
        std::int32_t nextButtonX{}, nextButtonY{};
        std::string oustersIspkName;
        std::int32_t prevButtonX{}, prevButtonY{};
        std::int32_t resetButtonX{}, resetButtonY{};
        std::int32_t saveButtonX{}, saveButtonY{};
        std::int32_t selectCenterFaceRadioX{}, selectCenterFaceRadioY{};
        std::int32_t selectLeftFaceRadioX{}, selectLeftFaceRadioY{};
        std::int32_t selectRaceOustersButtonX{}, selectRaceOustersButtonY{};
        std::int32_t selectRaceSlayerButtonX{}, selectRaceSlayerButtonY{};
        std::int32_t selectRaceVampireButtonX{}, selectRaceVampireButtonY{};
        std::int32_t selectRightFaceRadioX{}, selectRightFaceRadioY{};
        std::int32_t slayerClassIconX{}, slayerClassIconY{};
        std::string slayerManIspkName, slayerWomanIspkName;
        std::int32_t stetMinusButtonX{}, stetMinusButtonY{}, stetPlusButtonX{}, stetPlusButtonY{};
                std::string useGradeSpkName, vampireIspkName;
                std::int32_t charViewX{}, charViewY{};
                std::int32_t childX{}, childY{};
                std::int32_t effectFireCenterX{}, effectFireCenterY{};
                std::int32_t effectFireLeft1X{}, effectFireLeft1Y{};
                std::int32_t effectFireLeft2X{}, effectFireLeft2Y{};
                std::int32_t effectFireRight1X{}, effectFireRight1Y{};
                std::int32_t effectFireRight2X{}, effectFireRight2Y{};
                std::int32_t femaleSelectRadioX{}, femaleSelectRadioY{};
                std::int32_t maleSelectRadioX{}, maleSelectRadioY{};
                std::int32_t selectRaceOustersRadioX{}, selectRaceOustersRadioY{};
                std::int32_t selectRaceSlayerRadioX{}, selectRaceSlayerRadioY{};
                std::int32_t selectRaceVampireRadioX{}, selectRaceVampireRadioY{};
                std::int32_t selectCenterFaceX{}, selectCenterFaceY{};
                std::int32_t selectLeftFaceX{}, selectLeftFaceY{};
                std::int32_t selectRightFaceX{}, selectRightFaceY{};
                std::int32_t hairColorBoxX{}, hairColorBoxY{};
                std::int32_t hairColorStartX{}, hairColorStartY{};
                std::int32_t skinColorBoxX{}, skinColorBoxY{};
                std::int32_t skinColorStartX{}, skinColorStartY{};
                std::int32_t opsetBoxX{}, opsetBoxY{};
                bool slayerClassLabelPosEnabled{ false };
                bool slayerCurrentClassTextPosEnabled{ false };
                        std::int32_t adultX{}, adultY{};
                        std::int32_t violenceX{}, violenceY{};
                        std::int32_t fearX{}, fearY{};
                        std::int32_t aniMarkY{};
                        std::int32_t textBarX{}, textBarY{};
                    };

    struct CharacterDeleteProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool topmost{ false }, keyboardControl{ false };
        std::string assembleBoxButtonSpkName;
        std::int32_t cancelButtonFrame{}, cancelButtonX{}, cancelButtonY{};
        std::int32_t messageBoxFrame{};
        std::int32_t okButtonFrame{}, okButtonX{}, okButtonY{};
        std::int32_t ssnPart1ByteLimit{}, ssnPart1X{}, ssnPart1Y{};
        std::int32_t ssnPart2ByteLimit{}, ssnPart2X{}, ssnPart2Y{};
    };

    struct CharacterSelectProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool topmost{ false }, keyboardControl{ false };
        std::int32_t aniArphaStart{};
        std::int32_t buttonBlueNormalFrame{}, buttonGreenNormalFrame{}, buttonRedNormalFrame{};
        std::int32_t buttonSelRoleNormalFrame{}, buttonYellowNormalFrame{};
        std::int32_t charCreateButtonX{}, charCreateButtonY{};
        std::int32_t charDeleteButtonX{}, charDeleteButtonY{};
        std::int32_t gameStartButtonX{}, gameStartButtonY{};
        std::string imageHays0SpkName, imageHays1SpkName, imageHays2SpkName, imageSpkName;
        std::string netmarbleAgreementSpkName;
        std::int32_t none1024Frame{}, none1280_1024Frame{}, none1280_960Frame{}, none800Frame{};
        std::int32_t ousters1024Frame{}, ousters1280_1024Frame{}, ousters1280_960Frame{};
        std::int32_t panalNoneFrame{}, panelX{}, panelY{};
        std::int32_t prevButtonX{}, prevButtonY{};
        std::int32_t selectBtnCenterX{}, selectBtnCenterY{};
        std::int32_t selectBtnLeftX{}, selectBtnLeftY{}, selectBtnRightX{}, selectBtnRightY{};
        std::int32_t slayer1024Frame{}, slayer1280_1024Frame{}, slayer1280_960Frame{};
        std::int32_t slayerCharInfo1024Frame{}, slayerCharInfo1280_1024Frame{}, slayerCharInfo1280_960Frame{};
        std::string useGradeSpkName;
                std::int32_t adultX{}, adultY{}, adultFrame{};
                std::int32_t childX{}, childY{}, childFrame{};
                std::int32_t violenceX{}, violenceY{}, violenceFrame{};
                std::int32_t fearX{}, fearY{}, fearFrame{};
                std::int32_t aniMarkX{}, aniMarkY{}, aniMarkFrame{};
                std::int32_t charInfoX{}, charInfoY{};
                std::int32_t slayerCharInfoX{}, slayerCharInfoY{};
                std::int32_t listInfoX{}, listInfoY{};
                std::int32_t listInfomationNormalFrame{}, listInfomationNoneFrame{};
                std::int32_t lockedSlot0X{}, lockedSlot0Y{}, lockedSlot1X{}, lockedSlot1Y{};
                std::int32_t panalFillFrame{};
                std::int32_t buttonSelRoleEnableFrame{};
                std::int32_t raceIconSlayerActiveFrame{}, raceIconVampireActiveFrame{}, raceIconOustersActiveFrame{};
            };

            struct ServerSelectProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool topmost{ false }, keyboardControl{ false };
        std::int32_t aniArphaStart{};
        std::int32_t channelListShowMax{}, channelListX{}, channelListY{};
        std::int32_t channelSelectBarChannelFrame{}, channelSelectBarStatusFrame{};
        std::string imageHaysSpkName, imageSpkName;
        std::int32_t nextButtonFrame{}, nextButtonX{}, nextButtonY{};
        std::int32_t prevButtonFrame{}, prevButtonX{}, prevButtonY{};
        std::int32_t selectChannelBarChannelX{}, selectChannelBarChannelY{};
        std::int32_t selectChannelBarIntervalX{}, selectChannelBarIntervalY{};
        std::int32_t selectChannelBarRaceInfoX{};
        std::int32_t selectChannelBarStatusX{}, selectChannelBarStatusY{};
        std::int32_t selectChannelNameFontX{}, selectChannelNameFontY{};
        std::int32_t selectChannelStatusFontX{}, selectChannelStatusFontY{};
        std::int32_t selectWorldBarIntervalX{}, selectWorldBarIntervalY{};
        std::int32_t selectWorldBarX{}, selectWorldBarY{};
        std::int32_t selectWorldNameFontX{}, selectWorldNameFontY{};
        std::int32_t selectWorldStatusFontX{}, selectWorldStatusFontY{};
        std::int32_t serverTitleFrame{};
        std::int32_t titleFrame1024High{}, titleFrame1024Low{}, titleFrame1024Mid{}, titleFrameNormal{};
        std::string titleHaysSpkName, titleSpkName;
        std::int32_t worldListShowMax{}, worldListX{}, worldListY{};
        std::int32_t worldSelectBarFrame{}, worldTitleFrame{};
                std::int32_t channelSelectBarRaceInfoFrame{};
                std::int32_t selectChannelBarRaceInfoY{};
            };

    struct LoginProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool topmost{ false }, keyboardControl{ false };
        std::int32_t cancelButtonFrame{}, cancelButtonX{}, cancelButtonY{};
        std::int32_t idByteLimit{}, idHeight{}, idWidth{}, idX{}, idY{};
        std::string loginSpkName, menuSpkName;
        std::int32_t newButtonFrame{}, newButtonX{}, newButtonY{};
        std::int32_t okButtonFrame{}, okButtonX{}, okButtonY{};
        std::int32_t passwordByteLimit{}, passwordHeight{}, passwordWidth{}, passwordX{}, passwordY{};
    };

    struct NewUserProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        std::int32_t accountAlreadyExistsFrame{}, accountCreatedFrame{}, accountNotCreatedFrame{};
        std::int32_t bgFrame{};
        std::int32_t closeButtonFrame{}, closeButtonX{}, closeButtonY{};
        std::string commonSpkName;
        std::int32_t createButtonX{}, createButtonY{}, createNormalFrame{};
        std::int32_t idByteLimit{};
        std::int32_t idEditHeight{}, idEditWidth{}, idEditX{}, idEditY{};
        std::string newUserSpkName;
        std::int32_t passwordByteLimit{}, passwordEditHeight{}, passwordEditWidth{}, passwordEditX{}, passwordEditY{};
        std::int32_t resultCloseButtonFrame{}, resultCloseButtonX{}, resultCloseButtonY{};
    };

    struct OptionsProfile
        {
            std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
            std::int32_t checkGap{}, checkX{}, checkY{};
            std::int32_t checkBackFrame{};
            std::int32_t closeButtonFrame{}, closeButtonX{}, closeButtonY{};
            std::string etcSpkName, oustersSpkName;
            std::int32_t radioBackFrame{};
            std::string scrollSpkName, slayerSpkName;
            std::int32_t tabControlFrame{}, tabControlHilightFrame{}, tabControlX{};
            std::int32_t tabGameFrame{}, tabGameHilightFrame{}, tabGameX{};
            std::int32_t tabGraphicFrame{}, tabGraphicHilightFrame{}, tabGraphicX{};
            std::int32_t tabOtherFrame{}, tabOtherHilightFrame{}, tabOtherX{};
            std::int32_t tabSoundFrame{}, tabSoundHilightFrame{}, tabSoundX{};
            std::int32_t tabY{};
            std::int32_t titleButtonExitFrame{}, titleButtonResetFrame{};
            std::int32_t titleCheckBackFrame{}, titleCloseButtonX{}, titleCloseButtonY{};
            std::string titleOption1024SpkName, titleOptionSpkName;
            std::int32_t titleRadioBackFrame{}, titleResetButtonX{}, titleResetButtonY{};
            std::string titleScrollSpkName, vampireSpkName;
            std::int32_t titleScrollX{}, titleScrollY{}, titleScrollWidth{}, titleScrollHeight{};
            std::int32_t optionScrollX{}, optionScrollY{}, optionScrollWidth{}, optionScrollHeight{};
            std::int32_t mouseSpeedValueX{}, mouseSpeedValueY{}, mouseSpeedValueWidth{}, mouseSpeedValueHeight{};
            std::int32_t gammaValueX{}, gammaValueY{}, gammaValueWidth{}, gammaValueHeight{};
            std::int32_t soundValueX{}, soundValueY{}, soundValueWidth{}, soundValueHeight{};
            std::int32_t musicValueX{}, musicValueY{}, musicValueWidth{}, musicValueHeight{};
            std::int32_t alphaValueX{}, alphaValueY{}, alphaValueWidth{}, alphaValueHeight{};
        };

    struct GenericBillingPromptProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool topmost{ false }, keyboardControl{ false };
        std::string backSpkName;
        std::int32_t cancelButtonFrame{}, cancelButtonX{}, cancelButtonY{};
        std::int32_t okButtonFrame{}, okButtonX{}, okButtonY{};
    };

    struct LegalDocumentsProfile
    {
        std::int32_t windowX{}, windowY{}, windowWidth{}, windowHeight{};
        bool topmost{ false }, keyboardControl{ false };
        std::string backSpkName;
        std::int32_t cancelButtonFrame{}, cancelButtonX{}, cancelButtonY{};
        std::string checkBoxSpkName;
        std::int32_t checkBoxX{}, checkBoxY{};
        std::int32_t okButtonFrame{}, okButtonX{}, okButtonY{};
        std::int32_t tabEULAX{}, tabPrivacyX{}, tabTermsX{};
    };

    struct CurrentInterfaceProfile {};

    // ── Aggregate profiles ──

    struct IngameUiProfile
    {
        GameHpBarProfile hpBar;
        GameChatProfile chat;
        GameCharInfoProfile charInfo;
        GameGearProfile gear;
        GameQuestProfile quest;
        GameTitleProfile title;
        GameItemListProfile itemList;
        GameImageNoticeProfile imageNotice;
        BulletinBoardProfile bulletinBoard;
    };

    struct InterfaceProfile
    {
        IngameUiProfile ingameUi;
        CharacterCreateProfile characterCreate;
    };

    struct RuntimeInterfaceProfile
    {
        DisplayResolution defaultResolution{ DisplayResolutionCatalog::Default() };
        std::vector<DisplayResolution> supportedResolutions;
        std::vector<InterfaceResolutionProfile> profiles;
        InterfaceProfileValidation validation;
        InterfaceProfile interfaceProfile;
        IngameUiProfile ingameUi;
        CharacterCreateProfile characterCreate;
    };

    class InterfaceProfileLoader final
    {
    public:
        [[nodiscard]] static RuntimeInterfaceProfile Load(const std::filesystem::path& path);
        [[nodiscard]] static InterfaceProfileValidation Validate(const RuntimeInterfaceProfile& profile);
    };
}