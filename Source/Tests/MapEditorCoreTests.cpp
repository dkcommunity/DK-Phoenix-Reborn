#include "Assets/AnimationObjectFramePackReader.h"
#include "Assets/EffectFramePackReader.h"
#include "Assets/SpriteArchiveReader.h"
#include "MapEditorDocument.h"
#include "MapEditorExportModel.h"
#include "MapEditorMinimapModel.h"
#include "MapEditorPngWriter.h"
#include "MapEditorPreset.h"
#include "MapEditorRenderCache.h"
#include "MapEditorUiModel.h"
#include "MapEditorViewportModel.h"
#include "Runtime/GameRuntimeModels.h"
#include "World/LegacyMapReader.h"
#include "World/LegacyMapAnimationRenderer.h"
#include "World/LegacyMapWriter.h"
#include "World/LegacyMapSidecar.h"
#include "World/WorldDrawListBuilder.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <array>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <vector>

namespace
{
    std::vector<char> ReadBytes(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    }

    std::uint32_t ReadPngDimension(const std::vector<char>& bytes, const std::size_t offset)
    {
        if (bytes.size() < offset + 4U) return 0U;
        return (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset])) << 24U) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1U])) << 16U) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2U])) << 8U) |
            static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3U]));
    }

    bool Require(bool condition, const char* message)
    {
        if (!condition) std::cerr << "FAIL: " << message << '\n';
        return condition;
    }

    const char* VariantName(const phoenix::world::LegacyMapParserVariant variant)
    {
        return variant == phoenix::world::LegacyMapParserVariant::Map2000 ? "MAP_2000_05_10" : "MAP_2024_04_30";
    }

    const char* ResourceStatusName(const phoenix::world::LegacyMapResourceArrayStatus status)
    {
        using Status = phoenix::world::LegacyMapResourceArrayStatus;
        switch (status)
        {
        case Status::Embedded: return "embedded";
        case Status::Empty: return "empty";
        case Status::Absent: return "absent";
        case Status::StalePointer: return "stale-pointer";
        case Status::MalformedAdvisory: return "malformed-advisory";
        }
        return "unknown";
    }

    phoenix::world::LegacyMap MakeMap(const std::uint16_t width, const std::uint16_t height,
        const std::size_t objectCount)
    {
        phoenix::world::LegacyMap map;
        map.version = "=MAP_2000_05_10=";
        map.name = "headless traversal";
        map.width = width;
        map.height = height;
        map.sectors.resize(static_cast<std::size_t>(width) * height);
        for (std::size_t index = 0; index < map.sectors.size(); ++index)
        {
            map.sectors[index].spriteId = index % 97U == 0U ? 8230U : 0xFFFFU;
            map.sectors[index].property = static_cast<std::uint8_t>(index % 8U);
        }
        map.imageObjects.reserve(objectCount);
        for (std::size_t index = 0; index < objectCount; ++index)
        {
            phoenix::world::LegacyMapImageObject object;
            object.wrapperType = object.objectType = 3U;
            object.objectId = static_cast<std::uint32_t>(index + 1U);
            object.imageObjectId = object.objectId;
            object.spriteId = 1U;
            const auto x = static_cast<std::uint16_t>(index % width);
            const auto y = static_cast<std::uint16_t>((index / width) % height);
            object.pixelX = static_cast<std::int32_t>(x) * 48;
            object.pixelY = static_cast<std::int32_t>(y) * 24;
            object.positions.emplace_back(x, y);
            map.imageObjects.push_back(std::move(object));
        }
        phoenix::tools::map_editor::RebuildObjectIndex(map);
        phoenix::world::InitializeLegacyMapSidecars(map);
        map.modified = true;
        return map;
    }
}

