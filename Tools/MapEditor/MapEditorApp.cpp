#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <wincodec.h>

#include "Core/EditorSpriteArchiveReader.h"
#include "Backends/DirectX11/DirectX11Renderer.h"
#include "Runtime/GameRuntimeModels.h"
#include "World/LegacyMapReader.h"
#include "World/LegacyMapAnimationRenderer.h"
#include "World/LegacyMapWriter.h"
#include "MapEditorDocument.h"
#include "MapEditorExportModel.h"
#include "MapEditorMinimapModel.h"
#include "MapEditorPngWriter.h"
#include "MapEditorPreset.h"
#include "MapEditorRenderCache.h"
#include "MapEditorUiModel.h"
#include "MapEditorViewportModel.h"
#include "Core/EditorWorldDrawListBuilder.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    constexpr wchar_t kWindowClass[] = L"PhoenixMapEditorWindow";
    constexpr wchar_t kMinimapClass[] = L"PhoenixMapEditorMinimap";
    constexpr wchar_t kNewMapClass[] = L"PhoenixMapEditorNewMap";
    constexpr wchar_t kHelpDialogClass[] = L"PhoenixMapEditorHelpDialog";
    constexpr wchar_t kFillEdgePortalClass[] = L"PhoenixMapEditorFillEdgePortal";
    constexpr int kLeftDockWidth = 220;
    constexpr int kRightDockWidth = 420;
    constexpr UINT_PTR kRenderTimer = 1;
    constexpr UINT kCommitPropertyMessage = WM_APP + 17U;
    constexpr UINT kCancelPropertyMessage = WM_APP + 18U;

#if defined(_DEBUG)
    enum class ProfileArea : std::size_t
    {
        MapRender,
        DrawListGeneration,
        MinimapRendering,
        SpriteBrowser,
        PropertyPanel,
        SpkDecoding,
        TextureUpload,
        Count
    };

    struct ProfileSample
    {
        double totalMilliseconds{ 0.0 };
        double maximumMilliseconds{ 0.0 };
        std::uint64_t calls{ 0 };
    };

    class DebugProfiler final
    {
    public:
        class Scope final
        {
        public:
            Scope(DebugProfiler& owner, const ProfileArea area) noexcept
                : owner_(owner), area_(area), start_(std::chrono::steady_clock::now()) {}
            ~Scope() { owner_.Add(area_, std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start_).count()); }
        private:
            DebugProfiler& owner_;
            ProfileArea area_;
            std::chrono::steady_clock::time_point start_;
        };

        [[nodiscard]] Scope Measure(const ProfileArea area) noexcept { return Scope(*this, area); }
        [[nodiscard]] const ProfileSample& Sample(const ProfileArea area) const noexcept
        {
            return samples_[static_cast<std::size_t>(area)];
        }
    private:
        void Add(const ProfileArea area, const double milliseconds) noexcept
        {
            auto& sample = samples_[static_cast<std::size_t>(area)];
            sample.totalMilliseconds += milliseconds;
            sample.maximumMilliseconds = std::max(sample.maximumMilliseconds, milliseconds);
            ++sample.calls;
        }
        std::array<ProfileSample, static_cast<std::size_t>(ProfileArea::Count)> samples_{};
    };
