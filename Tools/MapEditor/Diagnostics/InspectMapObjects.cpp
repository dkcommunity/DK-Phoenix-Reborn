#include "Core/EditorWorldDrawListBuilder.h"
#include "Core/EditorSpriteArchiveReader.h"
#include "World/LegacyMapReader.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2)
    {
        std::wcerr << L"usage: InspectMapObjects <map>\n";
        return 2;
    }

    phoenix::world::LegacyMapReader reader;
    auto loaded = reader.Load(std::filesystem::path(argv[1]));
    if (!loaded.Succeeded())
    {
        std::cerr << loaded.error << '\n';
        return 1;
    }

    constexpr std::array<std::uint32_t, 6> ids{ 21014U, 21015U, 21016U, 21017U, 21018U, 21019U };
    phoenix::tools::map_editor::EditorWorldDrawListBuilder builder;
    const auto drawList = builder.BuildFullMap(*loaded.map, nullptr, 0U);
    phoenix::tools::map_editor::EditorSpriteArchiveReader spriteReader;
    const auto imageRoot = std::filesystem::path(argv[1]).parent_path().parent_path() / L"Image";
    std::size_t failures = 0U;
    for (const auto id : ids)
    {
        std::size_t matches = 0U;
        for (const auto& object : loaded.map->imageObjects)
        {
            if (object.objectId != id && object.imageObjectId != id && object.spriteId != id) continue;
            ++matches;
            std::cout << "query=" << id << " objectId=" << object.objectId
                << " imageObjectId=" << object.imageObjectId << " spriteId=" << object.spriteId
                << " wrapper=" << static_cast<unsigned>(object.wrapperType)
                << " viewpoint=" << object.viewpoint << " pixel=" << object.pixelX << ',' << object.pixelY
                << " positions=" << object.positions.size();
            const auto admitted = std::find_if(drawList.staticObjects.begin(), drawList.staticObjects.end(),
                [&](const auto& item) { return item.object == &object; }) != drawList.staticObjects.end();
            const auto decoded = spriteReader.ReadSprite565(imageRoot / L"imageobject.spk",
                imageRoot / L"imageobject.spki", object.spriteId,
                phoenix::assets::LegacyAssetFormat::SpritePack);
            std::cout << " admitted=" << admitted;
            if (decoded.value)
                std::cout << " decoded=1 size=" << decoded.value->image.width << 'x' << decoded.value->image.height;
            else
            {
                std::cout << " decoded=0 issues=" << decoded.issues.size();
                for (const auto& issue : decoded.issues)
                    std::wcout << L" [" << issue.sourcePath.wstring() << L":" << issue.byteOffset
                        << L" " << issue.message << L"]";
            }
            std::cout << '\n';
            if (!admitted || !decoded.value || decoded.value->image.Empty()) ++failures;
        }
        if (matches == 0U)
        {
            std::cout << "query=" << id << " NOT_FOUND\n";
            ++failures;
        }
    }
    std::cout << (failures == 0U ? "PASS" : "FAIL") << " editor_object_visibility failures=" << failures << '\n';
    return failures == 0U ? 0 : 1;
}
