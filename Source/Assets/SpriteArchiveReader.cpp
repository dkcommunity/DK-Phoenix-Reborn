#include "Assets/SpriteArchiveReader.h"

#include "Core/BinaryReader.h"
#include "Diagnostics/PhoenixDiagnostics.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <chrono>
#include <deque>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>

namespace phoenix::assets
{
    namespace
    {
        constexpr std::uint16_t kMaxSafeSpriteDimension = 8192;

        LegacyReadIssue MakeIssue(
            const std::filesystem::path& path,
            LegacyAssetFormat format,
            std::uint64_t offset,
            std::wstring message)
        {
            return LegacyReadIssue{ LegacyReadSeverity::Error, format, path, offset, std::move(message) };
        }

        constexpr std::uint64_t kMaxCachedSpriteBytes = 192ULL * 1024ULL * 1024ULL;
        constexpr std::uint64_t kMaxSingleCachedSpriteFileBytes = 32ULL * 1024ULL * 1024ULL;

        std::mutex& SpriteByteCacheMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        std::map<std::wstring, std::shared_ptr<const std::vector<std::byte>>>& SpriteByteCache()
        {
            static std::map<std::wstring, std::shared_ptr<const std::vector<std::byte>>> cache;
            return cache;
        }

        std::deque<std::wstring>& SpriteByteCacheOrder()
        {
            static std::deque<std::wstring> order;
            return order;
        }

        std::uint64_t& SpriteByteCacheSize()
        {
            static std::uint64_t size = 0;
            return size;
        }

        std::mutex& SpriteAtlasCacheMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        std::map<std::wstring, std::shared_ptr<const LegacySpriteAtlasReport>>& SpriteAtlasCache()
        {
            static std::map<std::wstring, std::shared_ptr<const LegacySpriteAtlasReport>> cache;
            return cache;
        }

        std::mutex& CombinedIndexCacheMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        std::map<std::wstring, std::shared_ptr<const LegacyIndexTable>>& CombinedIndexCache()
        {
            static std::map<std::wstring, std::shared_ptr<const LegacyIndexTable>> cache;
            return cache;
        }

        std::wstring NormalizeCachePath(const std::filesystem::path& path)
        {
            std::error_code ec;
            auto absolute = std::filesystem::absolute(path, ec);
            if (ec)
            {
                absolute = path;
            }
            return absolute.lexically_normal().wstring();
        }

        std::wstring AtlasCacheKey(
            const std::filesystem::path& archivePath,
            const std::filesystem::path& indexPath,
            const LegacyAssetFormat format)
        {
            return NormalizeCachePath(archivePath) +
                L"|" +
                NormalizeCachePath(indexPath) +
                L"|" +
                std::to_wstring(static_cast<int>(format));
        }

        std::wstring CombinedIndexCacheKey(
            const std::filesystem::path& indexPath,
            const LegacyAssetFormat format)
        {
            return NormalizeCachePath(indexPath) +
                L"|" +
                std::to_wstring(static_cast<int>(format));
        }

        std::shared_ptr<const std::vector<std::byte>> ReadAllBytes(
            const std::filesystem::path& path,
            bool* cacheHit = nullptr);

        struct CombinedIndexLookup
        {
            std::shared_ptr<const LegacyIndexTable> table;
            std::vector<LegacyReadIssue> issues;
            bool built{ false };
        };

        CombinedIndexLookup GetCombinedIndex(
            const std::filesystem::path& indexPath,
            const LegacyAssetFormat format)
        {
            CombinedIndexLookup result;
            const auto key = CombinedIndexCacheKey(indexPath, format);
            std::lock_guard lock(CombinedIndexCacheMutex());
            if (const auto cached = CombinedIndexCache().find(key); cached != CombinedIndexCache().end())
            {
                result.table = cached->second;
                return result;
            }

            const auto bytes = ReadAllBytes(indexPath);
            if (!bytes || bytes->empty())
            {
                result.issues.push_back(MakeIssue(indexPath, format, 0, L"Combined sprite index could not be opened or is empty."));
                return result;
            }

            core::BinaryReader reader(bytes->data(), bytes->size());
            const auto count = reader.ReadU16LE();
            if (!count)
            {
                result.issues.push_back(MakeIssue(indexPath, format, reader.Position(), L"Combined sprite index missing count field."));
                return result;
            }

            LegacyIndexTable table;
            table.count = *count;
            table.offsets.reserve(table.count);
            for (std::uint16_t index = 0; index < table.count; ++index)
            {
                const auto offset = reader.ReadU32LE();
                if (!offset)
                {
                    result.issues.push_back(MakeIssue(indexPath, format, reader.Position(), L"Combined sprite index ended before all offsets were read."));
                    return result;
                }
                table.offsets.push_back(*offset);
            }

            result.table = std::make_shared<const LegacyIndexTable>(std::move(table));
            CombinedIndexCache().emplace(key, result.table);
            result.built = true;
            return result;
        }

        std::optional<std::uint32_t> FindNextCombinedOffset(
            const LegacyIndexTable& index,
            const std::uint32_t spriteIndex)
        {
            const auto current = index.offsets[spriteIndex];
            for (std::size_t candidate = static_cast<std::size_t>(spriteIndex) + 1U;
                candidate < index.offsets.size();
                ++candidate)
            {
                if (index.offsets[candidate] > current)
                {
                    return index.offsets[candidate];
                }
            }
            return std::nullopt;
        }

        std::vector<std::byte> ReadAllBytesFromDisk(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return {};
            }

            const std::vector<char> chars{
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            };