#endif

    enum ControlId : int
    {
        IdNew = 100,
        IdOpen,
        IdSave,
        IdSaveAs,
        IdExit,
        IdUndo,
        IdRedo,
        IdCopy,
        IdPaste,
        IdDelete,
        IdGrid,
        IdViewMinimap,
        IdViewProperties,
        IdViewSprites,
        IdViewPresets,
        IdToolSelect,
        IdToolTile,
        IdToolErase,
        IdToolImage,
        IdToolAnimation,
        IdToolPortal,
        IdToolMove,
        IdZoomOut,
        IdZoomIn,
        IdZoomTrack,
        IdFitMap,
        IdActualSize,
        IdResetCamera,
        IdScreenshotFullMap,
        IdScreenshotGameMinimap,
        IdScreenshotHighQualityMinimap,
        IdBrowseSpk,
        IdBrowseSplit,
        IdBrowseAnimation,
        IdSpriteSearch,
        IdSpriteList,
        IdSpritePreview,
        IdPaletteTabs,
        IdTilePalette,
        IdObjectPalette,
        IdAnimationPalette,
        IdLayerTiles,
        IdLayerImages,
        IdLayerAnimations,
        IdLayerPortals,
        IdLayerCollision,
        IdLayerNpc,
        IdLayerMonsters,
        IdLayerGrid,
        IdProperties,
        IdPropertyEdit,
        IdPropertySpin,
        IdStatus,
        IdMinimap,
        IdPanelSplitter,
        IdPresetList,
        IdPresetName,
        IdPresetSave,
        IdPresetRename,
        IdPresetDelete,
        IdPresetRefresh,
        IdPresetPreview,
        IdAddAsPreset,
        IdHelpPortalTypes,
        IdFillEdgePortal
    };

    enum class ToolMode
    {
        Select,
        Tile,
        EraseTile,
        Image,
        Animation,
        Portal,
        Move
    };

    enum class PaletteKind
    {
        Tile,
        Image,
        Animation
    };

    using PropertyKey = phoenix::tools::map_editor::PropertyField;

    struct PropertyRow
    {
        std::wstring name;
        std::wstring value;
        PropertyKey key{ PropertyKey::ReadOnly };
        std::optional<std::size_t> positionIndex;
    };

    struct SpriteSource
    {
        std::filesystem::path archive;
        std::filesystem::path index;
        phoenix::assets::LegacyAssetFormat format{ phoenix::assets::LegacyAssetFormat::SpritePack };
        std::uint16_t count{ 0 };
        std::uint16_t splitUnit{ 0 };
        bool split{ false };
        phoenix::tools::map_editor::SpriteResourceType type{
            phoenix::tools::map_editor::SpriteResourceType::Tile };

        [[nodiscard]] bool SameSource(const SpriteSource& other) const noexcept
        {
            return archive == other.archive && index == other.index && format == other.format &&
                splitUnit == other.splitUnit && split == other.split && type == other.type;
        }
    };

    struct PresetCatalogEntry
    {
        std::filesystem::path path;
        std::optional<phoenix::tools::map_editor::MapPreset> preset;
        std::string error;
    };

    struct SpriteMetadata
    {
        bool attempted{ false };
        std::uint32_t width{ 0 };
        std::uint32_t height{ 0 };
        int thumbnailIndex{ -1 };
        std::filesystem::path sourcePath;
        std::uint32_t splitBase{ 0 };
        std::uint32_t internalIndex{ 0 };
        std::uint16_t representativeSpriteId{ 0xFFFFU };
        bool decoded{ false };
        std::optional<phoenix::world::LegacyMapAnimationSpriteMetadata> animation;
    };

    struct NewMapOptions
    {
        std::wstring name{ L"Untitled" };
        std::uint16_t width{ 64 };
        std::uint16_t height{ 64 };
        bool accepted{ false };
        bool closed{ false };
        HWND nameEdit{};
        HWND widthEdit{};
        HWND heightEdit{};
    };

    LRESULT CALLBACK NewMapWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* options = reinterpret_cast<NewMapOptions*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            options = static_cast<NewMapOptions*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(options));
        }
        if (options == nullptr) return DefWindowProcW(window, message, wParam, lParam);
        if (message == WM_CREATE)
        {
            const auto instance = reinterpret_cast<LPCREATESTRUCTW>(lParam)->hInstance;
            CreateWindowW(L"STATIC", L"Map name", WS_CHILD | WS_VISIBLE, 14, 16, 86, 22, window, nullptr, instance, nullptr);
            options->nameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", options->name.c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 105, 12, 190, 25, window, nullptr, instance, nullptr);
            CreateWindowW(L"STATIC", L"Width", WS_CHILD | WS_VISIBLE, 14, 51, 86, 22, window, nullptr, instance, nullptr);
            options->widthEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"64",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, 105, 47, 90, 25, window, nullptr, instance, nullptr);
            CreateWindowW(L"STATIC", L"Height", WS_CHILD | WS_VISIBLE, 14, 86, 86, 22, window, nullptr, instance, nullptr);
            options->heightEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"64",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, 105, 82, 90, 25, window, nullptr, instance, nullptr);
            CreateWindowW(L"BUTTON", L"Create", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                120, 124, 82, 28, window, reinterpret_cast<HMENU>(IDOK), instance, nullptr);
            CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                210, 124, 82, 28, window, reinterpret_cast<HMENU>(IDCANCEL), instance, nullptr);
            return 0;
        }
        if (message == WM_COMMAND && LOWORD(wParam) == IDOK)
        {
            wchar_t name[256]{}; wchar_t width[16]{}; wchar_t height[16]{};
            GetWindowTextW(options->nameEdit, name, static_cast<int>(std::size(name)));
            GetWindowTextW(options->widthEdit, width, static_cast<int>(std::size(width)));
            GetWindowTextW(options->heightEdit, height, static_cast<int>(std::size(height)));
            wchar_t* widthEnd = nullptr; wchar_t* heightEnd = nullptr;
            const auto parsedWidth = wcstoul(width, &widthEnd, 10);
            const auto parsedHeight = wcstoul(height, &heightEnd, 10);
            if (name[0] == L'\0' || widthEnd == width || *widthEnd != L'\0' || heightEnd == height || *heightEnd != L'\0' ||
                parsedWidth == 0U || parsedWidth > 4096U || parsedHeight == 0U || parsedHeight > 4096U)
            {
                MessageBoxW(window, L"Enter a map name and dimensions between 1 and 4096.", L"Invalid map settings", MB_ICONWARNING);
                return 0;
            }
            options->name = name;
            options->width = static_cast<std::uint16_t>(parsedWidth);
            options->height = static_cast<std::uint16_t>(parsedHeight);
            options->accepted = true;
            DestroyWindow(window);
            return 0;
        }
        if (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL) { DestroyWindow(window); return 0; }
        if (message == WM_CLOSE) { DestroyWindow(window); return 0; }
        if (message == WM_DESTROY) { options->closed = true; return 0; }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    std::wstring ToWide(const std::string& value);

    struct FillEdgePortalOptions
    {
        phoenix::tools::map_editor::EdgePortalFillParams params;
        bool accepted{ false };
        bool closed{ false };
        HWND zoneEdit{};
        HWND startXEdit{};
        HWND startYEdit{};
        HWND endXEdit{};
        HWND endYEdit{};
        HWND typeEdit{};
        HWND preview{};
    };

    bool ReadUnsignedField(HWND edit, unsigned long& value, const unsigned long maximum)
    {
        wchar_t text[16]{};
        GetWindowTextW(edit, text, static_cast<int>(std::size(text)));
        wchar_t* end = nullptr;
        value = wcstoul(text, &end, 10);
        return end != text && *end == L'\0' && value <= maximum;
    }

    void RefreshFillEdgePortalPreview(FillEdgePortalOptions& options)
    {
        if (options.preview == nullptr) return;
        unsigned long zone = 0, startX = 0, startY = 0, endX = 0, endY = 0, type = 0;
        if (!ReadUnsignedField(options.zoneEdit, zone, 65535UL) ||
            !ReadUnsignedField(options.startXEdit, startX, 255UL) ||
            !ReadUnsignedField(options.startYEdit, startY, 255UL) ||
            !ReadUnsignedField(options.endXEdit, endX, 255UL) ||
            !ReadUnsignedField(options.endYEdit, endY, 255UL) ||
            !ReadUnsignedField(options.typeEdit, type, 255UL))
        {
            SetWindowTextW(options.preview, L"Enter Zone ID 1-65535, dest 0-255, type 0/1/2/5/6/7.");
            return;
        }
        options.params.destZoneId = static_cast<std::uint16_t>(zone);
        options.params.destStartX = static_cast<std::uint8_t>(startX);
        options.params.destStartY = static_cast<std::uint8_t>(startY);
        options.params.destEndX = static_cast<std::uint8_t>(endX);
        options.params.destEndY = static_cast<std::uint8_t>(endY);
        options.params.type = static_cast<std::uint8_t>(type);
        SetWindowTextW(options.preview, ToWide(phoenix::tools::map_editor::DescribeEdgePortalFill(options.params)).c_str());
    }

    LRESULT CALLBACK FillEdgePortalWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* options = reinterpret_cast<FillEdgePortalOptions*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            options = static_cast<FillEdgePortalOptions*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(options));
        }
        if (options == nullptr) return DefWindowProcW(window, message, wParam, lParam);
        if (message == WM_CREATE)
        {
            const auto instance = reinterpret_cast<LPCREATESTRUCTW>(lParam)->hInstance;
            CreateWindowW(L"STATIC", L"Dest Zone ID", WS_CHILD | WS_VISIBLE, 14, 16, 110, 22, window, nullptr, instance, nullptr);
            options->zoneEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL, 130, 12, 90, 25, window, nullptr, instance, nullptr);
            CreateWindowW(L"STATIC", L"Dest start X", WS_CHILD | WS_VISIBLE, 14, 51, 110, 22, window, nullptr, instance, nullptr);
            options->startXEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, 130, 47, 90, 25, window, nullptr, instance, nullptr);
            CreateWindowW(L"STATIC", L"Dest start Y", WS_CHILD | WS_VISIBLE, 236, 51, 90, 22, window, nullptr, instance, nullptr);
            options->startYEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, 330, 47, 90, 25, window, nullptr, instance, nullptr);
            CreateWindowW(L"STATIC", L"Dest end X", WS_CHILD | WS_VISIBLE, 14, 86, 110, 22, window, nullptr, instance, nullptr);
            options->endXEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, 130, 82, 90, 25, window, nullptr, instance, nullptr);
            CreateWindowW(L"STATIC", L"Dest end Y", WS_CHILD | WS_VISIBLE, 236, 86, 90, 22, window, nullptr, instance, nullptr);
            options->endYEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, 330, 82, 90, 25, window, nullptr, instance, nullptr);
            CreateWindowW(L"STATIC", L"Portal type", WS_CHILD | WS_VISIBLE, 14, 121, 110, 22, window, nullptr, instance, nullptr);
            options->typeEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, 130, 117, 90, 25, window, nullptr, instance, nullptr);
            options->preview = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_EDITCONTROL,
                14, 156, 406, 40, window, nullptr, instance, nullptr);
            CreateWindowW(L"BUTTON", L"Fill", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                246, 208, 82, 28, window, reinterpret_cast<HMENU>(IDOK), instance, nullptr);
            CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                338, 208, 82, 28, window, reinterpret_cast<HMENU>(IDCANCEL), instance, nullptr);
            RefreshFillEdgePortalPreview(*options);
            return 0;
        }
        if (message == WM_COMMAND && HIWORD(wParam) == EN_CHANGE)
        {
            RefreshFillEdgePortalPreview(*options);
            return 0;
        }
        if (message == WM_COMMAND && LOWORD(wParam) == IDOK)
        {
            unsigned long zone = 0, startX = 0, startY = 0, endX = 0, endY = 0, type = 0;
            if (!ReadUnsignedField(options->zoneEdit, zone, 65535UL) || zone == 0UL ||
                !ReadUnsignedField(options->startXEdit, startX, 255UL) ||
                !ReadUnsignedField(options->startYEdit, startY, 255UL) ||
                !ReadUnsignedField(options->endXEdit, endX, 255UL) ||
                !ReadUnsignedField(options->endYEdit, endY, 255UL) ||
                !ReadUnsignedField(options->typeEdit, type, 255UL))
            {
                MessageBoxW(window,
                    L"Enter Zone ID 1-65535, destination X/Y 0-255, and portal type 0, 1, 2, 5, 6 or 7.",
                    L"Invalid edge portal fill", MB_ICONWARNING);
                return 0;
            }
            if (type == 3UL || type == 4UL || type > 7UL)
            {
                MessageBoxW(window, L"Edge fill does not support multi-target types 3/4 or client-only types.",
                    L"Invalid edge portal fill", MB_ICONWARNING);
                return 0;
            }
            options->params.destZoneId = static_cast<std::uint16_t>(zone);
            options->params.destStartX = static_cast<std::uint8_t>(startX);
            options->params.destStartY = static_cast<std::uint8_t>(startY);
            options->params.destEndX = static_cast<std::uint8_t>(endX);
            options->params.destEndY = static_cast<std::uint8_t>(endY);
            options->params.type = static_cast<std::uint8_t>(type);
            options->accepted = true;
            DestroyWindow(window);
            return 0;
        }
        if (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL) { DestroyWindow(window); return 0; }
        if (message == WM_CLOSE) { DestroyWindow(window); return 0; }
        if (message == WM_DESTROY) { options->closed = true; return 0; }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    // Legacy Portal.h / Zone.cpp — harita edit sırasında Portal Type seçimi için referans.
    constexpr wchar_t kPortalTypeHelpText[] =
        L"Portal Type — Legacy referans (Portal.h / Zone.cpp)\r\n"
        L"Veri .map içinde değil; .smp / .mip sidecar'larda tutulur.\r\n"
        L"\r\n"
        L"=== Server enum (asıl anlam) ===\r\n"
        L"\r\n"
        L"Değer   İsim                  Anlam\r\n"
        L"-----   --------------------  ----------------------------------------------\r\n"
        L"0       PORTAL_NORMAL         Herkese açık, tek hedef\r\n"
        L"1       PORTAL_SLAYER         Sadece Slayer\r\n"
        L"2       PORTAL_VAMPIRE        Sadece Vampire\r\n"
        L"3       PORTAL_MULTI_TARGET   Çoklu hedef (liste)\r\n"
        L"4       PORTAL_PRIVATE        Sidecar'da multi/heliport gibi işlenir; çoklu hedefe izin var\r\n"
        L"5       PORTAL_GUILD          Lonca portalı\r\n"
        L"6       PORTAL_BATTLE         Battle; runtime'da çoğu yerde Normal gibi kurulur\r\n"
        L"7       PORTAL_OUSTERS        Sadece Ousters\r\n"
        L"\r\n"
        L"=== Client-only tipler (.mip) ===\r\n"
        L"\r\n"
        L"Değer     Not\r\n"
        L"--------  --------------------------------------------------------------\r\n"
        L"200–203   Server'a yazılmaz (type < 200 → server portal)\r\n"
        L"255       Client-only\r\n"
        L"\r\n"
        L"=== Kritik kurallar ===\r\n"
        L"\r\n"
        L"1) Çoklu target yalnızca type 3 ve 4.\r\n"
        L"2) Portal enable için Portal targets dolu olmalı (zone:x:y veya zone:x:y;...).\r\n"
        L"3) Race kısıtı type ile gelir: Slayer→1, Vampire→2, Ousters→7, herkes→0.\r\n"
        L"4) Portal aracı + sağ tık: o karedeki .mip/.smp portal kaydını siler; tile silinmez.\r\n"
        L"5) Portal sürükle + Enter: kenar dolgusu (ZoneID + dest start/end). Kalınlık aynı dest.\r\n"
        L"6) Shift+sağ tık: seçimdeki tüm portal kayıtlarını siler; tile silinmez.\r\n";

    LRESULT CALLBACK HelpDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_CREATE)
        {
            const auto instance = reinterpret_cast<LPCREATESTRUCTW>(lParam)->hInstance;
            RECT client{};
            GetClientRect(window, &client);
            const int margin = 12;
            const int buttonH = 28;
            const int buttonW = 88;
            const int editW = (std::max)(1, static_cast<int>(client.right) - margin * 2);
            const int editH = (std::max)(1, static_cast<int>(client.bottom) - margin * 3 - buttonH);
            const auto edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", kPortalTypeHelpText,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                margin, margin, editW, editH, window, nullptr, instance, nullptr);
            if (edit != nullptr)
            {
                // Consolas: tablo hizası okunaklı kalsın.
                const auto mono = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                    FIXED_PITCH | FF_MODERN, L"Consolas");
                if (mono != nullptr)
                {
                    SendMessageW(edit, WM_SETFONT, reinterpret_cast<WPARAM>(mono), TRUE);
                    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(mono));
                }
            }
            const int closeX = (std::max)(margin, static_cast<int>(client.right) - margin - buttonW);
            CreateWindowW(L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                closeX, static_cast<int>(client.bottom) - margin - buttonH, buttonW, buttonH,
                window, reinterpret_cast<HMENU>(IDOK), instance, nullptr);
            return 0;
        }
        if (message == WM_SIZE)
        {
            const int margin = 12;
            const int buttonH = 28;
            const int buttonW = 88;
            const int width = LOWORD(lParam);
            const int height = HIWORD(lParam);
            if (HWND edit = FindWindowExW(window, nullptr, L"EDIT", nullptr))
                MoveWindow(edit, margin, margin, (std::max)(1, width - margin * 2),
                    (std::max)(1, height - margin * 3 - buttonH), TRUE);
            if (HWND close = GetDlgItem(window, IDOK))
                MoveWindow(close, (std::max)(margin, width - margin - buttonW),
                    height - margin - buttonH, buttonW, buttonH, TRUE);
            return 0;
        }
        if (message == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL))
        {
            DestroyWindow(window);
            return 0;
        }
        if (message == WM_CLOSE)
        {
            DestroyWindow(window);
            return 0;
        }
        if (message == WM_DESTROY)
        {
            if (const auto font = reinterpret_cast<HFONT>(GetWindowLongPtrW(window, GWLP_USERDATA)))
                DeleteObject(font);
            return 0;
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    std::wstring ToWide(const std::string& value)
    {
        if (value.empty()) return {};
        const auto count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (count <= 0) return std::wstring(value.begin(), value.end());
        std::wstring result(static_cast<std::size_t>(count), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), count);
        return result;
    }

    std::string ToUtf8(const std::wstring& value)
    {
        if (value.empty()) return {};
        const auto count = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (count <= 0) return std::string(value.size(), '?');
        std::string result(static_cast<std::size_t>(count), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), count, nullptr, nullptr);
        return result;
    }

    std::filesystem::path OpenFileDialog(HWND owner, const wchar_t* filter, bool save)
    {
        wchar_t path[32768]{};
        OPENFILENAMEW dialog{ sizeof(dialog) };
        dialog.hwndOwner = owner;
        dialog.lpstrFilter = filter;
        dialog.lpstrFile = path;
        dialog.nMaxFile = static_cast<DWORD>(std::size(path));
        dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
        dialog.lpstrDefExt = save ? L"map" : nullptr;
        const auto ok = save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
        return ok ? std::filesystem::path(path) : std::filesystem::path{};
    }

    std::filesystem::path SavePngDialog(HWND owner, const wchar_t* suggestedName)
    {
        wchar_t path[32768]{};
        wcsncpy_s(path, suggestedName, _TRUNCATE);
        OPENFILENAMEW dialog{ sizeof(dialog) };
        dialog.hwndOwner = owner;
        dialog.lpstrFilter = L"PNG image (*.png)\0*.png\0";
        dialog.lpstrFile = path;
        dialog.nMaxFile = static_cast<DWORD>(std::size(path));
        dialog.lpstrDefExt = L"png";
        dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        return GetSaveFileNameW(&dialog) ? std::filesystem::path(path) : std::filesystem::path{};
    }

    std::uint16_t ReadSplitUnit(const std::filesystem::path& folder)
    {
        std::ifstream file(folder / L"header.inf");
        std::vector<unsigned> values;
        unsigned value = 0;
        while (file >> value) values.push_back(value);
        if (values.size() >= 2U && values[1] <= 65535U) return static_cast<std::uint16_t>(values[1]);
        return 0;
    }

    std::filesystem::path SplitSpriteArchivePath(const std::filesystem::path& folder,
        const std::uint32_t splitBase, const wchar_t* extension = L"spk")
    {
        wchar_t filename[32]{};
        swprintf_s(filename, L"%05u.%s", splitBase, extension);
        return folder / filename;
    }

    phoenix::graphics::ImageData CreatePortalLabelImage()
    {
        constexpr std::array<std::array<const char*, 7>, 6> glyphs{ {
            { "11110", "10001", "10001", "11110", "10000", "10000", "10000" },
            { "01110", "10001", "10001", "10001", "10001", "10001", "01110" },
            { "11110", "10001", "10001", "11110", "10100", "10010", "10001" },
            { "11111", "00100", "00100", "00100", "00100", "00100", "00100" },
            { "01110", "10001", "10001", "11111", "10001", "10001", "10001" },
            { "10000", "10000", "10000", "10000", "10000", "10000", "11111" }
        } };
        constexpr std::uint32_t scale = 2U;
        constexpr std::uint32_t sourceWidth = 37U;
        constexpr std::uint32_t sourceHeight = 9U;
        phoenix::graphics::ImageData image;
        image.width = sourceWidth * scale;
        image.height = sourceHeight * scale;
        image.format = phoenix::graphics::PixelFormat::Rgba8;
        image.pixels.resize(static_cast<std::size_t>(image.width) * image.height * 4U);
        for (std::uint32_t y = 0; y < image.height; ++y)
        {
            for (std::uint32_t x = 0; x < image.width; ++x)
            {
                const auto pixel = (static_cast<std::size_t>(y) * image.width + x) * 4U;
                image.pixels[pixel] = 8U; image.pixels[pixel + 1U] = 10U;
                image.pixels[pixel + 2U] = 12U; image.pixels[pixel + 3U] = 190U;
            }
        }
        for (std::size_t glyph = 0; glyph < glyphs.size(); ++glyph)
        {
            for (std::uint32_t row = 0; row < 7U; ++row)
            {
                for (std::uint32_t column = 0; column < 5U; ++column)
                {
                    if (glyphs[glyph][row][column] != '1') continue;
                    const auto sourceX = 1U + static_cast<std::uint32_t>(glyph) * 6U + column;
                    const auto sourceY = 1U + row;
                    for (std::uint32_t dy = 0; dy < scale; ++dy)
                    {
                        for (std::uint32_t dx = 0; dx < scale; ++dx)
                        {
                            const auto x = sourceX * scale + dx;
                            const auto y = sourceY * scale + dy;
                            const auto pixel = (static_cast<std::size_t>(y) * image.width + x) * 4U;
                            image.pixels[pixel] = image.pixels[pixel + 1U] = image.pixels[pixel + 2U] = 255U;
                            image.pixels[pixel + 3U] = 255U;
                        }
                    }
                }
            }
        }
        return image;
    }

    class MapEditorApp final
    {
    public:
        int Run(HINSTANCE instance, int show)
        {
            instance_ = instance;
            INITCOMMONCONTROLSEX common{ sizeof(common), ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
            InitCommonControlsEx(&common);

            WNDCLASSEXW minimapClass{ sizeof(minimapClass) };
            minimapClass.hInstance = instance;
            minimapClass.lpfnWndProc = &MapEditorApp::MinimapProc;
            minimapClass.lpszClassName = kMinimapClass;
            minimapClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            minimapClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            RegisterClassExW(&minimapClass);

            WNDCLASSEXW newMapClass{ sizeof(newMapClass) };
            newMapClass.hInstance = instance;
            newMapClass.lpfnWndProc = NewMapWindowProc;
            newMapClass.lpszClassName = kNewMapClass;
            newMapClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            newMapClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
            RegisterClassExW(&newMapClass);

            WNDCLASSEXW fillEdgeClass{ sizeof(fillEdgeClass) };
            fillEdgeClass.hInstance = instance;
            fillEdgeClass.lpfnWndProc = FillEdgePortalWindowProc;
            fillEdgeClass.lpszClassName = kFillEdgePortalClass;
            fillEdgeClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            fillEdgeClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
            RegisterClassExW(&fillEdgeClass);

            WNDCLASSEXW helpClass{ sizeof(helpClass) };
            helpClass.hInstance = instance;
            helpClass.lpfnWndProc = HelpDialogProc;
            helpClass.lpszClassName = kHelpDialogClass;
            helpClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            helpClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
            RegisterClassExW(&helpClass);

            WNDCLASSEXW windowClass{ sizeof(windowClass) };
            windowClass.hInstance = instance;
            windowClass.lpfnWndProc = &MapEditorApp::WindowProc;
            windowClass.lpszClassName = kWindowClass;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            RegisterClassExW(&windowClass);

            window_ = CreateWindowExW(0, kWindowClass, L"DK Phoenix Map Editor",
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 1500, 920,
                nullptr, nullptr, instance, this);
            if (window_ == nullptr) return 1;
            dpi_ = GetDpiForWindow(window_);
            CreateUi();
            ApplyDpiFont();

            RECT client{};
            GetClientRect(window_, &client);
            if (!renderer_.Initialize({ static_cast<std::uint32_t>(client.right), static_cast<std::uint32_t>(client.bottom), true, window_ }) ||
                !renderer_.InitializeUiSpritePipeline())
            {
                MessageBoxW(window_, L"DirectX 11 renderer initialization failed.", L"Map Editor", MB_ICONERROR);
                return 2;
            }
            phoenix::graphics::ImageData white;
            white.width = white.height = 1;
            white.format = phoenix::graphics::PixelFormat::Rgba8;
            white.pixels = { 255, 255, 255, 255 };
            whiteTexture_ = renderer_.UploadTexture(white);
            portalLabelTexture_ = renderer_.UploadTexture(CreatePortalLabelImage());

            ShowWindow(window_, show);
            UpdateWindow(window_);
            SetTimer(window_, kRenderTimer, 16, nullptr);

            ACCEL accelerators[] = {
                { FCONTROL | FVIRTKEY, 'N', IdNew }, { FCONTROL | FVIRTKEY, 'O', IdOpen },
                { FCONTROL | FVIRTKEY, 'S', IdSave }, { FCONTROL | FVIRTKEY, 'Z', IdUndo },
                { FCONTROL | FVIRTKEY, 'Y', IdRedo }, { FCONTROL | FVIRTKEY, 'C', IdCopy },
                { FCONTROL | FVIRTKEY, 'V', IdPaste }, { FVIRTKEY, VK_DELETE, IdDelete }
            };
            const auto acceleratorTable = CreateAcceleratorTableW(accelerators, static_cast<int>(std::size(accelerators)));

            MSG message{};
            while (GetMessageW(&message, nullptr, 0, 0) > 0)
            {
                const auto focus = GetFocus();
                const bool editingText = focus == propertyEdit_ || focus == spriteSearch_ ||
                    (propertyEdit_ != nullptr && IsChild(propertyEdit_, focus)) ||
                    ShouldDeferPropertyKillFocus(focus);
                if (editingText || acceleratorTable == nullptr || !TranslateAcceleratorW(window_, acceleratorTable, &message))
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
            }
            if (acceleratorTable != nullptr) DestroyAcceleratorTable(acceleratorTable);
            ReleaseTextures();
            ReleaseMinimapBitmap();
            if (spriteImages_ != nullptr) { ImageList_Destroy(spriteImages_); spriteImages_ = nullptr; }
            renderer_.ReleaseTexture(whiteTexture_);
            renderer_.ReleaseTexture(portalLabelTexture_);
            renderer_.Shutdown();
            return static_cast<int>(message.wParam);
        }

    private:
        static LRESULT CALLBACK SplitterSubclassProc(HWND window, UINT message, WPARAM wParam,
            LPARAM lParam, UINT_PTR, DWORD_PTR reference)
        {
            auto* self = reinterpret_cast<MapEditorApp*>(reference);
            if (self == nullptr) return DefSubclassProc(window, message, wParam, lParam);
            if (message == WM_SETCURSOR)
            {
                SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                return TRUE;
            }
            if (message == WM_LBUTTONDOWN)
            {
                self->splitterDragging_ = true;
                SetCapture(window);
                return 0;
            }
            if (message == WM_MOUSEMOVE && self->splitterDragging_)
            {
                POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                MapWindowPoints(window, self->window_, &point, 1);
                self->SetPanelSplitterFromClientY(point.y);
                return 0;
            }
            if (message == WM_LBUTTONUP || message == WM_CAPTURECHANGED)
            {
                self->splitterDragging_ = false;
                if (GetCapture() == window) ReleaseCapture();
                return 0;
            }
            return DefSubclassProc(window, message, wParam, lParam);
        }

        static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            auto* self = reinterpret_cast<MapEditorApp*>(GetWindowLongPtrW(window, GWLP_USERDATA));
            if (message == WM_NCCREATE)
            {
                self = static_cast<MapEditorApp*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
                self->window_ = window;
                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            return self != nullptr ? self->HandleMessage(message, wParam, lParam) : DefWindowProcW(window, message, wParam, lParam);
        }

        static LRESULT CALLBACK MinimapProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (message == WM_PAINT)
            {
                PAINTSTRUCT paint{};
                const auto dc = BeginPaint(window, &paint);
                RECT rect{};
                GetClientRect(window, &rect);
                FillRect(dc, &rect, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
                auto* self = reinterpret_cast<MapEditorApp*>(GetWindowLongPtrW(GetParent(window), GWLP_USERDATA));
                if (self != nullptr && self->map_)
                {
                    const auto& map = *self->map_;
                    const auto width = std::max(1L, rect.right - rect.left);
                    const auto height = std::max(1L, rect.bottom - rect.top);
                    self->EnsureMinimapBitmap(dc);
                    if (self->minimapBitmap_ != nullptr)
                    {
                        SetStretchBltMode(dc, COLORONCOLOR);
                        StretchBlt(dc, rect.left, rect.top, width, height, self->minimapMemoryDc_, 0, 0,
                            self->minimapBitmapWidth_, self->minimapBitmapHeight_, SRCCOPY);
                    }

                    const auto viewport = phoenix::tools::map_editor::CalculateMinimapViewportRect(
                        map.width, map.height, self->cameraX_, self->cameraY_,
                        self->viewport_.width, self->viewport_.height, self->zoom_);
                    RECT cameraRect{
                        rect.left + static_cast<LONG>(viewport.left * width / map.width),
                        rect.top + static_cast<LONG>(viewport.top * height / map.height),
                        rect.left + static_cast<LONG>(viewport.right * width / map.width),
                        rect.top + static_cast<LONG>(viewport.bottom * height / map.height)
                    };
                    HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 64, 64));
                    const auto old = SelectObject(dc, pen);
                    const auto oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
                    Rectangle(dc, cameraRect.left, cameraRect.top,
                        std::max(cameraRect.left + 1, cameraRect.right), std::max(cameraRect.top + 1, cameraRect.bottom));
                    SelectObject(dc, oldBrush);
                    SelectObject(dc, old);
                    DeleteObject(pen);
                }
                EndPaint(window, &paint);
                return 0;
            }
            return DefWindowProcW(window, message, wParam, lParam);
        }

        static LRESULT CALLBACK PropertyEditProc(
            HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR reference)
        {
            auto* self = reinterpret_cast<MapEditorApp*>(reference);
            if (self == nullptr)
                return DefSubclassProc(window, message, wParam, lParam);

            if (message == WM_KEYDOWN && wParam == VK_RETURN)
            {
                PostMessageW(self->window_, kCommitPropertyMessage, 0, 0);
                return 0;
            }
            if (message == WM_KEYDOWN && wParam == VK_ESCAPE)
            {
                PostMessageW(self->window_, kCancelPropertyMessage, 0, 0);
                return 0;
            }
            if (message == WM_KILLFOCUS && !self->propertyEditClosing_)
            {
                // ComboBox dropdown acilirken KILLFOCUS sik sik wParam=NULL veya list
                // HWND hazir olmadan gelir. Commit edip DestroyWindow yapmak listeyi
                // hic acilmadan yok eder — "sayilar gorunmuyor / dropdown yok".
                if (self->IsEditingComboProperty())
                    return DefSubclassProc(window, message, wParam, lParam);

                const auto newFocus = reinterpret_cast<HWND>(wParam);
                if (self->ShouldDeferPropertyKillFocus(newFocus))
                    return DefSubclassProc(window, message, wParam, lParam);

                PostMessageW(self->window_, kCommitPropertyMessage, 0, 0);
                return DefSubclassProc(window, message, wParam, lParam);
            }
            const auto result = DefSubclassProc(window, message, wParam, lParam);
            if (window == self->propertySpin_ && message == WM_LBUTTONUP)
                PostMessageW(self->window_, kCommitPropertyMessage, 0, 0);
            return result;
        }

        LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
        {
            switch (message)
            {
            case WM_SIZE:
                Layout(LOWORD(lParam), HIWORD(lParam));
                if (wParam != SIZE_MINIMIZED && LOWORD(lParam) != 0U && HIWORD(lParam) != 0U && renderer_.IsInitialized())
                    (void)renderer_.ResizeSwapChain(LOWORD(lParam), HIWORD(lParam));
                return 0;
            case WM_GETMINMAXINFO:
            {
                auto* limits = reinterpret_cast<MINMAXINFO*>(lParam);
                limits->ptMinTrackSize.x = MulDiv(900, static_cast<int>(dpi_), 96);
                limits->ptMinTrackSize.y = MulDiv(600, static_cast<int>(dpi_), 96);
                return 0;
            }
            case WM_DPICHANGED:
            {
                dpi_ = HIWORD(wParam);
                const auto* suggested = reinterpret_cast<RECT*>(lParam);
                SetWindowPos(window_, nullptr, suggested->left, suggested->top,
                    suggested->right - suggested->left, suggested->bottom - suggested->top,
                    SWP_NOACTIVATE | SWP_NOZORDER);
                ApplyDpiFont();
                return 0;
            }
            case WM_TIMER:
                if (wParam == kRenderTimer) TickAndRender();
                return 0;
            case kCommitPropertyMessage:
                // Dropdown hâlâ açıksa yok etme (ertelemis KILLFOCUS / CLOSEUP yarisi).
                if (propertyEdit_ != nullptr && IsEditingComboProperty() &&
                    SendMessageW(propertyEdit_, CB_GETDROPPEDSTATE, 0, 0) != 0)
                    return 0;
                CommitPropertyEdit();
                return 0;
            case kCancelPropertyMessage:
                CancelPropertyEdit();
                return 0;
            case WM_COMMAND:
                HandleCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
                return 0;
            case WM_NOTIFY:
                return HandleNotify(reinterpret_cast<NMHDR*>(lParam));
            case WM_DRAWITEM:
                if (wParam == IdSpritePreview) return DrawSpritePreview(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
                if (wParam == IdPresetPreview) return DrawPresetPreview(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
                if (wParam == IdTilePalette || wParam == IdObjectPalette || wParam == IdAnimationPalette)
                    return DrawPaletteItem(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
                break;
            case WM_MOUSEMOVE:
                HandleMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
                return 0;
            case WM_LBUTTONDOWN:
                SetFocus(window_);
                HandleLeftDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
                return 0;
            case WM_LBUTTONUP:
                HandleLeftUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
                return 0;
            case WM_MBUTTONDOWN:
                if (!PointInViewport(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) return 0;
                panning_ = true;
                lastMouse_ = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                SetCapture(window_);
                return 0;
            case WM_MBUTTONUP:
                panning_ = false;
                if (GetCapture() == window_) ReleaseCapture();
                return 0;
            case WM_CAPTURECHANGED:
                panning_ = false;
                dragging_ = false;
                dragHistoryStarted_ = false;
                rectangleSelecting_ = false;
                lastPaintTile_.reset();
                paintHistoryStarted_ = false;
                presetDragging_ = false;
                return 0;
            case WM_RBUTTONDOWN:
                if (!PointInViewport(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))) return 0;
                hoverTile_ = ScreenToTile(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                if (tool_ == ToolMode::Portal)
                {
                    if (!map_) return 0;
                    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                    if (shift && selectedArea_)
                    {
                        const phoenix::tools::map_editor::MapSelectionRect rect{
                            selectedArea_->left, selectedArea_->top, selectedArea_->right, selectedArea_->bottom };
                        if (phoenix::tools::map_editor::ClearPortalsInRect(*map_, history_, rect))
                        {
                            UpdateTitle();
                            UpdateProperties();
                            InvalidateMinimapCache();
                            UpdateStatus();
                        }
                        return 0;
                    }
                    if (hoverTile_)
                    {
                        ClearPortalAtHover();
                        return 0;
                    }
                }
                if (tool_ == ToolMode::Tile && hoverTile_) { PaintTile(0xFFFFU); paintHistoryStarted_ = false; }
                else if (tool_ == ToolMode::Select && selectedArea_)
                {
                    HMENU context = CreatePopupMenu();
                    AppendMenuW(context, MF_STRING, IdAddAsPreset, L"Add as Preset");
                    POINT screen{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                    ClientToScreen(window_, &screen);
                    const auto command = TrackPopupMenu(context, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                        screen.x, screen.y, 0, window_, nullptr);
                    DestroyMenu(context);
                    if (command == IdAddAsPreset) BeginPresetCapture();
                }
                return 0;
            case WM_MOUSEWHEEL:
            {
                POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ScreenToClient(window_, &point);
                if (!PointInViewport(point.x, point.y)) return 0;
                const auto delta = GET_WHEEL_DELTA_WPARAM(wParam);
                SetZoom(zoom_ * (delta > 0 ? 1.1F : 0.9F));
                return 0;
            }
            case WM_HSCROLL:
                if (reinterpret_cast<HWND>(lParam) == zoomTrack_)
                {
                    SetZoom(static_cast<float>(SendMessageW(zoomTrack_, TBM_GETPOS, 0, 0)) / 100.0F);
                    return 0;
                }
                break;
            case WM_KEYDOWN:
                if (HandleKey(wParam)) return 0;
                break;
            case WM_KEYUP:
                HandleKeyUp(wParam);
                return 0;
            case WM_KILLFOCUS:
                movementKeys_.fill(false);
                selectionNudgeKeys_.fill(false);
                return 0;
            case WM_CLOSE:
                if (ConfirmDiscard()) DestroyWindow(window_);
                return 0;
            case WM_DESTROY:
                KillTimer(window_, kRenderTimer);
                if (presetPreviewBitmap_ != nullptr) { DeleteObject(presetPreviewBitmap_); presetPreviewBitmap_ = nullptr; }
                if (uiFont_ != nullptr) { DeleteObject(uiFont_); uiFont_ = nullptr; }
                PostQuitMessage(0);
                return 0;
            }
            return DefWindowProcW(window_, message, wParam, lParam);
        }

        void CreateUi()
        {
            auto menu = CreateMenu();
            auto file = CreatePopupMenu();
            AppendMenuW(file, MF_STRING, IdNew, L"&New\tCtrl+N");
            AppendMenuW(file, MF_STRING, IdOpen, L"&Open...\tCtrl+O");
            AppendMenuW(file, MF_STRING, IdSave, L"&Save\tCtrl+S");
            AppendMenuW(file, MF_STRING, IdSaveAs, L"Save &As...");
            AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(file, MF_STRING, IdExit, L"E&xit");
            AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(file), L"&File");
            auto edit = CreatePopupMenu();
            AppendMenuW(edit, MF_STRING, IdUndo, L"&Undo\tCtrl+Z");
            AppendMenuW(edit, MF_STRING, IdRedo, L"&Redo\tCtrl+Y");
            AppendMenuW(edit, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(edit, MF_STRING, IdCopy, L"&Copy\tCtrl+C");
            AppendMenuW(edit, MF_STRING, IdPaste, L"&Paste\tCtrl+V");
            AppendMenuW(edit, MF_STRING, IdDelete, L"&Delete\tDel");
            AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(edit), L"&Edit");
            auto view = CreatePopupMenu();
            AppendMenuW(view, MF_STRING | MF_CHECKED, IdGrid, L"&Grid");
            AppendMenuW(view, MF_STRING | MF_CHECKED, IdViewMinimap, L"&Minimap");
            AppendMenuW(view, MF_STRING | MF_CHECKED, IdViewProperties, L"&Property Panel");
            AppendMenuW(view, MF_STRING | MF_CHECKED, IdViewSprites, L"&Sprite Browser");
            AppendMenuW(view, MF_STRING, IdViewPresets, L"&Preset Editor");
            AppendMenuW(view, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(view, MF_STRING, IdFitMap, L"&Fit Map to View");
            AppendMenuW(view, MF_STRING, IdActualSize, L"&Actual Size / 100%");
            AppendMenuW(view, MF_STRING, IdResetCamera, L"&Reset Camera");
            AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(view), L"&View");
            auto tools = CreatePopupMenu();
            AppendMenuW(tools, MF_STRING, IdToolSelect, L"&Select");
            AppendMenuW(tools, MF_STRING, IdToolTile, L"&Paint Tile\tT");
            AppendMenuW(tools, MF_STRING, IdToolErase, L"&Erase Tile\tE");
            AppendMenuW(tools, MF_STRING, IdToolImage, L"Place &Object\tO");
            AppendMenuW(tools, MF_STRING, IdToolAnimation, L"Place &Animation");
            AppendMenuW(tools, MF_STRING, IdToolPortal, L"Edit &Portal");
            AppendMenuW(tools, MF_STRING, IdFillEdgePortal, L"Fill &Edge Portal...\tEnter");
            AppendMenuW(tools, MF_STRING, IdToolMove, L"&Move\tM");
            AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(tools), L"&Tools");
            auto screenshot = CreatePopupMenu();
            AppendMenuW(screenshot, MF_STRING, IdScreenshotFullMap, L"Export &Full Map...");
            AppendMenuW(screenshot, MF_STRING, IdScreenshotGameMinimap, L"Export &Game Minimap (200x100)...");
            AppendMenuW(screenshot, MF_STRING, IdScreenshotHighQualityMinimap,
                L"Export &High Quality Minimap (800x400)...");
            AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(screenshot), L"&Screenshot");
            auto help = CreatePopupMenu();
            AppendMenuW(help, MF_STRING, IdHelpPortalTypes, L"&Portal Types...");
            AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(help), L"&Help");
            SetMenu(window_, menu);

            const DWORD buttonStyle = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
            newButton_ = CreateWindowW(L"BUTTON", L"New", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdNew), instance_, nullptr);
            openButton_ = CreateWindowW(L"BUTTON", L"Open", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdOpen), instance_, nullptr);
            saveButton_ = CreateWindowW(L"BUTTON", L"Save", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdSave), instance_, nullptr);
            undoButton_ = CreateWindowW(L"BUTTON", L"Undo", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdUndo), instance_, nullptr);
            redoButton_ = CreateWindowW(L"BUTTON", L"Redo", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdRedo), instance_, nullptr);
            selectButton_ = CreateWindowW(L"BUTTON", L"Select", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdToolSelect), instance_, nullptr);
            tileButton_ = CreateWindowW(L"BUTTON", L"Paint", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdToolTile), instance_, nullptr);
            eraseButton_ = CreateWindowW(L"BUTTON", L"Erase", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdToolErase), instance_, nullptr);
            imageButton_ = CreateWindowW(L"BUTTON", L"Object", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdToolImage), instance_, nullptr);
            animationButton_ = CreateWindowW(L"BUTTON", L"Anim", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdToolAnimation), instance_, nullptr);
            portalButton_ = CreateWindowW(L"BUTTON", L"Portal", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdToolPortal), instance_, nullptr);
            moveButton_ = CreateWindowW(L"BUTTON", L"Move", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdToolMove), instance_, nullptr);
            toolbarLayerLabel_ = CreateWindowW(L"STATIC", L"Layers:",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                0, 0, 0, 0, window_, nullptr, instance_, nullptr);
            const DWORD layerButtonStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | BS_PUSHLIKE;
            layerTiles_ = CreateWindowW(L"BUTTON", L"Tiles", layerButtonStyle,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdLayerTiles), instance_, nullptr);
            layerImages_ = CreateWindowW(L"BUTTON", L"Objects", layerButtonStyle,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdLayerImages), instance_, nullptr);
            layerAnimations_ = CreateWindowW(L"BUTTON", L"Animations", layerButtonStyle,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdLayerAnimations), instance_, nullptr);
            layerPortals_ = CreateWindowW(L"BUTTON", L"Portals", layerButtonStyle,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdLayerPortals), instance_, nullptr);
            Button_SetCheck(layerTiles_, BST_CHECKED);
            Button_SetCheck(layerImages_, BST_CHECKED);
            Button_SetCheck(layerAnimations_, BST_CHECKED);
            Button_SetCheck(layerPortals_, BST_CHECKED);
            zoomOutButton_ = CreateWindowW(L"BUTTON", L"-", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdZoomOut), instance_, nullptr);
            zoomInButton_ = CreateWindowW(L"BUTTON", L"+", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdZoomIn), instance_, nullptr);
            fitMapButton_ = CreateWindowW(L"BUTTON", L"Fit", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdFitMap), instance_, nullptr);
            zoomTrack_ = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdZoomTrack), instance_, nullptr);
            SendMessageW(zoomTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(2, 300));
            SendMessageW(zoomTrack_, TBM_SETPOS, TRUE, 100);

            leftDockLabel_ = CreateWindowW(L"STATIC", L"Palettes", WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 0, 0, 0, window_, nullptr, instance_, nullptr);
            paletteTabs_ = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdPaletteTabs), instance_, nullptr);
            TCITEMW tab{ TCIF_TEXT };
            tab.pszText = const_cast<wchar_t*>(L"Tiles"); TabCtrl_InsertItem(paletteTabs_, 0, &tab);
            tab.pszText = const_cast<wchar_t*>(L"Objects"); TabCtrl_InsertItem(paletteTabs_, 1, &tab);
            tab.pszText = const_cast<wchar_t*>(L"Animation"); TabCtrl_InsertItem(paletteTabs_, 2, &tab);
            paletteContentLabel_ = CreateWindowW(L"STATIC", L"Tile Browser", WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 0, 0, 0, window_, nullptr, instance_, nullptr);
            const DWORD paletteStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL |
                LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_NODATA;
            tilePalette_ = CreateWindowW(L"LISTBOX", L"", paletteStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdTilePalette), instance_, nullptr);
            objectPalette_ = CreateWindowW(L"LISTBOX", L"", paletteStyle & ~WS_VISIBLE, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdObjectPalette), instance_, nullptr);
            animationPalette_ = CreateWindowW(L"LISTBOX", L"", paletteStyle & ~WS_VISIBLE, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdAnimationPalette), instance_, nullptr);
            for (const auto palette : { tilePalette_, objectPalette_, animationPalette_ })
            {
                SendMessageW(palette, LB_SETITEMHEIGHT, 0, 70);
                SendMessageW(palette, LB_SETCOUNT, 1, 0);
            }

            spriteLabel_ = CreateWindowW(L"STATIC", L"Sprite Browser", WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 0, 0, 0, window_, nullptr, instance_, nullptr);
            browseSpkButton_ = CreateWindowW(L"BUTTON", L"Tilelist", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdBrowseSpk), instance_, nullptr);
            browseSplitButton_ = CreateWindowW(L"BUTTON", L"Objectlist", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdBrowseSplit), instance_, nullptr);
            browseAnimationButton_ = CreateWindowW(L"BUTTON", L"Effectlist", buttonStyle, 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdBrowseAnimation), instance_, nullptr);
            spriteSearch_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdSpriteSearch), instance_, nullptr);
            SendMessageW(spriteSearch_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Sprite ID search"));

            spriteList_ = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_VISIBLE | LVS_ICON | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_AUTOARRANGE,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdSpriteList), instance_, nullptr);
            ListView_SetExtendedListViewStyle(spriteList_, LVS_EX_DOUBLEBUFFER);
            spriteImages_ = ImageList_Create(64, 64, ILC_COLOR32 | ILC_MASK, 32, 32);
            ListView_SetImageList(spriteList_, spriteImages_, LVSIL_NORMAL);
            ResetSpriteThumbnailCache();
            spritePreview_ = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | SS_SUNKEN,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdSpritePreview), instance_, nullptr);

            propertyLabel_ = CreateWindowW(L"STATIC", L"Properties", WS_CHILD | WS_VISIBLE | SS_LEFT,
                0, 0, 0, 0, window_, nullptr, instance_, nullptr);
            properties_ = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdProperties), instance_, nullptr);
            ListView_SetExtendedListViewStyle(properties_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
            LVCOLUMNW propertyColumn{ LVCF_TEXT | LVCF_WIDTH };
            propertyColumn.pszText = const_cast<wchar_t*>(L"Property"); propertyColumn.cx = 130;
            ListView_InsertColumn(properties_, 0, &propertyColumn);
            propertyColumn.pszText = const_cast<wchar_t*>(L"Value"); propertyColumn.cx = 200;
            ListView_InsertColumn(properties_, 1, &propertyColumn);

            presetLabel_ = CreateWindowW(L"STATIC", L"Preset Editor", WS_CHILD | SS_LEFT,
                0, 0, 0, 0, window_, nullptr, instance_, nullptr);
            presetName_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdPresetName), instance_, nullptr);
            SendMessageW(presetName_, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Preset name"));
            presetSave_ = CreateWindowW(L"BUTTON", L"Save", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdPresetSave), instance_, nullptr);
            presetList_ = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdPresetList), instance_, nullptr);
            ListView_SetExtendedListViewStyle(presetList_, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            LVCOLUMNW presetColumn{ LVCF_TEXT | LVCF_WIDTH };
            presetColumn.pszText = const_cast<wchar_t*>(L"Preset"); presetColumn.cx = 220;
            ListView_InsertColumn(presetList_, 0, &presetColumn);
            presetColumn.pszText = const_cast<wchar_t*>(L"Contents"); presetColumn.cx = 150;
            ListView_InsertColumn(presetList_, 1, &presetColumn);
            presetPreview_ = CreateWindowW(L"STATIC", L"", WS_CHILD | SS_OWNERDRAW | SS_SUNKEN,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdPresetPreview), instance_, nullptr);
            presetRename_ = CreateWindowW(L"BUTTON", L"Rename", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdPresetRename), instance_, nullptr);
            presetDelete_ = CreateWindowW(L"BUTTON", L"Delete", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdPresetDelete), instance_, nullptr);
            presetRefresh_ = CreateWindowW(L"BUTTON", L"Refresh", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdPresetRefresh), instance_, nullptr);
            panelSplitter_ = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_NOTIFY,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdPanelSplitter), instance_, nullptr);
            SetWindowSubclass(panelSplitter_, SplitterSubclassProc, 1U, reinterpret_cast<DWORD_PTR>(this));
            status_ = CreateWindowExW(0, STATUSCLASSNAMEW, L"Ready", WS_CHILD | WS_VISIBLE,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdStatus), instance_, nullptr);
            minimap_ = CreateWindowExW(WS_EX_CLIENTEDGE, kMinimapClass, L"", WS_CHILD | WS_VISIBLE,
                0, 0, 0, 0, window_, reinterpret_cast<HMENU>(IdMinimap), instance_, nullptr);
            SetTool(ToolMode::Select);
        }

        static BOOL CALLBACK SetChildFont(HWND child, LPARAM font)
        {
            SendMessageW(child, WM_SETFONT, static_cast<WPARAM>(font), TRUE);
            return TRUE;
        }

        void ApplyDpiFont()
        {
            NONCLIENTMETRICSW metrics{ sizeof(metrics) };
            if (!SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, dpi_)) return;
            const auto next = CreateFontIndirectW(&metrics.lfMessageFont);
            if (next == nullptr) return;
            EnumChildWindows(window_, SetChildFont, reinterpret_cast<LPARAM>(next));
            const auto paletteItemHeight = MulDiv(70, static_cast<int>(dpi_), 96);
            for (const auto palette : { tilePalette_, objectPalette_, animationPalette_ })
                SendMessageW(palette, LB_SETITEMHEIGHT, 0, paletteItemHeight);
            if (uiFont_ != nullptr) DeleteObject(uiFont_);
            uiFont_ = next;
            RECT client{}; GetClientRect(window_, &client); Layout(client.right, client.bottom);
        }

        void Layout(int width, int height)
        {
            const auto scale = [this](const int value) { return MulDiv(value, static_cast<int>(dpi_), 96); };
            clientWidth_ = width;
            clientHeight_ = height;
            const bool wrappedToolbar = phoenix::tools::map_editor::CalculateToolbarRowCount(
                width, scale(1360)) == 2;
            const int toolbarH = scale(wrappedToolbar ? 72 : 38);
            const int statusH = scale(22);
            const HWND fileButtons[] = { newButton_, openButton_, saveButton_, undoButton_, redoButton_ };
            const HWND toolButtons[] = { selectButton_, tileButton_, eraseButton_, imageButton_, animationButton_, portalButton_, moveButton_ };
            const auto placeButtons = [&](const auto& buttons, int& x, const int y) {
                for (const auto button : buttons)
                {
                    MoveWindow(button, x, y, scale(58), scale(28), TRUE);
                    x += scale(61);
                }
            };
            const auto placeLayers = [&](int& x, const int y) {
                MoveWindow(toolbarLayerLabel_, x, y, scale(48), scale(28), TRUE);
                x += scale(50);
                for (const auto button : { layerTiles_, layerImages_, layerAnimations_, layerPortals_ })
                {
                    const int buttonWidth = button == layerAnimations_ ? scale(82) : scale(64);
                    MoveWindow(button, x, y, buttonWidth, scale(28), TRUE);
                    x += buttonWidth + scale(3);
                }
            };
            const int layerGroupWidth = scale(50 + 64 + 3 + 64 + 3 + 82 + 3 + 64);
            const auto placeZoom = [&](int& x, const int y) {
                MoveWindow(zoomOutButton_, x + scale(4), y, scale(28), scale(28), TRUE);
                MoveWindow(zoomTrack_, x + scale(36), y + scale(1), scale(120), scale(26), TRUE);
                MoveWindow(zoomInButton_, x + scale(160), y, scale(28), scale(28), TRUE);
                MoveWindow(fitMapButton_, x + scale(192), y, scale(42), scale(28), TRUE);
                x += scale(237);
            };

            if (!wrappedToolbar)
            {
                int x = scale(4);
                placeButtons(fileButtons, x, scale(4));
                placeButtons(toolButtons, x, scale(4));
                placeZoom(x, scale(4));
                int layerX = std::max(x + scale(4), width - layerGroupWidth - scale(4));
                placeLayers(layerX, scale(4));
            }
            else
            {
                int toolX = scale(4);
                placeButtons(toolButtons, toolX, scale(4));
                int layerX = std::max(toolX + scale(4), width - layerGroupWidth - scale(4));
                placeLayers(layerX, scale(4));
                int commandX = scale(4);
                placeButtons(fileButtons, commandX, scale(38));
                placeZoom(commandX, scale(38));
            }

            const int contentBottom = std::max(toolbarH + 1, height - statusH);
            const int leftW = std::min(scale(kLeftDockWidth), std::max(0, width / 3));
            const bool anyRight = showSpriteBrowser_ || showPropertyPanel_ || showPresetEditor_;
            const int rightW = anyRight ? std::min(scale(kRightDockWidth), std::max(0, width / 2)) : 0;
            const int rightX = std::max(leftW, width - rightW);
            MoveWindow(leftDockLabel_, 8, toolbarH + 5, leftW - 16, 20, TRUE);
            MoveWindow(paletteTabs_, 6, toolbarH + 25, leftW - 12, 28, TRUE);
            MoveWindow(paletteContentLabel_, 8, toolbarH + 54, leftW - 16, 20, TRUE);
            const int minimapH = showMinimap_ ? std::clamp(scale(150), scale(90), std::max(scale(90), (contentBottom - toolbarH) / 3)) : 0;
            const int paletteH = std::max(scale(80), contentBottom - toolbarH - scale(100) - minimapH);
            for (const auto palette : { tilePalette_, objectPalette_, animationPalette_ })
                MoveWindow(palette, 8, toolbarH + 74, leftW - 16, paletteH, TRUE);
            ShowWindow(minimap_, showMinimap_ ? SW_SHOW : SW_HIDE);
            if (showMinimap_)
                MoveWindow(minimap_, 8, toolbarH + scale(82) + paletteH, leftW - 16, minimapH, TRUE);

            int rightY = toolbarH + 4;
            const int innerRightW = std::max(1, rightW - 16);
            const bool regularRight = !showPresetEditor_;
            ShowWindow(spriteLabel_, regularRight && showSpriteBrowser_ ? SW_SHOW : SW_HIDE);
            ShowWindow(browseSpkButton_, regularRight && showSpriteBrowser_ ? SW_SHOW : SW_HIDE);
            ShowWindow(browseSplitButton_, regularRight && showSpriteBrowser_ ? SW_SHOW : SW_HIDE);
            ShowWindow(browseAnimationButton_, regularRight && showSpriteBrowser_ ? SW_SHOW : SW_HIDE);
            ShowWindow(spriteSearch_, regularRight && showSpriteBrowser_ ? SW_SHOW : SW_HIDE);
            ShowWindow(spriteList_, regularRight && showSpriteBrowser_ ? SW_SHOW : SW_HIDE);
            ShowWindow(spritePreview_, SW_HIDE);
            if (regularRight && showSpriteBrowser_)
            {
                MoveWindow(spriteLabel_, rightX + 8, rightY, innerRightW, 20, TRUE); rightY += 21;
                const int sourceButtonWidth = std::max(scale(72), (innerRightW - scale(6)) / 3);
                MoveWindow(browseSpkButton_, rightX + 8, rightY, sourceButtonWidth, scale(26), TRUE);
                MoveWindow(browseSplitButton_, rightX + 8 + sourceButtonWidth + scale(3), rightY, sourceButtonWidth, scale(26), TRUE);
                MoveWindow(browseAnimationButton_, rightX + 8 + (sourceButtonWidth + scale(3)) * 2, rightY,
                    std::max(scale(64), innerRightW - (sourceButtonWidth + scale(3)) * 2), scale(26), TRUE);
                rightY += scale(30);
                MoveWindow(spriteSearch_, rightX + 8, rightY, innerRightW, scale(26), TRUE); rightY += scale(30);
            }
            ShowWindow(propertyLabel_, regularRight && showPropertyPanel_ ? SW_SHOW : SW_HIDE);
            ShowWindow(properties_, regularRight && showPropertyPanel_ ? SW_SHOW : SW_HIDE);
            ShowWindow(panelSplitter_, regularRight && showSpriteBrowser_ && showPropertyPanel_ ? SW_SHOW : SW_HIDE);
            for (const auto control : { presetLabel_, presetName_, presetSave_, presetList_, presetPreview_,
                presetRename_, presetDelete_, presetRefresh_ })
                ShowWindow(control, showPresetEditor_ ? SW_SHOW : SW_HIDE);
            if (showPresetEditor_)
            {
                const int x = rightX + 8;
                MoveWindow(presetLabel_, x, rightY, innerRightW, scale(22), TRUE); rightY += scale(24);
                MoveWindow(presetName_, x, rightY, std::max(scale(80), innerRightW - scale(72)), scale(26), TRUE);
                MoveWindow(presetSave_, x + std::max(scale(83), innerRightW - scale(69)), rightY,
                    scale(69), scale(26), TRUE); rightY += scale(30);
                const int previewHeight = std::clamp(scale(180), scale(120),
                    std::max(scale(120), (contentBottom - rightY) / 3));
                const int buttonHeight = scale(28);
                const int listHeight = std::max(scale(100), contentBottom - rightY - previewHeight - buttonHeight - scale(12));
                MoveWindow(presetList_, x, rightY, innerRightW, listHeight, TRUE); rightY += listHeight + scale(4);
                MoveWindow(presetPreview_, x, rightY, innerRightW, previewHeight, TRUE); rightY += previewHeight + scale(4);
                const int actionWidth = std::max(scale(68), (innerRightW - scale(6)) / 3);
                MoveWindow(presetRename_, x, rightY, actionWidth, buttonHeight, TRUE);
                MoveWindow(presetDelete_, x + actionWidth + scale(3), rightY, actionWidth, buttonHeight, TRUE);
                MoveWindow(presetRefresh_, x + (actionWidth + scale(3)) * 2, rightY,
                    std::max(scale(68), innerRightW - (actionWidth + scale(3)) * 2), buttonHeight, TRUE);
                ListView_SetColumnWidth(presetList_, 0, std::max(scale(120), innerRightW - scale(150)));
                ListView_SetColumnWidth(presetList_, 1, scale(145));
            }
            const int availableRightHeight = std::max(0, contentBottom - rightY - scale(4));
            if (!showPresetEditor_ && showSpriteBrowser_ && showPropertyPanel_)
            {
                const auto split = phoenix::tools::map_editor::CalculateSplitterLayout(availableRightHeight,
                    static_cast<int>(std::lround(availableRightHeight * panelSplitRatio_)), scale(150), scale(130), scale(6));
                spritePanelTop_ = rightY;
                spritePanelAvailableHeight_ = availableRightHeight;
                MoveWindow(spriteList_, rightX + 8, rightY, innerRightW, split.firstExtent, TRUE);
                const auto grid = phoenix::tools::map_editor::CalculateSpriteGridLayout(innerRightW, scale(92), 5, scale(72));
                ListView_SetIconSpacing(spriteList_, grid.cellWidth, grid.cellHeight);
                rightY += split.firstExtent;
                MoveWindow(panelSplitter_, rightX + 8, rightY, innerRightW, scale(6), TRUE); rightY += scale(6);
                MoveWindow(propertyLabel_, rightX + 8, rightY, innerRightW, scale(20), TRUE); rightY += scale(20);
                MoveWindow(properties_, rightX + 8, rightY, innerRightW, std::max(1, contentBottom - rightY - scale(4)), TRUE);
                ListView_SetColumnWidth(properties_, 1, std::max(80, innerRightW - 138));
            }
            else if (!showPresetEditor_ && showSpriteBrowser_)
            {
                MoveWindow(spriteList_, rightX + 8, rightY, innerRightW, availableRightHeight, TRUE);
                const auto grid = phoenix::tools::map_editor::CalculateSpriteGridLayout(innerRightW, scale(92), 5, scale(72));
                ListView_SetIconSpacing(spriteList_, grid.cellWidth, grid.cellHeight);
            }
            else if (!showPresetEditor_ && showPropertyPanel_)
            {
                MoveWindow(propertyLabel_, rightX + 8, rightY, innerRightW, scale(20), TRUE); rightY += scale(20);
                MoveWindow(properties_, rightX + 8, rightY, innerRightW, std::max(1, contentBottom - rightY - scale(4)), TRUE);
                ListView_SetColumnWidth(properties_, 1, std::max(80, innerRightW - 138));
            }
            SendMessageW(status_, WM_SIZE, 0, 0);
            viewport_ = { leftW, toolbarH, std::max(1, rightX - leftW), std::max(1, contentBottom - toolbarH) };
        }

        void SetPanelSplitterFromClientY(const int clientY)
        {
            if (spritePanelAvailableHeight_ <= 0) return;
            const auto requested = clientY - spritePanelTop_;
            const auto split = phoenix::tools::map_editor::CalculateSplitterLayout(spritePanelAvailableHeight_, requested,
                MulDiv(150, static_cast<int>(dpi_), 96), MulDiv(130, static_cast<int>(dpi_), 96),
                MulDiv(6, static_cast<int>(dpi_), 96));
            panelSplitRatio_ = static_cast<float>(split.firstExtent) /
                static_cast<float>(std::max(1, spritePanelAvailableHeight_));
            RECT client{}; GetClientRect(window_, &client); Layout(client.right, client.bottom);
        }

        void HandleCommand(int id, int notification, HWND source)
        {
            if (id == IdPropertyEdit &&
                (notification == BN_CLICKED || notification == CBN_SELCHANGE ||
                    notification == CBN_CLOSEUP))
            {
                // Combo/checkbox mesaji bitmeden DestroyWindow AV veya sessiz fail uretir.
                // Combo commit: sadece secim/kapanis (KILLFOCUS ile degil).
                PostMessageW(window_, kCommitPropertyMessage, 0, 0);
                return;
            }
            if (id == IdSpriteSearch && notification == EN_CHANGE) { RefreshSpriteFilter(); return; }
            if ((id == IdTilePalette || id == IdObjectPalette || id == IdAnimationPalette) && notification == LBN_SELCHANGE)
            {
                SelectPaletteSprite(source, id == IdTilePalette ? PaletteKind::Tile :
                    id == IdObjectPalette ? PaletteKind::Image : PaletteKind::Animation);
                return;
            }
            if (id == IdPresetSave && notification == BN_CLICKED) { SavePresetDraft(); return; }
            if (id == IdPresetRename && notification == BN_CLICKED) { RenameSelectedPreset(); return; }
            if (id == IdPresetDelete && notification == BN_CLICKED) { DeleteSelectedPreset(); return; }
            if (id == IdPresetRefresh && notification == BN_CLICKED) { RefreshPresetCatalog(); return; }
            switch (id)
            {
            case IdNew: NewMap(); break;
            case IdOpen: OpenMap(); break;
            case IdSave: Save(false); break;
            case IdSaveAs: Save(true); break;
            case IdExit: SendMessageW(window_, WM_CLOSE, 0, 0); break;
            case IdUndo: Undo(); break;
            case IdRedo: Redo(); break;
            case IdCopy: CopySelection(); break;
            case IdPaste: Paste(); break;
            case IdDelete: DeleteSelection(); break;
            case IdGrid:
                showGrid_ = !showGrid_;
                CheckMenuItem(GetMenu(window_), IdGrid, MF_BYCOMMAND | (showGrid_ ? MF_CHECKED : MF_UNCHECKED));
                break;
            case IdViewMinimap: TogglePanel(showMinimap_, IdViewMinimap); break;
            case IdViewProperties: TogglePanel(showPropertyPanel_, IdViewProperties); break;
            case IdViewSprites: TogglePanel(showSpriteBrowser_, IdViewSprites); break;
            case IdViewPresets:
                showPresetEditor_ = !showPresetEditor_;
                CheckMenuItem(GetMenu(window_), IdViewPresets,
                    MF_BYCOMMAND | (showPresetEditor_ ? MF_CHECKED : MF_UNCHECKED));
                if (showPresetEditor_) RefreshPresetCatalog();
                { RECT client{}; GetClientRect(window_, &client); Layout(client.right, client.bottom); }
                break;
            case IdToolSelect: SetTool(ToolMode::Select); break;
            case IdToolTile: SetTool(ToolMode::Tile); break;
            case IdToolErase: SetTool(ToolMode::EraseTile); break;
            case IdToolImage: SetTool(ToolMode::Image); break;
            case IdToolAnimation: SetTool(ToolMode::Animation); break;
            case IdToolPortal: SetTool(ToolMode::Portal); break;
            case IdToolMove: SetTool(ToolMode::Move); break;
            case IdZoomOut: SetZoom(zoom_ - 0.1F); break;
            case IdZoomIn: SetZoom(zoom_ + 0.1F); break;
            case IdFitMap: FitMapToView(); break;
            case IdActualSize: SetZoom(1.0F); break;
            case IdResetCamera: ResetCamera(); break;
            case IdScreenshotFullMap: ExportScreenshot(false, false); break;
            case IdScreenshotGameMinimap: ExportScreenshot(true, false); break;
            case IdScreenshotHighQualityMinimap: ExportScreenshot(true, true); break;
            case IdBrowseSpk: LoadFixedSpriteSource(phoenix::tools::map_editor::SpriteResourceType::Tile); break;
            case IdBrowseSplit: LoadFixedSpriteSource(phoenix::tools::map_editor::SpriteResourceType::Object); break;
            case IdBrowseAnimation: LoadFixedSpriteSource(phoenix::tools::map_editor::SpriteResourceType::Effect); break;
            case IdLayerTiles: showTiles_ = Button_GetCheck(layerTiles_) == BST_CHECKED; InvalidateMinimapCache(); break;
            case IdLayerImages: showImages_ = Button_GetCheck(layerImages_) == BST_CHECKED; InvalidateMinimapCache(); break;
            case IdLayerAnimations: showAnimations_ = Button_GetCheck(layerAnimations_) == BST_CHECKED; InvalidateMinimapCache(); break;
            case IdLayerPortals: showPortals_ = Button_GetCheck(layerPortals_) == BST_CHECKED; InvalidateMinimapCache(); break;
            case IdAddAsPreset: BeginPresetCapture(); break;
            case IdHelpPortalTypes: ShowPortalTypeHelp(); break;
            case IdFillEdgePortal: ShowFillEdgePortalDialog(); break;
            }
        }

        void ShowPortalTypeHelp()
        {
            const int dialogWidth = MulDiv(640, static_cast<int>(dpi_), 96);
            const int dialogHeight = MulDiv(520, static_cast<int>(dpi_), 96);
            RECT owner{};
            GetWindowRect(window_, &owner);
            const auto dialog = CreateWindowExW(WS_EX_DLGMODALFRAME, kHelpDialogClass, L"Help — Portal Types",
                WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE,
                owner.left + (owner.right - owner.left - dialogWidth) / 2,
                owner.top + (owner.bottom - owner.top - dialogHeight) / 2,
                dialogWidth, dialogHeight, window_, nullptr, instance_, nullptr);
            if (dialog == nullptr) return;
            EnableWindow(window_, FALSE);
            MSG message{};
            while (IsWindow(dialog) && GetMessageW(&message, nullptr, 0, 0) > 0)
            {
                if (!IsDialogMessageW(dialog, &message))
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
            }
            EnableWindow(window_, TRUE);
            SetForegroundWindow(window_);
        }

        phoenix::tools::map_editor::MapSelectionRect CurrentPortalFillRect() const
        {
            if (selectedArea_)
            {
                return { selectedArea_->left, selectedArea_->top, selectedArea_->right, selectedArea_->bottom };
            }
            if (selectedTile_)
            {
                return { selectedTile_->first, selectedTile_->second, selectedTile_->first, selectedTile_->second };
            }
            return {};
        }

        void ShowFillEdgePortalDialog()
        {
            if (!map_) return;
            phoenix::tools::map_editor::EdgePortalFillParams params;
            params.rect = CurrentPortalFillRect();
            if (!params.rect.IsValid())
            {
                MessageBoxW(window_, L"Select a tile rectangle with the Portal tool first.",
                    L"Fill Edge Portal", MB_ICONINFORMATION);
                return;
            }
            FillEdgePortalOptions options;
            options.params = params;
            const int dialogWidth = MulDiv(450, static_cast<int>(dpi_), 96);
            const int dialogHeight = MulDiv(280, static_cast<int>(dpi_), 96);
            RECT owner{};
            GetWindowRect(window_, &owner);
            const auto dialog = CreateWindowExW(WS_EX_DLGMODALFRAME, kFillEdgePortalClass, L"Fill Edge Portal",
                WS_POPUP | WS_CAPTION | WS_SYSMENU, owner.left + (owner.right - owner.left - dialogWidth) / 2,
                owner.top + (owner.bottom - owner.top - dialogHeight) / 2, dialogWidth, dialogHeight,
                window_, nullptr, instance_, &options);
            if (dialog == nullptr) return;
            EnableWindow(window_, FALSE);
            ShowWindow(dialog, SW_SHOW);
            MSG message{};
            while (!options.closed && GetMessageW(&message, nullptr, 0, 0) > 0)
            {
                if (!IsDialogMessageW(dialog, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
            }
            EnableWindow(window_, TRUE);
            SetForegroundWindow(window_);
            if (!options.accepted) return;
            const auto existing = phoenix::tools::map_editor::CountPortalsInRect(*map_, options.params.rect);
            if (existing > 0U)
            {
                const auto confirm = L"This selection already contains " + std::to_wstring(existing) +
                    L" portal tile(s). Overwrite them?";
                if (MessageBoxW(window_, confirm.c_str(), L"Fill Edge Portal",
                    MB_ICONWARNING | MB_YESNO) != IDYES) return;
            }
            const auto result = phoenix::tools::map_editor::FillEdgePortals(*map_, history_, options.params);
            if (!result.applied)
            {
                MessageBoxW(window_, ToWide(result.error).c_str(), L"Fill Edge Portal", MB_ICONERROR);
                return;
            }
            selectedTile_ = std::make_pair(options.params.rect.left, options.params.rect.top);
            selectedArea_ = phoenix::tools::map_editor::MapPresetRect{
                options.params.rect.left, options.params.rect.top,
                options.params.rect.right, options.params.rect.bottom };
            UpdateTitle();
            UpdateProperties();
            InvalidateMinimapCache();
            UpdateStatus();
        }

        void TogglePanel(bool& visible, const int menuId)
        {
            visible = !visible;
            CheckMenuItem(GetMenu(window_), menuId, MF_BYCOMMAND | (visible ? MF_CHECKED : MF_UNCHECKED));
            RECT client{}; GetClientRect(window_, &client); Layout(client.right, client.bottom);
        }

        std::filesystem::path PresetDirectory() const
        {
            std::array<wchar_t, 32768> executable{};
            const auto length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
            if (length == 0U || length >= executable.size()) return std::filesystem::path(L"Tool/MapEditor/Presets");
            return std::filesystem::path(executable.data()).parent_path().parent_path() / L"Presets";
        }

        std::optional<std::size_t> SelectedPresetIndex() const
        {
            const auto selected = ListView_GetNextItem(presetList_, -1, LVNI_SELECTED);
            if (selected < 0 || static_cast<std::size_t>(selected) >= presetCatalog_.size()) return std::nullopt;
            return static_cast<std::size_t>(selected);
        }

        void RefreshPresetCatalog(const std::filesystem::path& selectPath = {})
        {
            presetListUpdating_ = true;
            ListView_DeleteAllItems(presetList_);
            presetCatalog_.clear();
            std::error_code error;
            const auto directory = PresetDirectory();
            std::filesystem::create_directories(directory, error);
            if (!error)
            {
                for (const auto& entry : std::filesystem::directory_iterator(directory, error))
                {
                    if (error) break;
                    if (!entry.is_regular_file() || _wcsicmp(entry.path().extension().c_str(), L".phxpreset") != 0) continue;
                    auto loaded = phoenix::tools::map_editor::LoadMapPreset(entry.path());
                    presetCatalog_.push_back({ entry.path(), std::move(loaded.preset), std::move(loaded.error) });
                }
                std::sort(presetCatalog_.begin(), presetCatalog_.end(), [](const auto& left, const auto& right) {
                    return _wcsicmp(left.path.filename().c_str(), right.path.filename().c_str()) < 0;
                });
            }
            int selectRow = -1;
            for (std::size_t index = 0; index < presetCatalog_.size(); ++index)
            {
                const auto& entry = presetCatalog_[index];
                std::wstring name = entry.preset ? ToWide(entry.preset->name) : L"[Invalid] " + entry.path.stem().wstring();
                std::wstring contents = entry.preset ? std::to_wstring(entry.preset->tiles.size()) + L" tiles, " +
                    std::to_wstring(entry.preset->objects.size()) + L" objects" : ToWide(entry.error);
                LVITEMW item{};
                item.mask = LVIF_TEXT;
                item.iItem = static_cast<int>(index);
                item.pszText = name.data();
                ListView_InsertItem(presetList_, &item);
                ListView_SetItemText(presetList_, static_cast<int>(index), 1, contents.data());
                if (!selectPath.empty() && std::filesystem::equivalent(entry.path, selectPath, error)) selectRow = static_cast<int>(index);
                error.clear();
            }
            presetListUpdating_ = false;
            if (selectRow >= 0)
            {
                ListView_SetItemState(presetList_, selectRow, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            }
            else if (!presetDraft_)
            {
                presetSelectedIndex_.reset();
                SetWindowTextW(presetName_, L"");
                BuildPresetPreview(nullptr);
            }
        }

        void SelectPresetFromList()
        {
            if (presetListUpdating_) return;
            const auto selected = SelectedPresetIndex();
            if (!selected) return;
            presetSelectedIndex_ = selected;
            presetDraft_.reset();
            const auto& entry = presetCatalog_[*selected];
            SetWindowTextW(presetName_, entry.preset ? ToWide(entry.preset->name).c_str() : entry.path.stem().c_str());
            BuildPresetPreview(entry.preset ? &*entry.preset : nullptr);
        }

        void BeginPresetCapture()
        {
            if (!map_ || !selectedArea_) return;
            auto captured = phoenix::tools::map_editor::CaptureMapPreset(*map_, *selectedArea_, "Preset");
            if (!captured.Succeeded())
            {
                MessageBoxW(window_, ToWide(captured.error).c_str(), L"Add as Preset", MB_ICONERROR);
                return;
            }
            presetDraft_ = std::move(*captured.preset);
            presetSelectedIndex_.reset();
            showPresetEditor_ = true;
            CheckMenuItem(GetMenu(window_), IdViewPresets, MF_BYCOMMAND | MF_CHECKED);
            RefreshPresetCatalog();
            ListView_SetItemState(presetList_, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
            SetWindowTextW(presetName_, L"Preset");
            BuildPresetPreview(&*presetDraft_);
            RECT client{}; GetClientRect(window_, &client); Layout(client.right, client.bottom);
            SetFocus(presetName_);
            SendMessageW(presetName_, EM_SETSEL, 0, -1);
        }

        std::optional<std::wstring> ReadPresetName()
        {
            const auto length = GetWindowTextLengthW(presetName_);
            std::wstring name(static_cast<std::size_t>(length) + 1U, L'\0');
            if (length > 0) GetWindowTextW(presetName_, name.data(), length + 1);
            name.resize(static_cast<std::size_t>(length));
            const auto valid = phoenix::tools::map_editor::ValidateMapPresetName(ToUtf8(name));
            if (!valid.succeeded)
            {
                MessageBoxW(window_, ToWide(valid.error).c_str(), L"Preset", MB_ICONWARNING);
                SetFocus(presetName_);
                return std::nullopt;
            }
            return name;
        }

        void SavePresetDraft()
        {
            const auto name = ReadPresetName();
            if (!name) return;
            phoenix::tools::map_editor::MapPreset* preset = presetDraft_ ? &*presetDraft_ : nullptr;
            if (preset == nullptr && presetSelectedIndex_ && *presetSelectedIndex_ < presetCatalog_.size() &&
                presetCatalog_[*presetSelectedIndex_].preset)
                preset = &*presetCatalog_[*presetSelectedIndex_].preset;
            if (preset == nullptr)
            {
                MessageBoxW(window_, L"Select a map area and use Add as Preset first.", L"Preset", MB_ICONINFORMATION);
                return;
            }
            preset->name = ToUtf8(*name);
            const auto path = PresetDirectory() / (*name + L".phxpreset");
            const bool editingSameFile = presetSelectedIndex_ && *presetSelectedIndex_ < presetCatalog_.size() &&
                _wcsicmp(presetCatalog_[*presetSelectedIndex_].path.c_str(), path.c_str()) == 0;
            if (std::filesystem::exists(path) && !editingSameFile && MessageBoxW(window_,
                L"A preset with this name already exists. Replace it?", L"Preset Editor",
                MB_ICONWARNING | MB_YESNO) != IDYES) return;
            const auto saved = phoenix::tools::map_editor::SaveMapPreset(*preset, path);
            if (!saved.succeeded)
            {
                MessageBoxW(window_, ToWide(saved.error).c_str(), L"Preset save failed", MB_ICONERROR);
                return;
            }
            presetDraft_.reset();
            RefreshPresetCatalog(path);
        }

        void RenameSelectedPreset()
        {
            const auto selected = SelectedPresetIndex();
            const auto name = ReadPresetName();
            if (!selected || !name || !presetCatalog_[*selected].preset) return;
            auto renamed = *presetCatalog_[*selected].preset;
            renamed.name = ToUtf8(*name);
            const auto oldPath = presetCatalog_[*selected].path;
            const auto newPath = PresetDirectory() / (*name + L".phxpreset");
            if (_wcsicmp(oldPath.c_str(), newPath.c_str()) != 0 && std::filesystem::exists(newPath))
            {
                MessageBoxW(window_, L"A different preset already uses this name.", L"Preset rename failed", MB_ICONWARNING);
                return;
            }
            const auto saved = phoenix::tools::map_editor::SaveMapPreset(renamed, newPath);
            if (!saved.succeeded)
            {
                MessageBoxW(window_, ToWide(saved.error).c_str(), L"Preset rename failed", MB_ICONERROR);
                return;
            }
            if (_wcsicmp(oldPath.c_str(), newPath.c_str()) != 0)
            {
                std::error_code error;
                std::filesystem::remove(oldPath, error);
                if (error)
                {
                    std::filesystem::remove(newPath, error);
                    MessageBoxW(window_, L"The old preset file could not be removed; rename was rolled back.",
                        L"Preset rename failed", MB_ICONERROR);
                    return;
                }
            }
            RefreshPresetCatalog(newPath);
        }

        void DeleteSelectedPreset()
        {
            const auto selected = SelectedPresetIndex();
            if (!selected) return;
            if (MessageBoxW(window_, L"Delete the selected preset permanently?", L"Preset Editor",
                MB_ICONWARNING | MB_YESNO) != IDYES) return;
            std::error_code error;
            std::filesystem::remove(presetCatalog_[*selected].path, error);
            if (error)
            {
                MessageBoxW(window_, ToWide(error.message()).c_str(), L"Preset delete failed", MB_ICONERROR);
                return;
            }
            presetDraft_.reset();
            RefreshPresetCatalog();
        }

        void BuildPresetPreview(const phoenix::tools::map_editor::MapPreset* preset)
        {
            if (presetPreviewBitmap_ != nullptr) DeleteObject(presetPreviewBitmap_);
            presetPreviewBitmap_ = nullptr;
            presetPreviewWidth_ = 320;
            presetPreviewHeight_ = 180;
            if (preset == nullptr) { InvalidateRect(presetPreview_, nullptr, TRUE); return; }
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = presetPreviewWidth_;
            info.bmiHeader.biHeight = -presetPreviewHeight_;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            void* raw = nullptr;
            presetPreviewBitmap_ = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &raw, nullptr, 0);
            if (presetPreviewBitmap_ == nullptr || raw == nullptr) return;
            auto* pixels = static_cast<std::uint32_t*>(raw);
            std::fill(pixels, pixels + static_cast<std::size_t>(presetPreviewWidth_) * presetPreviewHeight_, RGB(28, 31, 35));
            const int margin = 10;
            const float sx = static_cast<float>(presetPreviewWidth_ - margin * 2) /
                static_cast<float>(std::max<std::uint16_t>(1U, preset->width));
            const float sy = static_cast<float>(presetPreviewHeight_ - margin * 2) /
                static_cast<float>(std::max<std::uint16_t>(1U, preset->height));
            const int contentWidth = presetPreviewWidth_ - margin * 2;
            const int contentHeight = presetPreviewHeight_ - margin * 2;
            for (int previewY = 0; previewY < contentHeight; ++previewY)
            {
                const auto tileY = std::min<std::uint16_t>(preset->height - 1U,
                    static_cast<std::uint16_t>(static_cast<std::uint64_t>(previewY) * preset->height / contentHeight));
                for (int previewX = 0; previewX < contentWidth; ++previewX)
                {
                    const auto tileX = std::min<std::uint16_t>(preset->width - 1U,
                        static_cast<std::uint16_t>(static_cast<std::uint64_t>(previewX) * preset->width / contentWidth));
                    const auto& tile = preset->tiles[static_cast<std::size_t>(tileY) * preset->width + tileX];
                    const auto color = tile.sector.spriteId == 0xFFFFU ? RGB(45, 48, 52) :
                        RGB(50 + tile.sector.spriteId * 31U % 130U, 70 + tile.sector.spriteId * 17U % 120U,
                            55 + tile.sector.spriteId * 47U % 130U);
                    pixels[static_cast<std::size_t>(margin + previewY) * presetPreviewWidth_ + margin + previewX] = color;
                }
            }
            const auto objectStep = std::max<std::size_t>(1U, (preset->objects.size() + 4095U) / 4096U);
            for (std::size_t objectIndex = 0; objectIndex < preset->objects.size(); objectIndex += objectStep)
            {
                const auto& entry = preset->objects[objectIndex];
                const auto relative = entry.relativePositions.empty() ? std::pair<std::int32_t, std::int32_t>{
                    entry.relativePixelX / 48, entry.relativePixelY / 24 } : entry.relativePositions.front();
                const int cx = margin + static_cast<int>((relative.first + 0.5F) * sx);
                const int cy = margin + static_cast<int>((relative.second + 0.5F) * sy);
                const int radius = std::clamp(static_cast<int>(std::min(sx, sy) * 0.35F), 2, 8);
                const auto color = entry.object.animation ? RGB(235, 165, 65) : RGB(100, 195, 235);
                for (int y = std::max(0, cy - radius); y <= std::min(presetPreviewHeight_ - 1, cy + radius); ++y)
                    for (int x = std::max(0, cx - radius); x <= std::min(presetPreviewWidth_ - 1, cx + radius); ++x)
                        if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= radius * radius)
                            pixels[static_cast<std::size_t>(y) * presetPreviewWidth_ + x] = color;
            }
            InvalidateRect(presetPreview_, nullptr, TRUE);
        }

        LRESULT DrawPresetPreview(const DRAWITEMSTRUCT& draw)
        {
            FillRect(draw.hDC, &draw.rcItem, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
            if (presetPreviewBitmap_ == nullptr)
            {
                RECT text = draw.rcItem;
                DrawTextW(draw.hDC, L"Select a valid preset", -1, &text,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                return TRUE;
            }
            HDC memory = CreateCompatibleDC(draw.hDC);
            const auto previous = SelectObject(memory, presetPreviewBitmap_);
            StretchBlt(draw.hDC, draw.rcItem.left, draw.rcItem.top,
                draw.rcItem.right - draw.rcItem.left, draw.rcItem.bottom - draw.rcItem.top,
                memory, 0, 0, presetPreviewWidth_, presetPreviewHeight_, SRCCOPY);
            SelectObject(memory, previous);
            DeleteDC(memory);
            return TRUE;
        }

        void PlacePresetAt(const std::size_t index, const std::pair<std::uint16_t, std::uint16_t> target)
        {
            if (!map_ || index >= presetCatalog_.size() || !presetCatalog_[index].preset) return;
            const auto& preset = *presetCatalog_[index].preset;
            const auto valid = phoenix::tools::map_editor::ValidateMapPresetPlacement(*map_, preset, target.first, target.second);
            if (!valid.succeeded)
            {
                MessageBoxW(window_, ToWide(valid.error).c_str(), L"Preset placement failed", MB_ICONWARNING);
                return;
            }
            BeginEdit();
            const auto placed = phoenix::tools::map_editor::PlaceMapPreset(*map_, preset, target.first, target.second);
            if (!placed.succeeded)
            {
                (void)history_.Undo(*map_);
                MessageBoxW(window_, ToWide(placed.error).c_str(), L"Preset placement failed", MB_ICONERROR);
                return;
            }
            selectedIds_.clear();
            selectedIds_.insert(placed.objectIds.begin(), placed.objectIds.end());
            selectedArea_ = phoenix::tools::map_editor::MapPresetRect{ target.first, target.second,
                static_cast<std::uint16_t>(target.first + preset.width - 1U),
                static_cast<std::uint16_t>(target.second + preset.height - 1U) };
            selectedTile_ = target;
            RebuildObjectIdIndex();
            MarkMapContentChanged();
            PopulatePalettes();
            UpdateTitle();
            UpdateProperties();
        }

        void SetTool(const ToolMode tool)
        {
            movementKeys_.fill(false);
            selectionNudgeKeys_.fill(false);
            tool_ = tool;
            const std::pair<HWND, ToolMode> buttons[] = { { selectButton_, ToolMode::Select }, { tileButton_, ToolMode::Tile },
                { eraseButton_, ToolMode::EraseTile }, { imageButton_, ToolMode::Image },
                { animationButton_, ToolMode::Animation }, { portalButton_, ToolMode::Portal }, { moveButton_, ToolMode::Move } };
            for (const auto& [button, mode] : buttons) SendMessageW(button, BM_SETSTATE, mode == tool_ ? TRUE : FALSE, 0);
            CheckMenuRadioItem(GetMenu(window_), IdToolSelect, IdToolMove,
                tool_ == ToolMode::Select ? IdToolSelect : tool_ == ToolMode::Tile ? IdToolTile :
                tool_ == ToolMode::EraseTile ? IdToolErase : tool_ == ToolMode::Image ? IdToolImage :
                tool_ == ToolMode::Animation ? IdToolAnimation : tool_ == ToolMode::Portal ? IdToolPortal : IdToolMove, MF_BYCOMMAND);
            UpdateStatus();
        }

        void SetZoom(const float zoom)
        {
            zoom_ = std::clamp(zoom, 0.02F, 3.0F);
            SendMessageW(zoomTrack_, TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(zoom_ * 100.0F)));
            UpdateStatus();
            InvalidateRect(minimap_, nullptr, FALSE);
        }

        void CenterCamera()
        {
            if (!map_) return;
            cameraX_ = (static_cast<float>(map_->width) - 1.0F) * 0.5F;
            cameraY_ = (static_cast<float>(map_->height) - 1.0F) * 0.5F;
            ClampCamera();
        }

        void FitMapToView()
        {
            if (!map_) return;
            CenterCamera();
            SetZoom(phoenix::tools::map_editor::CalculateFitZoom(
                phoenix::tools::map_editor::CalculateMapCanvasBounds(*map_), viewport_.width, viewport_.height));
        }

        void ResetCamera()
        {
            CenterCamera();
            SetZoom(1.0F);
        }

        LRESULT HandleNotify(NMHDR* header)
        {
            if (header->idFrom == IdPresetList)
            {
                if (header->code == LVN_ITEMCHANGED)
                {
                    SelectPresetFromList();
                    return 0;
                }
                if (header->code == LVN_BEGINDRAG)
                {
                    const auto* drag = reinterpret_cast<NMLISTVIEW*>(header);
                    if (drag->iItem >= 0 && static_cast<std::size_t>(drag->iItem) < presetCatalog_.size() &&
                        presetCatalog_[static_cast<std::size_t>(drag->iItem)].preset)
                    {
                        presetDragIndex_ = static_cast<std::size_t>(drag->iItem);
                        presetDragging_ = true;
                        SetCapture(window_);
                    }
                    return 0;
                }
                return 0;
            }
            if (header->idFrom == IdPaletteTabs && header->code == TCN_SELCHANGE)
            {
                const int selected = TabCtrl_GetCurSel(paletteTabs_);
                ShowWindow(tilePalette_, selected == 0 ? SW_SHOW : SW_HIDE);
                ShowWindow(objectPalette_, selected == 1 ? SW_SHOW : SW_HIDE);
                ShowWindow(animationPalette_, selected == 2 ? SW_SHOW : SW_HIDE);
                SetWindowTextW(paletteContentLabel_, selected == 0 ? L"Tile Browser" :
                    selected == 1 ? L"Object Browser" : L"Animation Browser");
                return 0;
            }
            if (header->idFrom == IdProperties)
            {
                if (header->code == NM_DBLCLK)
                {
                    const auto* activation = reinterpret_cast<NMITEMACTIVATE*>(header);
                    // FULLROWSELECT ile cift tik kolon 0'a da duser; value edit yine acilsin.
                    if (activation->iItem >= 0)
                        BeginPropertyEdit(activation->iItem);
                }
                return 0;
            }
            if (header->idFrom != IdSpriteList) return 0;
            if (header->code == LVN_GETDISPINFOW)
            {
                auto* info = reinterpret_cast<NMLVDISPINFOW*>(header);
                if (info->item.iItem < 0 || static_cast<std::size_t>(info->item.iItem) >= spriteVisibleIds_.size()) return 0;
                const auto id = static_cast<std::uint32_t>(spriteVisibleIds_[static_cast<std::size_t>(info->item.iItem)]);
                if (spriteSource_.type != phoenix::tools::map_editor::SpriteResourceType::Animation &&
                    spriteSource_.type != phoenix::tools::map_editor::SpriteResourceType::Effect && id >= spriteSource_.count) return 0;
                if ((info->item.mask & LVIF_IMAGE) != 0)
                    info->item.iImage = GetSpriteMetadata(spriteSource_.type, static_cast<std::uint16_t>(id)).thumbnailIndex;
                if ((info->item.mask & LVIF_TEXT) == 0) return 0;
                const std::wstring text = std::to_wstring(id);
                wcsncpy_s(info->item.pszText, static_cast<std::size_t>(info->item.cchTextMax), text.c_str(), _TRUNCATE);
                return 0;
            }
            if (header->code == LVN_ITEMCHANGED)
            {
                const auto selected = ListView_GetNextItem(spriteList_, -1, LVNI_SELECTED);
                if (selected >= 0 && static_cast<std::size_t>(selected) < spriteVisibleIds_.size())
                {
                    selectedSpriteId_ = spriteVisibleIds_[static_cast<std::size_t>(selected)];
                    selectedSpriteType_ = spriteSource_.type;
                    selectedSpriteMetadata_ = &GetSpriteMetadata(selectedSpriteType_, selectedSpriteId_);
                    SetTool(selectedSpriteType_ == phoenix::tools::map_editor::SpriteResourceType::Tile ? ToolMode::Tile :
                        selectedSpriteType_ == phoenix::tools::map_editor::SpriteResourceType::Object ? ToolMode::Image : ToolMode::Animation);
                    InvalidateRect(spritePreview_, nullptr, TRUE);
                    UpdateProperties();
                }
            }
            return 0;
        }

        bool HandleKey(WPARAM key)
        {
            const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (ctrl && key == 'N') { NewMap(); return true; }
            if (ctrl && key == 'O') { OpenMap(); return true; }
            if (ctrl && key == 'S') { Save(false); return true; }
            if (ctrl && key == 'Z') { Undo(); return true; }
            if (ctrl && key == 'Y') { Redo(); return true; }
            if (ctrl && key == 'C') { CopySelection(); return true; }
            if (ctrl && key == 'V') { Paste(); return true; }
            if (ctrl && key == 'X') { CopySelection(); DeleteSelection(); return true; }
            if (ctrl && key == 'A') { SelectAllObjects(); return true; }
            if (key == VK_DELETE) { DeleteSelection(); return true; }
            if (!ctrl && key == 'T') { SetTool(ToolMode::Tile); return true; }
            if (!ctrl && key == 'E') { SetTool(ToolMode::EraseTile); return true; }
            if (!ctrl && key == 'O') { SetTool(ToolMode::Image); return true; }
            if (!ctrl && key == 'M') { SetTool(ToolMode::Move); return true; }
            if (!ctrl && tool_ == ToolMode::Select &&
                (key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN))
                return NudgeSelectionWithKey(key);
            if (!ctrl && tool_ == ToolMode::Move && (key == 'W' || key == 'A' || key == 'S' || key == 'D' ||
                key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN))
                return SetMovementKey(key, true);
            if (!ctrl && (key == VK_ADD || key == VK_OEM_PLUS)) { SetZoom(zoom_ * 1.1F); return true; }
            if (!ctrl && (key == VK_SUBTRACT || key == VK_OEM_MINUS)) { SetZoom(zoom_ * 0.9F); return true; }
            if (!ctrl && (key == '0' || key == VK_NUMPAD0)) { SetZoom(1.0F); return true; }
            if (!ctrl && key == 'F') { FitMapToView(); return true; }
            if (!ctrl && key == VK_RETURN && tool_ == ToolMode::Portal && selectedArea_)
            {
                ShowFillEdgePortalDialog();
                return true;
            }
            return false;
        }

        bool SetMovementKey(const WPARAM key, const bool down) noexcept
        {
            std::size_t index = movementKeys_.size();
            if (key == 'W' || key == VK_UP) index = 0U;
            else if (key == 'S' || key == VK_DOWN) index = 1U;
            else if (key == 'A' || key == VK_LEFT) index = 2U;
            else if (key == 'D' || key == VK_RIGHT) index = 3U;
            if (index == movementKeys_.size()) return false;
            movementKeys_[index] = down;
            return true;
        }

        void HandleKeyUp(const WPARAM key) noexcept
        {
            (void)SetMovementKey(key, false);
            const auto index = DirectionKeyIndex(key);
            if (index < selectionNudgeKeys_.size()) selectionNudgeKeys_[index] = false;
        }

        static std::size_t DirectionKeyIndex(const WPARAM key) noexcept
        {
            if (key == VK_UP) return 0U;
            if (key == VK_DOWN) return 1U;
            if (key == VK_LEFT) return 2U;
            if (key == VK_RIGHT) return 3U;
            return 4U;
        }

        bool NudgeSelectionWithKey(const WPARAM key)
        {
            if (!map_) return true;
            const auto keyIndex = DirectionKeyIndex(key);
            if (keyIndex >= selectionNudgeKeys_.size()) return false;
            const int dx = key == VK_LEFT ? -1 : key == VK_RIGHT ? 1 : 0;
            const int dy = key == VK_UP ? -1 : key == VK_DOWN ? 1 : 0;
            if (selectedArea_)
            {
                const phoenix::tools::map_editor::MapSelectionRect selection{
                    selectedArea_->left, selectedArea_->top, selectedArea_->right, selectedArea_->bottom };
                if (!phoenix::tools::map_editor::CanMoveMapSelection(*map_, selection, selectedIds_, dx, dy)) return true;
                const bool transactionActive = std::any_of(
                    selectionNudgeKeys_.begin(), selectionNudgeKeys_.end(), [](const bool down) { return down; });
                if (!transactionActive) BeginEdit();
                selectionNudgeKeys_[keyIndex] = true;
                if (!phoenix::tools::map_editor::MoveMapSelection(*map_, selection, selectedIds_, dx, dy)) return true;
                selectedArea_->left = static_cast<std::uint16_t>(static_cast<int>(selectedArea_->left) + dx);
                selectedArea_->right = static_cast<std::uint16_t>(static_cast<int>(selectedArea_->right) + dx);
                selectedArea_->top = static_cast<std::uint16_t>(static_cast<int>(selectedArea_->top) + dy);
                selectedArea_->bottom = static_cast<std::uint16_t>(static_cast<int>(selectedArea_->bottom) + dy);
                selectedTile_ = std::make_pair(selectedArea_->left, selectedArea_->top);
                hoverTile_ = selectedTile_;
                RebuildObjectIdIndex();
                MarkMapContentChanged();
                UpdateProperties();
                return true;
            }
            if (!selectedIds_.empty())
            {
                if (!CanMoveSelection(dx, dy)) return true;
                const bool transactionActive = std::any_of(
                    selectionNudgeKeys_.begin(), selectionNudgeKeys_.end(), [](const bool down) { return down; });
                if (!transactionActive) BeginEdit();
                selectionNudgeKeys_[keyIndex] = true;
                MoveSelection(dx, dy, false);
            }
            return true;
        }

        void SelectAllObjects()
        {
            if (!map_) return;
            selectedIds_.clear();
            selectedArea_.reset();
            selectedTile_.reset();
            for (const auto& object : map_->imageObjects) selectedIds_.insert(object.objectId);
            UpdateProperties();
        }

        bool ConfirmDiscard()
        {
            if (!map_ || !map_->modified) return true;
            const auto answer = MessageBoxW(window_, L"The map has unsaved changes. Save before continuing?", L"Map Editor",
                MB_ICONWARNING | MB_YESNOCANCEL);
            if (answer == IDCANCEL) return false;
            if (answer == IDNO) return true;
            return Save(false);
        }

        void NewMap()
        {
            if (!ConfirmDiscard()) return;
            NewMapOptions options;
            const int dialogWidth = MulDiv(330, static_cast<int>(dpi_), 96);
            const int dialogHeight = MulDiv(205, static_cast<int>(dpi_), 96);
            RECT owner{}; GetWindowRect(window_, &owner);
            const auto dialog = CreateWindowExW(WS_EX_DLGMODALFRAME, kNewMapClass, L"New Legacy Map",
                WS_POPUP | WS_CAPTION | WS_SYSMENU, owner.left + (owner.right - owner.left - dialogWidth) / 2,
                owner.top + (owner.bottom - owner.top - dialogHeight) / 2, dialogWidth, dialogHeight,
                window_, nullptr, instance_, &options);
            if (dialog == nullptr) return;
            EnableWindow(window_, FALSE);
            ShowWindow(dialog, SW_SHOW);
            MSG message{};
            while (!options.closed && GetMessageW(&message, nullptr, 0, 0) > 0)
            {
                if (!IsDialogMessageW(dialog, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
            }
            EnableWindow(window_, TRUE);
            SetForegroundWindow(window_);
            if (!options.accepted) return;
            phoenix::world::LegacyMap fresh;
            fresh.version = "=MAP_2000_05_10=";
            fresh.name = ToUtf8(options.name);
            fresh.description = "Created by Phoenix Map Editor";
            fresh.width = options.width;
            fresh.height = options.height;
            fresh.sectors.resize(static_cast<std::size_t>(fresh.width) * fresh.height);
            for (auto& sector : fresh.sectors) sector.spriteId = 0xFFFFU;
            fresh.sectorImageObjectIndices.resize(fresh.sectors.size());
            phoenix::world::InitializeLegacyMapSidecars(fresh);
            fresh.modified = true;
            map_ = std::move(fresh);
            MarkMapReplaced();
            currentPath_.clear();
            history_.Clear(); selectedIds_.clear(); clipboard_.clear(); selectedTile_.reset(); selectedArea_.reset();
            CenterCamera(); SetZoom(1.0F);
            ResolveDefaultResources();
            PopulatePalettes(); UpdateTitle(); UpdateProperties(); InvalidateRect(minimap_, nullptr, FALSE);
        }

        void OpenMap()
        {
            if (!ConfirmDiscard()) return;
            const auto path = OpenFileDialog(window_, L"Legacy map (*.map)\0*.map\0All files\0*.*\0", false);
            if (path.empty()) return;
            phoenix::world::LegacyMapReader reader;
            auto result = reader.Load(path);
            if (!result.Succeeded())
            {
                MessageBoxW(window_, ToWide(result.error).c_str(), L"Map load failed", MB_ICONERROR);
                return;
            }
            map_ = std::move(*result.map);
            phoenix::world::LegacyMapSidecarReader sidecarReader;
            const auto sidecars = sidecarReader.Load(*map_, path);
            if (!sidecars.succeeded)
            {
                map_.reset();
                MessageBoxW(window_, ToWide(sidecars.error).c_str(), L"Server sidecar load failed", MB_ICONERROR);
                return;
            }
            MarkMapReplaced();
            currentPath_ = path;
            history_.Clear(); selectedIds_.clear(); clipboard_.clear();
            selectedTile_.reset(); selectedArea_.reset();
            CenterCamera(); SetZoom(1.0F);
            ResolveDefaultResources();
            PopulatePalettes();
            UpdateTitle();
            UpdateProperties();
            InvalidateRect(minimap_, nullptr, FALSE);
        }

        void ResolveDefaultResources()
        {
            auto root = currentPath_.parent_path().parent_path() / L"Image";
            if (!std::filesystem::exists(root))
            {
                wchar_t modulePath[32768]{};
                GetModuleFileNameW(nullptr, modulePath, static_cast<DWORD>(std::size(modulePath)));
                root = std::filesystem::path(modulePath).parent_path().parent_path().parent_path().parent_path() /
                    L"GameData" / L"data" / L"Image";
            }
            if (!std::filesystem::exists(root)) root = std::filesystem::current_path() / L"GameData" / L"data" / L"Image";
            if (root == resourceRoot_ && animationRenderer_.Loaded()) return;
            ReleaseTextures();
            decodedSprites_.clear();
            resourceRoot_ = root;
            paletteResourceRoot_.clear();
            tileArchive_ = root / L"tile.spk";
            tileIndex_ = root / L"tile.spki";
            std::string tileLogicalIndexError;
            if (!tileLogicalIndex_.Load(root.parent_path() / L"Info" / L"TILE.IDX", tileLogicalIndexError))
                tileLogicalIndex_.Clear();
            phoenix::tools::map_editor::EditorSpriteArchiveReader tileIndexReader;
            const auto physicalTileArchive = tileIndexReader.ReadArchiveInfo(
                tileArchive_, tileIndex_, phoenix::assets::LegacyAssetFormat::SpritePack);
            physicalTileSpriteCount_ = physicalTileArchive.value ? physicalTileArchive.value->spriteCount : 0U;
            drawListCache_.SetTileSpriteIndex(
                tileLogicalIndex_.Loaded() ? &tileLogicalIndex_ : nullptr, physicalTileSpriteCount_);
            objectArchive_ = root / L"imageobject.spk";
            objectIndex_ = root / L"imageobject.spki";
            tileSplitUnit_ = ReadSplitUnit(tileArchive_);
            objectSplitUnit_ = ReadSplitUnit(objectArchive_);
            effectAlphaArchive_ = root / L"effect.aspk";
            effectAlphaIndex_ = root / L"effect.aspki";
            effectAlphaPalette_ = root / L"effect.ppk";
            effectAlphaSplitUnit_ = ReadSplitUnit(effectAlphaArchive_);
            effectShadowArchive_ = root / L"effect.sspk";
            effectShadowIndex_ = root / L"effect.sspki";
            effectScreenArchive_ = root / L"effectscreen.spk";
            effectScreenIndex_ = root / L"effectscreen.spki";
            effectScreenPalette_ = root / L"effectscreen.ppk";
            if (!animationRenderer_.Load(phoenix::world::LegacyMapAnimationFramePackPaths{
                root / L"animationobject.afpk", root / L"animationobjectshadow.afpk",
                root / L"effect.efpk", root / L"shadoweffect.efpk", root / L"effectscreen.efpk" }))
                MessageBoxW(window_, L"Animation object frame packs could not be loaded.", L"Map Editor", MB_ICONERROR);
            animationStartMs_ = GetTickCount64();
            effectPaletteIds_ = animationRenderer_.EffectFrameIds(
                phoenix::world::LegacyMapAnimationSpriteSource::EffectAlpha);
            spriteMetadata_.clear();
            spriteSourceCatalog_.clear();
            ResetSpriteThumbnailCache();
            selectedSpriteMetadata_ = nullptr;
            spriteSourceLoaded_ = false;
            spriteSourceSelection_.Reset();
        }

        bool Save(bool saveAs)
        {
            if (!map_) return false;
            auto path = currentPath_;
            if (saveAs || path.empty()) path = OpenFileDialog(window_, L"Legacy map (*.map)\0*.map\0", true);
            if (path.empty()) return false;
            phoenix::world::LegacyMapBundleWriter writer;
            const auto result = writer.Save(*map_, path);
            if (!result.succeeded)
            {
                MessageBoxW(window_, ToWide(result.error).c_str(), L"Map save failed", MB_ICONERROR);
                return false;
            }
            phoenix::world::LegacyMapReader reader;
            auto reload = reader.Load(path);
            if (!reload.Succeeded())
            {
                MessageBoxW(window_, L"Map was written but failed immediate verification reload.", L"Map Editor", MB_ICONERROR);
                return false;
            }
            map_ = std::move(*reload.map);
            phoenix::world::LegacyMapSidecarReader sidecarReader;
            const auto sidecars = sidecarReader.Load(*map_, path);
            if (!sidecars.succeeded)
            {
                MessageBoxW(window_, ToWide(sidecars.error).c_str(), L"Map bundle verification failed", MB_ICONERROR);
                return false;
            }
            MarkMapReplaced(false);
            currentPath_ = path;
            history_.Clear();
            PopulatePalettes();
            UpdateTitle();
            UpdateProperties();
            const auto message = result.originalMapBytesPreserved
                ? L"Client and server map bundle saved; original .map payload preserved."
                : (result.backupCreated ? L"Client and server map bundle saved. Automatic backups created."
                    : L"Client and server map bundle saved.");
            SendMessageW(status_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(message));
            // destinationKnown=false olan portal'lar icin kullaniciya warning goster,
            // ama Save'i durdurma. Bu portal'lar .smp'ye X/Y=0,0 olarak yazildi.
            if (!result.portalDestinationsUnknown.empty())
            {
                std::wstring warn = L"Warning: " + std::to_wstring(result.portalDestinationsUnknown.size()) +
                    L" portal target X/Y is unknown (no .smp or .mip/.smp mismatch).\r\n"
                    L"These portals were saved with destination X/Y = 0,0.\r\n"
                    L"Affected sectors (x,y):\r\n";
                for (const auto& coord : result.portalDestinationsUnknown)
                {
                    warn += L"  (" + std::to_wstring(coord.first) + L"," + std::to_wstring(coord.second) + L")\r\n";
                    if (warn.size() > 4000U) { warn += L"  ...\r\n"; break; }
                }
                warn += L"\nEdit each portal target manually in the property panel to set real X/Y.";
                MessageBoxW(window_, warn.c_str(), L"Map saved with portal warnings", MB_ICONWARNING);
            }
            return true;
        }

        struct ScreenshotDrawCommand
        {
            const phoenix::graphics::ImageData* image{};
            int x{ 0 };
            int y{ 0 };
            bool screenBlend{ false };
        };

        const phoenix::graphics::ImageData* ResolveScreenshotFrameImage(
            const phoenix::world::LegacyMapResolvedFrame& frame)
        {
            auto archive = objectArchive_;
            auto index = objectIndex_;
            auto format = phoenix::assets::LegacyAssetFormat::SpritePack;
            std::filesystem::path palette;
            bool alpha = false;
            if (frame.source == phoenix::world::LegacyMapAnimationSpriteSource::ImageObjectShadow)
            {
                archive.replace_extension(L".sspk");
                index.replace_extension(L".sspki");
                format = phoenix::assets::LegacyAssetFormat::ShadowSpritePack;
            }
            else if (frame.source == phoenix::world::LegacyMapAnimationSpriteSource::EffectAlpha)
            {
                archive = effectAlphaArchive_; index = effectAlphaIndex_; palette = effectAlphaPalette_;
                format = phoenix::assets::LegacyAssetFormat::AlphaSpritePack; alpha = true;
            }
            else if (frame.source == phoenix::world::LegacyMapAnimationSpriteSource::EffectShadow)
            {
                archive = effectShadowArchive_; index = effectShadowIndex_;
                format = phoenix::assets::LegacyAssetFormat::ShadowSpritePack;
            }
            else if (frame.source == phoenix::world::LegacyMapAnimationSpriteSource::EffectScreen)
            {
                archive = effectScreenArchive_; index = effectScreenIndex_; palette = effectScreenPalette_;
                format = phoenix::assets::LegacyAssetFormat::IndexedSpritePack;
            }
            return DecodeSprite(archive, index, frame.spriteId, format, palette, frame.paletteId, alpha);
        }

        std::vector<ScreenshotDrawCommand> BuildScreenshotCommands()
        {
            std::vector<ScreenshotDrawCommand> commands;
            if (!map_) return commands;
            const auto& list = drawListCache_.Ensure(*map_, contentRevision_);
            commands.reserve(list.tiles.size() + list.staticObjects.size() * 2U);
            const auto appendObject = [&](const phoenix::world::WorldStaticObjectDrawItem& item) {
                if (item.object == nullptr) return;
                const auto visual = animationRenderer_.Resolve(*item.object, 0U);
                for (const auto& frame : phoenix::world::LegacyMapAnimationRenderer::OrderedFrames(visual))
                {
                    const auto* image = ResolveScreenshotFrameImage(frame);
                    if (image == nullptr || image->Empty()) continue;
                    commands.push_back({ image, item.anchor.x + frame.cX, item.anchor.y + frame.cY,
                        frame.blendMode == phoenix::world::LegacyMapAnimationBlendMode::Screen });
                }
            };
            for (const auto& tile : list.tiles)
            {
                const auto* image = DecodeSprite(tileArchive_, tileIndex_, tile.spriteId,
                    phoenix::assets::LegacyAssetFormat::SpritePack, {}, 0xFFFFU, false);
                if (image != nullptr && !image->Empty()) commands.push_back({ image, tile.anchor.x, tile.anchor.y, false });
            }
            for (const auto layerIndex : phoenix::tools::map_editor::BuildLegacyEditorObjectLayerOrder(list))
            {
                const auto& layer = list.layers[layerIndex];
                appendObject(list.staticObjects[layer.index]);
            }
            return commands;
        }

        static void CompositeScreenshotCommand(std::vector<std::uint8_t>& target, const int targetWidth,
            const int targetHeight, const int stripY, const ScreenshotDrawCommand& command,
            const float scale, const int offsetX, const int offsetY)
        {
            const auto& image = *command.image;
            const auto destination = phoenix::tools::map_editor::CalculateScreenshotSpriteRect(
                command.x, command.y, image.width, image.height, scale, offsetX, offsetY);
            const int destinationX = destination.x;
            const int destinationY = destination.y;
            const int destinationWidth = destination.width;
            const int destinationHeight = destination.height;
            const int top = std::max(stripY, destinationY);
            const int bottom = std::min(stripY + targetHeight, destinationY + destinationHeight);
            const int left = std::max(0, destinationX);
            const int right = std::min(targetWidth, destinationX + destinationWidth);
            if (left >= right || top >= bottom) return;
            for (int y = top; y < bottom; ++y)
            {
                const auto sourceY = std::min(image.height - 1U, static_cast<std::uint32_t>(
                    static_cast<std::uint64_t>(y - destinationY) * image.height / destinationHeight));
                for (int x = left; x < right; ++x)
                {
                    const auto sourceX = std::min(image.width - 1U, static_cast<std::uint32_t>(
                        static_cast<std::uint64_t>(x - destinationX) * image.width / destinationWidth));
                    const auto sourceOffset = (static_cast<std::size_t>(sourceY) * image.width + sourceX) * 4U;
                    const auto destinationOffset = (static_cast<std::size_t>(y - stripY) * targetWidth + x) * 4U;
                    const auto sourceR = image.pixels[sourceOffset + (image.format == phoenix::graphics::PixelFormat::Rgba8 ? 0U : 2U)];
                    const auto sourceG = image.pixels[sourceOffset + 1U];
                    const auto sourceB = image.pixels[sourceOffset + (image.format == phoenix::graphics::PixelFormat::Rgba8 ? 2U : 0U)];
                    const auto sourceA = image.pixels[sourceOffset + 3U];
                    if (sourceA == 0U) continue;
                    const auto alpha = static_cast<unsigned>(sourceA);
                    if (command.screenBlend)
                    {
                        const auto screen = [](const unsigned source, const unsigned destination) {
                            return 255U - ((255U - source) * (255U - destination) + 127U) / 255U;
                        };
                        target[destinationOffset] = static_cast<std::uint8_t>((screen(sourceB, target[destinationOffset]) * alpha +
                            target[destinationOffset] * (255U - alpha) + 127U) / 255U);
                        target[destinationOffset + 1U] = static_cast<std::uint8_t>((screen(sourceG, target[destinationOffset + 1U]) * alpha +
                            target[destinationOffset + 1U] * (255U - alpha) + 127U) / 255U);
                        target[destinationOffset + 2U] = static_cast<std::uint8_t>((screen(sourceR, target[destinationOffset + 2U]) * alpha +
                            target[destinationOffset + 2U] * (255U - alpha) + 127U) / 255U);
                    }
                    else
                    {
                        target[destinationOffset] = static_cast<std::uint8_t>((sourceB * alpha +
                            target[destinationOffset] * (255U - alpha) + 127U) / 255U);
                        target[destinationOffset + 1U] = static_cast<std::uint8_t>((sourceG * alpha +
                            target[destinationOffset + 1U] * (255U - alpha) + 127U) / 255U);
                        target[destinationOffset + 2U] = static_cast<std::uint8_t>((sourceR * alpha +
                            target[destinationOffset + 2U] * (255U - alpha) + 127U) / 255U);
                    }
                    target[destinationOffset + 3U] = 255U;
                }
            }
        }

        void ExportScreenshot(const bool gameMinimap, const bool highQualityMinimap)
        {
            if (!map_) return;
            const auto suggested = highQualityMinimap ? L"minimap-800x400.png" :
                gameMinimap ? L"game-minimap.png" : L"full-map.png";
            const auto path = SavePngDialog(window_, suggested);
            if (path.empty()) return;
            const auto bounds = phoenix::tools::map_editor::CalculateMapCanvasBounds(*map_);
            const auto commands = BuildScreenshotCommands();
            bool succeeded = false;
            if (!gameMinimap)
            {
                const auto plan = phoenix::tools::map_editor::BuildFullMapScreenshotPlan(bounds);
#if defined(_DEBUG)
                screenshotChunkCount_ += plan.stripCount;
                screenshotPeakTemporaryBytes_ = std::max(screenshotPeakTemporaryBytes_, plan.peakTemporaryBytes);
#endif
                succeeded = phoenix::tools::map_editor::WritePngImage(path, plan.width, plan.height, [&](IWICBitmapFrameEncode* frame) {
                    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(plan.width) * plan.stripHeight * 4U);
                    for (std::uint32_t y = 0; y < plan.height; y += plan.stripHeight)
                    {
                        const auto rows = std::min(plan.stripHeight, plan.height - y);
                        std::fill(pixels.begin(), pixels.begin() + static_cast<std::size_t>(plan.width) * rows * 4U,
                            std::uint8_t{ 0U });
                        for (std::size_t pixel = 0; pixel < static_cast<std::size_t>(plan.width) * rows; ++pixel)
                            pixels[pixel * 4U + 3U] = 255U;
                        for (const auto& command : commands)
                            CompositeScreenshotCommand(pixels, static_cast<int>(plan.width), static_cast<int>(rows),
                                static_cast<int>(y), command, 1.0F, -static_cast<int>(bounds.left), -static_cast<int>(bounds.top));
                        if (FAILED(frame->WritePixels(rows, plan.width * 4U, plan.width * rows * 4U, pixels.data()))) return false;
                    }
                    return true;
                });
            }
            else
            {
                const std::uint32_t outputWidth = highQualityMinimap ? 800U : 200U;
                const std::uint32_t outputHeight = highQualityMinimap ? 400U : 100U;
                const auto plan = phoenix::tools::map_editor::BuildGameMinimapScreenshotPlan(
                    bounds, outputWidth, outputHeight);
                const auto fit = phoenix::tools::map_editor::CalculateGameMinimapFit(
                    bounds, outputWidth, outputHeight);
                const int supersample = highQualityMinimap ? 2 : 4;
                const unsigned sampleCount = static_cast<unsigned>(supersample * supersample);
                const auto highWidth = plan.width * supersample;
                const auto highHeight = plan.height * supersample;
                std::vector<std::uint8_t> high(static_cast<std::size_t>(highWidth) * highHeight * 4U, 0U);
                for (std::size_t pixel = 0; pixel < static_cast<std::size_t>(highWidth) * highHeight; ++pixel)
                    high[pixel * 4U + 3U] = 255U;
                for (const auto& command : commands)
                    CompositeScreenshotCommand(high, static_cast<int>(highWidth), static_cast<int>(highHeight), 0,
                        command, fit.scale * supersample,
                        fit.contentX * supersample - static_cast<int>(std::lround(bounds.left * fit.scale * supersample)),
                        fit.contentY * supersample - static_cast<int>(std::lround(bounds.top * fit.scale * supersample)));
                std::vector<std::uint8_t> output(plan.peakTemporaryBytes, 0U);
                for (std::uint32_t y = 0; y < plan.height; ++y)
                    for (std::uint32_t x = 0; x < plan.width; ++x)
                        for (std::uint32_t channel = 0; channel < 4U; ++channel)
                {
                    unsigned sum = 0U;
                    for (int sy = 0; sy < supersample; ++sy) for (int sx = 0; sx < supersample; ++sx)
                        sum += high[(static_cast<std::size_t>(y * supersample + sy) * highWidth +
                            x * supersample + sx) * 4U + channel];
                    output[(static_cast<std::size_t>(y) * plan.width + x) * 4U + channel] =
                        static_cast<std::uint8_t>((sum + sampleCount / 2U) / sampleCount);
                }
#if defined(_DEBUG)
                ++screenshotChunkCount_;
                screenshotPeakTemporaryBytes_ = std::max(screenshotPeakTemporaryBytes_, high.size() + output.size());
#endif
                succeeded = phoenix::tools::map_editor::WritePngImage(path, plan.width, plan.height,
                    [&](IWICBitmapFrameEncode* frame) {
                    return SUCCEEDED(frame->WritePixels(plan.height, plan.width * 4U,
                        static_cast<UINT>(output.size()), output.data()));
                });
            }
            MessageBoxW(window_, succeeded ? L"Screenshot exported." : L"Screenshot export failed.",
                L"Screenshot", succeeded ? MB_ICONINFORMATION : MB_ICONERROR);
        }

        void BeginEdit()
        {
            if (!map_) return;
            history_.Begin(*map_);
            map_->modified = true;
            UpdateTitle();
        }

        void ClearPortalAtHover()
        {
            if (!map_ || !hoverTile_) return;
            if (!phoenix::tools::map_editor::ClearPortalOnTile(
                    *map_, history_, hoverTile_->first, hoverTile_->second))
                return;
            selectedIds_.clear();
            selectedTile_ = hoverTile_;
            selectedArea_.reset();
            UpdateTitle();
            UpdateProperties();
            InvalidateMinimapCache();
        }

        void MarkMapContentChanged()
        {
            ++contentRevision_;
            ++minimapRevision_;
            drawListCache_.Invalidate();
            minimapCacheState_.Invalidate();
            InvalidateRect(minimap_, nullptr, FALSE);
        }

        void MarkMapReplaced(const bool minimapChanged = true)
        {
            RebuildObjectIdIndex();
            ++contentRevision_;
            drawListCache_.Invalidate();
            if (minimapChanged)
            {
                ++minimapRevision_;
                minimapCacheState_.Invalidate();
                ReleaseMinimapBitmap();
            }
        }

        void Undo()
        {
            if (!map_ || !history_.Undo(*map_)) return;
            selectedIds_.clear();
            selectedArea_.reset();
            RebuildObjectIdIndex();
            MarkMapContentChanged();
            PopulatePalettes();
            UpdateTitle(); UpdateProperties(); InvalidateRect(minimap_, nullptr, FALSE);
        }

        void Redo()
        {
            if (!map_ || !history_.Redo(*map_)) return;
            selectedIds_.clear();
            selectedArea_.reset();
            RebuildObjectIdIndex();
            MarkMapContentChanged();
            PopulatePalettes();
            UpdateTitle(); UpdateProperties(); InvalidateRect(minimap_, nullptr, FALSE);
        }

        void RebuildObjectIndex()
        {
            if (!map_) return;
            phoenix::tools::map_editor::RebuildObjectIndex(*map_);
            RebuildObjectIdIndex();
        }

        void RebuildObjectIdIndex()
        {
            objectIndexById_.clear();
            if (!map_) return;
            for (std::size_t index = 0; index < map_->imageObjects.size(); ++index)
                objectIndexById_.emplace(map_->imageObjects[index].objectId, index);
        }

        void LoadFixedSpriteSource(const phoenix::tools::map_editor::SpriteResourceType type)
        {
            ResolveDefaultResources();
            const auto fixed = phoenix::tools::map_editor::ResolveFixedSpriteSource(resourceRoot_, type);
            if (!std::filesystem::exists(fixed.archive) ||
                ((type == phoenix::tools::map_editor::SpriteResourceType::Tile ||
                    type == phoenix::tools::map_editor::SpriteResourceType::Object ||
                    type == phoenix::tools::map_editor::SpriteResourceType::Effect) && !std::filesystem::exists(fixed.index)))
            {
                const auto message = L"Configured sprite resource does not exist:\n" + fixed.archive.wstring();
                MessageBoxW(window_, message.c_str(), L"Sprite Browser", MB_ICONERROR);
                return;
            }
            if (!spriteSourceSelection_.Select(fixed)) return;
            if (const auto cached = spriteSourceCatalog_.find(type); cached != spriteSourceCatalog_.end() &&
                cached->second.archive == fixed.archive && cached->second.index == fixed.index)
            {
                spriteSource_ = cached->second;
                spriteSourceLoaded_ = true;
                selectedSpriteMetadata_ = nullptr;
                RefreshSpriteFilter();
                UpdateProperties();
                return;
            }
            SpriteSource source;
            source.archive = fixed.archive;
            source.index = fixed.index;
            source.type = type;
            source.split = type == phoenix::tools::map_editor::SpriteResourceType::Tile ||
                type == phoenix::tools::map_editor::SpriteResourceType::Object;
            source.splitUnit = source.split ? ReadSplitUnit(source.archive) : 0U;
            if (!LoadSpriteSource(std::move(source))) spriteSourceSelection_.Reset();
        }

        bool LoadSpriteSource(SpriteSource source)
        {
            if (spriteSourceLoaded_ && spriteSource_.SameSource(source)) return true;
#if defined(_DEBUG)
            ++splitFolderScans_;
#endif
            if (source.type == phoenix::tools::map_editor::SpriteResourceType::Animation)
            {
                source.count = static_cast<std::uint16_t>(std::min<std::size_t>(
                    animationRenderer_.BodySequenceCount(), 65535U));
            }
            else if (source.type == phoenix::tools::map_editor::SpriteResourceType::Effect)
            {
                source.count = static_cast<std::uint16_t>(std::min<std::size_t>(effectPaletteIds_.size(), 65535U));
            }
            else
            {
                phoenix::tools::map_editor::EditorSpriteArchiveReader reader;
                const auto archive = reader.ReadArchiveInfo(source.archive, source.index, source.format);
                if (!archive.value)
                {
                    MessageBoxW(window_, L"Sprite archive metadata could not be loaded.", L"Sprite Browser", MB_ICONERROR);
                    return false;
                }
                source.count = archive.value->spriteCount;
            }
            spriteSource_ = std::move(source);
            spriteSourceCatalog_[spriteSource_.type] = spriteSource_;
            spriteSourceLoaded_ = true;
            selectedSpriteMetadata_ = nullptr;
            RefreshSpriteFilter();
            UpdateProperties();
            return true;
        }

        void RefreshSpriteFilter()
        {
            spriteVisibleIds_.clear();
            wchar_t query[64]{};
            GetWindowTextW(spriteSearch_, query, static_cast<int>(std::size(query)));
            if (query[0] == L'\0')
            {
                if (spriteSource_.type == phoenix::tools::map_editor::SpriteResourceType::Animation)
                    spriteVisibleIds_ = animationPaletteIds_;
                else if (spriteSource_.type == phoenix::tools::map_editor::SpriteResourceType::Effect)
                    spriteVisibleIds_ = effectPaletteIds_;
                else
                {
                    spriteVisibleIds_.reserve(spriteSource_.count);
                    for (std::uint32_t id = 0; id < spriteSource_.count; ++id)
                        spriteVisibleIds_.push_back(static_cast<std::uint16_t>(id));
                }
            }
            else
            {
                wchar_t* end = nullptr;
                const auto id = wcstoul(query, &end, 10);
                if (end != query && *end == L'\0' && id <= 65535U)
                {
                    const auto spriteId = static_cast<std::uint16_t>(id);
                    if (((spriteSource_.type == phoenix::tools::map_editor::SpriteResourceType::Animation &&
                        std::find(animationPaletteIds_.begin(), animationPaletteIds_.end(), spriteId) != animationPaletteIds_.end()) ||
                        (spriteSource_.type == phoenix::tools::map_editor::SpriteResourceType::Effect &&
                            std::find(effectPaletteIds_.begin(), effectPaletteIds_.end(), spriteId) != effectPaletteIds_.end())) ||
                        (spriteSource_.type != phoenix::tools::map_editor::SpriteResourceType::Animation &&
                            spriteSource_.type != phoenix::tools::map_editor::SpriteResourceType::Effect && id < spriteSource_.count))
                        spriteVisibleIds_.push_back(spriteId);
                }
            }
            ListView_SetItemCountEx(spriteList_, static_cast<int>(spriteVisibleIds_.size()),
                LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
            InvalidateRect(spriteList_, nullptr, TRUE);
        }

        int CreateThumbnail(const phoenix::graphics::ImageData& image)
        {
            if (spriteImages_ == nullptr || image.Empty() ||
                (image.format != phoenix::graphics::PixelFormat::Rgba8 && image.format != phoenix::graphics::PixelFormat::Bgra8)) return -1;
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            constexpr int thumbnailSize = 64;
            info.bmiHeader.biWidth = thumbnailSize;
            info.bmiHeader.biHeight = -thumbnailSize;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            void* pixels = nullptr;
            const auto bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
            if (bitmap == nullptr || pixels == nullptr) return -1;
            auto* target = static_cast<std::uint8_t*>(pixels);
            for (std::size_t pixel = 0; pixel < static_cast<std::size_t>(thumbnailSize * thumbnailSize); ++pixel)
            {
                target[pixel * 4U] = 32U; target[pixel * 4U + 1U] = 32U;
                target[pixel * 4U + 2U] = 32U; target[pixel * 4U + 3U] = 255U;
            }
            const float scale = std::min(62.0F / std::max(1U, image.width), 62.0F / std::max(1U, image.height));
            const int drawW = std::max(1, static_cast<int>(std::lround(image.width * scale)));
            const int drawH = std::max(1, static_cast<int>(std::lround(image.height * scale)));
            const int offsetX = (thumbnailSize - drawW) / 2;
            const int offsetY = (thumbnailSize - drawH) / 2;
            for (int y = 0; y < drawH; ++y)
            {
                const auto sourceY = std::min(image.height - 1U, static_cast<std::uint32_t>(y / scale));
                for (int x = 0; x < drawW; ++x)
                {
                    const auto sourceX = std::min(image.width - 1U, static_cast<std::uint32_t>(x / scale));
                    const auto source = (static_cast<std::size_t>(sourceY) * image.width + sourceX) * 4U;
                    const auto destination = (static_cast<std::size_t>(offsetY + y) * thumbnailSize + offsetX + x) * 4U;
                    if (image.format == phoenix::graphics::PixelFormat::Rgba8)
                    {
                        target[destination] = image.pixels[source + 2U];
                        target[destination + 1U] = image.pixels[source + 1U];
                        target[destination + 2U] = image.pixels[source];
                    }
                    else
                    {
                        target[destination] = image.pixels[source];
                        target[destination + 1U] = image.pixels[source + 1U];
                        target[destination + 2U] = image.pixels[source + 2U];
                    }
                    target[destination + 3U] = image.pixels[source + 3U];
                }
            }
            const int result = ImageList_Add(spriteImages_, bitmap, nullptr);
            DeleteObject(bitmap);
#if defined(_DEBUG)
            if (result >= 0) ++thumbnailCreationMisses_;
#endif
            return result;
        }

        void ResetSpriteThumbnailCache()
        {
            if (spriteImages_ == nullptr) return;
            ImageList_RemoveAll(spriteImages_);
            phoenix::graphics::ImageData missing;
            missing.width = missing.height = 8U;
            missing.format = phoenix::graphics::PixelFormat::Rgba8;
            missing.pixels.resize(8U * 8U * 4U, 255U);
            for (std::size_t pixel = 0; pixel < 64U; ++pixel)
            {
                missing.pixels[pixel * 4U] = (pixel / 8U + pixel % 8U) % 2U == 0U ? 180U : 70U;
                missing.pixels[pixel * 4U + 1U] = 30U;
                missing.pixels[pixel * 4U + 2U] = 60U;
            }
            errorThumbnailIndex_ = CreateThumbnail(missing);
        }

        static std::uint64_t SpriteMetadataKey(const phoenix::tools::map_editor::SpriteResourceType type,
            const std::uint16_t spriteId) noexcept
        {
            return (static_cast<std::uint64_t>(type) << 32U) | spriteId;
        }

        const SpriteMetadata& GetSpriteMetadata(const phoenix::tools::map_editor::SpriteResourceType type,
            const std::uint16_t spriteId)
        {
            auto& metadata = spriteMetadata_[SpriteMetadataKey(type, spriteId)];
            if (metadata.attempted) return metadata;
#if defined(_DEBUG)
            const auto browserProfile = profiler_.Measure(ProfileArea::SpriteBrowser);
#endif
            metadata.attempted = true;
            const phoenix::graphics::ImageData* image = nullptr;
            if (type == phoenix::tools::map_editor::SpriteResourceType::Animation)
            {
                phoenix::world::LegacyMapImageObject object;
                object.animation = true;
                object.wrapperType = 5U;
                object.frameId = spriteId;
                const auto resolved = animationRenderer_.Resolve(object, 0U);
                if (resolved.body)
                {
                    const auto& frame = *resolved.body;
                    metadata.representativeSpriteId = frame.spriteId;
                    metadata.animation = phoenix::world::LegacyMapAnimationSpriteMetadata{
                        frame.source, spriteId, frame.frameIndex, frame.frameCount, frame.cX, frame.cY };
                    metadata.splitBase = objectSplitUnit_ == 0U ? 0U :
                        (frame.spriteId / objectSplitUnit_) * objectSplitUnit_;
                    metadata.sourcePath = objectSplitUnit_ == 0U ? objectArchive_ :
                        SplitSpriteArchivePath(objectArchive_, metadata.splitBase);
                    metadata.internalIndex = frame.spriteId - metadata.splitBase;
                    image = DecodeSprite(objectArchive_, objectIndex_, frame.spriteId,
                        phoenix::assets::LegacyAssetFormat::SpritePack, {}, 0xFFFFU, false);
                }
            }
            else if (type == phoenix::tools::map_editor::SpriteResourceType::Effect)
            {
                phoenix::world::LegacyMapImageObject object;
                object.animation = true;
                object.wrapperType = 5U;
                object.frameId = spriteId;
                object.bltType = 1U;
                const auto resolved = animationRenderer_.Resolve(object, 0U);
                if (resolved.body)
                {
                    const auto& frame = *resolved.body;
                    metadata.representativeSpriteId = frame.spriteId;
                    metadata.animation = phoenix::world::LegacyMapAnimationSpriteMetadata{
                        frame.source, spriteId, frame.frameIndex, frame.frameCount, frame.cX, frame.cY };
                    metadata.splitBase = effectAlphaSplitUnit_ == 0U ? 0U :
                        (frame.spriteId / effectAlphaSplitUnit_) * effectAlphaSplitUnit_;
                    metadata.sourcePath = effectAlphaSplitUnit_ == 0U ? effectAlphaArchive_ :
                        SplitSpriteArchivePath(effectAlphaArchive_, metadata.splitBase, L"aspk");
                    metadata.internalIndex = frame.spriteId - metadata.splitBase;
                    image = DecodeSprite(effectAlphaArchive_, effectAlphaIndex_, frame.spriteId,
                        phoenix::assets::LegacyAssetFormat::AlphaSpritePack, effectAlphaPalette_,
                        frame.paletteId, true);
                }
            }
            else
            {
                const auto fixed = phoenix::tools::map_editor::ResolveFixedSpriteSource(resourceRoot_, type);
                const auto splitUnit = type == phoenix::tools::map_editor::SpriteResourceType::Tile
                    ? tileSplitUnit_ : objectSplitUnit_;
                metadata.splitBase = splitUnit == 0U ? 0U : (spriteId / splitUnit) * splitUnit;
                metadata.sourcePath = splitUnit == 0U ? fixed.archive :
                    SplitSpriteArchivePath(fixed.archive, metadata.splitBase);
                metadata.internalIndex = spriteId - metadata.splitBase;
                metadata.representativeSpriteId = spriteId;
                image = DecodeSprite(fixed.archive, fixed.index, spriteId,
                    phoenix::assets::LegacyAssetFormat::SpritePack, {}, 0xFFFFU, false);
            }
            if (image != nullptr)
            {
                metadata.decoded = true;
                metadata.width = image->width;
                metadata.height = image->height;
                metadata.thumbnailIndex = CreateThumbnail(*image);
            }
            if (metadata.thumbnailIndex < 0) metadata.thumbnailIndex = errorThumbnailIndex_;
            return metadata;
        }

        LRESULT DrawSpritePreview(const DRAWITEMSTRUCT& draw) const
        {
            FillRect(draw.hDC, &draw.rcItem, reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1));
            if (selectedSpriteMetadata_ != nullptr && selectedSpriteMetadata_->thumbnailIndex >= 0 && spriteImages_ != nullptr)
                ImageList_Draw(spriteImages_, selectedSpriteMetadata_->thumbnailIndex, draw.hDC,
                    draw.rcItem.left + std::max(0L, (draw.rcItem.right - draw.rcItem.left - 48L) / 2L),
                    draw.rcItem.top + std::max(0L, (draw.rcItem.bottom - draw.rcItem.top - 48L) / 2L), ILD_NORMAL);
            return TRUE;
        }

        void SetPaletteCount(HWND list, const std::uint32_t count, const bool available)
        {
            SendMessageW(list, LB_SETCOUNT, static_cast<WPARAM>(available && count != 0U ? count : 1U), 0);
            InvalidateRect(list, nullptr, TRUE);
        }

        void PopulatePalettes()
        {
            if (resourceRoot_.empty() || paletteResourceRoot_ == resourceRoot_) return;
            phoenix::tools::map_editor::EditorSpriteArchiveReader reader;
            const auto tileInfo = reader.ReadArchiveInfo(
                tileArchive_, tileIndex_, phoenix::assets::LegacyAssetFormat::SpritePack);
            const auto objectInfo = reader.ReadArchiveInfo(
                objectArchive_, objectIndex_, phoenix::assets::LegacyAssetFormat::SpritePack);
            tilePaletteAvailable_ = tileInfo.value.has_value() && tileInfo.value->spriteCount != 0U;
            objectPaletteAvailable_ = objectInfo.value.has_value() && objectInfo.value->spriteCount != 0U;
            tilePaletteCount_ = tilePaletteAvailable_ ? tileInfo.value->spriteCount : 0U;
            objectPaletteCount_ = objectPaletteAvailable_ ? objectInfo.value->spriteCount : 0U;
            tilePalettePlaceholder_ = tilePaletteAvailable_ ? L"" : L"Tile archive unavailable";
            objectPalettePlaceholder_ = objectPaletteAvailable_ ? L"" : L"Image Object archive unavailable";

            animationPaletteIds_.clear();
            const auto sequenceCount = std::min<std::size_t>(animationRenderer_.BodySequenceCount(), 65536U);
            animationPaletteIds_.reserve(sequenceCount);
            for (std::size_t frameId = 0; frameId < sequenceCount; ++frameId)
            {
                if (animationRenderer_.BodyFrameCount(static_cast<std::uint16_t>(frameId)) != 0U)
                    animationPaletteIds_.push_back(static_cast<std::uint16_t>(frameId));
            }
            animationPaletteAvailable_ = !animationPaletteIds_.empty();
            animationPalettePlaceholder_ = animationPaletteAvailable_ ? L"" : L"Animation frame pack unavailable";
            SetPaletteCount(tilePalette_, tilePaletteCount_, tilePaletteAvailable_);
            SetPaletteCount(objectPalette_, objectPaletteCount_, objectPaletteAvailable_);
            SetPaletteCount(animationPalette_, static_cast<std::uint32_t>(animationPaletteIds_.size()), animationPaletteAvailable_);
            paletteResourceRoot_ = resourceRoot_;
        }

        void SelectPaletteSprite(HWND list, const PaletteKind kind)
        {
            const auto row = ListBox_GetCurSel(list);
            if (row == LB_ERR) return;
            std::uint16_t selected = 0U;
            if (kind == PaletteKind::Tile)
            {
                if (!tilePaletteAvailable_ || static_cast<std::uint32_t>(row) >= tilePaletteCount_) return;
                selected = static_cast<std::uint16_t>(row);
            }
            else if (kind == PaletteKind::Image)
            {
                if (!objectPaletteAvailable_ || static_cast<std::uint32_t>(row) >= objectPaletteCount_) return;
                selected = static_cast<std::uint16_t>(row);
            }
            else
            {
                if (!animationPaletteAvailable_ || static_cast<std::size_t>(row) >= animationPaletteIds_.size()) return;
                selected = animationPaletteIds_[static_cast<std::size_t>(row)];
            }
            selectedSpriteId_ = selected;
            selectedSpriteType_ = kind == PaletteKind::Tile ? phoenix::tools::map_editor::SpriteResourceType::Tile :
                kind == PaletteKind::Image ? phoenix::tools::map_editor::SpriteResourceType::Object :
                phoenix::tools::map_editor::SpriteResourceType::Animation;
            selectedSpriteMetadata_ = &GetSpriteMetadata(selectedSpriteType_, selectedSpriteId_);
            SetTool(kind == PaletteKind::Tile ? ToolMode::Tile : kind == PaletteKind::Image ? ToolMode::Image : ToolMode::Animation);
            UpdateProperties();
        }

        LRESULT DrawPaletteItem(const DRAWITEMSTRUCT& draw)
        {
            if (draw.itemID == static_cast<UINT>(-1)) return TRUE;
            const bool selected = (draw.itemState & ODS_SELECTED) != 0U;
            FillRect(draw.hDC, &draw.rcItem, GetSysColorBrush(selected ? COLOR_HIGHLIGHT : COLOR_WINDOW));
            SetBkMode(draw.hDC, TRANSPARENT);
            SetTextColor(draw.hDC, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

            wchar_t text[160]{};
            int thumbnail = errorThumbnailIndex_;
            if (draw.CtlID == IdTilePalette)
            {
                if (tilePaletteAvailable_ && draw.itemID < tilePaletteCount_)
                {
                    swprintf_s(text, L"Tile Sprite %u", draw.itemID);
                    thumbnail = GetSpriteMetadata(phoenix::tools::map_editor::SpriteResourceType::Tile,
                        static_cast<std::uint16_t>(draw.itemID)).thumbnailIndex;
                }
                else wcsncpy_s(text, tilePalettePlaceholder_.c_str(), _TRUNCATE);
            }
            else if (draw.CtlID == IdObjectPalette)
            {
                if (objectPaletteAvailable_ && draw.itemID < objectPaletteCount_)
                {
                    swprintf_s(text, L"Image Object Sprite %u", draw.itemID);
                    thumbnail = GetSpriteMetadata(phoenix::tools::map_editor::SpriteResourceType::Object,
                        static_cast<std::uint16_t>(draw.itemID)).thumbnailIndex;
                }
                else wcsncpy_s(text, objectPalettePlaceholder_.c_str(), _TRUNCATE);
            }
            else
            {
                if (animationPaletteAvailable_ && draw.itemID < animationPaletteIds_.size())
                {
                    const auto frameId = animationPaletteIds_[draw.itemID];
                    swprintf_s(text, L"Animation %u  (%zu frames)", frameId,
                        animationRenderer_.BodyFrameCount(frameId));
                    thumbnail = GetSpriteMetadata(phoenix::tools::map_editor::SpriteResourceType::Animation,
                        frameId).thumbnailIndex;
                }
                else wcsncpy_s(text, animationPalettePlaceholder_.c_str(), _TRUNCATE);
            }
            RECT textRect = draw.rcItem;
            if (spriteImages_ != nullptr && thumbnail >= 0)
                ImageList_Draw(spriteImages_, thumbnail, draw.hDC, draw.rcItem.left + 3,
                    draw.rcItem.top + std::max(0L, (draw.rcItem.bottom - draw.rcItem.top - 64L) / 2L), ILD_NORMAL);
            textRect.left += 72;
            DrawTextW(draw.hDC, text, -1, &textRect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
            if ((draw.itemState & ODS_FOCUS) != 0U) DrawFocusRect(draw.hDC, &draw.rcItem);
            return TRUE;
        }

        void HandleMouseMove(int x, int y, WPARAM keys)
        {
            if (panning_)
            {
                const auto dx = x - lastMouse_.x;
                const auto dy = y - lastMouse_.y;
                cameraX_ -= static_cast<float>(dx) / (48.0F * zoom_);
                cameraY_ -= static_cast<float>(dy) / (24.0F * zoom_);
                ClampCamera();
                lastMouse_ = { x, y };
                InvalidateRect(minimap_, nullptr, FALSE);
            }
            const auto previousHover = hoverTile_;
            hoverTile_ = PointInViewport(x, y) ? ScreenToTile(x, y) : std::nullopt;
            if (dragging_ && tool_ == ToolMode::Move && hoverTile_) MoveSelectedTo(*hoverTile_);
            if (rectangleSelecting_)
            {
                selectionCurrent_ = { x, y };
            }
            if ((keys & MK_LBUTTON) != 0 && (tool_ == ToolMode::Tile || tool_ == ToolMode::EraseTile) &&
                hoverTile_ && lastPaintTile_ != hoverTile_)
            {
                PaintTile(tool_ == ToolMode::EraseTile ? 0xFFFFU : selectedSpriteId_);
            }
            if (previousHover != hoverTile_) UpdateStatus();
        }

        void HandleLeftDown(int x, int y, WPARAM keys)
        {
            if (!PointInViewport(x, y)) return;
            hoverTile_ = ScreenToTile(x, y);
            if (!map_ || !hoverTile_) return;
            if (tool_ == ToolMode::Tile || tool_ == ToolMode::EraseTile)
            {
                selectedTile_ = hoverTile_;
                selectedArea_ = phoenix::tools::map_editor::MapPresetRect{
                    hoverTile_->first, hoverTile_->second, hoverTile_->first, hoverTile_->second };
                PaintTile(tool_ == ToolMode::EraseTile ? 0xFFFFU : selectedSpriteId_);
                SetCapture(window_);
                return;
            }
            if (tool_ == ToolMode::Image || tool_ == ToolMode::Animation) { PlaceObject(tool_ == ToolMode::Animation); return; }
            if (tool_ == ToolMode::Portal)
            {
                selectedIds_.clear();
                selectedTile_ = hoverTile_;
                selectedArea_ = phoenix::tools::map_editor::MapPresetRect{
                    hoverTile_->first, hoverTile_->second, hoverTile_->first, hoverTile_->second };
                rectangleSelecting_ = true;
                selectionStart_ = selectionCurrent_ = { x, y };
                SetCapture(window_);
                UpdateProperties();
                return;
            }

            const auto id = HitObject(x, y);
            const bool preserve = (keys & MK_CONTROL) != 0;
            if (!preserve) selectedIds_.clear();
            if (id)
            {
                if (!preserve)
                {
                    selectedTile_.reset();
                    selectedArea_.reset();
                }
                if (!selectedIds_.insert(*id).second && preserve) selectedIds_.erase(*id);
                if (tool_ == ToolMode::Move && selectedIds_.count(*id) != 0U)
                {
                    dragging_ = true;
                    dragHistoryStarted_ = false;
                    dragAnchor_ = *hoverTile_;
                    SetCapture(window_);
                }
            }
            else if (tool_ == ToolMode::Select)
            {
                selectedTile_ = hoverTile_;
                selectedArea_ = phoenix::tools::map_editor::MapPresetRect{
                    hoverTile_->first, hoverTile_->second, hoverTile_->first, hoverTile_->second };
                rectangleSelecting_ = true;
                rectanglePreserve_ = preserve;
                selectionStart_ = selectionCurrent_ = { x, y };
                SetCapture(window_);
            }
            UpdateProperties();
        }

        void HandleLeftUp(int x, int y, WPARAM)
        {
            if (presetDragging_)
            {
                const auto dragIndex = presetDragIndex_;
                presetDragging_ = false;
                if (GetCapture() == window_) ReleaseCapture();
                if (PointInViewport(x, y))
                {
                    const auto target = ScreenToTile(x, y);
                    if (target) PlacePresetAt(dragIndex, *target);
                }
                return;
            }
            if (rectangleSelecting_)
            {
                selectionCurrent_ = { x, y };
                RECT selection{ std::min(selectionStart_.x, selectionCurrent_.x), std::min(selectionStart_.y, selectionCurrent_.y),
                    std::max(selectionStart_.x, selectionCurrent_.x), std::max(selectionStart_.y, selectionCurrent_.y) };
                const auto first = ScreenToTileClamped(selectionStart_.x, selectionStart_.y);
                const auto last = ScreenToTileClamped(selectionCurrent_.x, selectionCurrent_.y);
                if (first && last)
                {
                    selectedArea_ = phoenix::tools::map_editor::MapPresetRect{
                        std::min(first->first, last->first), std::min(first->second, last->second),
                        std::max(first->first, last->first), std::max(first->second, last->second) };
                    selectedTile_ = std::make_pair(selectedArea_->left, selectedArea_->top);
                }
                if (tool_ != ToolMode::Portal)
                {
                    if (!rectanglePreserve_) selectedIds_.clear();
                    for (const auto& [id, rect] : objectRects_)
                    {
                        RECT intersection{};
                        if (IntersectRect(&intersection, &selection, &rect)) selectedIds_.insert(id);
                    }
                    if (map_ && selectedArea_)
                    {
                        for (const auto& object : map_->imageObjects)
                        {
                            bool inside = std::any_of(object.positions.begin(), object.positions.end(), [&](const auto& position) {
                                return position.first >= selectedArea_->left && position.first <= selectedArea_->right &&
                                    position.second >= selectedArea_->top && position.second <= selectedArea_->bottom;
                            });
                            if (!inside && object.positions.empty())
                            {
                                const auto objectX = object.pixelX / 48;
                                const auto objectY = object.pixelY / 24;
                                inside = objectX >= selectedArea_->left && objectX <= selectedArea_->right &&
                                    objectY >= selectedArea_->top && objectY <= selectedArea_->bottom;
                            }
                            if (inside) selectedIds_.insert(object.objectId);
                        }
                    }
                }
                else
                {
                    selectedIds_.clear();
                }
                rectangleSelecting_ = false;
                UpdateProperties();
                UpdateStatus();
            }
            dragging_ = false;
            dragHistoryStarted_ = false;
            dragAnchor_.reset();
            lastPaintTile_.reset();
            paintHistoryStarted_ = false;
            if (GetCapture() == window_) ReleaseCapture();
        }

        std::optional<std::pair<std::uint16_t, std::uint16_t>> ScreenToTile(int x, int y) const
        {
            if (!map_) return std::nullopt;
            const float cx = viewport_.x + viewport_.width * 0.5F;
            const float cy = viewport_.y + viewport_.height * 0.5F;
            const float baseX = cx + (static_cast<float>(x) - cx) / zoom_;
            const float baseY = cy + (static_cast<float>(y) - cy) / zoom_;
            const auto origin = CurrentCameraOrigin();
            const auto tx = static_cast<int>(std::floor((baseX - origin.x) / 48.0F));
            const auto ty = static_cast<int>(std::floor((baseY - origin.y) / 24.0F));
            if (tx < 0 || ty < 0 || tx >= map_->width || ty >= map_->height) return std::nullopt;
            return std::make_pair(static_cast<std::uint16_t>(tx), static_cast<std::uint16_t>(ty));
        }

        std::optional<std::pair<std::uint16_t, std::uint16_t>> ScreenToTileClamped(int x, int y) const
        {
            if (!map_) return std::nullopt;
            x = std::clamp(x, viewport_.x, viewport_.x + viewport_.width - 1);
            y = std::clamp(y, viewport_.y, viewport_.y + viewport_.height - 1);
            const float cx = viewport_.x + viewport_.width * 0.5F;
            const float cy = viewport_.y + viewport_.height * 0.5F;
            const float baseX = cx + (static_cast<float>(x) - cx) / zoom_;
            const float baseY = cy + (static_cast<float>(y) - cy) / zoom_;
            const auto origin = CurrentCameraOrigin();
            const auto tx = std::clamp(static_cast<int>(std::floor((baseX - origin.x) / 48.0F)),
                0, static_cast<int>(map_->width) - 1);
            const auto ty = std::clamp(static_cast<int>(std::floor((baseY - origin.y) / 24.0F)),
                0, static_cast<int>(map_->height) - 1);
            return std::make_pair(static_cast<std::uint16_t>(tx), static_cast<std::uint16_t>(ty));
        }

        void PaintTile(std::uint16_t spriteId)
        {
            if (!map_ || !hoverTile_ || lastPaintTile_ == hoverTile_) return;
            const auto index = static_cast<std::size_t>(hoverTile_->second) * map_->width + hoverTile_->first;
            if (map_->sectors[index].spriteId == spriteId) return;
            if (spriteId != 0xFFFFU && !ResourceExists(true, spriteId))
            {
                lastPaintTile_ = hoverTile_;
                MessageBoxW(window_, L"The selected tile sprite does not exist in the active Legacy tile archive.",
                    L"Tile painting failed", MB_ICONWARNING);
                return;
            }
            if (!paintHistoryStarted_) { BeginEdit(); paintHistoryStarted_ = true; }
            map_->sectors[index].spriteId = spriteId;
            if (spriteId != 0xFFFFU) EnsureResource(true, spriteId);
            lastPaintTile_ = hoverTile_;
            MarkMapContentChanged();
            PopulatePalettes();
            InvalidateRect(minimap_, nullptr, FALSE);
        }

        bool EnsureResource(bool tile, std::uint32_t spriteId)
        {
            auto& resources = tile ? map_->tileSpriteFilePositions : map_->imageObjectSpriteFilePositions;
            if (std::any_of(resources.begin(), resources.end(), [spriteId](const auto& e) { return e.spriteId == spriteId; })) return true;
            phoenix::tools::map_editor::EditorSpriteArchiveReader reader;
            const auto index = reader.ReadIndex(tile ? tileIndex_ : objectIndex_, phoenix::assets::LegacyAssetFormat::SpritePack);
            if (!index.value || spriteId >= index.value->offsets.size()) return false;
            resources.push_back({ spriteId, static_cast<std::int32_t>(index.value->offsets[spriteId]) });
            return true;
        }

        bool ResourceExists(const bool tile, const std::uint32_t spriteId) const
        {
            if (!map_) return false;
            const auto& resources = tile ? map_->tileSpriteFilePositions : map_->imageObjectSpriteFilePositions;
            if (std::any_of(resources.begin(), resources.end(), [spriteId](const auto& entry) { return entry.spriteId == spriteId; })) return true;
            phoenix::tools::map_editor::EditorSpriteArchiveReader reader;
            const auto index = reader.ReadIndex(tile ? tileIndex_ : objectIndex_, phoenix::assets::LegacyAssetFormat::SpritePack);
            return index.value && spriteId < index.value->offsets.size();
        }

        void PlaceObject(bool animation)
        {
            if (!map_ || !hoverTile_) return;
            const bool effect = animation &&
                selectedSpriteType_ == phoenix::tools::map_editor::SpriteResourceType::Effect;
            if (!animation && !ResourceExists(false, selectedSpriteId_))
            {
                MessageBoxW(window_, L"The selected object sprite does not exist in the active Legacy object archive.",
                    L"Object placement failed", MB_ICONWARNING);
                return;
            }
            if (animation && !effect && animationRenderer_.BodyFrameCount(selectedSpriteId_) == 0U)
            {
                MessageBoxW(window_, L"The selected animation frame ID does not exist in animationobject.afpk.",
                    L"Animation placement failed", MB_ICONWARNING);
                return;
            }
            if (effect && animationRenderer_.EffectFrameCount(selectedSpriteId_,
                phoenix::world::LegacyMapAnimationSpriteSource::EffectAlpha) == 0U)
            {
                MessageBoxW(window_, L"The selected effect frame ID does not exist in effect.efpk.",
                    L"Effect placement failed", MB_ICONWARNING);
                return;
            }
            if (effect)
            {
                const auto& metadata = GetSpriteMetadata(
                    phoenix::tools::map_editor::SpriteResourceType::Effect, selectedSpriteId_);
                selectedSpriteMetadata_ = &metadata;
                if (!metadata.decoded || metadata.representativeSpriteId == 0xFFFFU)
                {
                    MessageBoxW(window_, L"The selected effect frame exists but its representative sprite could not be decoded.",
                        L"Effect placement failed", MB_ICONWARNING);
                    return;
                }
            }
            BeginEdit();
            std::uint32_t id = 1;
            for (const auto& object : map_->imageObjects) id = std::max(id, object.objectId + 1U);
            phoenix::world::LegacyMapImageObject object;
            object.wrapperType = animation ? 5U : 3U;
            object.objectType = object.wrapperType;
            object.objectId = id;
            object.imageObjectId = id;
            object.x = 0xFFFFU;
            object.y = 0U;
            object.spriteId = effect && selectedSpriteMetadata_ != nullptr
                ? selectedSpriteMetadata_->representativeSpriteId : selectedSpriteId_;
            object.pixelX = static_cast<std::int32_t>(hoverTile_->first) * 48;
            object.pixelY = static_cast<std::int32_t>(hoverTile_->second) * 24;
            object.viewpoint = hoverTile_->second;
            object.animation = animation;
            object.frameId = animation ? selectedSpriteId_ : 0U;
            object.bltType = effect ? 1U : 0U;
            object.maxFrame = animation
                ? static_cast<std::uint8_t>(std::min<std::size_t>(255U, effect
                    ? animationRenderer_.EffectFrameCount(object.frameId,
                        phoenix::world::LegacyMapAnimationSpriteSource::EffectAlpha)
                    : animationRenderer_.BodyFrameCount(object.frameId)))
                : 0U;
            object.loop = animation;
            object.positions.emplace_back(hoverTile_->first, hoverTile_->second);
            map_->imageObjects.push_back(std::move(object));
            if (!animation) (void)EnsureResource(false, selectedSpriteId_);
            RebuildObjectIndex();
            MarkMapContentChanged();
            selectedIds_ = { id };
            selectedTile_.reset();
            selectedArea_.reset();
            PopulatePalettes();
            UpdateProperties();
        }

        std::optional<std::uint32_t> HitObject(int x, int y) const
        {
            for (auto it = objectRects_.rbegin(); it != objectRects_.rend(); ++it)
            {
                if (x >= it->second.left && x < it->second.right && y >= it->second.top && y < it->second.bottom) return it->first;
            }
            return std::nullopt;
        }

        void MoveSelectedTo(const std::pair<std::uint16_t, std::uint16_t>& tile)
        {
            if (!dragAnchor_ || tile == *dragAnchor_ || selectedIds_.empty()) return;
            const auto dx = static_cast<int>(tile.first) - dragAnchor_->first;
            const auto dy = static_cast<int>(tile.second) - dragAnchor_->second;
            if (!dragHistoryStarted_) { BeginEdit(); dragHistoryStarted_ = true; }
            MoveSelection(dx, dy, false);
            dragAnchor_ = tile;
        }

        bool CanMoveSelection(const int dx, const int dy) const
        {
            if (!map_ || selectedIds_.empty() || (dx == 0 && dy == 0)) return false;
            if (selectedArea_ && (static_cast<int>(selectedArea_->left) + dx < 0 ||
                static_cast<int>(selectedArea_->top) + dy < 0 ||
                static_cast<int>(selectedArea_->right) + dx >= map_->width ||
                static_cast<int>(selectedArea_->bottom) + dy >= map_->height)) return false;
            return std::all_of(selectedIds_.begin(), selectedIds_.end(), [&](const auto objectId) {
                const auto found = objectIndexById_.find(objectId);
                if (found == objectIndexById_.end() || found->second >= map_->imageObjects.size()) return false;
                return phoenix::tools::map_editor::CanMoveObject(*map_, found->second, dx, dy);
            });
        }

        void MoveSelection(int dx, int dy, bool recordHistory = true)
        {
            if (!CanMoveSelection(dx, dy)) return;
            if (recordHistory) BeginEdit();
            for (const auto objectId : selectedIds_)
            {
                const auto found = objectIndexById_.find(objectId);
                if (found == objectIndexById_.end() || found->second >= map_->imageObjects.size()) continue;
                (void)phoenix::tools::map_editor::MoveObject(*map_, found->second, dx, dy);
            }
            if (selectedArea_)
            {
                selectedArea_->left = static_cast<std::uint16_t>(static_cast<int>(selectedArea_->left) + dx);
                selectedArea_->right = static_cast<std::uint16_t>(static_cast<int>(selectedArea_->right) + dx);
                selectedArea_->top = static_cast<std::uint16_t>(static_cast<int>(selectedArea_->top) + dy);
                selectedArea_->bottom = static_cast<std::uint16_t>(static_cast<int>(selectedArea_->bottom) + dy);
                selectedTile_ = std::make_pair(selectedArea_->left, selectedArea_->top);
            }
            MarkMapContentChanged();
            UpdateProperties();
        }

        void DeleteSelection()
        {
            if (!map_) return;
            std::optional<phoenix::tools::map_editor::MapSelectionRect> tiles;
            if (selectedArea_)
                tiles = phoenix::tools::map_editor::MapSelectionRect{
                    selectedArea_->left, selectedArea_->top, selectedArea_->right, selectedArea_->bottom };
            else if (selectedTile_)
                tiles = phoenix::tools::map_editor::MapSelectionRect{
                    selectedTile_->first, selectedTile_->second, selectedTile_->first, selectedTile_->second };
            if (!phoenix::tools::map_editor::CanDeleteMapSelection(*map_, tiles, selectedIds_)) return;
            BeginEdit();
            if (!phoenix::tools::map_editor::DeleteMapSelection(*map_, tiles, selectedIds_)) return;
            selectedIds_.clear();
            selectedTile_.reset();
            selectedArea_.reset();
            MarkMapContentChanged();
            PopulatePalettes();
            UpdateProperties();
        }

        void CopySelection()
        {
            clipboard_.clear();
            if (!map_) return;
            for (const auto& object : map_->imageObjects)
                if (selectedIds_.count(object.objectId) != 0U) clipboard_.push_back(object);
        }

        void Paste()
        {
            if (!map_ || clipboard_.empty()) return;
            BeginEdit();
            std::uint32_t id = 1;
            for (const auto& object : map_->imageObjects) id = std::max(id, object.objectId + 1U);
            selectedIds_.clear();
            for (auto object : clipboard_)
            {
                object.objectId = id++;
                object.imageObjectId = object.objectId;
                object.x = std::min<std::uint16_t>(map_->width - 1U, object.x + 1U);
                object.y = std::min<std::uint16_t>(map_->height - 1U, object.y + 1U);
                object.pixelX += 48;
                object.pixelY += 24;
                for (auto& position : object.positions)
                {
                    position.first = std::min<std::uint16_t>(map_->width - 1U, position.first + 1U);
                    position.second = std::min<std::uint16_t>(map_->height - 1U, position.second + 1U);
                }
                selectedIds_.insert(object.objectId);
                map_->imageObjects.push_back(std::move(object));
            }
            RebuildObjectIndex();
            MarkMapContentChanged();
            PopulatePalettes();
            UpdateProperties();
        }

        void ClampCamera()
        {
            if (!map_) return;
            cameraX_ = std::clamp(cameraX_, 0.0F, std::max(0.0F, static_cast<float>(map_->width - 1U)));
            cameraY_ = std::clamp(cameraY_, 0.0F, std::max(0.0F, static_cast<float>(map_->height - 1U)));
        }

        bool PointInViewport(const int x, const int y) const noexcept
        {
            return x >= viewport_.x && y >= viewport_.y &&
                x < viewport_.x + viewport_.width && y < viewport_.y + viewport_.height;
        }

        phoenix::graphics::TextureDescriptor ResolveTexture(bool tile, std::uint8_t wrapperType, std::uint32_t spriteId,
            phoenix::world::LegacyMapAnimationSpriteSource source = phoenix::world::LegacyMapAnimationSpriteSource::ImageObject,
            std::uint16_t paletteId = 0xFFFFU)
        {
            const auto key = tile ? static_cast<std::uint32_t>(spriteId) :
                0x10000000U ^ (static_cast<std::uint32_t>(source) * 0x45D9F3BU) ^
                (static_cast<std::uint32_t>(paletteId) * 0x119DE1F3U) ^
                (static_cast<std::uint32_t>(wrapperType) << 16U) ^ spriteId;
            auto& cache = tile ? tileTextures_ : objectTextures_;
            if (const auto found = cache.find(key); found != cache.end()) return found->second;
            const bool shadow = wrapperType == 4U || wrapperType == 6U;
            auto archive = tile ? tileArchive_ : objectArchive_;
            auto index = tile ? tileIndex_ : objectIndex_;
            auto format = shadow ? phoenix::assets::LegacyAssetFormat::ShadowSpritePack : phoenix::assets::LegacyAssetFormat::SpritePack;
            if (!tile && source == phoenix::world::LegacyMapAnimationSpriteSource::EffectAlpha)
            {
                archive = effectAlphaArchive_; index = effectAlphaIndex_; format = phoenix::assets::LegacyAssetFormat::AlphaSpritePack;
            }
            else if (!tile && source == phoenix::world::LegacyMapAnimationSpriteSource::EffectShadow)
            {
                archive = effectShadowArchive_; index = effectShadowIndex_; format = phoenix::assets::LegacyAssetFormat::ShadowSpritePack;
            }
            else if (!tile && source == phoenix::world::LegacyMapAnimationSpriteSource::EffectScreen)
            {
                archive = effectScreenArchive_; index = effectScreenIndex_; format = phoenix::assets::LegacyAssetFormat::IndexedSpritePack;
            }
            else if (shadow)
            {
                archive.replace_extension(L".sspk");
                index.replace_extension(L".sspki");
            }
            const auto palette = source == phoenix::world::LegacyMapAnimationSpriteSource::EffectAlpha
                ? effectAlphaPalette_ : source == phoenix::world::LegacyMapAnimationSpriteSource::EffectScreen
                    ? effectScreenPalette_ : std::filesystem::path{};
            const auto* image = DecodeSprite(archive, index, spriteId, format, palette, paletteId,
                source == phoenix::world::LegacyMapAnimationSpriteSource::EffectAlpha);
            if (image == nullptr || image->Empty()) return {};
#if defined(_DEBUG)
            const auto uploadProfile = profiler_.Measure(ProfileArea::TextureUpload);
            ++textureUploadMisses_;
#endif
            const auto texture = renderer_.UploadTexture(*image);
            if (texture.handle.IsValid()) cache.emplace(key, texture);
            return texture;
        }

        const phoenix::graphics::ImageData* DecodeSprite(const std::filesystem::path& archive,
            const std::filesystem::path& index, const std::uint32_t spriteId,
            const phoenix::assets::LegacyAssetFormat format, const std::filesystem::path& palette,
            const std::uint16_t paletteId, const bool alpha)
        {
            const auto key = archive.wstring() + L"|" + index.wstring() + L"|" + palette.wstring() + L"|" +
                std::to_wstring(static_cast<unsigned>(format)) + L"|" + std::to_wstring(spriteId) + L"|" +
                std::to_wstring(paletteId) + (alpha ? L"|a" : L"|o");
            if (const auto found = decodedSprites_.find(key); found != decodedSprites_.end())
                return found->second ? &*found->second : nullptr;
#if defined(_DEBUG)
            const auto decodeProfile = profiler_.Measure(ProfileArea::SpkDecoding);
            ++spriteDecodeCacheMisses_;
#endif
            phoenix::tools::map_editor::EditorSpriteArchiveReader reader;
            auto& cached = decodedSprites_[key];
            if (!palette.empty() && spriteId > std::numeric_limits<std::uint16_t>::max())
            {
                cached = std::nullopt;
                return nullptr;
            }
            const auto decoded = palette.empty()
                ? reader.ReadSprite565(archive, index, spriteId, format)
                : reader.ReadPalettedSprite565(archive, index, static_cast<std::uint16_t>(spriteId),
                    palette, paletteId, alpha);
            if (!decoded.value || decoded.value->image.Empty())
            {
                cached = std::nullopt;
                return nullptr;
            }
            cached = std::move(decoded.value->image);
            return &*cached;
        }

        phoenix::render::ScreenPoint ScalePoint(phoenix::render::ScreenPoint point) const
        {
            const float cx = viewport_.x + viewport_.width * 0.5F;
            const float cy = viewport_.y + viewport_.height * 0.5F;
            return { static_cast<int>(std::lround(cx + (point.x - cx) * zoom_)),
                static_cast<int>(std::lround(cy + (point.y - cy) * zoom_)) };
        }

        phoenix::render::ScreenPoint CurrentCameraOrigin() const noexcept
        {
            return {
                viewport_.x + viewport_.width / 2 - static_cast<int>(std::lround(cameraX_ * 48.0F)) - 24,
                viewport_.y + viewport_.height / 2 - static_cast<int>(std::lround(cameraY_ * 24.0F)) - 12
            };
        }

        phoenix::render::ScreenPoint WorldToScreen(const phoenix::render::ScreenPoint point) const
        {
            const auto origin = CurrentCameraOrigin();
            return ScalePoint({ point.x + origin.x, point.y + origin.y });
        }

        phoenix::tools::map_editor::ScaledSpriteRect WorldSpriteRectToScreen(
            const phoenix::render::ScreenPoint point, const std::uint32_t width, const std::uint32_t height) const
        {
            const auto origin = CurrentCameraOrigin();
            return phoenix::tools::map_editor::CalculateScaledSpriteRect(
                point.x + origin.x, point.y + origin.y, width, height,
                viewport_.x + viewport_.width * 0.5F, viewport_.y + viewport_.height * 0.5F, zoom_);
        }

        void TickAndRender()
        {
            const auto now = std::chrono::steady_clock::now();
            if (lastRenderTick_.time_since_epoch().count() != 0 && map_)
            {
                const auto delta = std::min(0.1F, std::chrono::duration<float>(now - lastRenderTick_).count());
                const float horizontal = (movementKeys_[3] ? 1.0F : 0.0F) - (movementKeys_[2] ? 1.0F : 0.0F);
                const float vertical = (movementKeys_[1] ? 1.0F : 0.0F) - (movementKeys_[0] ? 1.0F : 0.0F);
                if (tool_ == ToolMode::Move && (horizontal != 0.0F || vertical != 0.0F))
                {
                    constexpr float speed = 12.0F;
                    const float normalization = horizontal != 0.0F && vertical != 0.0F ? 0.70710678F : 1.0F;
                    cameraX_ += horizontal * speed * normalization * delta;
                    cameraY_ += vertical * speed * normalization * delta;
                    ClampCamera();
                    InvalidateRect(minimap_, nullptr, FALSE);
                }
            }
            lastRenderTick_ = now;
            Render();
#if defined(_DEBUG)
            if (lastProfileStatusTick_.time_since_epoch().count() == 0 ||
                now - lastProfileStatusTick_ >= std::chrono::seconds(1))
            {
                UpdateProfilerStatus();
                lastProfileStatusTick_ = now;
            }
#endif
        }

#if defined(_DEBUG)
        void UpdateProfilerStatus()
        {
            constexpr std::array<const wchar_t*, static_cast<std::size_t>(ProfileArea::Count)> names{
                L"Render", L"Draw list", L"Minimap", L"Browser", L"Properties", L"SPK decode", L"Upload" };
            if (profileStatusIndex_ % (names.size() + 1U) == names.size())
            {
                wchar_t counters[256]{};
                swprintf_s(counters, L"scan %llu decode %llu thumb %llu upload %llu props %llu mini %llu draw %llu shot %llu/%zuB",
                    static_cast<unsigned long long>(splitFolderScans_),
                    static_cast<unsigned long long>(spriteDecodeCacheMisses_),
                    static_cast<unsigned long long>(thumbnailCreationMisses_),
                    static_cast<unsigned long long>(textureUploadMisses_),
                    static_cast<unsigned long long>(propertiesRefreshCount_),
                    static_cast<unsigned long long>(minimapCacheState_.RebuildCount()),
                    static_cast<unsigned long long>(drawListCache_.RebuildCount()),
                    static_cast<unsigned long long>(screenshotChunkCount_), screenshotPeakTemporaryBytes_);
                SendMessageW(status_, SB_SETTEXTW, 6, reinterpret_cast<LPARAM>(counters));
                ++profileStatusIndex_;
                return;
            }
            const auto nameIndex = profileStatusIndex_ % (names.size() + 1U);
            const auto area = static_cast<ProfileArea>(nameIndex);
            const auto& sample = profiler_.Sample(area);
            const auto average = sample.calls == 0U ? 0.0 : sample.totalMilliseconds / static_cast<double>(sample.calls);
            wchar_t text[192]{};
            swprintf_s(text, L"%s avg %.2f ms max %.2f ms calls %llu", names[nameIndex],
                average, sample.maximumMilliseconds, static_cast<unsigned long long>(sample.calls));
            SendMessageW(status_, SB_SETTEXTW, 6, reinterpret_cast<LPARAM>(text));
            ++profileStatusIndex_;
        }
#endif

        void EnsureVisibleSet(const phoenix::world::WorldDrawList& list)
        {
            if (!map_) return;
            const auto centerX = static_cast<int>(std::floor(cameraX_));
            const auto centerY = static_cast<int>(std::floor(cameraY_));
            const auto zoomKey = static_cast<int>(std::lround(zoom_ * 10000.0F));
            if (visibleRevision_ == contentRevision_ && visibleCenterX_ == centerX && visibleCenterY_ == centerY &&
                visibleWidth_ == viewport_.width && visibleHeight_ == viewport_.height && visibleZoomKey_ == zoomKey)
                return;

            visibleTileIndices_.clear();
            visibleRowLayers_.clear();
            const auto radius = phoenix::tools::map_editor::CalculateVisibleTileRadius(
                viewport_.width, viewport_.height, zoom_);
            const int minX = std::max(0, centerX - radius);
            const int maxX = std::min(static_cast<int>(map_->width) - 1, centerX + radius);
            const int minY = std::max(0, centerY - radius);
            const int maxY = std::min(static_cast<int>(map_->height) - 1, centerY + radius);
            visibleTileIndices_.reserve(static_cast<std::size_t>(std::max(0, maxX - minX + 1)) *
                static_cast<std::size_t>(std::max(0, maxY - minY + 1)));
            for (int y = minY; y <= maxY; ++y)
                for (int x = minX; x <= maxX; ++x)
                {
                    const auto index = drawListCache_.TileIndex(
                        static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y));
                    if (index != phoenix::tools::map_editor::MapEditorDrawListCache::MissingIndex)
                        visibleTileIndices_.push_back(index);
                }

            const phoenix::ui::UiRect visibilityViewport{ 0, 0,
                static_cast<int>(std::ceil(viewport_.width / std::max(0.02F, zoom_))),
                static_cast<int>(std::ceil(viewport_.height / std::max(0.02F, zoom_))) };
            const auto objectBounds = phoenix::tools::map_editor::EditorWorldDrawListBuilder::ResolveVisibleSectorBounds(
                map_->width, map_->height, static_cast<std::uint16_t>(centerX),
                static_cast<std::uint16_t>(centerY), visibilityViewport);
            for (std::size_t layerIndex = 0; layerIndex < list.layers.size(); ++layerIndex)
            {
                const auto& layer = list.layers[layerIndex];
                if (layer.kind != phoenix::world::WorldLayerDrawKind::StaticObject || layer.index >= list.staticObjects.size()) continue;
                const auto* object = list.staticObjects[layer.index].object;
                if (object == nullptr) continue;
                const bool visible = std::any_of(object->positions.begin(), object->positions.end(), [&](const auto& position) {
                    return position.first >= objectBounds.minX && position.first <= objectBounds.maxX &&
                        position.second >= objectBounds.minY && position.second <= objectBounds.maxY;
                });
                if (!visible) continue;
                visibleRowLayers_.push_back(layerIndex);
            }
            visibleRevision_ = contentRevision_;
            visibleCenterX_ = centerX;
            visibleCenterY_ = centerY;
            visibleWidth_ = viewport_.width;
            visibleHeight_ = viewport_.height;
            visibleZoomKey_ = zoomKey;
        }

        void Render()
        {
#if defined(_DEBUG)
            const auto renderProfile = profiler_.Measure(ProfileArea::MapRender);
#endif
            if (!renderer_.IsInitialized()) return;
            renderer_.SetUiScissorRect(phoenix::backends::directx11::UiScissorRect{
                viewport_.x, viewport_.y, viewport_.width, viewport_.height });
            renderer_.ClearFrame(0.035F, 0.04F, 0.045F, 1.0F);
            objectRects_.clear();
            if (map_ && viewport_.width > 0 && viewport_.height > 0)
            {
#if defined(_DEBUG)
                if (!drawListCache_.ValidFor(contentRevision_))
                {
                    const auto drawListProfile = profiler_.Measure(ProfileArea::DrawListGeneration);
                    lastDrawList_ = &drawListCache_.Ensure(*map_, contentRevision_);
                }
                else lastDrawList_ = &drawListCache_.Ensure(*map_, contentRevision_);
#else
                lastDrawList_ = &drawListCache_.Ensure(*map_, contentRevision_);
#endif
                EnsureVisibleSet(*lastDrawList_);

                frameQuads_.clear();
                const auto legacyMapFrame = (GetTickCount64() - animationStartMs_) / phoenix::world::LegacyMapAnimationTickMs;
                auto queueObject = [&](const phoenix::world::WorldStaticObjectDrawItem& item) {
                    if (item.object == nullptr) return;
                    const bool animation = item.object->wrapperType == 5U || item.object->wrapperType == 6U ||
                        item.object->wrapperType == 7U || item.object->animation;
                    if ((animation && !showAnimations_) || (!animation && !showImages_)) return;
                    const auto visual = animationRenderer_.Resolve(*item.object, legacyMapFrame);
                    for (const auto& frame : phoenix::world::LegacyMapAnimationRenderer::OrderedFrames(visual))
                    {
                        const auto texture = ResolveTexture(false, frame.wrapperType, frame.spriteId, frame.source, frame.paletteId);
                        if (!texture.handle.IsValid()) continue;
                        const auto scaled = WorldSpriteRectToScreen(
                            { item.anchor.x + frame.cX, item.anchor.y + frame.cY }, texture.width, texture.height);
                        const phoenix::render::ScreenPoint point{ scaled.x, scaled.y };
                        const auto width = static_cast<std::uint32_t>(scaled.width);
                        const auto height = static_cast<std::uint32_t>(scaled.height);
                        const phoenix::ui::UiRect rect{ point.x, point.y, scaled.width, scaled.height };
                        if (!phoenix::tools::map_editor::EditorWorldDrawListBuilder::Intersects(rect, viewport_)) continue;
                        frameQuads_.push_back({ texture, point, width, height });
                        if (frame.blendMode == phoenix::world::LegacyMapAnimationBlendMode::Screen)
                            frameQuads_.back().blendMode = phoenix::backends::directx11::TexturedQuadBlendMode::Screen;
                        if (frame.wrapperType != 6U) objectRects_.emplace_back(item.object->objectId,
                            RECT{ point.x, point.y, point.x + static_cast<LONG>(width), point.y + static_cast<LONG>(height) });
                    }
                };

                if (showTiles_) for (const auto tileIndex : visibleTileIndices_)
                {
                    const auto& tile = lastDrawList_->tiles[tileIndex];
                    const auto texture = ResolveTexture(true, 0U, tile.spriteId);
                    if (!texture.handle.IsValid()) continue;
                    const auto scaled = WorldSpriteRectToScreen(tile.anchor, texture.width, texture.height);
                    frameQuads_.push_back({ texture, { scaled.x, scaled.y },
                        static_cast<std::uint32_t>(scaled.width), static_cast<std::uint32_t>(scaled.height) });
                }
                (void)renderer_.DrawTexturedQuads(frameQuads_);
                frameQuads_.clear();
                for (const auto layerIndex : visibleRowLayers_)
                {
                    const auto& layer = lastDrawList_->layers[layerIndex];
                    if (layer.index < lastDrawList_->staticObjects.size())
                        queueObject(lastDrawList_->staticObjects[layer.index]);
                }
                (void)renderer_.DrawTexturedQuads(frameQuads_);

                DrawOverlays();
            }
            (void)renderer_.PresentFrame();
        }

        void InvalidateMinimapCache()
        {
            minimapCacheState_.Invalidate();
            InvalidateRect(minimap_, nullptr, FALSE);
        }

        void EnsureMinimapBitmap(HDC)
        {
            if (!map_ || !minimapCacheState_.NeedsRebuild(minimapRevision_)) return;
#if defined(_DEBUG)
            const auto minimapProfile = profiler_.Measure(ProfileArea::MinimapRendering);
#endif
            ReleaseMinimapBitmap();
            minimapBitmapWidth_ = std::max(1, std::min<int>(1024, map_->width));
            minimapBitmapHeight_ = std::max(1, std::min<int>(1024, map_->height));
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = minimapBitmapWidth_;
            info.bmiHeader.biHeight = -minimapBitmapHeight_;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            void* rawPixels = nullptr;
            minimapBitmap_ = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &rawPixels, nullptr, 0);
            if (minimapBitmap_ == nullptr || rawPixels == nullptr) return;
            minimapMemoryDc_ = CreateCompatibleDC(nullptr);
            if (minimapMemoryDc_ == nullptr)
            {
                DeleteObject(minimapBitmap_);
                minimapBitmap_ = nullptr;
                return;
            }
            minimapPreviousBitmap_ = SelectObject(minimapMemoryDc_, minimapBitmap_);
            auto* pixels = static_cast<DWORD*>(rawPixels);
            for (int outputY = 0; outputY < minimapBitmapHeight_; ++outputY)
            {
                const auto mapY = static_cast<std::uint16_t>(static_cast<std::uint64_t>(outputY) * map_->height /
                    static_cast<unsigned>(minimapBitmapHeight_));
                for (int outputX = 0; outputX < minimapBitmapWidth_; ++outputX)
                {
                    const auto mapX = static_cast<std::uint16_t>(static_cast<std::uint64_t>(outputX) * map_->width /
                        static_cast<unsigned>(minimapBitmapWidth_));
                    const auto* sector = map_->Sector(mapX, mapY);
                    DWORD color = RGB(26, 31, 28);
                    if (showTiles_ && sector != nullptr && sector->spriteId != 0xFFFFU && sector->spriteId != 0xFFFFFFFFU)
                    {
                        constexpr std::array<COLORREF, 4> terrainColors{
                            RGB(35, 58, 38), RGB(48, 76, 47), RGB(63, 92, 55), RGB(78, 105, 65) };
                        color = terrainColors[(sector->spriteId + sector->light) % terrainColors.size()];
                    }
                    pixels[static_cast<std::size_t>(outputY) * minimapBitmapWidth_ + outputX] = color;
                }
            }
            for (const auto& object : map_->imageObjects)
            {
                const bool animation = object.animation || object.wrapperType == 5U ||
                    object.wrapperType == 6U || object.wrapperType == 7U;
                if ((animation && !showAnimations_) || (!animation && !showImages_) || object.positions.empty()) continue;
                const auto x = std::min(minimapBitmapWidth_ - 1, static_cast<int>(
                    static_cast<std::uint64_t>(object.positions.front().first) * minimapBitmapWidth_ / map_->width));
                const auto y = std::min(minimapBitmapHeight_ - 1, static_cast<int>(
                    static_cast<std::uint64_t>(object.positions.front().second) * minimapBitmapHeight_ / map_->height));
                for (int dy = 0; dy < 2 && y + dy < minimapBitmapHeight_; ++dy)
                    for (int dx = 0; dx < 2 && x + dx < minimapBitmapWidth_; ++dx)
                        pixels[static_cast<std::size_t>(y + dy) * minimapBitmapWidth_ + x + dx] = RGB(220, 190, 90);
            }
            if (showPortals_ && map_->sidecars.sectors.size() ==
                static_cast<std::size_t>(map_->width) * map_->height)
            {
                for (std::uint16_t y = 0; y < map_->height; ++y)
                {
                    for (std::uint16_t x = 0; x < map_->width; ++x)
                    {
                        const auto* server = phoenix::world::LegacyServerSector(*map_, x, y);
                        if (server == nullptr || !server->portal) continue;
                        const auto px = std::min(minimapBitmapWidth_ - 1, static_cast<int>(
                            static_cast<std::uint64_t>(x) * minimapBitmapWidth_ / map_->width));
                        const auto py = std::min(minimapBitmapHeight_ - 1, static_cast<int>(
                            static_cast<std::uint64_t>(y) * minimapBitmapHeight_ / map_->height));
                        for (int dy = 0; dy < 2 && py + dy < minimapBitmapHeight_; ++dy)
                            for (int dx = 0; dx < 2 && px + dx < minimapBitmapWidth_; ++dx)
                                pixels[static_cast<std::size_t>(py + dy) * minimapBitmapWidth_ + px + dx] = RGB(80, 80, 255);
                    }
                }
            }
            minimapCacheState_.MarkRebuilt(minimapRevision_);
        }

        void ReleaseMinimapBitmap() noexcept
        {
            if (minimapMemoryDc_ != nullptr)
            {
                if (minimapPreviousBitmap_ != nullptr) SelectObject(minimapMemoryDc_, minimapPreviousBitmap_);
                DeleteDC(minimapMemoryDc_);
                minimapMemoryDc_ = nullptr;
                minimapPreviousBitmap_ = nullptr;
            }
            if (minimapBitmap_ != nullptr)
            {
                DeleteObject(minimapBitmap_);
                minimapBitmap_ = nullptr;
            }
        }

        void DrawOverlays()
        {
            if (!whiteTexture_.handle.IsValid() || !lastDrawList_) return;
            overlayQuads_.clear();
            auto line = [&](int x, int y, int width, int height, float opacity) {
                overlayQuads_.push_back({ whiteTexture_, { x, y }, static_cast<std::uint32_t>(std::max(1, width)),
                    static_cast<std::uint32_t>(std::max(1, height)), opacity });
            };
            if (showPortals_ && map_ && portalLabelTexture_.handle.IsValid())
            {
                const auto centerX = static_cast<int>(std::floor(cameraX_));
                const auto centerY = static_cast<int>(std::floor(cameraY_));
                const auto radius = phoenix::tools::map_editor::CalculateVisibleTileRadius(
                    viewport_.width, viewport_.height, zoom_);
                const int minX = std::max(0, centerX - radius);
                const int maxX = std::min(static_cast<int>(map_->width) - 1, centerX + radius);
                const int minY = std::max(0, centerY - radius);
                const int maxY = std::min(static_cast<int>(map_->height) - 1, centerY + radius);
                for (int y = minY; y <= maxY; ++y)
                {
                    for (int x = minX; x <= maxX; ++x)
                    {
                        const auto* server = phoenix::world::LegacyServerSector(
                            *map_, static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y));
                        if (server == nullptr || !server->portal) continue;
                        const auto point = WorldToScreen(phoenix::render::ScreenPoint{
                            static_cast<std::int32_t>(x) * 48,
                            static_cast<std::int32_t>(y) * 24 });
                        overlayQuads_.push_back({ portalLabelTexture_, {
                            point.x + static_cast<int>(24.0F * zoom_) - static_cast<int>(portalLabelTexture_.width / 2U),
                            point.y + static_cast<int>(12.0F * zoom_) - static_cast<int>(portalLabelTexture_.height / 2U) },
                            portalLabelTexture_.width, portalLabelTexture_.height, 1.0F });
                    }
                }
            }
            if (showGrid_)
            {
                for (const auto tileIndex : visibleTileIndices_)
                {
                    const auto point = WorldToScreen(lastDrawList_->tiles[tileIndex].anchor);
                    const int w = std::max(1, static_cast<int>(48.0F * zoom_));
                    const int h = std::max(1, static_cast<int>(24.0F * zoom_));
                    line(point.x, point.y, w, 1, 0.16F);
                    line(point.x, point.y, 1, h, 0.16F);
                }
            }
            for (const auto& [id, rect] : objectRects_)
            {
                if (selectedIds_.count(id) == 0U) continue;
                line(rect.left, rect.top, rect.right - rect.left, 2, 0.9F);
                line(rect.left, rect.bottom - 2, rect.right - rect.left, 2, 0.9F);
                line(rect.left, rect.top, 2, rect.bottom - rect.top, 0.9F);
                line(rect.right - 2, rect.top, 2, rect.bottom - rect.top, 0.9F);
            }
            if (selectedArea_)
            {
                const auto first = WorldToScreen(phoenix::world::ProjectIsometric(selectedArea_->left, selectedArea_->top,
                    { 0, 0 }, lastDrawList_->camera.projection));
                const auto last = WorldToScreen(phoenix::world::ProjectIsometric(selectedArea_->right, selectedArea_->bottom,
                    { 0, 0 }, lastDrawList_->camera.projection));
                const int width = std::max(1, last.x - first.x + static_cast<int>(48 * zoom_));
                const int height = std::max(1, last.y - first.y + static_cast<int>(24 * zoom_));
                line(first.x, first.y, width, 2, 0.95F);
                line(first.x, first.y + height - 2, width, 2, 0.95F);
                line(first.x, first.y, 2, height, 0.95F);
                line(first.x + width - 2, first.y, 2, height, 0.95F);
            }
            else if (selectedTile_)
            {
                const auto anchor = WorldToScreen(phoenix::world::ProjectIsometric(selectedTile_->first, selectedTile_->second,
                    { 0, 0 }, lastDrawList_->camera.projection));
                const int width = std::max(1, static_cast<int>(48 * zoom_));
                const int height = std::max(1, static_cast<int>(24 * zoom_));
                line(anchor.x, anchor.y, width, 2, 0.95F);
                line(anchor.x, anchor.y + height - 2, width, 2, 0.95F);
                line(anchor.x, anchor.y, 2, height, 0.95F);
                line(anchor.x + width - 2, anchor.y, 2, height, 0.95F);
            }
            if (rectangleSelecting_)
            {
                const int left = std::min(selectionStart_.x, selectionCurrent_.x);
                const int top = std::min(selectionStart_.y, selectionCurrent_.y);
                const int width = std::max(1, std::abs(static_cast<int>(selectionCurrent_.x - selectionStart_.x)));
                const int height = std::max(1, std::abs(static_cast<int>(selectionCurrent_.y - selectionStart_.y)));
                line(left, top, width, 2, 0.85F); line(left, top + height - 2, width, 2, 0.85F);
                line(left, top, 2, height, 0.85F); line(left + width - 2, top, 2, height, 0.85F);
            }
            (void)renderer_.DrawTexturedQuads(overlayQuads_);
        }

        void ReleaseTextures()
        {
            for (const auto& [_, texture] : tileTextures_) renderer_.ReleaseTexture(texture);
            for (const auto& [_, texture] : objectTextures_) renderer_.ReleaseTexture(texture);
            tileTextures_.clear(); objectTextures_.clear();
        }

        void UpdateTitle()
        {
            std::wstring title = L"DK Phoenix Map Editor";
            if (map_) title += L" - " + currentPath_.filename().wstring() + (map_->modified ? L" *" : L"");
            SetWindowTextW(window_, title.c_str());
        }

        void UpdateStatus()
        {
            const bool hasMap = map_.has_value();
            EnableWindow(saveButton_, hasMap);
            EnableWindow(undoButton_, hasMap && history_.CanUndo());
            EnableWindow(redoButton_, hasMap && history_.CanRedo());
            EnableMenuItem(GetMenu(window_), IdSave, MF_BYCOMMAND | (hasMap ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(GetMenu(window_), IdSaveAs, MF_BYCOMMAND | (hasMap ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(GetMenu(window_), IdUndo, MF_BYCOMMAND | (hasMap && history_.CanUndo() ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(GetMenu(window_), IdRedo, MF_BYCOMMAND | (hasMap && history_.CanRedo() ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(GetMenu(window_), IdCopy, MF_BYCOMMAND | (!selectedIds_.empty() ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(GetMenu(window_), IdPaste, MF_BYCOMMAND | (!clipboard_.empty() ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(GetMenu(window_), IdDelete, MF_BYCOMMAND |
                (hasMap && (!selectedIds_.empty() || selectedTile_) ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(GetMenu(window_), IdFillEdgePortal, MF_BYCOMMAND |
                (hasMap && (selectedArea_ || selectedTile_) ? MF_ENABLED : MF_GRAYED));
#if defined(_DEBUG)
            const int parts[] = { 230, 390, 570, 730, 900, 1080, -1 };
#else
            const int parts[] = { 230, 390, 570, 730, 900, -1 };
#endif
            SendMessageW(status_, SB_SETPARTS, static_cast<WPARAM>(std::size(parts)), reinterpret_cast<LPARAM>(parts));
            const wchar_t* tool = tool_ == ToolMode::Select ? L"Select" : tool_ == ToolMode::Tile ? L"Paint Tile" :
                tool_ == ToolMode::EraseTile ? L"Erase Tile" : tool_ == ToolMode::Image ? L"Place Object" :
                tool_ == ToolMode::Animation ? L"Place Animation" :
                tool_ == ToolMode::Portal ? L"Edit Portal (Enter fill, RClick clear)" : L"Move";
            std::wstring mapText = map_ ? ToWide(map_->name) + L"  " + std::to_wstring(map_->width) + L"x" + std::to_wstring(map_->height) : L"No map";
            std::wstring coordinate = hoverTile_ ? L"Tile " + std::to_wstring(hoverTile_->first) + L"," + std::to_wstring(hoverTile_->second) : L"Tile -,-";
            if (tool_ == ToolMode::Portal && selectedArea_)
            {
                const auto width = static_cast<unsigned>(selectedArea_->right - selectedArea_->left) + 1U;
                const auto height = static_cast<unsigned>(selectedArea_->bottom - selectedArea_->top) + 1U;
                coordinate += L"  " + std::to_wstring(width * height) + L" tiles — Fill edge portal (Enter)";
            }
            std::wstring world = hoverTile_ ? L"World " + std::to_wstring(hoverTile_->first * 48U) + L"," + std::to_wstring(hoverTile_->second * 24U) : L"World -,-";
            const std::wstring zoom = L"Zoom " + std::to_wstring(static_cast<int>(std::lround(zoom_ * 100.0F))) + L"%";
            std::wstring selection = L"Sprite " + std::to_wstring(selectedSpriteId_);
            if (!selectedIds_.empty()) selection += L" / Object " + std::to_wstring(*selectedIds_.begin()) +
                (selectedIds_.size() > 1U ? L" +" + std::to_wstring(selectedIds_.size() - 1U) : L"");
            else selection += L" / no object";
            const std::wstring state = std::wstring(tool) + (map_ && map_->modified ? L"  Modified" : L"  Saved");
            const std::wstring values[] = { mapText, coordinate, world, zoom, selection, state };
            for (int index = 0; index < static_cast<int>(std::size(values)); ++index)
                SendMessageW(status_, SB_SETTEXTW, index, reinterpret_cast<LPARAM>(values[index].c_str()));
        }

        static bool IsBooleanProperty(const PropertyKey key) noexcept
        {
            return key == PropertyKey::Animation || key == PropertyKey::Transparent || key == PropertyKey::Loop ||
                key == PropertyKey::BlockUnderground || key == PropertyKey::BlockGround || key == PropertyKey::BlockFlying ||
                key == PropertyKey::SafetyCommon || key == PropertyKey::SafetySlayer ||
                key == PropertyKey::SafetyVampire || key == PropertyKey::SafetyOusters || key == PropertyKey::PortalEnabled;
        }

        static bool IsComboProperty(const PropertyKey key) noexcept
        {
            return key == PropertyKey::WrapperType || key == PropertyKey::BltType || key == PropertyKey::Direction ||
                key == PropertyKey::PortalType;
        }

        bool IsEditingComboProperty() const noexcept
        {
            if (propertyEdit_ == nullptr || propertyEditRow_ < 0 ||
                static_cast<std::size_t>(propertyEditRow_) >= propertyRows_.size())
                return false;
            return IsComboProperty(propertyRows_[static_cast<std::size_t>(propertyEditRow_)].key);
        }

        void FillPropertyCombo(const PropertyKey key, const std::wstring& current)
        {
            const auto addItem = [&](const int value, const wchar_t* label) {
                const auto text = label != nullptr ? std::wstring(label) : std::to_wstring(value);
                const auto row = ComboBox_AddString(propertyEdit_, text.c_str());
                ComboBox_SetItemData(propertyEdit_, row, value);
                if (std::to_wstring(value) == current) ComboBox_SetCurSel(propertyEdit_, row);
            };

            if (key == PropertyKey::WrapperType)
            {
                for (int value = 3; value <= 7; ++value) addItem(value, nullptr);
            }
            else if (key == PropertyKey::BltType)
            {
                for (int value = 0; value <= 3; ++value) addItem(value, nullptr);
            }
            else if (key == PropertyKey::Direction)
            {
                for (int value = 0; value <= 7; ++value) addItem(value, nullptr);
            }
            else if (key == PropertyKey::PortalType)
            {
                // Help menüsündeki Legacy enum ile aynı etiketler (değer ItemData'da).
                static constexpr struct { int value; const wchar_t* label; } kPortalTypes[] = {
                    { 0, L"0 - PORTAL_NORMAL" },
                    { 1, L"1 - PORTAL_SLAYER" },
                    { 2, L"2 - PORTAL_VAMPIRE" },
                    { 3, L"3 - PORTAL_MULTI_TARGET" },
                    { 4, L"4 - PORTAL_PRIVATE" },
                    { 5, L"5 - PORTAL_GUILD" },
                    { 6, L"6 - PORTAL_BATTLE" },
                    { 7, L"7 - PORTAL_OUSTERS" },
                    { 200, L"200 - Client-only (.mip)" },
                    { 201, L"201 - Client-only (.mip)" },
                    { 202, L"202 - Client-only (.mip)" },
                    { 203, L"203 - Client-only (.mip)" },
                    { 255, L"255 - Client-only" },
                };
                for (const auto& entry : kPortalTypes)
                    addItem(entry.value, entry.label);
            }
            if (ComboBox_GetCurSel(propertyEdit_) == CB_ERR)
                ComboBox_SetCurSel(propertyEdit_, 0);
        }

        bool ShouldDeferPropertyKillFocus(HWND newFocus) const
        {
            if (newFocus == nullptr)
                return false;
            if (newFocus == propertySpin_ || newFocus == propertyEdit_)
                return true;
            if (propertyEdit_ != nullptr && IsChild(propertyEdit_, newFocus))
                return true;
            if (propertySpin_ != nullptr && IsChild(propertySpin_, newFocus))
                return true;

            // CBS_DROPDOWNLIST listesi WS_POPUP'tir; IsChild false doner. Commit edip
            // DestroyWindow yapmak secimi/checkbox etkileşimini bozar ve AV riski yaratir.
            if (propertyEdit_ != nullptr)
            {
                COMBOBOXINFO info{};
                info.cbSize = sizeof(info);
                if (GetComboBoxInfo(propertyEdit_, &info))
                {
                    if (newFocus == info.hwndList || newFocus == info.hwndItem)
                        return true;
                    if (info.hwndList != nullptr && IsChild(info.hwndList, newFocus))
                        return true;
                }
            }
            return false;
        }

        std::wstring ReadPropertyEditValue() const
        {
            if (propertyEdit_ == nullptr)
                return {};

            const auto row = propertyEditRow_;
            if (row < 0 || static_cast<std::size_t>(row) >= propertyRows_.size())
                return {};

            const auto key = propertyRows_[static_cast<std::size_t>(row)].key;
            if (IsBooleanProperty(key))
                return Button_GetCheck(propertyEdit_) == BST_CHECKED ? L"1" : L"0";

            if (IsComboProperty(key))
            {
                const auto selected = ComboBox_GetCurSel(propertyEdit_);
                if (selected == CB_ERR)
                    return {};
                // Etiketli satırlarda (Portal Type) gerçek değer ItemData'da.
                const auto data = ComboBox_GetItemData(propertyEdit_, selected);
                return std::to_wstring(static_cast<int>(data));
            }

            const auto length = GetWindowTextLengthW(propertyEdit_);
            std::wstring value(static_cast<std::size_t>(length) + 1U, L'\0');
            if (length > 0)
                GetWindowTextW(propertyEdit_, value.data(), length + 1);
            value.resize(static_cast<std::size_t>(length));
            return value;
        }

        void BeginPropertyEdit(const int row)
        {
            if (row < 0 || static_cast<std::size_t>(row) >= propertyRows_.size() ||
                propertyRows_[static_cast<std::size_t>(row)].key == PropertyKey::ReadOnly)
                return;
            CancelPropertyEdit();
            RECT rect{};
            if (!ListView_GetSubItemRect(properties_, row, 1, LVIR_BOUNDS, &rect)) return;
            MapWindowPoints(properties_, window_, reinterpret_cast<POINT*>(&rect), 2);
            propertyEditRow_ = row;
            const auto key = propertyRows_[static_cast<std::size_t>(row)].key;
            const auto& value = propertyRows_[static_cast<std::size_t>(row)].value;
            const wchar_t* className = IsBooleanProperty(key) ? L"BUTTON" : IsComboProperty(key) ? L"COMBOBOX" : L"EDIT";
            DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
            if (IsBooleanProperty(key)) style |= BS_AUTOCHECKBOX;
            else if (IsComboProperty(key)) style |= CBS_DROPDOWNLIST | WS_VSCROLL;
            else style |= WS_BORDER | ES_AUTOHSCROLL;
            propertyEdit_ = CreateWindowExW(0, className, IsBooleanProperty(key) ? L"Enabled" : value.c_str(), style,
                rect.left, rect.top, rect.right - rect.left, IsComboProperty(key) ? 180 : rect.bottom - rect.top,
                window_, reinterpret_cast<HMENU>(IdPropertyEdit), instance_, nullptr);
            if (propertyEdit_ == nullptr) return;
            // ListView uzerinde kalsin; aksi halde tik ListView'e gider, dropdown acilmaz.
            SetWindowPos(propertyEdit_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            if (IsBooleanProperty(key)) Button_SetCheck(propertyEdit_, value == L"1" ? BST_CHECKED : BST_UNCHECKED);
            else if (IsComboProperty(key)) FillPropertyCombo(key, value);
            else if (key != PropertyKey::MapName && key != PropertyKey::Description && key != PropertyKey::PortalTargets)
            {
                propertySpin_ = CreateWindowExW(0, UPDOWN_CLASSW, L"", WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT,
                    rect.right - 20, rect.top, 20, rect.bottom - rect.top, window_, reinterpret_cast<HMENU>(IdPropertySpin), instance_, nullptr);
                SendMessageW(propertySpin_, UDM_SETBUDDY, reinterpret_cast<WPARAM>(propertyEdit_), 0);
                SendMessageW(propertySpin_, UDM_SETRANGE32, static_cast<WPARAM>(INT_MIN), static_cast<LPARAM>(INT_MAX));
                SetWindowSubclass(propertySpin_, PropertyEditProc, 2U, reinterpret_cast<DWORD_PTR>(this));
                SetWindowPos(propertySpin_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            }
            SetWindowSubclass(propertyEdit_, PropertyEditProc, 1U, reinterpret_cast<DWORD_PTR>(this));
            if (!IsBooleanProperty(key) && !IsComboProperty(key)) SendMessageW(propertyEdit_, EM_SETSEL, 0, -1);
            SetFocus(propertyEdit_);
            if (IsComboProperty(key))
                SendMessageW(propertyEdit_, CB_SHOWDROPDOWN, TRUE, 0);
        }

        void CancelPropertyEdit()
        {
            if (propertyEdit_ == nullptr) return;
            propertyEditClosing_ = true;
            RemoveWindowSubclass(propertyEdit_, PropertyEditProc, 1U);
            DestroyWindow(propertyEdit_);
            propertyEdit_ = nullptr;
            if (propertySpin_ != nullptr)
            {
                RemoveWindowSubclass(propertySpin_, PropertyEditProc, 2U);
                DestroyWindow(propertySpin_);
                propertySpin_ = nullptr;
            }
            propertyEditRow_ = -1;
            propertyEditClosing_ = false;
        }

        void CommitPropertyEdit()
        {
            if (propertyEdit_ == nullptr || propertyEditRow_ < 0) return;
            const auto row = propertyEditRow_;
            const auto key = propertyRows_[static_cast<std::size_t>(row)].key;
            const auto value = ReadPropertyEditValue();
            const auto positionIndex = propertyRows_[static_cast<std::size_t>(row)].positionIndex;
            CancelPropertyEdit();
            (void)ApplyProperty(key, value, positionIndex);
            SetFocus(properties_);
        }

        bool ApplyProperty(const PropertyKey key, const std::wstring& text,
            const std::optional<std::size_t> positionIndex = std::nullopt)
        {
            if (!map_ || key == PropertyKey::ReadOnly) return false;
            if (key == PropertyKey::TileSpriteId || key == PropertyKey::SpriteId)
            {
                wchar_t* end = nullptr;
                const auto parsed = wcstoul(text.c_str(), &end, 10);
                const bool allowEmptyTile = key == PropertyKey::TileSpriteId && parsed == 0xFFFFU;
                if (end == text.c_str() || *end != L'\0' || parsed > 0xFFFFU ||
                    (!allowEmptyTile && !ResourceExists(key == PropertyKey::TileSpriteId, static_cast<std::uint16_t>(parsed))))
                {
                    MessageBoxW(window_, L"Sprite ID does not exist in the active Legacy archive/index.",
                        L"Invalid property value", MB_ICONWARNING);
                    return false;
                }
            }
            phoenix::tools::map_editor::PropertyTarget target;
            target.tile = selectedTile_ ? selectedTile_ : hoverTile_;
            if (!selectedIds_.empty()) target.objectId = *selectedIds_.begin();
            target.positionIndex = positionIndex;
            const auto result = phoenix::tools::map_editor::LegacyMapPropertyEditor::Apply(
                *map_, history_, key, ToUtf8(text), target);
            if (!result.applied)
            {
                MessageBoxW(window_, ToWide(result.error).c_str(), L"Invalid property value", MB_ICONWARNING);
                return false;
            }
            const bool resourceValid = result.resourceSpriteId == 0xFFFFFFFFU ||
                (!result.tileResourceChanged && !result.objectResourceChanged) ||
                (result.tileResourceChanged && EnsureResource(true, result.resourceSpriteId)) ||
                (result.objectResourceChanged && EnsureResource(false, result.resourceSpriteId));
            if (!resourceValid)
            {
                (void)history_.Undo(*map_);
                UpdateTitle();
                UpdateProperties();
                MessageBoxW(window_, L"Sprite ID does not exist in the active Legacy archive/index.",
                    L"Invalid property value", MB_ICONWARNING);
                return false;
            }
            RebuildObjectIndex();
            const bool sidecarOnly = key == PropertyKey::BlockUnderground || key == PropertyKey::BlockGround ||
                key == PropertyKey::BlockFlying || key == PropertyKey::SafetyCommon || key == PropertyKey::SafetySlayer ||
                key == PropertyKey::SafetyVampire || key == PropertyKey::SafetyOusters ||
                key == PropertyKey::PortalEnabled || key == PropertyKey::PortalType || key == PropertyKey::PortalTargets;
            const bool renderingChanged = !sidecarOnly && key != PropertyKey::MapName && key != PropertyKey::ZoneId &&
                key != PropertyKey::ZoneGroupId && key != PropertyKey::ZoneType &&
                key != PropertyKey::ZoneLevel && key != PropertyKey::Description;
            if (renderingChanged) MarkMapContentChanged();
            if (result.tileResourceChanged || result.objectResourceChanged) PopulatePalettes();
            if (renderingChanged) InvalidateRect(minimap_, nullptr, FALSE);
            UpdateTitle();
            UpdateProperties();
            return true;
        }

        void UpdateProperties()
        {
#if defined(_DEBUG)
            const auto propertyProfile = profiler_.Measure(ProfileArea::PropertyPanel);
            ++propertiesRefreshCount_;
#endif
            CancelPropertyEdit();
            propertyRows_.clear();
            const auto add = [&](std::wstring name, std::wstring value, const PropertyKey key = PropertyKey::ReadOnly,
                const std::optional<std::size_t> positionIndex = std::nullopt) {
                propertyRows_.push_back({ std::move(name), std::move(value), key, positionIndex });
            };
            if (!map_) add(L"Map", L"No map loaded");
            else
            {
                add(L"Map name", ToWide(map_->name), PropertyKey::MapName);
                add(L"Zone ID", std::to_wstring(map_->zoneId), PropertyKey::ZoneId);
                add(L"Zone group", std::to_wstring(map_->zoneGroupId), PropertyKey::ZoneGroupId);
                add(L"Zone type", std::to_wstring(map_->zoneType), PropertyKey::ZoneType);
                add(L"Zone level", std::to_wstring(map_->zoneLevel), PropertyKey::ZoneLevel);
                add(L"Description", ToWide(map_->description), PropertyKey::Description);
                add(L"Map size", std::to_wstring(map_->width) + L" x " + std::to_wstring(map_->height));
                add(L"Object count", std::to_wstring(map_->imageObjects.size()));
                const auto tileSelection = selectedTile_ ? selectedTile_ : hoverTile_;
                if (tileSelection)
                {
                    const auto& tile = map_->sectors[static_cast<std::size_t>(tileSelection->second) * map_->width + tileSelection->first];
                    add(L"Tile coordinate", std::to_wstring(tileSelection->first) + L", " + std::to_wstring(tileSelection->second));
                    add(L"Tile sprite ID", std::to_wstring(tile.spriteId), PropertyKey::TileSpriteId);
                    add(L"Tile property", std::to_wstring(tile.property), PropertyKey::TileProperty);
                    add(L"Tile light", std::to_wstring(tile.light), PropertyKey::TileLight);
                    add(L"Block underground", (tile.property & phoenix::world::kLegacyBlockUnderground) ? L"1" : L"0",
                        PropertyKey::BlockUnderground);
                    add(L"Block ground", (tile.property & phoenix::world::kLegacyBlockGround) ? L"1" : L"0",
                        PropertyKey::BlockGround);
                    add(L"Block flying", (tile.property & phoenix::world::kLegacyBlockFlying) ? L"1" : L"0",
                        PropertyKey::BlockFlying);
                    if (const auto* server = phoenix::world::LegacyServerSector(*map_, tileSelection->first, tileSelection->second))
                    {
                        add(L"Safety common", (server->safetyFlags & phoenix::world::kLegacySafetyCommon) ? L"1" : L"0",
                            PropertyKey::SafetyCommon);
                        add(L"Safety slayer", (server->safetyFlags & phoenix::world::kLegacySafetySlayer) ? L"1" : L"0",
                            PropertyKey::SafetySlayer);
                        add(L"Safety vampire", (server->safetyFlags & phoenix::world::kLegacySafetyVampire) ? L"1" : L"0",
                            PropertyKey::SafetyVampire);
                        add(L"Safety ousters", (server->safetyFlags & phoenix::world::kLegacySafetyOusters) ? L"1" : L"0",
                            PropertyKey::SafetyOusters);
                        add(L"Portal enabled", server->portal ? L"1" : L"0", PropertyKey::PortalEnabled);
                        if (server->portal)
                        {
                            add(L"Portal type", std::to_wstring(server->portal->type), PropertyKey::PortalType);
                            std::wstring targets;
                            for (const auto& target : server->portal->targets)
                            {
                                if (!targets.empty()) targets += L";";
                                targets += std::to_wstring(target.zoneId) + L":";
                                // destinationKnown=false ise "?(no .smp)" göster — kullanıcı
                                // neden X/Y'nin eksik olduğunu anlasın. .smp yüklenmemişse veya
                                // .mip/.smp arasında sürüm uyumsuzluğu varsa bu durum normal.
                                targets += target.destinationKnown ? std::to_wstring(target.x) : L"?(no .smp)";
                                targets += L":";
                                targets += target.destinationKnown ? std::to_wstring(target.y) : L"?(no .smp)";
                            }
                            add(L"Portal targets", std::move(targets), PropertyKey::PortalTargets);
                        }
                        else
                        {
                            add(L"Portal type", L"0", PropertyKey::PortalType);
                            add(L"Portal targets", L"", PropertyKey::PortalTargets);
                        }
                    }
                }
                if (!selectedIds_.empty())
                {
                    const auto id = *selectedIds_.begin();
                    const auto found = std::find_if(map_->imageObjects.begin(), map_->imageObjects.end(), [id](const auto& o) { return o.objectId == id; });
                    if (found != map_->imageObjects.end())
                    {
                        const auto& o = *found;
                        add(L"Selected objects", std::to_wstring(selectedIds_.size()));
                        add(L"Object ID", std::to_wstring(o.objectId));
                        add(L"Image object ID", std::to_wstring(o.imageObjectId));
                        add(L"Object type", std::to_wstring(o.objectType), PropertyKey::ObjectType);
                        add(L"Wrapper type", std::to_wstring(o.wrapperType), PropertyKey::WrapperType);
                        add(L"Sprite ID", std::to_wstring(o.spriteId), PropertyKey::SpriteId);
                        add(L"Wall direction", std::to_wstring(o.x), PropertyKey::WallDirection);
                        add(L"Wall value", std::to_wstring(o.y), PropertyKey::WallValue);
                        add(L"Pixel X", std::to_wstring(o.pixelX), PropertyKey::PixelX); add(L"Pixel Y", std::to_wstring(o.pixelY), PropertyKey::PixelY);
                        for (std::size_t positionIndex = 0; positionIndex < o.positions.size(); ++positionIndex)
                        {
                            add(L"Sector " + std::to_wstring(positionIndex) + L" X",
                                std::to_wstring(o.positions[positionIndex].first), PropertyKey::PositionX, positionIndex);
                            add(L"Sector " + std::to_wstring(positionIndex) + L" Y",
                                std::to_wstring(o.positions[positionIndex].second), PropertyKey::PositionY, positionIndex);
                        }
                        add(L"Viewpoint", std::to_wstring(o.viewpoint), PropertyKey::Viewpoint);
                        add(L"Animation", o.animation ? L"1" : L"0", PropertyKey::Animation);
                        add(L"Transparent", o.transparent ? L"1" : L"0", PropertyKey::Transparent);
                        if (o.wrapperType == 5U || o.wrapperType == 6U || o.wrapperType == 7U)
                        {
                            add(L"Frame ID", std::to_wstring(o.frameId), PropertyKey::FrameId);
                            add(L"Max frame", std::to_wstring(o.maxFrame), PropertyKey::MaxFrame);
                            add(L"Blt type", std::to_wstring(o.bltType), PropertyKey::BltType);
                            add(L"Direction", std::to_wstring(o.direction), PropertyKey::Direction);
                            add(L"Sound frame", std::to_wstring(o.soundFrame), PropertyKey::SoundFrame);
                            add(L"Sound ID", std::to_wstring(o.soundId), PropertyKey::SoundId);
                            add(L"Loop", o.loop ? L"1" : L"0", PropertyKey::Loop);
                            add(L"Min delay", std::to_wstring(o.minDelay), PropertyKey::MinDelay);
                            add(L"Max delay", std::to_wstring(o.maxDelay), PropertyKey::MaxDelay);
                            add(L"Start hour", std::to_wstring(o.startHour), PropertyKey::StartHour);
                            add(L"End hour", std::to_wstring(o.endHour), PropertyKey::EndHour);
                            if (o.wrapperType == 7U)
                                add(L"Interaction type", std::to_wstring(o.interactionType), PropertyKey::InteractionType);
                        }
                        add(L"Position records", std::to_wstring(o.positions.size()));
                    }
                }
            }
            if (selectedSpriteMetadata_ != nullptr)
            {
                const auto& metadata = *selectedSpriteMetadata_;
                const auto typeName = selectedSpriteType_ == phoenix::tools::map_editor::SpriteResourceType::Tile ? L"Tile" :
                    selectedSpriteType_ == phoenix::tools::map_editor::SpriteResourceType::Object ? L"Object" :
                    selectedSpriteType_ == phoenix::tools::map_editor::SpriteResourceType::Animation ? L"Animation" : L"Effect";
                add(L"--- Sprite ---", L"read-only");
                add(L"Sprite ID", std::to_wstring(selectedSpriteId_));
                add(L"Resource type", typeName);
                add(L"Source path", metadata.sourcePath.empty() ? L"n/a" : metadata.sourcePath.wstring());
                add(L"Split archive/base", metadata.sourcePath.empty() ? L"n/a" : std::to_wstring(metadata.splitBase));
                add(L"Internal index", metadata.sourcePath.empty() ? L"n/a" : std::to_wstring(metadata.internalIndex));
                add(L"Frame count", metadata.animation ? std::to_wstring(metadata.animation->frameCount) : L"1");
                add(L"Width / height", metadata.decoded ? std::to_wstring(metadata.width) + L" x " +
                    std::to_wstring(metadata.height) : L"n/a");
                add(L"Origin", L"n/a");
                add(L"cX", metadata.animation ? std::to_wstring(metadata.animation->cX) : L"n/a");
                add(L"cY", metadata.animation ? std::to_wstring(metadata.animation->cY) : L"n/a");
                add(L"Frame timing", metadata.animation ?
                    std::to_wstring(phoenix::world::LegacyMapAnimationTickMs) + L" ms/frame" : L"n/a");
                add(L"Sprite format", selectedSpriteType_ == phoenix::tools::map_editor::SpriteResourceType::Effect
                    ? L"Alpha indexed ASPK" : L"RGB565 SPK");
                add(L"Cache/decode", metadata.decoded ? L"decoded, thumbnail cached" : L"decode failed (negative cached)");
            }
            ListView_DeleteAllItems(properties_);
            for (std::size_t index = 0; index < propertyRows_.size(); ++index)
            {
                LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(index);
                item.pszText = propertyRows_[index].name.data();
                ListView_InsertItem(properties_, &item);
                ListView_SetItemText(properties_, static_cast<int>(index), 1, propertyRows_[index].value.data());
            }
            UpdateStatus();
        }

        HINSTANCE instance_{};
        HWND window_{};
        UINT dpi_{ 96U };
        HFONT uiFont_{};
        HWND newButton_{};
        HWND openButton_{};
        HWND saveButton_{};
        HWND undoButton_{};
        HWND redoButton_{};
        HWND selectButton_{};
        HWND tileButton_{};
        HWND eraseButton_{};
        HWND imageButton_{};
        HWND animationButton_{};
        HWND portalButton_{};
        HWND moveButton_{};
        HWND toolbarLayerLabel_{};
        HWND zoomOutButton_{};
        HWND zoomInButton_{};
        HWND fitMapButton_{};
        HWND zoomTrack_{};
        HWND leftDockLabel_{};
        HWND paletteTabs_{};
        HWND paletteContentLabel_{};
        HWND tilePalette_{};
        HWND objectPalette_{};
        HWND animationPalette_{};
        HWND layerTiles_{};
        HWND layerImages_{};
        HWND layerAnimations_{};
        HWND layerPortals_{};
        HWND spriteLabel_{};
        HWND browseSpkButton_{};
        HWND browseSplitButton_{};
        HWND browseAnimationButton_{};
        HWND spriteSearch_{};
        HWND spriteList_{};
        HWND spritePreview_{};
        HWND propertyLabel_{};
        HWND properties_{};
        HWND propertyEdit_{};
        HWND propertySpin_{};
        HWND presetLabel_{};
        HWND presetName_{};
        HWND presetSave_{};
        HWND presetList_{};
        HWND presetPreview_{};
        HWND presetRename_{};
        HWND presetDelete_{};
        HWND presetRefresh_{};
        HWND status_{};
        HWND minimap_{};
        HWND panelSplitter_{};
        HIMAGELIST spriteImages_{};
        int errorThumbnailIndex_{ -1 };
        phoenix::ui::UiRect viewport_{};
        int clientWidth_{ 1 };
        int clientHeight_{ 1 };
        phoenix::backends::directx11::DirectX11Renderer renderer_;
        phoenix::graphics::TextureDescriptor whiteTexture_{};
        phoenix::graphics::TextureDescriptor portalLabelTexture_{};
        std::optional<phoenix::world::LegacyMap> map_;
        std::filesystem::path currentPath_;
        std::filesystem::path tileArchive_;
        std::filesystem::path tileIndex_;
        phoenix::world::LegacyTileSpriteIndex tileLogicalIndex_;
        std::uint32_t physicalTileSpriteCount_{ 0U };
        std::filesystem::path objectArchive_;
        std::filesystem::path objectIndex_;
        std::filesystem::path effectAlphaArchive_;
        std::filesystem::path effectAlphaIndex_;
        std::filesystem::path effectAlphaPalette_;
        std::filesystem::path effectShadowArchive_;
        std::filesystem::path effectShadowIndex_;
        std::filesystem::path effectScreenArchive_;
        std::filesystem::path effectScreenIndex_;
        std::filesystem::path effectScreenPalette_;
        std::filesystem::path resourceRoot_;
        std::filesystem::path paletteResourceRoot_;
        SpriteSource spriteSource_;
        std::map<phoenix::tools::map_editor::SpriteResourceType, SpriteSource> spriteSourceCatalog_;
        phoenix::tools::map_editor::SpriteSourceSelectionState spriteSourceSelection_;
        bool spriteSourceLoaded_{ false };
        std::uint16_t tileSplitUnit_{ 0U };
        std::uint16_t objectSplitUnit_{ 0U };
        std::uint16_t effectAlphaSplitUnit_{ 0U };
        ToolMode tool_{ ToolMode::Select };
        std::uint16_t selectedSpriteId_{ 0 };
        phoenix::tools::map_editor::SpriteResourceType selectedSpriteType_{
            phoenix::tools::map_editor::SpriteResourceType::Tile };
        std::uint32_t tilePaletteCount_{ 0U };
        std::uint32_t objectPaletteCount_{ 0U };
        std::vector<std::uint16_t> animationPaletteIds_;
        std::vector<std::uint16_t> effectPaletteIds_;
        std::wstring tilePalettePlaceholder_{ L"Open a map to load tile sprites" };
        std::wstring objectPalettePlaceholder_{ L"Open a map to load Image Object sprites" };
        std::wstring animationPalettePlaceholder_{ L"Open a map to load animations" };
        bool tilePaletteAvailable_{ false };
        bool objectPaletteAvailable_{ false };
        bool animationPaletteAvailable_{ false };
        float cameraX_{ 0.0F };
        float cameraY_{ 0.0F };
        float zoom_{ 1.0F };
        bool showGrid_{ true };
        bool showTiles_{ true };
        bool showImages_{ true };
        bool showAnimations_{ true };
        bool showPortals_{ true };
        bool showSpriteBrowser_{ true };
        bool showPropertyPanel_{ true };
        bool showPresetEditor_{ false };
        bool showMinimap_{ true };
        bool splitterDragging_{ false };
        float panelSplitRatio_{ 0.42F };
        int spritePanelTop_{ 0 };
        int spritePanelAvailableHeight_{ 0 };
        bool panning_{ false };
        bool dragging_{ false };
        bool dragHistoryStarted_{ false };
        bool rectangleSelecting_{ false };
        bool rectanglePreserve_{ false };
        bool paintHistoryStarted_{ false };
        bool presetDragging_{ false };
        bool presetListUpdating_{ false };
        std::size_t presetDragIndex_{ 0U };
        std::array<bool, 4> movementKeys_{};
        std::array<bool, 4> selectionNudgeKeys_{};
        POINT lastMouse_{};
        POINT selectionStart_{};
        POINT selectionCurrent_{};
        std::optional<std::pair<std::uint16_t, std::uint16_t>> hoverTile_;
        std::optional<std::pair<std::uint16_t, std::uint16_t>> selectedTile_;
        std::optional<phoenix::tools::map_editor::MapPresetRect> selectedArea_;
        std::optional<std::pair<std::uint16_t, std::uint16_t>> lastPaintTile_;
        std::optional<std::pair<std::uint16_t, std::uint16_t>> dragAnchor_;
        const phoenix::world::WorldDrawList* lastDrawList_{ nullptr };
        phoenix::tools::map_editor::MapEditorDrawListCache drawListCache_;
        phoenix::tools::map_editor::RevisionCacheState minimapCacheState_;
        std::uint64_t contentRevision_{ 1U };
        std::uint64_t minimapRevision_{ 1U };
        std::uint64_t visibleRevision_{ 0U };
        int visibleCenterX_{ -1 };
        int visibleCenterY_{ -1 };
        int visibleWidth_{ -1 };
        int visibleHeight_{ -1 };
        int visibleZoomKey_{ -1 };
        std::vector<std::size_t> visibleTileIndices_;
        std::vector<std::size_t> visibleRowLayers_;
        std::set<std::uint32_t> selectedIds_;
        std::map<std::uint32_t, std::size_t> objectIndexById_;
        std::vector<phoenix::world::LegacyMapImageObject> clipboard_;
        std::vector<PresetCatalogEntry> presetCatalog_;
        std::optional<std::size_t> presetSelectedIndex_;
        std::optional<phoenix::tools::map_editor::MapPreset> presetDraft_;
        phoenix::tools::map_editor::MapEditHistory history_;
        std::map<std::uint32_t, phoenix::graphics::TextureDescriptor> tileTextures_;
        std::map<std::uint32_t, phoenix::graphics::TextureDescriptor> objectTextures_;
        std::map<std::wstring, std::optional<phoenix::graphics::ImageData>> decodedSprites_;
        std::vector<phoenix::backends::directx11::TexturedQuad> frameQuads_;
        std::vector<phoenix::backends::directx11::TexturedQuad> overlayQuads_;
        std::map<std::uint64_t, SpriteMetadata> spriteMetadata_;
        std::vector<std::uint16_t> spriteVisibleIds_;
        const SpriteMetadata* selectedSpriteMetadata_{ nullptr };
        std::vector<std::pair<std::uint32_t, RECT>> objectRects_;
        std::vector<PropertyRow> propertyRows_;
        int propertyEditRow_{ -1 };
        bool propertyEditClosing_{ false };
        phoenix::world::LegacyMapAnimationRenderer animationRenderer_;
        ULONGLONG animationStartMs_{ 0 };
        HBITMAP minimapBitmap_{};
        HDC minimapMemoryDc_{};
        HGDIOBJ minimapPreviousBitmap_{};
        int minimapBitmapWidth_{ 0 };
        int minimapBitmapHeight_{ 0 };
        HBITMAP presetPreviewBitmap_{};
        int presetPreviewWidth_{ 0 };
        int presetPreviewHeight_{ 0 };
        std::chrono::steady_clock::time_point lastRenderTick_{};
#if defined(_DEBUG)
        DebugProfiler profiler_;
        std::chrono::steady_clock::time_point lastProfileStatusTick_{};
        std::size_t profileStatusIndex_{ 0U };
        std::uint64_t splitFolderScans_{ 0U };
        std::uint64_t spriteDecodeCacheMisses_{ 0U };
        std::uint64_t thumbnailCreationMisses_{ 0U };
        std::uint64_t textureUploadMisses_{ 0U };
        std::uint64_t propertiesRefreshCount_{ 0U };
        std::uint64_t screenshotChunkCount_{ 0U };
        std::size_t screenshotPeakTemporaryBytes_{ 0U };
#endif
    };
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    MapEditorApp app;
    const auto result = app.Run(instance, show);
    CoUninitialize();
    return result;
}