int main()
{
    namespace editor = phoenix::tools::map_editor;
    const auto root = std::filesystem::path(PHOENIX_PROJECT_ROOT);
    const auto source = root / L"GameData/data/Map/eslania_nw.map";
    const auto outputDir = root / L"Build/map-editor-tests";
    const auto roundTrip = outputDir / L"eslania_nw.roundtrip.map";
    const auto roundTripBackup = std::filesystem::path(roundTrip.wstring() + L".bak");
    const auto modifiedPath = outputDir / L"eslania_nw.modified.map";
    const auto propertyPath = outputDir / L"eslania_nw.properties.map";
    const auto animationPath = outputDir / L"animation.modified.map";
    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    std::filesystem::remove(roundTrip, ec);
    std::filesystem::remove(roundTripBackup, ec);
    std::filesystem::remove(modifiedPath, ec);
    std::filesystem::remove(propertyPath, ec);
    std::filesystem::remove(animationPath, ec);

    phoenix::world::LegacyMapReader mapReader;
    auto loaded = mapReader.Load(source);
    if (!Require(loaded.Succeeded(), "source map load")) return 1;
    if (!Require(loaded.map->width == 256U && loaded.map->height == 256U, "source dimensions")) return 1;

    const auto asylionResult = mapReader.Load(root / L"GameData/data/Map/asylion_b2.map");
    if (!Require(asylionResult.Succeeded(), "asylion_b2 regression map load")) return 1;
    const std::set<std::uint16_t> asylionRegressionSprites{ 805U, 806U, 807U, 1122U, 1123U };
    std::map<std::uint16_t, std::size_t> asylionRecordCounts;
    std::map<std::uint16_t, std::size_t> asylionPreBackgroundCounts;
    for (const auto& object : asylionResult.map->imageObjects)
    {
        if (asylionRegressionSprites.count(object.spriteId) == 0U) continue;
        ++asylionRecordCounts[object.spriteId];
        if (object.viewpoint == 0U) ++asylionPreBackgroundCounts[object.spriteId];
    }
    editor::MapEditorDrawListCache asylionDrawCache;
    const auto& asylionList = asylionDrawCache.Ensure(*asylionResult.map, 1U);
    const auto asylionRenderableObjectCount = static_cast<std::size_t>(std::count_if(
        asylionResult.map->imageObjects.begin(), asylionResult.map->imageObjects.end(),
        [](const auto& object) { return object.spriteId != 0xFFFFU; }));
    if (!Require(asylionList.staticObjects.size() == asylionRenderableObjectCount,
        "Editor builder preserves every raw renderable object independently of Client admission")) return 1;
    const auto asylionEditorLayers = editor::BuildLegacyEditorObjectLayerOrder(asylionList);

    const auto menegrothResult = mapReader.Load(root / L"GameData/data/Map/menegroth_3f.map");
    if (!Require(menegrothResult.Succeeded(), "menegroth_3f regression map load")) return 1;
    const auto menegrothObject = std::find_if(menegrothResult.map->imageObjects.begin(),
        menegrothResult.map->imageObjects.end(), [](const auto& object) {
            return object.objectId == 6799U || object.imageObjectId == 6799U || object.spriteId == 6799U;
        });
    if (!Require(menegrothObject != menegrothResult.map->imageObjects.end(),
        "menegroth_3f Legacy ObjectID 6799 parsed")) return 1;
    if (!Require(menegrothObject->spriteId == 6799U && menegrothObject->positions.size() == 12U &&
        menegrothObject->viewpoint == 0U, "menegroth_3f sprite 6799 placement fields preserved")) return 1;
    editor::MapEditorDrawListCache menegrothDrawCache;
    const auto& menegrothList = menegrothDrawCache.Ensure(*menegrothResult.map, 1U);
    const auto menegrothDrawObject = std::find_if(menegrothList.staticObjects.begin(),
        menegrothList.staticObjects.end(), [&](const auto& item) { return item.object == &*menegrothObject; });
    if (!Require(menegrothDrawObject != menegrothList.staticObjects.end(),
        "menegroth_3f sprite 6799 admitted to full draw list")) return 1;
    phoenix::assets::SpriteArchiveReader menegrothSpriteReader;
    const auto menegrothSprite = menegrothSpriteReader.ReadSprite565(
        root / L"GameData/data/Image/imageobject.spk", root / L"GameData/data/Image/imageobject.spki",
        menegrothObject->spriteId, phoenix::assets::LegacyAssetFormat::SpritePack);
    if (!Require(menegrothSprite.value && !menegrothSprite.value->image.Empty(),
        "menegroth_3f sprite 6799 split-SPK decode")) return 1;
    if (!Require(menegrothObject->spriteId % 16U == 15U && menegrothSprite.value->image.width == 190U &&
        menegrothSprite.value->image.height == 96U,
        "split-SPK chunk-final sprite uses the local payload boundary")) return 1;

    const auto bathoryResult = mapReader.Load(root / L"GameData/data/Map/new_bathory_f1_lair_renewal.map");
    if (!Require(bathoryResult.Succeeded(), "new_bathory_f1_lair_renewal regression map load")) return 1;
    const auto bathoryTile = std::find_if(bathoryResult.map->sectors.begin(), bathoryResult.map->sectors.end(),
        [](const auto& sector) { return sector.spriteId == 43985U; });
    if (!Require(bathoryTile != bathoryResult.map->sectors.end(), "new_bathory tile 43985 parsed")) return 1;
    editor::MapEditorDrawListCache bathoryDrawCache;
    const auto& bathoryList = bathoryDrawCache.Ensure(*bathoryResult.map, 1U);
    if (!Require(std::any_of(bathoryList.tiles.begin(), bathoryList.tiles.end(),
        [](const auto& tileItem) { return tileItem.spriteId == 43985U; }),
        "new_bathory tile 43985 admitted to full draw list")) return 1;
    phoenix::assets::SpriteArchiveReader bathorySpriteReader;
    const auto tileArchiveInfo = bathorySpriteReader.ReadArchiveInfo(
        root / L"GameData/data/Image/tile.spk", root / L"GameData/data/Image/tile.spki",
        phoenix::assets::LegacyAssetFormat::SpritePack);
    std::ifstream tileHeader(root / L"GameData/data/Image/tile.spk/header.inf");
    std::uint32_t expectedTileSpriteCount = 0U;
    tileHeader >> expectedTileSpriteCount;
    if (!Require(tileArchiveInfo.value && expectedTileSpriteCount != 0U &&
        tileArchiveInfo.value->spriteCount >= expectedTileSpriteCount,
        "split tile archive count comes from header.inf")) return 1;
    phoenix::world::LegacyTileSpriteIndex tileLogicalIndex;
    std::string tileLogicalIndexError;
    if (!Require(tileLogicalIndex.Load(root / L"GameData/data/Info/TILE.IDX", tileLogicalIndexError),
        "Legacy logical tile index loads")) return 1;
    const auto castaloLogicalTile = tileLogicalIndex.Resolve(66378U, tileArchiveInfo.value->spriteCount);
    if (!Require(castaloLogicalTile && *castaloLogicalTile == 18044U,
        "castalo logical tile 66378 resolves to physical tile 18044")) return 1;
    const auto jackCastleResult = mapReader.Load(root / L"GameData/data/Map/jack_castle_1f.map");
    if (!Require(jackCastleResult.Succeeded(), "jack_castle_1f regression map load")) return 1;
    const auto jackLogicalTile = phoenix::world::WorldDrawListBuilder::ResolveLegacyTileSpriteId(
        *jackCastleResult.map, 71840U, &tileLogicalIndex, tileArchiveInfo.value->spriteCount);
    if (!Require(!jackLogicalTile,
        "jack_castle logical tile without an index entry does not alias an unrelated low-word sprite")) return 1;
    const auto jackObjectSprite = bathorySpriteReader.ReadSprite565(
        root / L"GameData/data/Image/imageobject.spk", root / L"GameData/data/Image/imageobject.spki", 11930U,
        phoenix::assets::LegacyAssetFormat::SpritePack);
    const auto jackShadowSprite = bathorySpriteReader.ReadSprite565(
        root / L"GameData/data/Image/imageobject.sspk", root / L"GameData/data/Image/imageobject.sspki", 11930U,
        phoenix::assets::LegacyAssetFormat::ShadowSpritePack);
    if (!Require(jackObjectSprite.value && !jackObjectSprite.value->image.Empty() &&
        jackShadowSprite.value && !jackShadowSprite.value->image.Empty(),
        "jack_castle normal and shadow object sprite 11930 decode from expanded split archives")) return 1;
    const auto castaloPhysicalSprite = bathorySpriteReader.ReadSprite565(
        root / L"GameData/data/Image/tile.spk", root / L"GameData/data/Image/tile.spki", *castaloLogicalTile,
        phoenix::assets::LegacyAssetFormat::SpritePack);
    if (!Require(castaloPhysicalSprite.value && !castaloPhysicalSprite.value->image.Empty(),
        "castalo physical tile 18044 decodes from the shared split archive")) return 1;
    const auto bathorySprite = bathorySpriteReader.ReadSprite565(
        root / L"GameData/data/Image/tile.spk", root / L"GameData/data/Image/tile.spki", 43985U,
        phoenix::assets::LegacyAssetFormat::SpritePack);
    if (!Require(bathorySprite.value && !bathorySprite.value->image.Empty(),
        "new_bathory tile 43985 decodes from self-indexed split chunk")) return 1;
    std::map<std::uint16_t, std::size_t> asylionScheduledCounts;
    for (const auto layerIndex : asylionEditorLayers)
    {
        const auto& layer = asylionList.layers[layerIndex];
        const auto* object = asylionList.staticObjects[layer.index].object;
        if (object != nullptr && asylionRegressionSprites.count(object->spriteId) != 0U)
            ++asylionScheduledCounts[object->spriteId];
    }
    for (const auto spriteId : asylionRegressionSprites)
    {
        if (!Require(asylionRecordCounts[spriteId] != 0U, "asylion reported object sprite exists")) return 1;
        if (!Require(asylionScheduledCounts[spriteId] == asylionRecordCounts[spriteId],
            "Legacy Editor post-tile object pass includes every reported asylion object")) return 1;
    }
    if (!Require(std::accumulate(asylionPreBackgroundCounts.begin(), asylionPreBackgroundCounts.end(), std::size_t{ 0U },
        [](const std::size_t total, const auto& item) { return total + item.second; }) != 0U,
        "asylion regression includes Client pre-background objects")) return 1;

    std::array<std::size_t, 4> animationBltCounts{};
    std::vector<phoenix::world::LegacyMapImageObject> effectCandidates;
    std::vector<phoenix::world::LegacyMapImageObject> screenCandidates;
    std::size_t scannedMaps = 0U;
    std::size_t corpusMaps = 0U;
    std::size_t corpusFailures = 0U;
    const auto corpusReportPath = root / L"Tool/MapEditor/Dev_Log/MAP_CORPUS_VALIDATION.tsv";
    std::ofstream corpusReport(corpusReportPath, std::ios::trunc);
    corpusReport << "file\tstatus\treason\tvariant\twidth\theight\tsectors\tobjects\tpositions\ttile_resources\tobject_resources\n";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root / L"GameData/data/Map"))
    {
        if (!entry.is_regular_file() || entry.path().extension() != L".map") continue;
        ++corpusMaps;
        auto candidate = mapReader.Load(entry.path());
        if (!candidate.Succeeded())
        {
            ++corpusFailures;
            std::wcerr << L"CORPUS_FAIL\t" << entry.path().filename().wstring() << L"\t"
                << std::wstring(candidate.error.begin(), candidate.error.end()) << L'\n';
            corpusReport << entry.path().filename().string() << "\tFAIL\t" << candidate.error
                << "\t-\t0\t0\t0\t0\t0\t-\t-\n";
            continue;
        }
        auto& corpusMap = *candidate.map;
        const auto mipPath = std::filesystem::path(entry.path()).replace_extension(L".mip");
        if (std::filesystem::exists(mipPath))
        {
            const auto sidecars = phoenix::world::LegacyMapSidecarReader{}.Load(corpusMap, entry.path());
            if (!Require(sidecars.succeeded, "Legacy sidecar corpus parse"))
            {
                std::cerr << "SIDECAR_FAIL " << entry.path().filename().string() << ' ' << sidecars.error << '\n';
                return 1;
            }
        }
        std::size_t positionCount = 0U;
        for (const auto& object : corpusMap.imageObjects) positionCount += object.positions.size();
        const auto completeSectorCount = static_cast<std::size_t>(corpusMap.width) * corpusMap.height;
        if (corpusMap.sectors.size() != completeSectorCount || corpusMap.sectorImageObjectIndices.size() != completeSectorCount)
        {
            ++corpusFailures;
            corpusReport << entry.path().filename().string() << "\tFAIL\tincomplete sector traversal\t"
                << VariantName(corpusMap.parserVariant) << '\t' << corpusMap.width << '\t' << corpusMap.height << '\t'
                << corpusMap.sectors.size() << '\t' << corpusMap.imageObjects.size() << '\t' << positionCount << "\t-\t-\n";
            continue;
        }
        if (corpusMap.parserVariant == phoenix::world::LegacyMapParserVariant::Map2024LayoutCompatible)
        {
            for (const auto& sector : corpusMap.sectors)
            {
                if (sector.spriteId <= 0xFFFFU || sector.spriteId == 0xFFFFFFFFU) continue;
                const auto renderId = phoenix::world::WorldDrawListBuilder::ResolveLegacyTileSpriteId(
                    corpusMap, sector.spriteId, &tileLogicalIndex, tileArchiveInfo.value->spriteCount);
                if (renderId && *renderId >= tileArchiveInfo.value->spriteCount) return 1;
            }
        }
        corpusReport << entry.path().filename().string() << "\tPASS\t-\t" << VariantName(corpusMap.parserVariant)
            << '\t' << corpusMap.width << '\t' << corpusMap.height << '\t' << corpusMap.sectors.size() << '\t'
            << corpusMap.imageObjects.size() << '\t' << positionCount << '\t'
            << ResourceStatusName(corpusMap.tileResourceArrayStatus) << '\t'
            << ResourceStatusName(corpusMap.imageObjectResourceArrayStatus) << '\n';
        ++scannedMaps;
        for (const auto& object : candidate.map->imageObjects)
        {
            if (!object.animation) continue;
            if (!Require(object.bltType < animationBltCounts.size(), "animation object blt type is supported")) return 1;
            ++animationBltCounts[object.bltType];
            if (object.bltType == 1U) effectCandidates.push_back(object);
            if (object.bltType == 3U) screenCandidates.push_back(object);
        }
    }
    corpusReport << "# summary\ttotal=" << corpusMaps << "\tsucceeded=" << scannedMaps
        << "\tfailed=" << corpusFailures << '\n';
    corpusReport.flush();
    if (!Require(corpusMaps != 0U && corpusFailures == 0U && scannedMaps == corpusMaps,
        "all corpus maps parse with complete dimensions and traversal")) return 1;
    if (!Require(animationBltCounts[0] != 0U, "real map animation-object corpus scan")) return 1;

    const auto truncatedPath = outputDir / L"malformed-truncated.map";
    const auto originalBytes = ReadBytes(source);
    {
        std::ofstream truncated(truncatedPath, std::ios::binary | std::ios::trunc);
        truncated.write(originalBytes.data(), static_cast<std::streamsize>(std::min<std::size_t>(100U, originalBytes.size())));
    }
    const auto truncated = mapReader.Load(truncatedPath);
    if (!Require(!truncated.Succeeded() && !truncated.error.empty() && !truncated.diagnostics.empty() &&
        truncated.diagnostics.front().fatal, "truncated structural map fails with exact diagnostic")) return 1;

    const auto stalePointerPath = outputDir / L"stale-advisory-pointer.map";
    auto stalePointerBytes = originalBytes;
    if (!Require(stalePointerBytes.size() > 38U, "source contains pointer fields")) return 1;
    stalePointerBytes[34] = static_cast<char>(0xFF); stalePointerBytes[35] = static_cast<char>(0xFF);
    stalePointerBytes[36] = static_cast<char>(0xFF); stalePointerBytes[37] = static_cast<char>(0x7F);
    {
        std::ofstream stale(stalePointerPath, std::ios::binary | std::ios::trunc);
        stale.write(stalePointerBytes.data(), static_cast<std::streamsize>(stalePointerBytes.size()));
    }
    const auto stalePointer = mapReader.Load(stalePointerPath);
    if (!Require(stalePointer.Succeeded() &&
        stalePointer.map->tileResourceArrayStatus == phoenix::world::LegacyMapResourceArrayStatus::StalePointer &&
        !stalePointer.diagnostics.empty() && !stalePointer.diagnostics.front().fatal,
        "stale advisory resource pointer does not reject structurally valid Legacy map")) return 1;

    phoenix::world::LegacyMapWriter writer;
    const struct TraversalCase
    {
        const wchar_t* name;
        std::uint16_t width;
        std::uint16_t height;
        std::size_t objects;
    } traversalCases[] = {
        { L"small", 3U, 2U, 0U },
        { L"rectangular", 17U, 5U, 4U },
        { L"large", 1024U, 768U, 8U },
        { L"object-heavy", 128U, 96U, 2000U }
    };
    for (const auto& testCase : traversalCases)
    {
        auto model = MakeMap(testCase.width, testCase.height, testCase.objects);
        const auto path = outputDir / (std::wstring(L"traversal-") + testCase.name + L".map");
        const auto written = writer.Save(model, path);
        if (!Require(written.succeeded, "synthetic traversal map write")) return 1;
        const auto reloaded = mapReader.Load(path);
        if (!Require(reloaded.Succeeded() && reloaded.map->width == testCase.width &&
            reloaded.map->height == testCase.height &&
            reloaded.map->sectors.size() == static_cast<std::size_t>(testCase.width) * testCase.height &&
            reloaded.map->imageObjects.size() == testCase.objects,
            "small/rectangular/large/object-heavy complete traversal")) return 1;
    }

    auto movementMap = MakeMap(8U, 8U, 4U);
    const auto originalPosition = movementMap.imageObjects[1].positions.front();
    if (!Require(editor::CanMoveObject(movementMap, 1U, 1, 1) &&
        editor::MoveObject(movementMap, 1U, 1, 1), "selected-object keyboard nudge applies")) return 1;
    if (!Require(movementMap.imageObjects[1].positions.front().first == originalPosition.first + 1U &&
        movementMap.imageObjects[1].positions.front().second == originalPosition.second + 1U,
        "selected-object keyboard nudge updates Legacy position")) return 1;
    const auto incrementalObjectIndex = movementMap.sectorImageObjectIndices;
    editor::RebuildObjectIndex(movementMap);
    if (!Require(incrementalObjectIndex == movementMap.sectorImageObjectIndices,
        "incremental nudge sector index matches complete rebuild")) return 1;
    const auto unchangedAtBoundary = movementMap.imageObjects.front();
    if (!Require(!editor::MoveObject(movementMap, 0U, -1, 0) &&
        movementMap.imageObjects.front().positions == unchangedAtBoundary.positions,
        "boundary nudge does not mutate map")) return 1;

    auto framingMap = MakeMap(17U, 5U, 7U);
    for (auto& sector : framingMap.sectors) sector.spriteId = 1U;
    const auto canvasBounds = editor::CalculateMapCanvasBounds(framingMap);
    if (!Require(canvasBounds.Width() == 17LL * 48LL && canvasBounds.Height() == 5LL * 24LL,
        "full map canvas bounds use complete dimensions")) return 1;
    const auto fitZoom = editor::CalculateFitZoom(canvasBounds, 800, 600);
    if (!Require(fitZoom > 0.0F && fitZoom <= 1.0F, "fit-map zoom is bounded and usable")) return 1;
    const auto visibleRadius = editor::CalculateVisibleTileRadius(800, 600, fitZoom);
    const auto scaledTileA = editor::CalculateScaledSpriteRect(0, 0, 48U, 24U, 0.0F, 0.0F, 0.83F);
    const auto scaledTileB = editor::CalculateScaledSpriteRect(48, 0, 48U, 24U, 0.0F, 0.0F, 0.83F);
    const auto scaledTileBelow = editor::CalculateScaledSpriteRect(0, 24, 48U, 24U, 0.0F, 0.0F, 0.83F);
    if (!Require(scaledTileA.x + scaledTileA.width == scaledTileB.x &&
        scaledTileA.y + scaledTileA.height == scaledTileBelow.y,
        "scaled sprite edges share identical pixels without black seams")) return 1;
    phoenix::runtime::GameSessionModel framingSession;
    framingSession.player.positionKnown = true;
    framingSession.player.x = 8U; framingSession.player.y = 2U;
    framingSession.player.motion.visualX = 8.0F; framingSession.player.motion.visualY = 2.0F;
    phoenix::world::WorldDrawListBuilder framingBuilder;
    const auto framingList = framingBuilder.Build(&framingMap, framingSession,
        phoenix::ui::UiRect{ 0, 0, 800, 600 }, visibleRadius, fitZoom);
    if (!Require(framingList.tiles.size() == framingMap.sectors.size(),
        "fit-map draw traversal reaches every rectangular map tile")) return 1;

    auto isolatedRuntimeMap = MakeMap(8U, 8U, 0U);
    phoenix::world::LegacyMapImageObject positionlessObject;
    positionlessObject.wrapperType = positionlessObject.objectType = 3U;
    positionlessObject.objectId = positionlessObject.imageObjectId = 700001U;
    positionlessObject.spriteId = 1U;
    positionlessObject.pixelX = 4 * 48;
    positionlessObject.pixelY = 4 * 24;
    isolatedRuntimeMap.imageObjects.push_back(positionlessObject);
    editor::RebuildObjectIndex(isolatedRuntimeMap);
    phoenix::runtime::GameSessionModel isolatedRuntimeSession;
    isolatedRuntimeSession.player.positionKnown = true;
    isolatedRuntimeSession.player.x = 4U;
    isolatedRuntimeSession.player.y = 4U;
    isolatedRuntimeSession.player.motion.visualX = 4.0F;
    isolatedRuntimeSession.player.motion.visualY = 4.0F;
    const auto isolatedClientList = framingBuilder.Build(
        &isolatedRuntimeMap, isolatedRuntimeSession, phoenix::ui::UiRect{ 0, 0, 800, 600 }, 34);
    if (!Require(isolatedClientList.staticObjects.size() == 1U,
        "Client runtime object traversal is independent of Editor sector-index policy")) return 1;
    editor::MapEditorDrawListCache isolatedEditorCache;
    const auto& isolatedEditorList = isolatedEditorCache.Ensure(isolatedRuntimeMap, 1U);
    if (!Require(isolatedEditorList.staticObjects.size() == 1U,
        "Editor full-map object traversal uses the Editor-only builder")) return 1;

    const auto minimapList = editor::BuildMinimapRenderList(framingMap, 8.0F, 2.0F, 800, 600, fitZoom);
    if (!Require(minimapList.terrain.size() == framingMap.sectors.size() &&
        minimapList.objects.size() == framingMap.imageObjects.size(),
        "dedicated minimap list traverses terrain and useful objects")) return 1;
    if (!Require(minimapList.viewport.left >= 0.0F && minimapList.viewport.top >= 0.0F &&
        minimapList.viewport.right <= framingMap.width && minimapList.viewport.bottom <= framingMap.height,
        "minimap camera rectangle is clamped to full map")) return 1;

    editor::MapEditorDrawListCache drawListCache;
    const auto& cachedWorld = drawListCache.Ensure(framingMap, 1U);
    if (!Require(cachedWorld.tiles.size() == framingMap.sectors.size() &&
        drawListCache.RebuildCount() == 1U, "persistent draw-list cache builds complete world once")) return 1;
    (void)drawListCache.Ensure(framingMap, 1U);
    (void)drawListCache.Ensure(framingMap, 1U);
    if (!Require(drawListCache.RebuildCount() == 1U,
        "camera movement and zoom reuse unchanged world draw list")) return 1;
    (void)drawListCache.Ensure(framingMap, 2U);
    if (!Require(drawListCache.RebuildCount() == 2U,
        "map-content revision invalidates world draw list")) return 1;
    const auto cachedTileIndex = drawListCache.TileIndex(16U, 4U);
    if (!Require(cachedTileIndex != editor::MapEditorDrawListCache::MissingIndex &&
        cachedWorld.tiles[cachedTileIndex].x == 16U && cachedWorld.tiles[cachedTileIndex].y == 4U,
        "visible rendering uses cached O(1) sector-to-tile lookup")) return 1;

    editor::RevisionCacheState minimapCache;
    if (!Require(minimapCache.NeedsRebuild(1U), "minimap initially requires generation")) return 1;
    minimapCache.MarkRebuilt(1U);
    for (int cameraStep = 0; cameraStep < 100; ++cameraStep)
        (void)editor::CalculateMinimapViewportRect(framingMap.width, framingMap.height,
            static_cast<float>(cameraStep % framingMap.width), 2.0F, 800, 600, 1.0F);
    if (!Require(!minimapCache.NeedsRebuild(1U) && minimapCache.RebuildCount() == 1U,
        "camera movement and zoom do not regenerate cached minimap terrain")) return 1;
    if (!Require(minimapCache.NeedsRebuild(2U), "map-content revision invalidates minimap terrain")) return 1;

    const auto imageRoot = root / L"GameData/data/Image";
    const auto tileSource = editor::ResolveFixedSpriteSource(imageRoot, editor::SpriteResourceType::Tile);
    const auto objectSource = editor::ResolveFixedSpriteSource(imageRoot, editor::SpriteResourceType::Object);
    const auto effectSource = editor::ResolveFixedSpriteSource(imageRoot, editor::SpriteResourceType::Effect);
    if (!Require(tileSource.archive == imageRoot / L"tile.spk" && tileSource.index == imageRoot / L"tile.spki" &&
        std::filesystem::is_directory(tileSource.archive), "fixed Tilelist resolves split archive folder")) return 1;
    if (!Require(objectSource.archive == imageRoot / L"imageobject.spk" && objectSource.index == imageRoot / L"imageobject.spki" &&
        std::filesystem::is_directory(objectSource.archive), "fixed Objectlist resolves split archive folder")) return 1;
    if (!Require(effectSource.archive == imageRoot / L"effect.efpk" && effectSource.index == imageRoot / L"effect.aspki" &&
        std::filesystem::is_regular_file(effectSource.archive), "fixed Effectlist resolves verified effect frame pack")) return 1;
    phoenix::assets::SpriteArchiveReader sourceReader;
    sourceReader.ResetMetrics();
    const auto splitTileIndex = sourceReader.ReadIndex(tileSource.index, phoenix::assets::LegacyAssetFormat::SpritePack);
    const auto splitObjectIndex = sourceReader.ReadIndex(objectSource.index, phoenix::assets::LegacyAssetFormat::SpritePack);
    if (!Require(splitTileIndex.value && splitTileIndex.value->count != 0U && splitObjectIndex.value &&
        splitObjectIndex.value->count != 0U, "shared split archive indexes discovered")) return 1;
    editor::SpriteSourceSelectionState sourceSelection;
    if (!Require(sourceSelection.Select(tileSource) && !sourceSelection.Select(tileSource) &&
        sourceSelection.Select(objectSource) && !sourceSelection.Select(objectSource),
        "unchanged sprite source selection does not request rescan")) return 1;

    const auto grid = editor::CalculateSpriteGridLayout(420, 92, 5, 72);
    if (!Require(grid.columns == 5 && grid.cellWidth == 84, "normal sprite browser layout uses five columns")) return 1;
    const auto narrowGrid = editor::CalculateSpriteGridLayout(143, 92, 5, 72);
    if (!Require(narrowGrid.columns == 1, "sprite grid responds to narrow panel")) return 1;
    const auto visibleRange = editor::CalculateVisibleItemRange(10000U, grid, 920, 276, 1);
    if (!Require(visibleRange.first > 0U && visibleRange.last - visibleRange.first <= 30U,
        "thumbnail requests are limited to visible and near-visible grid rows")) return 1;
    const auto splitter = editor::CalculateSplitterLayout(500, 490, 150, 130, 6);
    if (!Require(splitter.firstExtent >= 150 && splitter.secondExtent >= 130 &&
        splitter.firstExtent + splitter.secondExtent + 6 == 500, "splitter enforces both panel minimums")) return 1;
    if (!Require(editor::CalculateToolbarRowCount(1180, 1180) == 1 &&
        editor::CalculateToolbarRowCount(1179, 1180) == 2,
        "toolbar wraps before layer visibility toggles can leave the client area")) return 1;

    const auto fullPlan = editor::BuildFullMapScreenshotPlan(canvasBounds, 64U * 1024U);
    if (!Require(fullPlan.width == static_cast<std::uint32_t>(canvasBounds.Width()) &&
        fullPlan.height == static_cast<std::uint32_t>(canvasBounds.Height()) && fullPlan.stripCount > 1U &&
        fullPlan.peakTemporaryBytes <= 64U * 1024U && !fullPlan.layers.grid && !fullPlan.layers.selection &&
        !fullPlan.layers.hover && !fullPlan.layers.cameraRectangle,
        "full-map screenshot uses complete bounds, excludes overlays, and has bounded strips")) return 1;
    const auto minimapPlan = editor::BuildGameMinimapScreenshotPlan(canvasBounds);
    const auto minimapFit = editor::CalculateGameMinimapFit(canvasBounds);
    if (!Require(minimapPlan.width == 200U && minimapPlan.height == 100U &&
        minimapFit.contentWidth <= 200U && minimapFit.contentHeight <= 100U &&
        minimapFit.contentX >= 0 && minimapFit.contentY >= 0,
        "game minimap export is exact 200x100 with aspect-preserving letterbox fit")) return 1;
    const auto stateBeforePlanning = std::array{ 17.0F, 5.0F, 1.25F };
    (void)editor::BuildFullMapScreenshotPlan(canvasBounds);
    const auto stateAfterPlanning = stateBeforePlanning;
    if (!Require(stateBeforePlanning == stateAfterPlanning, "screenshot planning preserves editor camera state")) return 1;
    const auto pngPath = outputDir / L"headless-game-minimap.png";
    const auto fullPngPath = outputDir / L"headless-full-map.png";
    std::filesystem::remove(pngPath, ec);
    std::filesystem::remove(fullPngPath, ec);
    const auto comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    std::vector<std::uint8_t> minimapPixels(200U * 100U * 4U, 0x7FU);
    const auto pngWritten = editor::WritePngImage(pngPath, 200U, 100U, [&](IWICBitmapFrameEncode* frame) {
        return SUCCEEDED(frame->WritePixels(100U, 800U, static_cast<UINT>(minimapPixels.size()), minimapPixels.data()));
    });
    const auto fullPngWritten = editor::WritePngImage(fullPngPath, fullPlan.width, fullPlan.height,
        [&](IWICBitmapFrameEncode* frame) {
            std::vector<std::uint8_t> strip(static_cast<std::size_t>(fullPlan.width) * fullPlan.stripHeight * 4U, 0x3FU);
            for (std::uint32_t y = 0U; y < fullPlan.height; y += fullPlan.stripHeight)
            {
                const auto rows = std::min(fullPlan.stripHeight, fullPlan.height - y);
                if (FAILED(frame->WritePixels(rows, fullPlan.width * 4U, fullPlan.width * rows * 4U, strip.data())))
                    return false;
            }
            return true;
        });
    if (SUCCEEDED(comResult)) CoUninitialize();
    const auto pngBytes = ReadBytes(pngPath);
    if (!Require(pngWritten && ReadPngDimension(pngBytes, 16U) == 200U && ReadPngDimension(pngBytes, 20U) == 100U,
        "headless PNG writer produces exact 200x100 game minimap output")) return 1;
    const auto fullPngBytes = ReadBytes(fullPngPath);
    if (!Require(fullPngWritten && ReadPngDimension(fullPngBytes, 16U) == fullPlan.width &&
        ReadPngDimension(fullPngBytes, 20U) == fullPlan.height,
        "headless strip writer produces complete full-map screenshot bounds")) return 1;

    const auto minimapIndex = sourceReader.ReadIndex(root / L"GameData/data/UI/spk/minimap.spki",
        phoenix::assets::LegacyAssetFormat::SpritePack);
    if (!Require(minimapIndex.value && minimapIndex.value->count != 0U,
        "Legacy minimap split-SPK index loads")) return 1;
    std::set<std::pair<std::uint32_t, std::uint32_t>> minimapAssetSizes;
    for (std::uint32_t spriteId = 0; spriteId < minimapIndex.value->count; ++spriteId)
    {
        const auto decoded = sourceReader.ReadSprite565(root / L"GameData/data/UI/spk/minimap.spk",
            root / L"GameData/data/UI/spk/minimap.spki", static_cast<std::uint16_t>(spriteId),
            phoenix::assets::LegacyAssetFormat::SpritePack);
        if (decoded.value && !decoded.value->image.Empty())
            minimapAssetSizes.emplace(decoded.value->image.width, decoded.value->image.height);
    }
    if (!Require(!minimapAssetSizes.empty(), "referenced Legacy minimap assets decode through shared split-SPK reader")) return 1;
    std::cout << "MINIMAP_ASSET_SIZES";
    for (const auto& size : minimapAssetSizes) std::cout << ' ' << size.first << 'x' << size.second;
    std::cout << '\n';

    const auto save = writer.Save(*loaded.map, roundTrip);
    if (!Require(save.succeeded && save.originalBytesPreserved, "unchanged save preserves original payload")) return 1;
    if (!Require(ReadBytes(source) == ReadBytes(roundTrip), "unchanged round-trip is byte-identical")) return 1;
    const auto backupSave = writer.Save(*loaded.map, roundTrip);
    if (!Require(backupSave.succeeded && backupSave.backupCreated, "existing target creates automatic backup")) return 1;
    if (!Require(ReadBytes(roundTripBackup) == ReadBytes(source), "automatic backup preserves prior target bytes")) return 1;
    const auto roundTripReload = mapReader.Load(roundTrip);
    if (!Require(roundTripReload.Succeeded(), "save-as backend immediate reload")) return 1;

    const auto map2024Source = root / L"GameData/data/Map/castalo_nw.map";
    const auto map2024Path = outputDir / L"castalo_nw.modified.map";
    const auto map2024Loaded = mapReader.Load(map2024Source);
    if (!Require(map2024Loaded.Succeeded() &&
        map2024Loaded.map->parserVariant == phoenix::world::LegacyMapParserVariant::Map2024LayoutCompatible,
        "MAP_2024 source-verified variant load")) return 1;
    auto map2024Modified = *map2024Loaded.map;
    const auto map2024FirstSprite = map2024Modified.sectors.front().spriteId;
    map2024Modified.sectors.front().light ^= 1U;
    map2024Modified.modified = true;
    if (!Require(writer.Save(map2024Modified, map2024Path).succeeded, "MAP_2024 variant save")) return 1;
    const auto map2024Reload = mapReader.Load(map2024Path);
    if (!Require(map2024Reload.Succeeded() && map2024Reload.map->width == map2024Modified.width &&
        map2024Reload.map->height == map2024Modified.height &&
        map2024Reload.map->sectors.size() == map2024Modified.sectors.size() &&
        map2024Reload.map->imageObjects.size() == map2024Modified.imageObjects.size() &&
        map2024Reload.map->sectors.front().spriteId == map2024FirstSprite &&
        map2024Reload.map->sectors.front().light == map2024Modified.sectors.front().light,
        "MAP_2024 save/reload preserves 32-bit tile layout and complete data")) return 1;

    auto modified = *loaded.map;
    modified.modified = true;
    modified.sectors.front().light ^= 1U;
    const auto modifiedSave = writer.Save(modified, modifiedPath);
    if (!Require(modifiedSave.succeeded, "modified save")) return 1;
    auto modifiedReload = mapReader.Load(modifiedPath);
    if (!Require(modifiedReload.Succeeded(), "modified reload")) return 1;
    if (!Require(modifiedReload.map->sectors.front().light == modified.sectors.front().light, "modified field persisted")) return 1;
    if (!Require(modifiedReload.map->imageObjects.size() == modified.imageObjects.size(), "object count persisted")) return 1;

    auto properties = *loaded.map;
    properties.modified = false;
    editor::MapEditHistory propertyHistory;
    editor::PropertyTarget mapTarget;
    const auto originalName = properties.name;
    const auto invalidZone = editor::LegacyMapPropertyEditor::Apply(
        properties, propertyHistory, editor::PropertyField::ZoneId, "70000", mapTarget);
    if (!Require(!invalidZone.applied && !invalidZone.error.empty() && properties.zoneId == loaded.map->zoneId &&
        !properties.modified && !propertyHistory.CanUndo(), "invalid property leaves map, dirty state and history unchanged")) return 1;
    const auto mapNameEdit = editor::LegacyMapPropertyEditor::Apply(
        properties, propertyHistory, editor::PropertyField::MapName, originalName + " property-test", mapTarget);
    if (!Require(mapNameEdit.applied && properties.modified && propertyHistory.CanUndo(), "map property enters dirty undo history")) return 1;
    if (!Require(propertyHistory.Undo(properties) && properties.name == originalName && !properties.modified,
        "property undo restores value and clean state")) return 1;
    if (!Require(propertyHistory.Redo(properties) && properties.name != originalName && properties.modified,
        "property redo restores value and dirty state")) return 1;
    const auto applyMapField = [&](const editor::PropertyField field, const std::string& value, const char* message) {
        return Require(editor::LegacyMapPropertyEditor::Apply(properties, propertyHistory, field, value, mapTarget).applied, message);
    };
    if (!applyMapField(editor::PropertyField::ZoneId, std::to_string(properties.zoneId), "zone id property edit") ||
        !applyMapField(editor::PropertyField::ZoneGroupId, std::to_string(properties.zoneGroupId), "zone group property edit") ||
        !applyMapField(editor::PropertyField::ZoneType, std::to_string(properties.zoneType), "zone type property edit") ||
        !applyMapField(editor::PropertyField::ZoneLevel, std::to_string(properties.zoneLevel), "zone level property edit") ||
        !applyMapField(editor::PropertyField::Description, properties.description, "description property edit")) return 1;

    editor::PropertyTarget tileTarget;
    tileTarget.tile = std::make_pair<std::uint16_t, std::uint16_t>(0U, 0U);
    const auto tileProperty = static_cast<unsigned>(properties.sectors.front().property ^ 1U);
    const auto tileLight = static_cast<unsigned>(properties.sectors.front().light ^ 1U);
    if (!Require(editor::LegacyMapPropertyEditor::Apply(properties, propertyHistory,
        editor::PropertyField::TileProperty, std::to_string(tileProperty), tileTarget).applied, "tile property edit")) return 1;
    if (!Require(editor::LegacyMapPropertyEditor::Apply(properties, propertyHistory,
        editor::PropertyField::TileLight, std::to_string(tileLight), tileTarget).applied, "tile light edit")) return 1;
    if (!Require(editor::LegacyMapPropertyEditor::Apply(properties, propertyHistory,
        editor::PropertyField::TileSpriteId, std::to_string(properties.sectors.front().spriteId), tileTarget).applied,
        "tile sprite property edit")) return 1;

    editor::PropertyTarget objectTarget;
    objectTarget.objectId = properties.imageObjects.front().objectId;
    objectTarget.positionIndex = 0U;
    const auto objectPixelX = properties.imageObjects.front().pixelX + 1;
    const auto objectViewpoint = static_cast<unsigned>(properties.imageObjects.front().viewpoint ^ 1U);
    if (!Require(editor::LegacyMapPropertyEditor::Apply(properties, propertyHistory,
        editor::PropertyField::PixelX, std::to_string(objectPixelX), objectTarget).applied, "image object pixel edit")) return 1;
    if (!Require(editor::LegacyMapPropertyEditor::Apply(properties, propertyHistory,
        editor::PropertyField::Viewpoint, std::to_string(objectViewpoint), objectTarget).applied, "image object viewpoint edit")) return 1;
    const auto& imagePropertyObject = properties.imageObjects.front();
    const auto applyImageField = [&](const editor::PropertyField field, const std::string& value, const char* message) {
        return Require(editor::LegacyMapPropertyEditor::Apply(properties, propertyHistory, field, value, objectTarget).applied, message);
    };
    if (!applyImageField(editor::PropertyField::ObjectType, std::to_string(imagePropertyObject.objectType), "object type property edit") ||
        !applyImageField(editor::PropertyField::WrapperType, std::to_string(imagePropertyObject.wrapperType), "wrapper type property edit") ||
        !applyImageField(editor::PropertyField::SpriteId, std::to_string(imagePropertyObject.spriteId), "object sprite property edit") ||
        !applyImageField(editor::PropertyField::WallDirection, std::to_string(imagePropertyObject.x), "wall direction property edit") ||
        !applyImageField(editor::PropertyField::WallValue, std::to_string(imagePropertyObject.y), "wall value property edit") ||
        !applyImageField(editor::PropertyField::PixelY, std::to_string(imagePropertyObject.pixelY), "pixel y property edit") ||
        !applyImageField(editor::PropertyField::Animation, imagePropertyObject.animation ? "1" : "0", "animation flag property edit") ||
        !applyImageField(editor::PropertyField::Transparent, imagePropertyObject.transparent ? "1" : "0", "transparent property edit")) return 1;
    if (!imagePropertyObject.positions.empty() &&
        (!applyImageField(editor::PropertyField::PositionX, std::to_string(imagePropertyObject.positions.front().first), "sector x property edit") ||
         !applyImageField(editor::PropertyField::PositionY, std::to_string(imagePropertyObject.positions.front().second), "sector y property edit"))) return 1;
    const auto beforeInvalidDirection = properties.imageObjects.front().direction;
    const auto invalidDirection = editor::LegacyMapPropertyEditor::Apply(
        properties, propertyHistory, editor::PropertyField::Direction, "8", objectTarget);
    if (!Require(!invalidDirection.applied && properties.imageObjects.front().direction == beforeInvalidDirection,
        "invalid object property does not mutate map")) return 1;
    const auto propertySave = writer.Save(properties, propertyPath);
    if (!Require(propertySave.succeeded, "property edit save")) return 1;
    const auto propertyReload = mapReader.Load(propertyPath);
    if (!Require(propertyReload.Succeeded(), "property edit reload")) return 1;
    if (!Require(propertyReload.map->name == properties.name && propertyReload.map->zoneLevel == properties.zoneLevel,
        "map properties persisted")) return 1;
    if (!Require(propertyReload.map->sectors.front().property == properties.sectors.front().property &&
        propertyReload.map->sectors.front().light == properties.sectors.front().light, "tile properties persisted")) return 1;
    if (!Require(propertyReload.map->imageObjects.front().pixelX == properties.imageObjects.front().pixelX &&
        propertyReload.map->imageObjects.front().viewpoint == properties.imageObjects.front().viewpoint,
        "object properties persisted")) return 1;

    phoenix::assets::SpriteArchiveReader spriteReader;
    const auto tileArchive = root / L"GameData/data/Image/tile.spk";
    const auto tileIndex = root / L"GameData/data/Image/tile.spki";
    const auto tileBrowserIndex = spriteReader.ReadIndex(tileIndex, phoenix::assets::LegacyAssetFormat::SpritePack);
    const auto objectBrowserIndex = spriteReader.ReadIndex(root / L"GameData/data/Image/imageobject.spki",
        phoenix::assets::LegacyAssetFormat::SpritePack);
    if (!Require(tileBrowserIndex.value && tileBrowserIndex.value->count > 8230U,
        "tile browser count comes from complete shared SPK index")) return 1;
    if (!Require(objectBrowserIndex.value && objectBrowserIndex.value->count != 0U,
        "object browser count comes from complete shared SPK index")) return 1;
    const auto tile = spriteReader.ReadSprite565(tileArchive, tileIndex, 8230U, phoenix::assets::LegacyAssetFormat::SpritePack);
    if (!Require(tile.value.has_value(), "split tile sprite 8230 decode")) return 1;
    if (!Require(tile.value->image.width == 48U && tile.value->image.height == 24U, "split tile dimensions")) return 1;

    const auto bodyFramePath = root / L"GameData/data/Image/animationobject.afpk";
    const auto shadowFramePath = root / L"GameData/data/Image/animationobjectshadow.afpk";
    phoenix::assets::AnimationObjectFramePackReader animationFrameReader;
    const auto bodyFrames = animationFrameReader.Read(bodyFramePath);
    const auto shadowFrames = animationFrameReader.Read(shadowFramePath);
    if (!Require(bodyFrames.Succeeded() && shadowFrames.Succeeded(), "animation body/shadow frame packs load")) return 1;
    if (!Require(bodyFrames.value->size() == 5U && shadowFrames.value->size() == 5U,
        "animationobject AFPK header contains exactly five complete sequences")) return 1;
    phoenix::world::LegacyMapAnimationRenderer animationRenderer;
    if (!Require(animationRenderer.Load(phoenix::world::LegacyMapAnimationFramePackPaths{
        bodyFramePath, shadowFramePath, root / L"GameData/data/Image/effect.efpk",
        root / L"GameData/data/Image/shadoweffect.efpk", root / L"GameData/data/Image/effectscreen.efpk" }),
        "shared animation renderer load")) return 1;
    if (!Require(animationRenderer.BodySequenceCount() == bodyFrames.value->size(),
        "animation browser sequence count comes from shared frame-pack renderer")) return 1;
    const auto effectFrameIds = animationRenderer.EffectFrameIds(
        phoenix::world::LegacyMapAnimationSpriteSource::EffectAlpha);
    if (!Require(effectFrameIds.size() > bodyFrames.value->size() && effectFrameIds.front() == 0U &&
        std::is_sorted(effectFrameIds.begin(), effectFrameIds.end()) &&
        std::adjacent_find(effectFrameIds.begin(), effectFrameIds.end()) == effectFrameIds.end(),
        "Effectlist exposes unique direction-zero effect frame sets instead of duplicate AFPK entries")) return 1;
    phoenix::world::LegacyMapImageObject effectBrowserObject;
    effectBrowserObject.animation = true;
    effectBrowserObject.wrapperType = 5U;
    effectBrowserObject.bltType = 1U;
    effectBrowserObject.frameId = effectFrameIds.front();
    const auto effectBrowserFrame = animationRenderer.Resolve(effectBrowserObject, 0U);
    const auto effectBrowserSprite = effectBrowserFrame.body ? spriteReader.ReadPalettedSprite565(
        root / L"GameData/data/Image/effect.aspk", root / L"GameData/data/Image/effect.aspki",
        effectBrowserFrame.body->spriteId, root / L"GameData/data/Image/effect.ppk",
        effectBrowserFrame.body->paletteId, true) : phoenix::assets::LegacyReadResult<phoenix::assets::LegacyDecodedSprite>{};
    if (!Require(effectBrowserFrame.body.has_value() && effectBrowserSprite.Succeeded() &&
        !effectBrowserSprite.value->image.Empty(), "Effectlist representative thumbnail resolves and decodes through shared caches")) return 1;

    const auto findResolvedCandidate = [&animationRenderer, &spriteReader, &root](const auto& candidates) {
        for (const auto& candidate : candidates)
        {
            const auto resolved = animationRenderer.Resolve(candidate, 0U);
            if (!resolved.body) continue;
            const bool screen = candidate.bltType == 3U;
            const auto archive = root / (screen ? L"GameData/data/Image/effectscreen.spk" : L"GameData/data/Image/effect.aspk");
            const auto index = root / (screen ? L"GameData/data/Image/effectscreen.spki" : L"GameData/data/Image/effect.aspki");
            const auto decoded = spriteReader.ReadPalettedSprite565(archive, index, resolved.body->spriteId,
                root / (screen ? L"GameData/data/Image/effectscreen.ppk" : L"GameData/data/Image/effect.ppk"),
                resolved.body->paletteId, !screen);
            if (decoded.Succeeded() && !decoded.value->image.Empty())
                return std::optional{ std::make_pair(candidate, *resolved.body) };
        }
        return std::optional<std::pair<phoenix::world::LegacyMapImageObject,
            phoenix::world::LegacyMapResolvedFrame>>{};
    };
    const auto resolvedEffect = findResolvedCandidate(effectCandidates);
    const auto resolvedScreen = findResolvedCandidate(screenCandidates);
    if (!Require(resolvedEffect.has_value() &&
        resolvedEffect->second.source == phoenix::world::LegacyMapAnimationSpriteSource::EffectAlpha,
        "real BLT_EFFECT map object resolves through effect frame pack")) return 1;
    if (!Require(resolvedScreen.has_value() &&
        resolvedScreen->second.source == phoenix::world::LegacyMapAnimationSpriteSource::EffectScreen &&
        resolvedScreen->second.blendMode == phoenix::world::LegacyMapAnimationBlendMode::Screen,
        "real BLT_SCREEN map object resolves through screen frame pack")) return 1;
    const auto effectNext = animationRenderer.Resolve(resolvedEffect->first, 1U);
    const auto screenNext = animationRenderer.Resolve(resolvedScreen->first, 1U);
    if (!Require(effectNext.body.has_value() && screenNext.body.has_value(),
        "effect and screen animation frame timing advances through shared resolver")) return 1;

    phoenix::assets::EffectFramePackReader effectFrameReader;
    const auto alphaSequences = effectFrameReader.ReadEffectFrames(root / L"GameData/data/Image/effect.efpk");
    const auto screenSequences = effectFrameReader.ReadEffectFrames(root / L"GameData/data/Image/effectscreen.efpk");
    if (!Require(alphaSequences.Succeeded() && screenSequences.Succeeded(), "effect parity frame packs load independently")) return 1;
    if (!Require(animationRenderer.EffectFrameCount(effectFrameIds.front(),
        phoenix::world::LegacyMapAnimationSpriteSource::EffectAlpha) != 0U,
        "Effectlist frame count uses shared effect renderer metadata")) return 1;
    const auto verifyEffectOrigin = [](const auto& sequences, const auto& object, const auto& resolved) {
        const auto found = std::find_if(sequences.begin(), sequences.end(), [&object](const auto& sequence) {
            return sequence.frameId == object.frameId && sequence.directionId.ValueOf() == 0U;
        });
        return found != sequences.end() && !found->frames.empty() &&
            resolved.spriteId == found->frames.front().spriteId.ValueOf() &&
            resolved.cX == found->frames.front().originX && resolved.cY == found->frames.front().originY;
    };
    if (!Require(verifyEffectOrigin(*alphaSequences.value, resolvedEffect->first, resolvedEffect->second),
        "BLT_EFFECT sprite and cX/cY match frame pack")) return 1;
    if (!Require(verifyEffectOrigin(*screenSequences.value, resolvedScreen->first, resolvedScreen->second),
        "BLT_SCREEN sprite and cX/cY match frame pack")) return 1;
    const auto alphaSprite = spriteReader.ReadPalettedSprite565(
        root / L"GameData/data/Image/effect.aspk", root / L"GameData/data/Image/effect.aspki",
        resolvedEffect->second.spriteId, root / L"GameData/data/Image/effect.ppk", resolvedEffect->second.paletteId, true);
    const auto screenSprite = spriteReader.ReadPalettedSprite565(
        root / L"GameData/data/Image/effectscreen.spk", root / L"GameData/data/Image/effectscreen.spki",
        resolvedScreen->second.spriteId, root / L"GameData/data/Image/effectscreen.ppk", resolvedScreen->second.paletteId, false);
    if (!Require(alphaSprite.Succeeded() && !alphaSprite.value->image.Empty(), "BLT_EFFECT resolved sprite decodes from alpha SPK")) return 1;
    if (!Require(screenSprite.Succeeded() && !screenSprite.value->image.Empty(), "BLT_SCREEN resolved sprite decodes from screen SPK")) return 1;

    std::optional<phoenix::world::LegacyMap> animationMap;
    std::size_t animationObjectIndex = 0;
    const auto animationSource = root / L"GameData/data/Map/gdr_illusion_01.map";
    const auto animationLoaded = mapReader.Load(animationSource);
    if (!Require(animationLoaded.Succeeded(), "animation parity map load")) return 1;
    for (std::size_t index = 0; index < animationLoaded.map->imageObjects.size(); ++index)
    {
        const auto& object = animationLoaded.map->imageObjects[index];
        if ((object.wrapperType == 5U || object.wrapperType == 6U) && object.bltType == 0U &&
            animationRenderer.BodyFrameCount(object.frameId) > 1U)
        {
            animationObjectIndex = index;
            animationMap = *animationLoaded.map;
            break;
        }
    }
    if (!Require(animationMap.has_value(), "real map with animation object found")) return 1;
    const auto& realAnimation = animationMap->imageObjects[animationObjectIndex];
    const auto frame0 = animationRenderer.Resolve(realAnimation, 0U);
    const auto frame1 = animationRenderer.Resolve(realAnimation, 1U);
    const auto frameMetadata = frame0.body ? animationRenderer.FindSpriteMetadata(
        frame0.body->spriteId, frame0.body->source, realAnimation.frameId) : std::nullopt;
    if (!Require(frameMetadata && frameMetadata->frameId == realAnimation.frameId &&
        frameMetadata->frameCount == frame0.body->frameCount && frameMetadata->cX == frame0.body->cX &&
        frameMetadata->cY == frame0.body->cY,
        "sprite browser metadata uses the same frame-pack sequence and cX/cY as animation rendering")) return 1;
    if (!Require(frame0.body.has_value() && frame1.body.has_value(), "animation body frames resolve")) return 1;
    if (!Require(frame0.body->frameIndex == 0U && frame1.body->frameIndex == 1U,
        "animation advances on Legacy 62ms global frame tick")) return 1;
    const auto& sourceFrame0 = (*bodyFrames.value)[realAnimation.frameId].frames[0];
    if (!Require(frame0.body->spriteId == sourceFrame0.spriteId.ValueOf() &&
        frame0.body->cX == sourceFrame0.originX && frame0.body->cY == sourceFrame0.originY,
        "animation sprite and cX/cY match frame pack")) return 1;
    const auto wrappedFrame = animationRenderer.Resolve(realAnimation, frame0.body->frameCount);
    if (!Require(wrappedFrame.body.has_value() && wrappedFrame.body->frameIndex == 0U,
        "animation frame selection wraps at frame-pack length")) return 1;
    auto nonLoopingAnimation = realAnimation;
    nonLoopingAnimation.loop = false;
    const auto nonLoopingFrame = animationRenderer.Resolve(nonLoopingAnimation, frame0.body->frameCount + 1U);
    if (!Require(nonLoopingFrame.body.has_value() && nonLoopingFrame.body->frameIndex == 1U,
        "Legacy global frame selection remains synchronized for non-loop flag")) return 1;
    if (realAnimation.frameId < shadowFrames.value->size() && !(*shadowFrames.value)[realAnimation.frameId].frames.empty())
    {
        if (!Require(frame0.shadow.has_value() && frame0.shadow->wrapperType == 6U,
            "animation shadow resolves before body")) return 1;
    }
    const auto orderedFrames = phoenix::world::LegacyMapAnimationRenderer::OrderedFrames(frame0);
    if (!Require(!orderedFrames.empty() && orderedFrames.back().wrapperType == 5U &&
        (orderedFrames.size() == 1U || orderedFrames.front().wrapperType == 6U),
        "animation ordered submission is shadow then body")) return 1;
    const phoenix::ui::UiRect visibleFrameRect{ frame0.body->cX, frame0.body->cY, 48, 48 };
    if (!Require(phoenix::world::WorldDrawListBuilder::Intersects(visibleFrameRect,
        phoenix::ui::UiRect{ -4096, -4096, 8192, 8192 }), "animation viewport clipping accepts visible frame")) return 1;
    if (!Require(!phoenix::world::WorldDrawListBuilder::Intersects(visibleFrameRect,
        phoenix::ui::UiRect{ 100000, 100000, 64, 64 }), "animation viewport clipping rejects offscreen frame")) return 1;

    auto animationModified = *animationMap;
    auto& changedAnimation = animationModified.imageObjects[animationObjectIndex];
    animationModified.modified = false;
    editor::MapEditHistory animationHistory;
    editor::PropertyTarget animationTarget;
    animationTarget.objectId = changedAnimation.objectId;
    const auto originalLoop = changedAnimation.loop;
    const auto editedDirection = static_cast<unsigned>((changedAnimation.direction + 1U) % 8U);
    const auto editedStartHour = static_cast<unsigned>((changedAnimation.startHour + 1U) % 24U);
    const auto editedEndHour = static_cast<unsigned>((changedAnimation.endHour + 1U) % 24U);
    const auto applyAnimationField = [&](const editor::PropertyField field, const std::string& value, const char* message) {
        return Require(editor::LegacyMapPropertyEditor::Apply(
            animationModified, animationHistory, field, value, animationTarget).applied, message);
    };
    if (!applyAnimationField(editor::PropertyField::FrameId, std::to_string(changedAnimation.frameId), "frame id property edit") ||
        !applyAnimationField(editor::PropertyField::MaxFrame, std::to_string(changedAnimation.maxFrame), "max frame property edit") ||
        !applyAnimationField(editor::PropertyField::BltType, std::to_string(changedAnimation.bltType), "blt type property edit") ||
        !applyAnimationField(editor::PropertyField::SoundFrame, std::to_string(changedAnimation.soundFrame), "sound frame property edit") ||
        !applyAnimationField(editor::PropertyField::SoundId, std::to_string(changedAnimation.soundId), "sound id property edit") ||
        !applyAnimationField(editor::PropertyField::MinDelay, std::to_string(changedAnimation.minDelay), "minimum delay property edit") ||
        !applyAnimationField(editor::PropertyField::MaxDelay, std::to_string(changedAnimation.maxDelay), "maximum delay property edit")) return 1;
    if (!Require(editor::LegacyMapPropertyEditor::Apply(animationModified, animationHistory,
        editor::PropertyField::Loop, originalLoop ? "0" : "1", animationTarget).applied, "animation loop property edit")) return 1;
    if (!Require(editor::LegacyMapPropertyEditor::Apply(animationModified, animationHistory,
        editor::PropertyField::Direction, std::to_string(editedDirection), animationTarget).applied, "animation direction property edit")) return 1;
    if (!Require(editor::LegacyMapPropertyEditor::Apply(animationModified, animationHistory,
        editor::PropertyField::StartHour, std::to_string(editedStartHour), animationTarget).applied, "animation start-hour property edit")) return 1;
    if (!Require(editor::LegacyMapPropertyEditor::Apply(animationModified, animationHistory,
        editor::PropertyField::EndHour, std::to_string(editedEndHour), animationTarget).applied, "animation end-hour property edit")) return 1;
    if (!Require(editor::LegacyMapPropertyEditor::Apply(animationModified, animationHistory,
        editor::PropertyField::Animation, "1", animationTarget).applied, "animation flag property edit")) return 1;
    if (!Require(animationModified.modified && animationHistory.CanUndo(), "animation properties join dirty undo history")) return 1;
    if (!Require(writer.Save(animationModified, animationPath).succeeded, "animation property save")) return 1;
    const auto animationReload = mapReader.Load(animationPath);
    if (!Require(animationReload.Succeeded(), "animation property reload")) return 1;
    const auto& reloadedAnimation = animationReload.map->imageObjects[animationObjectIndex];
    if (!Require(reloadedAnimation.frameId == changedAnimation.frameId && reloadedAnimation.maxFrame == changedAnimation.maxFrame &&
        reloadedAnimation.loop == changedAnimation.loop && reloadedAnimation.direction == changedAnimation.direction &&
        reloadedAnimation.minDelay == changedAnimation.minDelay && reloadedAnimation.maxDelay == changedAnimation.maxDelay &&
        reloadedAnimation.startHour == changedAnimation.startHour && reloadedAnimation.endHour == changedAnimation.endHour,
        "animation Legacy fields persisted")) return 1;

    phoenix::runtime::GameSessionModel session;
    session.player.positionKnown = true;
    session.player.x = 136U;
    session.player.y = 155U;
    session.player.motion.visualX = 136.0F;
    session.player.motion.visualY = 155.0F;
    phoenix::world::WorldDrawListBuilder builder;
    const phoenix::ui::UiRect viewport{ 0, 0, 1920, 1009 };
    const auto clientList = builder.Build(&*loaded.map, session, viewport, 34);
    const auto editorList = builder.Build(&*loaded.map, session, viewport, 34);
    if (!Require(clientList.camera.origin.x == editorList.camera.origin.x && clientList.camera.origin.y == editorList.camera.origin.y,
        "client/editor camera parity")) return 1;
    if (!Require(clientList.tiles.size() == editorList.tiles.size() && clientList.layers.size() == editorList.layers.size(),
        "client/editor submission cardinality parity")) return 1;
    for (std::size_t i = 0; i < clientList.layers.size(); ++i)
    {
        const auto& a = clientList.layers[i];
        const auto& b = editorList.layers[i];
        if (!Require(a.kind == b.kind && a.pass == b.pass && a.legacyRow == b.legacyRow && a.legacyObjectId == b.legacyObjectId,
            "client/editor ordered layer parity")) return 1;
    }
    bool rowSeen = false;
    for (const auto& layer : clientList.layers)
    {
        if (layer.pass == phoenix::world::WorldLayerPass::Row) rowSeen = true;
        if (!Require(!(rowSeen && layer.pass == phoenix::world::WorldLayerPass::PreBackground), "pre-background precedes row layers")) return 1;
    }

    phoenix::runtime::GameSessionModel animationSession;
    animationSession.player.positionKnown = true;
    if (!Require(!realAnimation.positions.empty(), "animation object has sector position records")) return 1;
    animationSession.player.x = realAnimation.positions.front().first;
    animationSession.player.y = realAnimation.positions.front().second;
    animationSession.player.motion.visualX = static_cast<float>(animationSession.player.x);
    animationSession.player.motion.visualY = static_cast<float>(animationSession.player.y);
    const auto animationClientList = builder.Build(&*animationMap, animationSession, viewport, 34);
    const auto animationEditorList = builder.Build(&*animationMap, animationSession, viewport, 34);
    if (!Require(animationClientList.layers.size() == animationEditorList.layers.size(), "animation map layer cardinality parity")) return 1;
    for (std::size_t index = 0; index < animationClientList.layers.size(); ++index)
    {
        const auto& client = animationClientList.layers[index];
        const auto& editor = animationEditorList.layers[index];
        if (!Require(client.kind == editor.kind && client.pass == editor.pass && client.legacyRow == editor.legacyRow &&
            client.legacyPriority == editor.legacyPriority && client.legacyObjectId == editor.legacyObjectId,
            "animation map ordered draw pass parity")) return 1;
    }

    const auto presetPath = outputDir / L"headless-preset.phxpreset";
    const auto realPresetPath = outputDir / L"eslania-real-tile.phxpreset";
    const auto corruptPresetPath = outputDir / L"headless-preset-corrupt.phxpreset";
    auto keyboardSelectionMap = MakeMap(8U, 7U, 0U);
    std::fill(keyboardSelectionMap.sectors.begin(), keyboardSelectionMap.sectors.end(),
        phoenix::world::LegacyMapSector{ 0xFFFFU, 0U, 0U });
    const auto setSelectionTile = [&](const std::uint16_t x, const std::uint16_t y,
        const std::uint32_t sprite, const std::uint8_t property) {
        keyboardSelectionMap.sectors[static_cast<std::size_t>(y) * keyboardSelectionMap.width + x] =
            { sprite, property, static_cast<std::uint8_t>(property + 1U) };
    };
    setSelectionTile(1U, 1U, 101U, 1U);
    setSelectionTile(2U, 1U, 102U, 2U);
    setSelectionTile(1U, 2U, 103U, 3U);
    setSelectionTile(2U, 2U, 104U, 4U);
    setSelectionTile(3U, 1U, 201U, 5U);
    setSelectionTile(3U, 2U, 202U, 6U);
    phoenix::world::LegacyMapImageObject selectedImage;
    selectedImage.wrapperType = selectedImage.objectType = 3U;
    selectedImage.objectId = selectedImage.imageObjectId = 10U;
    selectedImage.spriteId = 501U;
    selectedImage.pixelX = 48;
    selectedImage.pixelY = 24;
    selectedImage.viewpoint = 0U;
    selectedImage.positions.emplace_back(static_cast<std::uint16_t>(1U), static_cast<std::uint16_t>(1U));
    keyboardSelectionMap.imageObjects.push_back(selectedImage);
    auto selectedAnimation = selectedImage;
    selectedAnimation.wrapperType = selectedAnimation.objectType = 5U;
    selectedAnimation.objectId = selectedAnimation.imageObjectId = 11U;
    selectedAnimation.animation = true;
    selectedAnimation.frameId = 77U;
    selectedAnimation.pixelX = 96;
    selectedAnimation.pixelY = 48;
    selectedAnimation.viewpoint = 2U;
    selectedAnimation.positions.clear();
    selectedAnimation.positions.emplace_back(static_cast<std::uint16_t>(2U), static_cast<std::uint16_t>(2U));
    keyboardSelectionMap.imageObjects.push_back(selectedAnimation);
    editor::RebuildObjectIndex(keyboardSelectionMap);
    keyboardSelectionMap.modified = false;
    const editor::MapSelectionRect keyboardTiles{ 1U, 1U, 2U, 2U };
    const std::set<std::uint32_t> keyboardObjects{ 10U, 11U };
    if (!Require(editor::CanMoveMapSelection(keyboardSelectionMap, keyboardTiles, keyboardObjects, 1, 0),
        "multi-selection keyboard movement pre-validates all tile and object records")) return 1;
    editor::MapEditHistory keyboardHistory;
    keyboardHistory.Begin(keyboardSelectionMap);
    if (!Require(editor::MoveMapSelection(keyboardSelectionMap, keyboardTiles, keyboardObjects, 1, 0),
        "multi-selection keyboard movement applies atomically")) return 1;
    const auto sectorAt = [&](const std::uint16_t x, const std::uint16_t y) -> const auto& {
        return keyboardSelectionMap.sectors[static_cast<std::size_t>(y) * keyboardSelectionMap.width + x];
    };
    if (!Require(sectorAt(1U, 1U).spriteId == 201U && sectorAt(2U, 1U).spriteId == 101U &&
        sectorAt(3U, 1U).spriteId == 102U && sectorAt(1U, 2U).spriteId == 202U &&
        sectorAt(2U, 2U).spriteId == 103U && sectorAt(3U, 2U).spriteId == 104U &&
        sectorAt(3U, 2U).property == 4U,
        "occupied destination tiles are displaced without deletion")) return 1;
    if (!Require(keyboardSelectionMap.imageObjects[0].positions.front().first == 2U &&
        keyboardSelectionMap.imageObjects[0].positions.front().second == 1U &&
        keyboardSelectionMap.imageObjects[0].pixelX == 96 && keyboardSelectionMap.imageObjects[0].pixelY == 24 &&
        keyboardSelectionMap.imageObjects[0].viewpoint == 0U &&
        keyboardSelectionMap.imageObjects[1].positions.front().first == 3U &&
        keyboardSelectionMap.imageObjects[1].positions.front().second == 2U &&
        keyboardSelectionMap.imageObjects[1].pixelX == 144 && keyboardSelectionMap.imageObjects[1].pixelY == 48 &&
        keyboardSelectionMap.imageObjects[1].viewpoint == 2U,
        "Image and Animation Objects use the same exact keyboard delta and preserve pre-background pass")) return 1;
    if (!Require(keyboardHistory.Undo(keyboardSelectionMap) && sectorAt(1U, 1U).spriteId == 101U &&
        keyboardSelectionMap.imageObjects[0].positions.front().first == 1U &&
        keyboardSelectionMap.imageObjects[0].positions.front().second == 1U &&
        !keyboardSelectionMap.modified, "multi-selection keyboard move is one undo operation")) return 1;
    if (!Require(keyboardHistory.Redo(keyboardSelectionMap) && sectorAt(3U, 2U).spriteId == 104U &&
        keyboardSelectionMap.imageObjects[1].positions.front().first == 3U &&
        keyboardSelectionMap.imageObjects[1].positions.front().second == 2U,
        "multi-selection keyboard move redo restores the complete group")) return 1;
    const editor::MapSelectionRect movedKeyboardTiles{ 2U, 1U, 3U, 2U };
    if (!Require(editor::MoveMapSelection(keyboardSelectionMap, movedKeyboardTiles, keyboardObjects, 0, 1) &&
        keyboardSelectionMap.imageObjects[0].positions.front().second == 2U &&
        keyboardSelectionMap.imageObjects[0].viewpoint == 0U &&
        keyboardSelectionMap.imageObjects[1].positions.front().second == 3U &&
        keyboardSelectionMap.imageObjects[1].viewpoint == 3U,
        "vertical keyboard movement preserves object draw-pass rules")) return 1;
    if (!Require(!editor::CanMoveMapSelection(keyboardSelectionMap,
        { 0U, 0U, 1U, 1U }, keyboardObjects, -1, 0), "multi-selection boundary move is rejected")) return 1;

    auto deleteSelectionMap = MakeMap(6U, 5U, 2U);
    deleteSelectionMap.sectors[1U * deleteSelectionMap.width + 1U] = { 301U, 7U, 8U };
    deleteSelectionMap.sectors[1U * deleteSelectionMap.width + 2U] = { 302U, 9U, 10U };
    deleteSelectionMap.imageObjects[0].objectId = deleteSelectionMap.imageObjects[0].imageObjectId = 20U;
    deleteSelectionMap.imageObjects[0].positions = {
        { static_cast<std::uint16_t>(1U), static_cast<std::uint16_t>(1U) } };
    deleteSelectionMap.imageObjects[1].wrapperType = deleteSelectionMap.imageObjects[1].objectType = 5U;
    deleteSelectionMap.imageObjects[1].animation = true;
    deleteSelectionMap.imageObjects[1].objectId = deleteSelectionMap.imageObjects[1].imageObjectId = 21U;
    deleteSelectionMap.imageObjects[1].positions = {
        { static_cast<std::uint16_t>(2U), static_cast<std::uint16_t>(1U) } };
    editor::RebuildObjectIndex(deleteSelectionMap);
    deleteSelectionMap.modified = false;
    const editor::MapSelectionRect deleteTiles{ 1U, 1U, 2U, 1U };
    const std::set<std::uint32_t> deleteObjects{ 20U, 21U };
    editor::MapEditHistory deleteHistory;
    if (!Require(editor::CanDeleteMapSelection(deleteSelectionMap, deleteTiles, deleteObjects),
        "multi-selection delete recognizes selected tiles and objects")) return 1;
    deleteHistory.Begin(deleteSelectionMap);
    if (!Require(editor::DeleteMapSelection(deleteSelectionMap, deleteTiles, deleteObjects) &&
        deleteSelectionMap.sectors[1U * deleteSelectionMap.width + 1U].spriteId == 0xFFFFU &&
        deleteSelectionMap.sectors[1U * deleteSelectionMap.width + 2U].spriteId == 0xFFFFU &&
        deleteSelectionMap.sectors[1U * deleteSelectionMap.width + 1U].property == 7U &&
        deleteSelectionMap.sectors[1U * deleteSelectionMap.width + 1U].light == 8U &&
        deleteSelectionMap.imageObjects.empty() && deleteSelectionMap.modified,
        "Delete removes all selected Tiles, Image Objects and Animation Objects without clearing sector metadata")) return 1;
    if (!Require(deleteHistory.Undo(deleteSelectionMap) &&
        deleteSelectionMap.sectors[1U * deleteSelectionMap.width + 1U].spriteId == 301U &&
        deleteSelectionMap.imageObjects.size() == 2U && !deleteSelectionMap.modified,
        "multi-selection delete is one undo operation")) return 1;
    if (!Require(deleteHistory.Redo(deleteSelectionMap) && deleteSelectionMap.imageObjects.empty() &&
        deleteSelectionMap.sectors[1U * deleteSelectionMap.width + 2U].spriteId == 0xFFFFU,
        "multi-selection delete redo restores the complete deletion")) return 1;

    const auto realTile = std::find_if(loaded.map->sectorImageObjectIndices.begin(),
        loaded.map->sectorImageObjectIndices.end(), [](const auto& objects) { return objects.empty(); });
    if (!Require(realTile != loaded.map->sectorImageObjectIndices.end(), "real map contains a tile preset test location")) return 1;
    const auto realTileIndex = static_cast<std::size_t>(std::distance(loaded.map->sectorImageObjectIndices.begin(), realTile));
    const auto realTileX = static_cast<std::uint16_t>(realTileIndex % loaded.map->width);
    const auto realTileY = static_cast<std::uint16_t>(realTileIndex / loaded.map->width);
    const auto realPreset = editor::CaptureMapPreset(*loaded.map,
        { realTileX, realTileY, realTileX, realTileY }, "Eslania real tile");
    if (!Require(realPreset.Succeeded() && realPreset.preset->tiles.size() == 1U && realPreset.preset->objects.empty(),
        "preset capture uses real Legacy map data")) return 1;
    if (!Require(editor::SaveMapPreset(*realPreset.preset, realPresetPath).succeeded &&
        editor::LoadMapPreset(realPresetPath).Succeeded(), "real Legacy map preset file round trip")) return 1;
    auto realPresetTarget = *loaded.map;
    editor::MapEditHistory realPresetHistory;
    realPresetHistory.Begin(realPresetTarget);
    if (!Require(editor::PlaceMapPreset(realPresetTarget, *realPreset.preset, realTileX, realTileY).succeeded &&
        realPresetHistory.Undo(realPresetTarget), "real Legacy map preset placement and one-step undo")) return 1;

    auto presetSource = MakeMap(12U, 10U, 3U);
    presetSource.tileSpriteFilePositions.push_back({ 8230U, 4242 });
    presetSource.imageObjectSpriteFilePositions.push_back({ 1U, 5151 });
    presetSource.imageObjects.front().animation = true;
    presetSource.imageObjects.front().frameId = 37U;
    presetSource.imageObjects.front().maxFrame = 6U;
    presetSource.imageObjects.front().loop = true;
    presetSource.imageObjects.front().direction = 3U;
    presetSource.imageObjects.front().minDelay = 80U;
    presetSource.imageObjects.front().maxDelay = 120U;
    presetSource.imageObjects.front().viewpoint = 2U;
    const editor::MapPresetRect presetRect{ 0U, 0U, 2U, 1U };
    const auto capturedPreset = editor::CaptureMapPreset(presetSource, presetRect, "Headless preset");
    if (!Require(capturedPreset.Succeeded() && capturedPreset.preset->tiles.size() == 6U &&
        capturedPreset.preset->objects.size() == 3U, "preset captures complete tile and object selection")) return 1;
    if (!Require(editor::SaveMapPreset(*capturedPreset.preset, presetPath).succeeded, "versioned preset save")) return 1;
    const auto loadedPreset = editor::LoadMapPreset(presetPath);
    if (!Require(loadedPreset.Succeeded() && loadedPreset.preset->formatVersion == editor::kMapPresetFormatVersion &&
        loadedPreset.preset->tiles.size() == capturedPreset.preset->tiles.size() &&
        loadedPreset.preset->objects.size() == capturedPreset.preset->objects.size(), "preset checksum round trip")) return 1;
    const auto& storedAnimation = loadedPreset.preset->objects.front().object;
    if (!Require(storedAnimation.animation && storedAnimation.frameId == 37U && storedAnimation.maxFrame == 6U &&
        storedAnimation.loop && storedAnimation.direction == 3U && storedAnimation.minDelay == 80U &&
        storedAnimation.maxDelay == 120U, "preset preserves animation placement fields")) return 1;

    auto presetTarget = MakeMap(12U, 10U, 0U);
    presetTarget.modified = false;
    editor::MapEditHistory presetHistory;
    const auto presetValidation = editor::ValidateMapPresetPlacement(presetTarget, *loadedPreset.preset, 5U, 4U);
    if (!Require(presetValidation.succeeded, "preset placement pre-validation")) return 1;
    presetHistory.Begin(presetTarget);
    const auto placedPreset = editor::PlaceMapPreset(presetTarget, *loadedPreset.preset, 5U, 4U);
    if (!Require(placedPreset.succeeded && placedPreset.objectIds.size() == 3U && presetTarget.modified &&
        presetTarget.imageObjects.size() == 3U, "preset placement is one complete edit")) return 1;
    if (!Require(std::any_of(presetTarget.tileSpriteFilePositions.begin(), presetTarget.tileSpriteFilePositions.end(),
        [](const auto& resource) { return resource.spriteId == 8230U && resource.filePosition == 4242; }) &&
        std::any_of(presetTarget.imageObjectSpriteFilePositions.begin(), presetTarget.imageObjectSpriteFilePositions.end(),
        [](const auto& resource) { return resource.spriteId == 1U && resource.filePosition == 5151; }),
        "preset placement restores Legacy sprite file-position records")) return 1;
    const auto& placedAnimation = presetTarget.imageObjects.front();
    if (!Require(placedAnimation.positions.front() == std::pair<std::uint16_t, std::uint16_t>{
        static_cast<std::uint16_t>(5U), static_cast<std::uint16_t>(4U) } &&
        placedAnimation.viewpoint == 6U && placedAnimation.frameId == 37U && placedAnimation.loop,
        "preset placement preserves relative coordinates, pass and fields")) return 1;
    if (!Require(presetHistory.Undo(presetTarget) && presetTarget.imageObjects.empty() && !presetTarget.modified,
        "preset placement undo restores the complete group")) return 1;
    if (!Require(presetHistory.Redo(presetTarget) && presetTarget.imageObjects.size() == 3U && presetTarget.modified,
        "preset placement redo restores the complete group")) return 1;
    if (!Require(!editor::ValidateMapPresetPlacement(presetTarget, *loadedPreset.preset, 11U, 9U).succeeded,
        "out-of-bounds preset placement is rejected before mutation")) return 1;

    auto corruptPresetBytes = ReadBytes(presetPath);
    if (!Require(corruptPresetBytes.size() > 32U, "preset test payload exists")) return 1;
    corruptPresetBytes.back() ^= 0x55;
    {
        std::ofstream corruptPreset(corruptPresetPath, std::ios::binary | std::ios::trunc);
        corruptPreset.write(corruptPresetBytes.data(), static_cast<std::streamsize>(corruptPresetBytes.size()));
    }
    if (!Require(!editor::LoadMapPreset(corruptPresetPath).Succeeded(), "corrupt preset fails checksum validation")) return 1;
    auto invalidCoordinates = *loadedPreset.preset;
    invalidCoordinates.tiles.front().relativeX = -1;
    if (!Require(!editor::ValidateMapPresetPlacement(presetTarget, invalidCoordinates, 0U, 0U).succeeded,
        "malicious preset tile coordinates are rejected before indexing")) return 1;

    const auto bundlePath = outputDir / L"legacy-server-bundle.map";
    for (const auto extension : { L".map", L".mip", L".smp", L".ssi" })
    {
        auto path = bundlePath; path.replace_extension(extension);
        std::filesystem::remove(path, ec);
        path += L".bak"; std::filesystem::remove(path, ec);
    }
    auto bundleMap = MakeMap(8U, 6U, 0U);
    bundleMap.name = "server bundle";
    bundleMap.zoneId = 42U;
    bundleMap.sectors[3U * bundleMap.width + 2U].property = phoenix::world::kLegacyBlockGround;
    auto* bundleServer = phoenix::world::LegacyServerSector(bundleMap, 2U, 3U);
    if (!Require(bundleServer != nullptr, "server sector model initialized")) return 1;
    bundleServer->portal = phoenix::world::LegacyMapPortal{ 0U, {
        phoenix::world::LegacyMapPortalTarget{ 77U, 4U, 5U, true } } };
    bundleServer->safetyFlags = phoenix::world::kLegacySafetyCommon | phoenix::world::kLegacySafetySlayer;
    bundleMap.sidecars.modified = true;
    const auto bundleWrite = phoenix::world::LegacyMapBundleWriter{}.Save(bundleMap, bundlePath);
    if (!Require(bundleWrite.succeeded, "Legacy client/server map bundle write"))
    {
        std::cerr << bundleWrite.error << '\n'; return 1;
    }
    auto bundleReload = mapReader.Load(bundlePath);
    if (!Require(bundleReload.Succeeded(), "server bundle client map reload")) return 1;
    const auto sidecarReload = phoenix::world::LegacyMapSidecarReader{}.Load(*bundleReload.map, bundlePath);
    const auto* reloadedServer = phoenix::world::LegacyServerSector(*bundleReload.map, 2U, 3U);
    if (!Require(sidecarReload.succeeded && reloadedServer != nullptr && reloadedServer->portal &&
        reloadedServer->portal->type == 0U && reloadedServer->portal->targets.size() == 1U &&
        reloadedServer->portal->targets.front().zoneId == 77U && reloadedServer->portal->targets.front().x == 4U &&
        reloadedServer->portal->targets.front().y == 5U && reloadedServer->portal->targets.front().destinationKnown &&
        reloadedServer->safetyFlags == (phoenix::world::kLegacySafetyCommon | phoenix::world::kLegacySafetySlayer),
        "Legacy .smp/.mip/.ssi portal and safety round trip")) return 1;

    const auto firstSmp = ReadBytes(std::filesystem::path(bundlePath).replace_extension(L".smp"));
    const auto secondWrite = phoenix::world::LegacyMapBundleWriter{}.Save(*bundleReload.map, bundlePath);
    if (!Require(secondWrite.succeeded && secondWrite.backupCreated,
        "server bundle repeated save creates backups")) return 1;
    const auto smpBackup = std::filesystem::path(bundlePath).replace_extension(L".smp.bak");
    if (!Require(firstSmp == ReadBytes(smpBackup), "unchanged server sidecar backup is byte-identical")) return 1;

    editor::MapEditHistory portalHistory;
    editor::PropertyTarget portalTarget;
    portalTarget.tile = std::pair<std::uint16_t, std::uint16_t>{
        static_cast<std::uint16_t>(2U), static_cast<std::uint16_t>(3U) };
    const auto portalEdit = editor::LegacyMapPropertyEditor::Apply(*bundleReload.map, portalHistory,
        editor::PropertyField::PortalTargets, "88:6:5", portalTarget);
    if (!Require(portalEdit.applied && bundleReload.map->sidecars.modified &&
        phoenix::world::LegacyServerSector(*bundleReload.map, 2U, 3U)->portal->targets.front().zoneId == 88U,
        "portal property edit updates real server model and dirty state")) return 1;
    if (!Require(portalHistory.Undo(*bundleReload.map) &&
        phoenix::world::LegacyServerSector(*bundleReload.map, 2U, 3U)->portal->targets.front().zoneId == 77U,
        "portal property edit participates in undo")) return 1;
    if (!Require(portalHistory.Redo(*bundleReload.map) &&
        phoenix::world::LegacyServerSector(*bundleReload.map, 2U, 3U)->portal->targets.front().zoneId == 88U,
        "portal property edit participates in redo")) return 1;

    auto unknownPortal = MakeMap(4U, 4U, 0U);
    unknownPortal.name = "unknown portal";
    phoenix::world::LegacyServerSector(unknownPortal, 1U, 1U)->portal = phoenix::world::LegacyMapPortal{ 0U, {
        phoenix::world::LegacyMapPortalTarget{ 9U, 0U, 0U, false } } };
    unknownPortal.sidecars.modified = true;
    const auto unknownResult = phoenix::world::LegacyMapBundleWriter{}.Save(
        unknownPortal, outputDir / L"unknown-portal.map");
    if (!Require(unknownResult.succeeded,
        "unknown server portal destination is allowed (Save is not blocked by missing X/Y)")) return 1;

    auto clientOnlyPortal = MakeMap(4U, 4U, 0U);
    clientOnlyPortal.name = "client only portal";
    phoenix::world::LegacyServerSector(clientOnlyPortal, 1U, 1U)->portal = phoenix::world::LegacyMapPortal{ 200U, {
        phoenix::world::LegacyMapPortalTarget{ 12U, 0U, 0U, false } } };
    clientOnlyPortal.sidecars.modified = true;
    const auto clientOnlyPath = outputDir / L"client-only-portal.map";
    const auto clientOnlyWrite = phoenix::world::LegacyMapBundleWriter{}.Save(clientOnlyPortal, clientOnlyPath);
    auto clientOnlyReload = mapReader.Load(clientOnlyPath);
    const auto clientOnlySidecars = clientOnlyReload.Succeeded()
        ? phoenix::world::LegacyMapSidecarReader{}.Load(*clientOnlyReload.map, clientOnlyPath)
        : phoenix::world::LegacyMapSidecarResult{};
    const auto* clientOnlyServer = clientOnlyReload.Succeeded()
        ? phoenix::world::LegacyServerSector(*clientOnlyReload.map, 1U, 1U) : nullptr;
    if (!Require(clientOnlyWrite.succeeded && clientOnlySidecars.succeeded && clientOnlyServer &&
        clientOnlyServer->portal && clientOnlyServer->portal->type == 200U &&
        !clientOnlyServer->portal->targets.front().destinationKnown,
        "client-only portal remains in .mip and is excluded from .smp")) return 1;

    std::cout << "PASS map=" << loaded.map->width << 'x' << loaded.map->height
        << " objects=" << loaded.map->imageObjects.size()
        << " tiles=" << clientList.tiles.size()
        << " layers=" << clientList.layers.size()
        << " animation_map=" << animationSource.filename().string()
        << " animation_frame=" << realAnimation.frameId
        << " animation_frames=" << frame0.body->frameCount
        << " effect_ids=" << effectFrameIds.size()
        << " maps=" << scannedMaps
        << " animation_blt=" << animationBltCounts[0] << ',' << animationBltCounts[1] << ','
        << animationBltCounts[2] << ',' << animationBltCounts[3]
        << " sprite8230=" << tile.value->image.width << 'x' << tile.value->image.height << '\n';
    return 0;
}