            std::vector<std::byte> bytes;
            bytes.reserve(chars.size());
            for (const auto value : chars)
            {
                bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(value)));
            }

            return bytes;
        }

        std::shared_ptr<const std::vector<std::byte>> ReadAllBytes(
            const std::filesystem::path& path,
            bool* cacheHit)
        {
            const auto key = NormalizeCachePath(path);
            {
                std::lock_guard lock(SpriteByteCacheMutex());
                if (const auto cached = SpriteByteCache().find(key); cached != SpriteByteCache().end())
                {
                    if (cacheHit != nullptr) *cacheHit = true;
                    return cached->second;
                }
            }

            if (cacheHit != nullptr) *cacheHit = false;

            auto bytes = ReadAllBytesFromDisk(path);
            if (bytes.empty() || bytes.size() > kMaxSingleCachedSpriteFileBytes)
            {
                return std::make_shared<const std::vector<std::byte>>(std::move(bytes));
            }

            std::lock_guard lock(SpriteByteCacheMutex());
            auto& cache = SpriteByteCache();
            if (const auto cached = cache.find(key); cached != cache.end())
            {
                return cached->second;
            }

            auto& order = SpriteByteCacheOrder();
            auto& totalSize = SpriteByteCacheSize();
            while (!order.empty() && totalSize + bytes.size() > kMaxCachedSpriteBytes)
            {
                const auto evictKey = order.front();
                order.pop_front();
                if (const auto it = cache.find(evictKey); it != cache.end())
                {
                    totalSize -= static_cast<std::uint64_t>(it->second->size());
                    cache.erase(it);
                }
            }

            totalSize += static_cast<std::uint64_t>(bytes.size());
            order.push_back(key);
            auto sharedBytes = std::make_shared<const std::vector<std::byte>>(std::move(bytes));
            auto [inserted, _] = cache.emplace(key, std::move(sharedBytes));
            return inserted->second;
        }

        std::uint8_t ToU8(const std::byte value) noexcept
        {
            return std::to_integer<std::uint8_t>(value);
        }

        std::uint16_t ReadU16LEAt(const std::vector<std::byte>& bytes, const std::size_t offset) noexcept
        {
            return static_cast<std::uint16_t>(ToU8(bytes[offset]) | (ToU8(bytes[offset + 1]) << 8U));
        }

        void WriteRgb565AsRgba(const std::uint16_t value, std::vector<std::uint8_t>& pixels, const std::size_t pixelIndex)
        {
            const auto r5 = static_cast<std::uint8_t>((value >> 11U) & 0x1FU);
            const auto g6 = static_cast<std::uint8_t>((value >> 5U) & 0x3FU);
            const auto b5 = static_cast<std::uint8_t>(value & 0x1FU);
            const auto base = pixelIndex * 4U;
            pixels[base + 0U] = static_cast<std::uint8_t>((r5 << 3U) | (r5 >> 2U));
            pixels[base + 1U] = static_cast<std::uint8_t>((g6 << 2U) | (g6 >> 4U));
            pixels[base + 2U] = static_cast<std::uint8_t>((b5 << 3U) | (b5 >> 2U));
            pixels[base + 3U] = 255U;
        }

        constexpr std::uint16_t kLegacyColorSetCount = 795;
        constexpr std::uint16_t kLegacyColorGradationCount = 30;
        constexpr std::uint16_t kLegacyColorGradationHalf = 15;

        std::uint16_t LegacyColor565(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue) noexcept
        {
            return static_cast<std::uint16_t>((red << 11U) | (green << 6U) | blue);
        }

        std::uint8_t LegacyRed(const std::uint16_t color) noexcept
        {
            return static_cast<std::uint8_t>(color >> 11U);
        }

        std::uint8_t LegacyGreen(const std::uint16_t color) noexcept
        {
            return static_cast<std::uint8_t>((color >> 6U) & 0x1FU);
        }

        std::uint8_t LegacyBlue(const std::uint16_t color) noexcept
        {
            return static_cast<std::uint8_t>(color & 0x1FU);
        }

        void FillLegacyGradient(
            std::uint16_t* target,
            const int step,
            const int red0,
            const int green0,
            const int blue0,
            const int red1,
            const int green1,
            const int blue1)
        {
            if (target == nullptr || step <= 0)
            {
                return;
            }

            if (step == 1)
            {
                *target = LegacyColor565(
                    static_cast<std::uint8_t>(std::clamp(red0, 0, 31)),
                    static_cast<std::uint8_t>(std::clamp(green0, 0, 31)),
                    static_cast<std::uint8_t>(std::clamp(blue0, 0, 31)));
                return;
            }

            float red = static_cast<float>(red0);
            float green = static_cast<float>(green0);
            float blue = static_cast<float>(blue0);
            const auto denom = static_cast<float>(step - 1);
            const auto stepRed = static_cast<float>(red1 - red0) / denom;
            const auto stepGreen = static_cast<float>(green1 - green0) / denom;
            const auto stepBlue = static_cast<float>(blue1 - blue0) / denom;

            for (int i = 0; i < step; ++i)
            {
                target[i] = LegacyColor565(
                    static_cast<std::uint8_t>(std::clamp(static_cast<int>(red), 0, 31)),
                    static_cast<std::uint8_t>(std::clamp(static_cast<int>(green), 0, 31)),
                    static_cast<std::uint8_t>(std::clamp(static_cast<int>(blue), 0, 31)));
                red += stepRed;
                green += stepGreen;
                blue += stepBlue;
            }
        }

        const std::array<std::array<std::uint16_t, kLegacyColorGradationCount>, kLegacyColorSetCount>& LegacyColorSetTable()
        {
            static const auto table = [] {
                std::array<std::array<std::uint16_t, kLegacyColorGradationCount>, kLegacyColorSetCount> colorSet{};
                constexpr std::array<std::array<std::uint8_t, 3>, 53> rgbPoint{ {
                    { 0, 0, 31 },
                    { 0, 31, 0 },
                    { 31, 0, 0 },
                    { 0, 31, 31 },
                    { 31, 0, 31 },
                    { 31, 31, 0 },
                    { 0, 0, 16 },
                    { 0, 16, 0 },
                    { 16, 0, 0 },
                    { 0, 16, 16 },
                    { 16, 0, 16 },
                    { 16, 16, 0 },
                    { 16, 31, 0 },
                    { 16, 0, 31 },
                    { 31, 16, 0 },
                    { 0, 16, 31 },
                    { 31, 0, 16 },
                    { 0, 31, 16 },
                    { 16, 31, 16 },
                    { 16, 16, 31 },
                    { 31, 16, 16 },
                    { 16, 31, 31 },
                    { 31, 16, 31 },
                    { 31, 31, 16 },
                    { 16, 16, 16 },
                    { 24, 24, 24 },
                    { 8, 8, 8 },
                    { 30, 24, 18 },
                    { 25, 15, 11 },
                    { 21, 12, 11 },
                    { 19, 15, 13 },
                    { 21, 18, 11 },
                    { 22, 16, 9 },
                    { 31, 20, 15 },
                    { 9, 24, 15 },
                    { 15, 20, 24 },
                    { 31, 31, 24 },
                    { 31, 18, 31 },
                    { 18, 12, 0 },
                    { 24, 31, 24 },
                    { 24, 28, 27 },
                    { 24, 0, 31 },
                    { 18, 24, 22 },
                    { 25, 13, 3 },
                    { 14, 22, 14 },
                    { 12, 18, 28 },
                    { 0, 25, 25 },
                    { 17, 0, 17 },
                    { 31, 31, 18 },
                    { 14, 16, 18 },
                    { 8, 7, 17 },
                    { 24, 18, 0 },
                    { 12, 3, 27 }
                } };

                std::uint16_t set = 0;
                for (std::size_t i = 0; i < rgbPoint.size(); ++i)
                {
                    const auto red = rgbPoint[i][0];
                    const auto green = rgbPoint[i][1];
                    const auto blue = rgbPoint[i][2];

                    for (int j = kLegacyColorGradationHalf; j >= 1; --j)
                    {
                        if (j == kLegacyColorGradationHalf)
                        {
                            FillLegacyGradient(colorSet[set].data(), j, 31, 31, 31, red, green, blue);
                        }
                        else
                        {
                            const auto color = colorSet[i * kLegacyColorGradationHalf][kLegacyColorGradationHalf - j];
                            FillLegacyGradient(
                                colorSet[set].data(),
                                j,
                                LegacyRed(color),
                                LegacyGreen(color),
                                LegacyBlue(color),
                                red,
                                green,
                                blue);
                        }

                        FillLegacyGradient(colorSet[set].data() + j, kLegacyColorGradationCount - j, red, green, blue, 0, 0, 0);
                        ++set;
                    }
                }

                return colorSet;
            }();

            return table;
        }

        std::uint16_t LegacyIndexedColor(const std::uint16_t value, const LegacyIndexedColorMapping* colorMapping) noexcept
        {
            const auto indexSet = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
            const auto gradation = static_cast<std::uint8_t>(value & 0xFFU);
            const auto colorSet = colorMapping == nullptr ? 0U : colorMapping->colorSetByIndex[indexSet];
            if (colorSet >= kLegacyColorSetCount || gradation >= kLegacyColorGradationCount)
            {
                return 0;
            }

            return LegacyColorSetTable()[colorSet][gradation];
        }

        std::uint32_t ArchiveByteSize(const std::filesystem::path& path)
        {
            std::error_code ec;
            const auto size = std::filesystem::file_size(path, ec);
            if (ec || size > std::numeric_limits<std::uint32_t>::max())
            {
                return 0;
            }

            return static_cast<std::uint32_t>(size);
        }

        std::optional<std::uint32_t> FindNextOffset(
            const std::vector<std::uint32_t>& offsets,
            const std::uint16_t index,
            const std::uint32_t archiveSize)
        {
            for (std::size_t i = static_cast<std::size_t>(index) + 1U; i < offsets.size(); ++i)
            {
                if (offsets[i] > offsets[index] && offsets[i] <= archiveSize)
                {
                    return offsets[i];
                }
            }

            if (archiveSize > offsets[index])
            {
                return archiveSize;
            }

            return std::nullopt;
        }

        struct LegacySplitPackHeader
        {
            std::uint32_t spriteCount{ 0 };
            std::uint16_t unitCount{ 0 };
        };

        std::optional<LegacySplitPackHeader> ReadSplitPackHeader(const std::filesystem::path& archivePath)
        {
            const auto headerBytes = ReadAllBytes(archivePath / L"header.inf");
            if (!headerBytes || headerBytes->empty())
            {
                return std::nullopt;
            }

            std::array<std::uint32_t, 2> values{};
            std::size_t valueCount = 0;
            std::uint32_t current = 0;
            bool readingNumber = false;
            for (const auto byte : *headerBytes)
            {
                const auto ch = static_cast<char>(ToU8(byte));
                if (ch >= '0' && ch <= '9')
                {
                    readingNumber = true;
                    const auto digit = static_cast<std::uint32_t>(ch - '0');
                    if (current <= (std::numeric_limits<std::uint32_t>::max() - digit) / 10U)
                    {
                        current = current * 10U + digit;
                    }
                    continue;
                }

                if (readingNumber)
                {
                    values[valueCount++] = current;
                    if (valueCount == values.size())
                    {
                        break;
                    }
                    current = 0;
                    readingNumber = false;
                }
            }

            if (readingNumber && valueCount < values.size())
            {
                values[valueCount++] = current;
            }

            if (valueCount < 2U || values[0] == 0U || values[1] == 0U ||
                values[1] > std::numeric_limits<std::uint16_t>::max())
            {
                return std::nullopt;
            }

            return LegacySplitPackHeader{ values[0], static_cast<std::uint16_t>(values[1]) };
        }

        std::filesystem::path SplitChunkPath(const std::filesystem::path& archivePath, const std::uint32_t baseSpriteIndex)
        {
            auto filename = std::to_wstring(baseSpriteIndex);
            if (filename.size() < 5U)
            {
                filename.insert(filename.begin(), 5U - filename.size(), L'0');
            }

            return archivePath / (filename + archivePath.extension().wstring());
        }

        bool IsDirectory(const std::filesystem::path& path)
        {
            std::error_code ec;
            return std::filesystem::is_directory(path, ec);
        }

        bool SkipLegacySpriteRecord(
            const std::vector<std::byte>& bytes,
            std::size_t& offset,
            const LegacyAssetFormat format)
        {
            if (offset + 4U > bytes.size())
            {
                return false;
            }

            const auto width = ReadU16LEAt(bytes, offset);
            const auto height = ReadU16LEAt(bytes, offset + 2U);
            offset += 4U;
            if (width == 0U || height == 0U)
            {
                return true;
            }

            if (width > kMaxSafeSpriteDimension || height > kMaxSafeSpriteDimension)
            {
                return false;
            }

            for (std::uint16_t row = 0; row < height; ++row)
            {
                if (offset + 2U > bytes.size())
                {
                    return false;
                }

                const auto rowWordCount = ReadU16LEAt(bytes, offset);
                offset += 2U;
                if (format == LegacyAssetFormat::ShadowSpritePack && rowWordCount == 0U)
                {
                    return false;
                }

                const auto rowBytes = static_cast<std::size_t>(rowWordCount) * 2U;
                if (offset + rowBytes > bytes.size())
                {
                    return false;
                }
                offset += rowBytes;
            }

            return true;
        }

        LegacyReadResult<LegacyDecodedSprite> DecodeSprite565Payload(
            const std::filesystem::path& sourcePath,
            const LegacyAssetFormat format,
            const std::vector<std::byte>& bytes,
            LegacySpriteMetadata metadata)
        {
            LegacyReadResult<LegacyDecodedSprite> result;
            core::BinaryReader reader(bytes.data(), bytes.size());
            if (!reader.Seek(metadata.offset))
            {
                result.issues.push_back(MakeIssue(sourcePath, format, metadata.offset, L"Sprite offset is outside archive."));
                return result;
            }

            const auto width = reader.ReadU16LE();
            const auto height = reader.ReadU16LE();
            if (!width || !height || *width > kMaxSafeSpriteDimension || *height > kMaxSafeSpriteDimension)
            {
                result.issues.push_back(MakeIssue(sourcePath, format, metadata.offset, L"Sprite header could not be read."));
                return result;
            }

            if ((metadata.width != 0U && metadata.width != *width) ||
                (metadata.height != 0U && metadata.height != *height))
            {
                result.issues.push_back(MakeIssue(sourcePath, format, metadata.offset, L"Sprite header does not match index metadata."));
                return result;
            }

            metadata.width = *width;
            metadata.height = *height;
            metadata.valid = true;

            graphics::ImageData image;
            image.width = *width;
            image.height = *height;
            image.format = graphics::PixelFormat::Rgba8;
            image.pixels.assign(static_cast<std::size_t>(image.width) * image.height * 4U, 0U);

            if (image.width == 0U || image.height == 0U)
            {
                result.value = LegacyDecodedSprite{ metadata, std::move(image) };
                return result;
            }

            const bool shadow = format == LegacyAssetFormat::ShadowSpritePack;
            for (std::uint32_t y = 0; y < image.height; ++y)
            {
                const auto rowWordCount = reader.ReadU16LE();
                if (!rowWordCount)
                {
                    result.issues.push_back(MakeIssue(sourcePath, format, reader.Position(), L"Sprite row length is missing."));
                    return result;
                }

                if (*rowWordCount == 0U)
                {
                    if (shadow)
                    {
                        result.issues.push_back(MakeIssue(sourcePath, format, reader.Position(), L"Shadow sprite row length is invalid."));
                        return result;
                    }
                    continue;
                }

                const auto rowStart = reader.Position();
                const auto rowBytes = static_cast<std::size_t>(*rowWordCount) * 2U;
                if (!reader.CanRead(rowBytes))
                {
                    result.issues.push_back(MakeIssue(sourcePath, format, rowStart, L"Sprite row payload exceeds archive."));
                    return result;
                }

                const auto rowData = reader.ReadBytes(rowBytes);
                if (!rowData || rowData->size() != rowBytes)
                {
                    result.issues.push_back(MakeIssue(sourcePath, format, rowStart, L"Sprite row payload could not be read."));
                    return result;
                }

                std::vector<std::uint16_t> rowWords;
                rowWords.reserve(*rowWordCount);
                for (std::size_t offset = 0; offset < rowData->size(); offset += 2U)
                {
                    rowWords.push_back(ReadU16LEAt(*rowData, offset));
                }

                const auto runCount = rowWords[0];
                if (shadow && rowWords.size() != 1U + static_cast<std::size_t>(runCount) * 2U)
                {
                    result.issues.push_back(MakeIssue(sourcePath, format, rowStart, L"Shadow sprite row run payload is invalid."));
                    return result;
                }

                std::size_t wordIndex = 1U;
                std::uint32_t x = 0;
                for (std::uint16_t run = 0; run < runCount; ++run)
                {
                    if (wordIndex + 1U >= rowWords.size())
                    {
                        result.issues.push_back(MakeIssue(sourcePath, format, rowStart, L"Sprite row run header is truncated."));
                        return result;
                    }

                    const auto transparentCount = rowWords[wordIndex++];
                    const auto colorCount = rowWords[wordIndex++];
                    x += transparentCount;
                    if (x + colorCount > image.width || (!shadow && wordIndex + colorCount > rowWords.size()))
                    {
                        result.issues.push_back(MakeIssue(sourcePath, format, rowStart, L"Sprite row run exceeds width or row payload."));
                        return result;
                    }

                    for (std::uint16_t color = 0; color < colorCount; ++color)
                    {
                        const auto pixelIndex = static_cast<std::size_t>(y) * image.width + x;
                        if (shadow)
                        {
                            const auto base = pixelIndex * 4U;
                            image.pixels[base + 0U] = 0U;
                            image.pixels[base + 1U] = 0U;
                            image.pixels[base + 2U] = 0U;
                            image.pixels[base + 3U] = 96U;
                        }
                        else
                        {
                            WriteRgb565AsRgba(rowWords[wordIndex++], image.pixels, pixelIndex);
                        }
                        ++x;
                    }
                }
            }

            metadata.nextOffset = static_cast<std::uint32_t>(std::min<std::size_t>(
                reader.Position(),
                static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())));
            metadata.byteSize = metadata.nextOffset > metadata.offset ? metadata.nextOffset - metadata.offset : 0U;
            result.value = LegacyDecodedSprite{ metadata, std::move(image) };
            return result;
        }

        LegacyReadResult<LegacyDecodedSprite> ReadSplitDirectorySprite565(
            const std::filesystem::path& archivePath,
            const std::uint32_t spriteIndex,
            const LegacyAssetFormat format)
        {
            LegacyReadResult<LegacyDecodedSprite> result;
            const auto header = ReadSplitPackHeader(archivePath);
            if (!header)
            {
                result.issues.push_back(MakeIssue(archivePath / L"header.inf", format, 0, L"Split sprite header could not be read."));
                return result;
            }

            if (spriteIndex >= header->spriteCount)
            {
                // Legacy CTypePack::Get() returns dummy for out-of-range but doesn't crash.
                // For split packs, the chunk file may still exist beyond header.inf count.
                // Try to read the chunk anyway — if it doesn't exist, chunkBytes check below fails gracefully.
            }

            const auto baseSpriteIndex =
                (static_cast<std::uint32_t>(spriteIndex) / header->unitCount) * header->unitCount;
            const auto chunkPath = SplitChunkPath(archivePath, baseSpriteIndex);
            const auto chunkBytes = ReadAllBytes(chunkPath);
            if (!chunkBytes || chunkBytes->size() < 2U)
            {
                result.issues.push_back(MakeIssue(chunkPath, format, 0, L"Split sprite chunk is missing or empty."));
                return result;
            }

            const auto chunkSpriteCount = ReadU16LEAt(*chunkBytes, 0);
            const auto localIndexValue = spriteIndex - baseSpriteIndex;
            if (localIndexValue > std::numeric_limits<std::uint16_t>::max())
            {
                result.issues.push_back(MakeIssue(chunkPath, format, localIndexValue, L"Split sprite local index is too large."));
                return result;
            }
            const auto localIndex = static_cast<std::uint16_t>(localIndexValue);
            if (localIndex >= chunkSpriteCount)
            {
                result.issues.push_back(MakeIssue(chunkPath, format, localIndex, L"Split sprite local index is out of range."));
                return result;
            }

            std::size_t offset = 2U;
            for (std::uint16_t index = 0; index < localIndex; ++index)
            {
                if (!SkipLegacySpriteRecord(*chunkBytes, offset, format))
                {
                    result.issues.push_back(MakeIssue(chunkPath, format, offset, L"Split sprite chunk ended before requested sprite."));
                    return result;
                }
            }

            if (offset > std::numeric_limits<std::uint32_t>::max())
            {
                result.issues.push_back(MakeIssue(chunkPath, format, offset, L"Split sprite offset is too large."));
                return result;
            }

            LegacySpriteMetadata metadata;
            metadata.spriteIndex = spriteIndex;
            metadata.offset = static_cast<std::uint32_t>(offset);
            return DecodeSprite565Payload(chunkPath, format, *chunkBytes, metadata);
        }

        std::optional<std::vector<std::uint16_t>> ReadPalette(
            const std::filesystem::path& palettePath, const std::uint16_t paletteIndex)
        {
            const auto bytes = ReadAllBytes(palettePath);
            if (!bytes || bytes->size() < 2U) return std::nullopt;
            core::BinaryReader reader(bytes->data(), bytes->size());
            const auto count = reader.ReadU16LE();
            if (!count || paletteIndex >= *count) return std::nullopt;
            for (std::uint16_t index = 0; index < *count; ++index)
            {
                const auto size = reader.ReadU8();
                if (!size) return std::nullopt;
                std::vector<std::uint16_t> colors;
                colors.reserve(*size);
                for (std::uint16_t color = 0; color < *size; ++color)
                {
                    const auto value = reader.ReadU16LE();
                    if (!value) return std::nullopt;
                    if (index == paletteIndex) colors.push_back(*value);
                }
                if (index == paletteIndex) return colors;
            }
            return std::nullopt;
        }

        LegacyReadResult<LegacyDecodedSprite> DecodePalettedSprite(
            const std::filesystem::path& sourcePath,
            const std::vector<std::byte>& bytes,
            LegacySpriteMetadata metadata,
            const std::vector<std::uint16_t>& palette,
            const bool alphaPixels)
        {
            const auto format = alphaPixels ? LegacyAssetFormat::AlphaSpritePack : LegacyAssetFormat::IndexedSpritePack;
            LegacyReadResult<LegacyDecodedSprite> result;
            core::BinaryReader reader(bytes.data(), bytes.size());
            if (!reader.Seek(metadata.offset)) return result;
            const auto dataSize = reader.ReadU32LE();
            const auto width = reader.ReadU16LE();
            const auto height = reader.ReadU16LE();
            if (!dataSize || !width || !height || *width > kMaxSafeSpriteDimension || *height > kMaxSafeSpriteDimension ||
                !reader.CanRead(static_cast<std::size_t>(*dataSize) + static_cast<std::size_t>(*height) * 2U))
            {
                result.issues.push_back(MakeIssue(sourcePath, format, metadata.offset, L"Paletted sprite record is truncated or invalid."));
                return result;
            }
            const auto data = reader.ReadBytes(*dataSize);
            if (!data) return result;
            std::vector<std::uint16_t> rowSizes;
            rowSizes.reserve(*height);
            std::size_t rowBytesTotal = 0U;
            for (std::uint16_t row = 0; row < *height; ++row)
            {
                const auto size = reader.ReadU16LE();
                if (!size) return result;
                rowSizes.push_back(*size);
                rowBytesTotal += *size;
            }
            if (rowBytesTotal > data->size())
            {
                result.issues.push_back(MakeIssue(sourcePath, format, metadata.offset, L"Paletted sprite row table exceeds its data block."));
                return result;
            }

            graphics::ImageData image;
            image.width = *width;
            image.height = *height;
            image.format = graphics::PixelFormat::Rgba8;
            image.pixels.assign(static_cast<std::size_t>(*width) * *height * 4U, 0U);
            std::size_t rowOffset = 0U;
            for (std::uint32_t y = 0; y < *height; ++y)
            {
                const auto rowEnd = rowOffset + rowSizes[y];
                if (rowOffset >= rowEnd) { rowOffset = rowEnd; continue; }
                std::size_t cursor = rowOffset;
                const auto runCount = ToU8((*data)[cursor++]);
                std::uint32_t x = 0U;
                for (std::uint8_t run = 0; run < runCount; ++run)
                {
                    if (cursor + 2U > rowEnd) break;
                    x += ToU8((*data)[cursor++]);
                    const auto colorCount = ToU8((*data)[cursor++]);
                    for (std::uint8_t color = 0; color < colorCount; ++color)
                    {
                        const auto bytesPerPixel = alphaPixels ? 2U : 1U;
                        if (cursor + bytesPerPixel > rowEnd || x >= *width) { cursor = rowEnd; break; }
                        const auto alpha = alphaPixels ? ToU8((*data)[cursor++]) : 32U;
                        const auto paletteEntry = ToU8((*data)[cursor++]);
                        if (paletteEntry < palette.size())
                        {
                            const auto pixel = static_cast<std::size_t>(y) * *width + x;
                            WriteRgb565AsRgba(palette[paletteEntry], image.pixels, pixel);
                            image.pixels[pixel * 4U + 3U] = static_cast<std::uint8_t>(
                                std::min<std::uint32_t>(255U, static_cast<std::uint32_t>(alpha) * 255U / 32U));
                        }
                        ++x;
                    }
                }
                rowOffset = rowEnd;
            }
            metadata.width = *width;
            metadata.height = *height;
            metadata.byteSize = static_cast<std::uint32_t>(reader.Position() - metadata.offset);
            metadata.nextOffset = metadata.offset + metadata.byteSize;
            metadata.valid = true;
            result.value = LegacyDecodedSprite{ metadata, std::move(image) };
            return result;
        }
    }

    LegacyReadResult<LegacyIndexTable> SpriteArchiveReader::ReadIndex(
        const std::filesystem::path& indexPath,
        const LegacyAssetFormat companionFormat) const
    {
        LegacyReadResult<LegacyIndexTable> result;
        auto bytes = ReadAllBytes(indexPath);
        if (!bytes || bytes->empty())
        {
            result.issues.push_back(MakeIssue(indexPath, companionFormat, 0, L"Sprite index could not be opened or is empty."));
            return result;
        }

        core::BinaryReader reader(bytes->data(), bytes->size());
        auto count = reader.ReadU16LE();
        if (!count)
        {
            result.issues.push_back(MakeIssue(indexPath, companionFormat, reader.Position(), L"Sprite index missing count field."));
            return result;
        }

        LegacyIndexTable table;
        table.count = *count;
        table.offsets.reserve(table.count);

        for (std::uint16_t i = 0; i < table.count; ++i)
        {
            auto offset = reader.ReadU32LE();
            if (!offset)
            {
                result.issues.push_back(MakeIssue(indexPath, companionFormat, reader.Position(), L"Sprite index ended before all offsets were read."));
                return result;
            }

            table.offsets.push_back(*offset);
        }

        result.value = std::move(table);
        return result;
    }

    void SpriteArchiveReader::ResetMetrics() noexcept
    {
        metrics_ = {};
    }

    LegacySpriteReadMetrics SpriteArchiveReader::Metrics() const noexcept
    {
        return metrics_;
    }

    LegacyReadResult<LegacySpriteArchiveInfo> SpriteArchiveReader::ReadArchiveInfo(
        const std::filesystem::path& archivePath,
        const std::filesystem::path& indexPath,
        const LegacyAssetFormat format) const
    {
        LegacyReadResult<LegacySpriteArchiveInfo> result;
        auto index = ReadIndex(indexPath, format);
        result.issues = std::move(index.issues);
        if (!index.value)
        {
            return result;
        }

        std::error_code ec;
        const bool directory = IsDirectory(archivePath);
        if (!std::filesystem::exists(archivePath, ec) ||
            (!directory && std::filesystem::file_size(archivePath, ec) == 0))
        {
            result.issues.push_back(MakeIssue(archivePath, format, 0, L"Sprite archive is missing or empty."));
            return result;
        }

        auto spriteCount = index.value->count;
        if (directory)
        {
            const auto header = ReadSplitPackHeader(archivePath);
            if (!header || header->spriteCount > std::numeric_limits<std::uint16_t>::max())
            {
                result.issues.push_back(MakeIssue(archivePath / L"header.inf", format, 0,
                    L"Split sprite header count is invalid."));
                return result;
            }
            spriteCount = static_cast<std::uint16_t>(std::max<std::uint32_t>(
                header->spriteCount, index.value->count));
        }

        result.value = LegacySpriteArchiveInfo{
            format,
            spriteCount,
            archivePath,
            indexPath
        };
        return result;
    }

    LegacyReadResult<LegacySpriteAtlasReport> SpriteArchiveReader::InspectSpriteAtlas(
        const std::filesystem::path& archivePath,
        const std::filesystem::path& indexPath,
        const LegacyAssetFormat format) const
    {
        LegacyReadResult<LegacySpriteAtlasReport> result;
        const auto cacheKey = AtlasCacheKey(archivePath, indexPath, format);
        {
            std::lock_guard lock(SpriteAtlasCacheMutex());
            if (const auto cached = SpriteAtlasCache().find(cacheKey); cached != SpriteAtlasCache().end())
            {
                result.value = *cached->second;
                return result;
            }
        }

        auto index = ReadIndex(indexPath, format);
        result.issues = std::move(index.issues);
        if (!index.value)
        {
            return result;
        }

        const auto archiveSize = ArchiveByteSize(archivePath);
        if (archiveSize == 0)
        {
            result.issues.push_back(MakeIssue(archivePath, format, 0, L"Sprite archive is missing, empty, or too large."));
            return result;
        }

        const auto bytes = ReadAllBytes(archivePath);
        if (!bytes || bytes->empty())
        {
            result.issues.push_back(MakeIssue(archivePath, format, 0, L"Sprite archive could not be opened."));
            return result;
        }

        LegacySpriteAtlasReport report;
        report.archivePath = archivePath;
        report.indexPath = indexPath;
        report.spriteCount = index.value->count;
        report.sprites.reserve(index.value->offsets.size());

        for (std::uint16_t spriteIndex = 0; spriteIndex < index.value->count; ++spriteIndex)
        {
            LegacySpriteMetadata metadata;
            metadata.spriteIndex = spriteIndex;
            metadata.offset = index.value->offsets[spriteIndex];
            if (const auto next = FindNextOffset(index.value->offsets, spriteIndex, archiveSize))
            {
                metadata.nextOffset = *next;
                metadata.byteSize = metadata.nextOffset - metadata.offset;
            }

            if (metadata.offset + 4U <= bytes->size())
            {
                metadata.width = ReadU16LEAt(*bytes, metadata.offset);
                metadata.height = ReadU16LEAt(*bytes, static_cast<std::size_t>(metadata.offset) + 2U);
                metadata.valid =
                    metadata.width <= kMaxSafeSpriteDimension &&
                    metadata.height <= kMaxSafeSpriteDimension &&
                    metadata.byteSize > 0 &&
                    metadata.offset < archiveSize;
            }

            report.sprites.push_back(metadata);
        }

        {
            std::lock_guard lock(SpriteAtlasCacheMutex());
            SpriteAtlasCache()[cacheKey] = std::make_shared<const LegacySpriteAtlasReport>(report);
        }
        result.value = std::move(report);
        return result;
    }

    LegacyReadResult<LegacyDecodedSprite> SpriteArchiveReader::ReadSprite565(
        const std::filesystem::path& archivePath,
        const std::filesystem::path& indexPath,
        const std::uint32_t spriteIndex,
        const LegacyAssetFormat format) const
    {
        if (IsDirectory(archivePath))
        {
            const auto header = ReadSplitPackHeader(archivePath);
            if (!header || header->unitCount == 0U)
            {
                LegacyReadResult<LegacyDecodedSprite> result;
                result.issues.push_back(MakeIssue(archivePath / L"header.inf", format, 0,
                    L"Split sprite header could not be read."));
                return result;
            }
            const auto combinedIndex = ReadIndex(indexPath, format);
            const auto effectiveSpriteCount = std::max<std::uint32_t>(
                header->spriteCount, combinedIndex.value ? combinedIndex.value->count : 0U);
            // For SpritePack/ShadowSpritePack split packs, skip the count limit check —
            // Legacy CTypePack::Get() tries chunk access regardless of m_Size.
            // ReadSplitDirectorySprite565 will fail gracefully if the chunk doesn't exist.
            if (spriteIndex >= effectiveSpriteCount &&
                format != LegacyAssetFormat::SpritePack &&
                format != LegacyAssetFormat::ShadowSpritePack)
            {
                LegacyReadResult<LegacyDecodedSprite> result;
                result.issues.push_back(MakeIssue(archivePath, format, spriteIndex,
                    L"Split sprite index is outside both header and combined-index coverage."));
                return result;
            }
            if (format == LegacyAssetFormat::SpritePack || format == LegacyAssetFormat::ShadowSpritePack)
            {
                PHX_LOG_ONCE(L"[SPRITE] directory SpritePack/ShadowSpritePack uses chunk-local Legacy record order");
                return ReadSplitDirectorySprite565(archivePath, spriteIndex, format);
            }
            const auto splitBase = (spriteIndex / header->unitCount) * header->unitCount;
            const auto chunkPath = SplitChunkPath(archivePath, splitBase);
            if (format == LegacyAssetFormat::AlphaSpritePack || format == LegacyAssetFormat::IndexedSpritePack)
            {
                return ReadSplitIndexedSprite565(
                    chunkPath, indexPath, spriteIndex, splitBase, format);
            }
            return ReadSplitSprite565(chunkPath, indexPath, spriteIndex, splitBase, format);
        }

        LegacyReadResult<LegacyDecodedSprite> result;
        auto atlas = InspectSpriteAtlas(archivePath, indexPath, format);
        result.issues = std::move(atlas.issues);
        if (!atlas.value)
        {
            return result;
        }

        if (spriteIndex >= atlas.value->spriteCount)
        {
            result.issues.push_back(MakeIssue(indexPath, format, spriteIndex, L"Sprite index is out of range."));
            return result;
        }

        const auto metadata = atlas.value->sprites[spriteIndex];
        if (!metadata.valid)
        {
            result.issues.push_back(MakeIssue(archivePath, format, metadata.offset, L"Sprite metadata is invalid."));
            return result;
        }

        auto bytes = ReadAllBytes(archivePath);
        return bytes ? DecodeSprite565Payload(archivePath, format, *bytes, metadata) : result;
    }

    LegacyReadResult<LegacyDecodedSprite> SpriteArchiveReader::ReadPalettedSprite565(
        const std::filesystem::path& archivePath,
        const std::filesystem::path& combinedIndexPath,
        const std::uint16_t spriteIndex,
        const std::filesystem::path& palettePackPath,
        const std::uint16_t paletteIndex,
        const bool alphaPixels) const
    {
        const auto format = alphaPixels ? LegacyAssetFormat::AlphaSpritePack : LegacyAssetFormat::IndexedSpritePack;
        LegacyReadResult<LegacyDecodedSprite> result;
        const auto palette = ReadPalette(palettePackPath, paletteIndex);
        const auto index = ReadIndex(combinedIndexPath, format);
        if (!palette || !index.value || spriteIndex >= index.value->offsets.size()) return result;

        auto sourcePath = archivePath;
        auto offset = index.value->offsets[spriteIndex];
        if (IsDirectory(archivePath))
        {
            const auto header = ReadSplitPackHeader(archivePath);
            if (!header || header->unitCount == 0U) return result;
            const auto splitBase = static_cast<std::uint16_t>(
                (static_cast<std::uint32_t>(spriteIndex) / header->unitCount) * header->unitCount);
            if (splitBase >= index.value->offsets.size() || offset < index.value->offsets[splitBase]) return result;
            sourcePath = SplitChunkPath(archivePath, splitBase);
            offset = offset - index.value->offsets[splitBase] + 2U;
        }
        const auto bytes = ReadAllBytes(sourcePath);
        if (!bytes || offset >= bytes->size()) return result;
        LegacySpriteMetadata metadata;
        metadata.spriteIndex = spriteIndex;
        metadata.offset = offset;
        return DecodePalettedSprite(sourcePath, *bytes, metadata, *palette, alphaPixels);
    }

    LegacyReadResult<LegacyDecodedSprite> SpriteArchiveReader::ReadSplitSprite565(
        const std::filesystem::path& archivePath,
        const std::filesystem::path& combinedIndexPath,
        const std::uint32_t combinedSpriteIndex,
        const std::uint32_t splitBaseSpriteIndex,
        const LegacyAssetFormat format) const
    {
        LegacyReadResult<LegacyDecodedSprite> result;
        const auto atlasStart = std::chrono::steady_clock::now();
        auto combinedIndex = GetCombinedIndex(combinedIndexPath, format);
        metrics_.atlasNanoseconds += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - atlasStart).count());
        combinedIndex.built ? ++metrics_.combinedIndexBuilds : ++metrics_.combinedIndexCacheHits;
        result.issues = std::move(combinedIndex.issues);
        if (!combinedIndex.table)
        {
            return result;
        }

        if (combinedSpriteIndex >= combinedIndex.table->count || splitBaseSpriteIndex >= combinedIndex.table->count)
        {
            result.issues.push_back(MakeIssue(combinedIndexPath, format, combinedSpriteIndex, L"Split sprite index is out of range."));
            return result;
        }

        LegacySpriteMetadata metadata;
        metadata.spriteIndex = combinedSpriteIndex;
        metadata.offset = combinedIndex.table->offsets[combinedSpriteIndex];
        metadata.nextOffset = FindNextCombinedOffset(*combinedIndex.table, combinedSpriteIndex).value_or(0U);
        const auto baseOffset = combinedIndex.table->offsets[splitBaseSpriteIndex];
        const auto archiveSize = ArchiveByteSize(archivePath);
        if (archiveSize == 0)
        {
            result.issues.push_back(MakeIssue(archivePath, format, 0, L"Split sprite archive is missing or empty."));
            return result;
        }

        // The final combined-index entry in a chunk points at the next chunk. The local file adds its own U16 count.
        const auto chunkPayloadEnd = archiveSize > 2U ? baseOffset + archiveSize - 2U : baseOffset;
        if (metadata.nextOffset <= metadata.offset || metadata.nextOffset > chunkPayloadEnd)
            metadata.nextOffset = chunkPayloadEnd;

        if (metadata.offset < baseOffset || metadata.nextOffset <= metadata.offset)
        {
            result.issues.push_back(MakeIssue(combinedIndexPath, format, metadata.offset, L"Split sprite metadata is invalid."));
            return result;
        }

        metadata.offset = metadata.offset - baseOffset + 2U;
        metadata.nextOffset = metadata.nextOffset - baseOffset + 2U;
        metadata.byteSize = metadata.nextOffset - metadata.offset;

        if (metadata.offset >= archiveSize || metadata.nextOffset > archiveSize)
        {
            result.issues.push_back(MakeIssue(archivePath, format, metadata.offset, L"Split sprite offset is outside archive."));
            return result;
        }

        bool cacheHit = false;
        const auto acquireStart = std::chrono::steady_clock::now();
        auto bytes = ReadAllBytes(archivePath, &cacheHit);
        metrics_.archiveAcquireNanoseconds += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - acquireStart).count());
        cacheHit ? ++metrics_.archiveCacheHits : ++metrics_.archiveCacheMisses;
        if (!bytes) return result;
        const auto decodeStart = std::chrono::steady_clock::now();
        auto decoded = DecodeSprite565Payload(archivePath, format, *bytes, metadata);
        metrics_.decodeNanoseconds += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - decodeStart).count());
        return decoded;
    }

    LegacyReadResult<LegacyDecodedSprite> SpriteArchiveReader::ReadSplitIndexedSprite565(
        const std::filesystem::path& archivePath,
        const std::filesystem::path& combinedIndexPath,
        const std::uint32_t combinedSpriteIndex,
        const std::uint32_t splitBaseSpriteIndex,
        const LegacyAssetFormat format,
        const LegacyIndexedColorMapping* colorMapping) const
    {
        if (format == LegacyAssetFormat::ShadowSpritePack)
        {
            return ReadSplitSprite565(
                archivePath,
                combinedIndexPath,
                combinedSpriteIndex,
                splitBaseSpriteIndex,
                format);
        }

        LegacyReadResult<LegacyDecodedSprite> result;
        const auto atlasStart = std::chrono::steady_clock::now();
        auto combinedIndex = GetCombinedIndex(combinedIndexPath, format);
        metrics_.atlasNanoseconds += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - atlasStart).count());
        combinedIndex.built ? ++metrics_.combinedIndexBuilds : ++metrics_.combinedIndexCacheHits;
        result.issues = std::move(combinedIndex.issues);
        if (!combinedIndex.table)
        {
            return result;
        }

        if (combinedSpriteIndex >= combinedIndex.table->count || splitBaseSpriteIndex >= combinedIndex.table->count)
        {
            result.issues.push_back(MakeIssue(combinedIndexPath, format, combinedSpriteIndex, L"Split indexed sprite index is out of range."));
            return result;
        }

        LegacySpriteMetadata metadata;
        metadata.spriteIndex = combinedSpriteIndex;
        metadata.offset = combinedIndex.table->offsets[combinedSpriteIndex];
        metadata.nextOffset = FindNextCombinedOffset(*combinedIndex.table, combinedSpriteIndex).value_or(0U);
        const auto baseOffset = combinedIndex.table->offsets[splitBaseSpriteIndex];
        const auto archiveSize = ArchiveByteSize(archivePath);
        if (archiveSize == 0)
        {
            result.issues.push_back(MakeIssue(archivePath, format, 0, L"Split indexed sprite archive is missing or empty."));
            return result;
        }

        const auto chunkPayloadEnd = archiveSize > 2U ? baseOffset + archiveSize - 2U : baseOffset;
        if (metadata.nextOffset <= metadata.offset || metadata.nextOffset > chunkPayloadEnd)
            metadata.nextOffset = chunkPayloadEnd;

        if (metadata.offset < baseOffset || metadata.nextOffset <= metadata.offset)
        {
            result.issues.push_back(MakeIssue(combinedIndexPath, format, metadata.offset, L"Split indexed sprite metadata is invalid."));
            return result;
        }

        metadata.offset = metadata.offset - baseOffset + 2U;
        metadata.nextOffset = metadata.nextOffset - baseOffset + 2U;
        metadata.byteSize = metadata.nextOffset - metadata.offset;
        if (metadata.offset >= archiveSize || metadata.offset + 4U > archiveSize)
        {
            result.issues.push_back(MakeIssue(archivePath, format, metadata.offset, L"Split indexed sprite offset is outside archive."));
            return result;
        }

        bool cacheHit = false;
        const auto acquireStart = std::chrono::steady_clock::now();
        auto bytes = ReadAllBytes(archivePath, &cacheHit);
        metrics_.archiveAcquireNanoseconds += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - acquireStart).count());
        cacheHit ? ++metrics_.archiveCacheHits : ++metrics_.archiveCacheMisses;
        if (!bytes)
        {
            result.issues.push_back(MakeIssue(archivePath, format, metadata.offset, L"Split indexed sprite archive could not be opened."));
            return result;
        }
        const auto decodeStart = std::chrono::steady_clock::now();
        core::BinaryReader reader(bytes->data(), bytes->size());
        if (!reader.Seek(metadata.offset))
        {
            result.issues.push_back(MakeIssue(archivePath, format, metadata.offset, L"Split indexed sprite offset is outside archive."));
            return result;
        }

        const auto width = reader.ReadU16LE();
        const auto height = reader.ReadU16LE();
        if (!width || !height || *width > kMaxSafeSpriteDimension || *height > kMaxSafeSpriteDimension)
        {
            result.issues.push_back(MakeIssue(archivePath, format, metadata.offset, L"Split indexed sprite header could not be read."));
            return result;
        }

        metadata.width = *width;
        metadata.height = *height;
        metadata.valid = true;

        graphics::ImageData image;
        image.width = *width;
        image.height = *height;
        image.format = graphics::PixelFormat::Rgba8;
        image.pixels.assign(static_cast<std::size_t>(image.width) * image.height * 4U, 0U);

        for (std::uint32_t y = 0; y < image.height; ++y)
        {
            const auto rowWordCount = reader.ReadU16LE();
            if (!rowWordCount || *rowWordCount == 0)
            {
                continue;
            }

            const auto rowStart = reader.Position();
            const auto rowBytes = static_cast<std::size_t>(*rowWordCount) * 2U;
            if (!reader.CanRead(rowBytes))
            {
                result.issues.push_back(MakeIssue(archivePath, format, rowStart, L"Split indexed sprite row payload exceeds archive."));
                return result;
            }

            const auto rowData = reader.ReadBytes(rowBytes);
            if (!rowData || rowData->size() != rowBytes)
            {
                result.issues.push_back(MakeIssue(archivePath, format, rowStart, L"Split indexed sprite row payload could not be read."));
                return result;
            }

            std::vector<std::uint16_t> rowWords;
            rowWords.reserve(*rowWordCount);
            for (std::size_t offset = 0; offset < rowData->size(); offset += 2U)
            {
                rowWords.push_back(ReadU16LEAt(*rowData, offset));
            }

            std::size_t wordIndex = 0;
            if (wordIndex >= rowWords.size())
            {
                continue;
            }

            const auto runCount = rowWords[wordIndex++];
            std::uint32_t x = 0;
            for (std::uint16_t run = 0; run < runCount; ++run)
            {
                if (wordIndex + 1U >= rowWords.size())
                {
                    break;
                }

                x += rowWords[wordIndex++];
                const auto indexedCount = rowWords[wordIndex++];
                for (std::uint16_t color = 0; color < indexedCount && wordIndex < rowWords.size(); ++color)
                {
                    if (x < image.width)
                    {
                        WriteRgb565AsRgba(LegacyIndexedColor(rowWords[wordIndex], colorMapping), image.pixels, static_cast<std::size_t>(y) * image.width + x);
                    }
                    ++wordIndex;
                    ++x;
                }

                if (wordIndex >= rowWords.size())
                {
                    break;
                }

                const auto rawCount = rowWords[wordIndex++];
                for (std::uint16_t color = 0; color < rawCount && wordIndex < rowWords.size(); ++color)
                {
                    if (x < image.width)
                    {
                        WriteRgb565AsRgba(rowWords[wordIndex], image.pixels, static_cast<std::size_t>(y) * image.width + x);
                    }
                    ++wordIndex;
                    ++x;
                }
            }
        }

        result.value = LegacyDecodedSprite{ metadata, std::move(image) };
        metrics_.decodeNanoseconds += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - decodeStart).count());
        return result;
    }
}
