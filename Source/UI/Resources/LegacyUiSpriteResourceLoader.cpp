#include "UI/Resources/LegacyUiSpriteResourceLoader.h"

#include "Assets/SpriteArchiveReader.h"

#include <filesystem>
#include <sstream>

namespace phoenix::ui
{
    namespace
    {
        constexpr std::size_t kMaxWarningCount = 96U;

        void PushWarning(std::vector<std::wstring>& warnings, std::wstring message)
        {
            if (warnings.size() < kMaxWarningCount)
            {
                warnings.push_back(std::move(message));
            }
        }

        std::wstring ToWide(const std::string& value)
        {
            return std::wstring(value.begin(), value.end());
        }

        bool IsSpriteKind(const content::LegacyUiSpriteKind kind) noexcept
        {
            return kind == content::LegacyUiSpriteKind::SpritePack ||
                kind == content::LegacyUiSpriteKind::IndexedSpritePack;
        }

        std::wstring DescribeRecord(const content::LegacyUiSpriteRecord& record)
        {
            std::wstringstream text;
            text << content::ToDisplayName(record.kind) << L' '
                 << ToWide(record.legacyDefine) << L" key=" << ToWide(record.key)
                 << L" scope=" << content::ToDisplayName(record.scope)
                 << L" race=" << content::ToDisplayName(record.race);
            return text.str();
        }

        bool HasSplitHeader(const std::filesystem::path& archivePath)
        {
            std::error_code ec;
            return std::filesystem::exists(archivePath.parent_path() / L"header.inf", ec);
        }
    }

    LegacyUiSpriteResourceReport LegacyUiSpriteResourceLoader::Load(
        const std::filesystem::path& dataRoot,
        const LegacyUiSpriteLoadMode mode) const
    {
        content::LegacyUiSpriteCatalog catalog;
        const auto catalogResult = catalog.Build(dataRoot);

        LegacyUiSpriteResourceReport report;
        report.catalogEntries = catalogResult.records.size();

        assets::SpriteArchiveReader reader;
        for (const auto& record : catalogResult.records)
        {
            const auto archivePath = catalogResult.ResolvePath(record);
            const auto indexPath = catalogResult.ResolveIndexPath(record);

            std::error_code ec;
            const bool archiveExists = std::filesystem::exists(archivePath, ec);
            if (!archiveExists)
            {
                ++report.missingAssets;
                if (record.required)
                {
                    PushWarning(report.warnings, L"Missing required UI asset: " + DescribeRecord(record) + L" path=" + archivePath.wstring());
                }
                continue;
            }
            ++report.presentAssets;

            if (record.catalogOnly)
            {
                continue;
            }

            if (!IsSpriteKind(record.kind))
            {
                const bool indexExists = std::filesystem::exists(indexPath, ec);
                if (indexExists)
                {
                    ++report.presentIndexes;
                }
                else if (record.indexRequired)
                {
                    ++report.missingIndexes;
                    PushWarning(report.warnings, L"Missing required UI companion index: " + DescribeRecord(record) + L" index=" + indexPath.wstring());
                }
                continue;
            }

            const bool indexExists = std::filesystem::exists(indexPath, ec) && !std::filesystem::is_directory(indexPath, ec);
            const bool splitHeaderExists = HasSplitHeader(archivePath);
            if (indexExists || splitHeaderExists)
            {
                ++report.presentIndexes;
            }
            else
            {
                ++report.missingIndexes;
                if (record.indexRequired)
                {
                    PushWarning(report.warnings, L"Missing required UI sprite index: " + DescribeRecord(record) + L" index=" + indexPath.wstring());
                    continue;
                }
            }

            if (mode == LegacyUiSpriteLoadMode::RuntimeInventory)
            {
                continue;
            }

            const auto auditPath = indexExists ? indexPath : (archivePath.parent_path() / L"header.inf");
            if (std::filesystem::is_directory(archivePath, ec))
            {
                ++report.auditedSpritePacks;
                continue;
            }

            const auto format = record.kind == content::LegacyUiSpriteKind::IndexedSpritePack
                ? assets::LegacyAssetFormat::IndexedSpritePack
                : assets::LegacyAssetFormat::SpritePack;
            auto atlas = reader.InspectSpriteAtlas(archivePath, auditPath, format);
            if (!atlas.value)
            {
                PushWarning(report.warnings, L"UI sprite atlas audit failed: " + DescribeRecord(record) + L" path=" + archivePath.wstring());
                continue;
            }

            ++report.auditedSpritePacks;
            if (record.expectedMinimumFrames != 0 && atlas.value->spriteCount < record.expectedMinimumFrames)
            {
                PushWarning(
                    report.warnings,
                    L"UI sprite frame count below contract: " + DescribeRecord(record) +
                    L" frames=" + std::to_wstring(atlas.value->spriteCount) +
                    L" expectedMin=" + std::to_wstring(record.expectedMinimumFrames));
            }

            if (!record.smokeDecode)
            {
                continue;
            }

            assets::LegacyReadResult<assets::LegacyDecodedSprite> decoded;
            const bool splitArchive = record.splitBaseFrameIndex != 0 ||
                archivePath.parent_path().extension() == L".spk" ||
                archivePath.parent_path().extension() == L".ispk";
            if (splitArchive)
            {
                decoded = reader.ReadSplitSprite565(
                    archivePath,
                    auditPath,
                    record.smokeFrameIndex,
                    record.splitBaseFrameIndex,
                    format);
            }
            else
            {
                decoded = reader.ReadSprite565(
                    archivePath,
                    auditPath,
                    record.smokeFrameIndex,
                    format);
            }

            if (!decoded.value || decoded.value->image.Empty())
            {
                ++report.failedSmokeFrames;
                PushWarning(report.warnings, L"UI smoke sprite decode failed: " + DescribeRecord(record));
                continue;
            }

            ++report.decodedSmokeFrames;
            report.smokeFrames.push_back(LegacyUiSpriteSmokeFrame{
                record.key,
                record.scope,
                record.race,
                record.smokeFrameIndex,
                std::move(decoded.value->image)
            });
        }

        return report;
    }
}
