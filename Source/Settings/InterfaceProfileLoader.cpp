#include "Settings/InterfaceProfileLoader.h"
#include "Diagnostics/PhoenixDiagnostics.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>

namespace phoenix::settings
{
    namespace
    {
        using SectionMap = std::map<std::string, std::map<std::string, std::string>>;

        std::string Trim(const std::string& value)
        {
            const auto begin = std::find_if_not(value.begin(), value.end(), [](const unsigned char ch) { return std::isspace(ch) != 0; });
            const auto end = std::find_if_not(value.rbegin(), value.rend(), [](const unsigned char ch) { return std::isspace(ch) != 0; }).base();
            if (begin >= end)
            {
                return {};
            }

            return std::string(begin, end);
        }

        DisplayResolution ParseResolution(const std::string& value)
        {
            DisplayResolution result;
            const auto separator = value.find('x');
            if (separator == std::string::npos)
            {
                return result;
            }

            result.width = static_cast<std::uint32_t>(std::stoul(value.substr(0, separator)));
            result.height = static_cast<std::uint32_t>(std::stoul(value.substr(separator + 1)));
            return result;
        }

        bool ParseBool(const std::string& value)
        {
            return value == "true" || value == "1" || value == "True" || value == "TRUE";
        }

        std::vector<std::uint32_t> ParseIntegerList(const std::string& value)
        {
            std::vector<std::uint32_t> result;
            std::stringstream stream(value);
            std::string token;
            while (std::getline(stream, token, ','))
            {
                token = Trim(token);
                if (!token.empty())
                {
                    result.push_back(static_cast<std::uint32_t>(std::stoul(token)));
                }
            }
            return result;
        }

        std::wstring ToWide(const std::string& value)
        {
            return std::wstring(value.begin(), value.end());
        }

        const std::string* FindValue(const SectionMap& sections, const std::string& section, const std::string& key)
        {
            const auto sectionIt = sections.find(section);
            if (sectionIt == sections.end())
            {
                return nullptr;
            }

            const auto valueIt = sectionIt->second.find(key);
            if (valueIt == sectionIt->second.end())
            {
                return nullptr;
            }

            return &valueIt->second;
        }

        SectionMap ReadSections(const std::filesystem::path& path)
        {
            SectionMap sections;
            std::ifstream file(path);
            std::string section;
            std::string line;
            while (std::getline(file, line))
            {
                line = Trim(line);
                if (line.empty() || line[0] == ';' || line[0] == '#')
                {
                    continue;
                }

                if (line.front() == '[' && line.back() == ']')
                {
                    section = line.substr(1, line.size() - 2);
                    sections.try_emplace(section);
                    continue;
                }

                const auto separator = line.find('=');
                if (separator == std::string::npos || section.empty())
                {
                    continue;
                }

                sections[section][Trim(line.substr(0, separator))] = Trim(line.substr(separator + 1));
            }

            return sections;
        }

        std::int32_t ReadInt(const SectionMap& sections, const std::string& section, const std::string& key, const std::int32_t fallback)
                {
                    if (const auto value = FindValue(sections, section, key))
                    {
                        try
                        {
                            return static_cast<std::int32_t>(std::stol(Trim(*value)));
                        }
                        catch (...)
                        {
                            return fallback;
                        }
                    }
                    return fallback;
                }

                bool ReadBool(const SectionMap& sections, const std::string& section, const std::string& key, const bool fallback)
                {
                    if (const auto value = FindValue(sections, section, key))
                    {
                        return ParseBool(Trim(*value));
                    }
                    return fallback;
                }

                // [GameHpBar_1280x960] — hpbar.spk sprite offset'leri
                GameHpBarProfile ReadGameHpBarProfile(const SectionMap& sections)
                {
                    constexpr const char* kSection = "GameHpBar_1280x960";
                    GameHpBarProfile profile;
                    // ← BURAYA EKLE (kSection tanımının hemen altına)
                    PHX_LOG_INFO(L"[INTERFACE] Section [GameHpBar_1280x960] found: " +
                        std::to_wstring(sections.count(kSection) > 0));

                    // Ayrıca her key okumadan önce log isteğe bağlı:
                    PHX_LOG_INFO(L"[INTERFACE] Reading PositionX from section: " + std::to_wstring(sections.count(kSection) > 0 && sections.at(kSection).count("PositionX") > 0));

                    // ── Genel pencere ──
                    profile.positionX = ReadInt(sections, kSection, "PositionX", profile.positionX);
                    profile.positionY = ReadInt(sections, kSection, "PositionY", profile.positionY);
                    profile.defaultWidthMode = ReadBool(sections, kSection, "DefaultWidthMode", profile.defaultWidthMode);
                    profile.defaultSmallMode = ReadBool(sections, kSection, "DefaultSmallMode", profile.defaultSmallMode);
                    profile.draggable = ReadBool(sections, kSection, "Draggable", profile.draggable);
                    profile.panelWidth = ReadInt(sections, kSection, "PanelWidth", profile.panelWidth);
                    profile.panelHeight = ReadInt(sections, kSection, "PanelHeight", profile.panelHeight);

                    // ── Büyük mod sprite offset'leri ──
                    profile.leftEdgeOffsetX = ReadInt(sections, kSection, "LeftEdgeOffsetX", profile.leftEdgeOffsetX);       // sprite 7
                    profile.leftEdgeOffsetY = ReadInt(sections, kSection, "LeftEdgeOffsetY", profile.leftEdgeOffsetY);
                    profile.centerOffsetX = ReadInt(sections, kSection, "CenterOffsetX", profile.centerOffsetX);            // sprite 40/41/44/45/48
                    profile.centerOffsetY = ReadInt(sections, kSection, "CenterOffsetY", profile.centerOffsetY);
                    profile.rightEdgeGapX = ReadInt(sections, kSection, "RightEdgeGapX", profile.rightEdgeGapX);            // sprite 9
                    profile.rightEdgeOffsetY = ReadInt(sections, kSection, "RightEdgeOffsetY", profile.rightEdgeOffsetY);
                    profile.greyBarOffsetX = ReadInt(sections, kSection, "GreyBarOffsetX", profile.greyBarOffsetX);         // sprite 11
                    profile.greyBarOffsetY = ReadInt(sections, kSection, "GreyBarOffsetY", profile.greyBarOffsetY);
                    profile.hpFillOffsetX = ReadInt(sections, kSection, "HpFillOffsetX", profile.hpFillOffsetX);            // sprite 8
                    profile.hpFillOffsetY = ReadInt(sections, kSection, "HpFillOffsetY", profile.hpFillOffsetY);
                    profile.mpFillOffsetX = ReadInt(sections, kSection, "MpFillOffsetX", profile.mpFillOffsetX);            // sprite 10
                    profile.mpFillOffsetY = ReadInt(sections, kSection, "MpFillOffsetY", profile.mpFillOffsetY);

                    // ── Küçük mod sprite offset'leri ──
                    profile.smallLeftEdgeOffsetX = ReadInt(sections, kSection, "SmallLeftEdgeOffsetX", profile.smallLeftEdgeOffsetX);   // sprite 12
                    profile.smallLeftEdgeOffsetY = ReadInt(sections, kSection, "SmallLeftEdgeOffsetY", profile.smallLeftEdgeOffsetY);
                    profile.smallHpFillOffsetX = ReadInt(sections, kSection, "SmallHpFillOffsetX", profile.smallHpFillOffsetX);         // sprite 13
                    profile.smallHpFillOffsetY = ReadInt(sections, kSection, "SmallHpFillOffsetY", profile.smallHpFillOffsetY);
                    profile.smallRightEdgeOffsetX = ReadInt(sections, kSection, "SmallRightEdgeOffsetX", profile.smallRightEdgeOffsetX); // sprite 14
                    profile.smallRightEdgeOffsetY = ReadInt(sections, kSection, "SmallRightEdgeOffsetY", profile.smallRightEdgeOffsetY);
                    profile.smallMpFillOffsetX = ReadInt(sections, kSection, "SmallMpFillOffsetX", profile.smallMpFillOffsetX);         // sprite 15
                    profile.smallMpFillOffsetY = ReadInt(sections, kSection, "SmallMpFillOffsetY", profile.smallMpFillOffsetY);
                    profile.smallGreyBarOffsetX = ReadInt(sections, kSection, "SmallGreyBarOffsetX", profile.smallGreyBarOffsetX);      // sprite 16
                    profile.smallGreyBarOffsetY = ReadInt(sections, kSection, "SmallGreyBarOffsetY", profile.smallGreyBarOffsetY);
                    profile.smallCenterOffsetX = ReadInt(sections, kSection, "SmallCenterOffsetX", profile.smallCenterOffsetX);         // sprite 40/41/44/45/48
                    profile.smallCenterOffsetY = ReadInt(sections, kSection, "SmallCenterOffsetY", profile.smallCenterOffsetY);

                    // ── Butonlar + Level bar ──
                    profile.buttonOffsetX = ReadInt(sections, kSection, "ButtonOffsetX", profile.buttonOffsetX);            // sprite 1-6
                    profile.buttonOffsetY = ReadInt(sections, kSection, "ButtonOffsetY", profile.buttonOffsetY);
                    profile.levelBarOffsetX = ReadInt(sections, kSection, "LevelBarOffsetX", profile.levelBarOffsetX);      // sprite 0
                    profile.levelBarOffsetY = ReadInt(sections, kSection, "LevelBarOffsetY", profile.levelBarOffsetY);

                    // ── Metin offset'leri ──
                    profile.textOffsetY = ReadInt(sections, kSection, "TextOffsetY", profile.textOffsetY);
                    profile.hpTextOffsetX = ReadInt(sections, kSection, "HpTextOffsetX", profile.hpTextOffsetX);
                    profile.mpTextOffsetX = ReadInt(sections, kSection, "MpTextOffsetX", profile.mpTextOffsetX);
                    profile.levelLabelOffsetX = ReadInt(sections, kSection, "LevelLabelOffsetX", profile.levelLabelOffsetX);

                    return profile;
                                    }

                                    InterfaceResolutionProfile ReadProfile(const SectionMap& sections, const std::string& sectionName)
                                    {
                                        InterfaceResolutionProfile profile;
                                        if (const auto width = FindValue(sections, sectionName, "Width"))
                                        {
                                            profile.resolution.width = static_cast<std::uint32_t>(std::stoul(*width));
                                        }
                                        if (const auto height = FindValue(sections, sectionName, "Height"))
                                        {
                                            profile.resolution.height = static_cast<std::uint32_t>(std::stoul(*height));
                                        }
                                        if (const auto canvas = FindValue(sections, sectionName, "ReferenceCanvas"))
                                        {
                                            profile.referenceCanvas = ParseResolution(*canvas);
                                        }
                                        if (const auto scalePolicy = FindValue(sections, sectionName, "ScalePolicy"))
                                        {
                                            profile.scalePolicy = ToWide(*scalePolicy);
                                        }
                                        if (const auto backgroundPolicy = FindValue(sections, sectionName, "BackgroundPolicy"))
                                        {
                                            profile.backgroundPolicy = ToWide(*backgroundPolicy);
                                        }
                                        if (const auto background = FindValue(sections, sectionName, "Background"))
                                        {
                                            profile.loginBackgroundSprites = ParseIntegerList(*background);
                                        }
                                        if (const auto swapChainOnly = FindValue(sections, sectionName, "SwapChainOnly"))
                                        {
                                            profile.swapChainOnly = ParseBool(*swapChainOnly);
                                        }
                                        if (const auto recreateRendererState = FindValue(sections, sectionName, "RecreateRendererState"))
                                        {
                                            profile.recreateRendererState = ParseBool(*recreateRendererState);
                                        }

                                        return profile;
                                    }
                                }

    RuntimeInterfaceProfile InterfaceProfileLoader::Load(const std::filesystem::path& path)
    {
        RuntimeInterfaceProfile result;
        const auto sections = ReadSections(path);
        if (const auto defaultResolution = FindValue(sections, "Interface", "DefaultResolution"))
        {
            result.defaultResolution = ParseResolution(*defaultResolution);
        }

        for (std::uint32_t index = 0; index < 2; ++index)
        {
            std::ostringstream key;
            key << "SupportedResolution" << index;
            if (const auto supported = FindValue(sections, "Interface", key.str()))
            {
                result.supportedResolutions.push_back(ParseResolution(*supported));
            }
        }

        result.profiles.push_back(ReadProfile(sections, "Resolution_1920x1080"));
                result.profiles.push_back(ReadProfile(sections, "Resolution_1024x768"));

                // ── Ingame UI profiles ──
                result.ingameUi.hpBar = ReadGameHpBarProfile(sections);

                result.validation = Validate(result);
        return result;
    }

    InterfaceProfileValidation InterfaceProfileLoader::Validate(const RuntimeInterfaceProfile& profile)
    {
        InterfaceProfileValidation validation;
        if (profile.defaultResolution.width != 1920 || profile.defaultResolution.height != 1080)
        {
            validation.errors.push_back(L"DefaultResolution must be 1920x1080.");
        }
        if (profile.supportedResolutions.size() != 2)
        {
            validation.errors.push_back(L"SupportedResolutionCount must resolve to exactly two entries.");
        }
        if (profile.profiles.size() != 2)
        {
            validation.errors.push_back(L"Exactly two resolution profiles are required.");
        }

        const auto hasResolution = [](const auto& collection, const std::uint32_t width, const std::uint32_t height) {
            return std::any_of(collection.begin(), collection.end(), [width, height](const auto& item) {
                return item.resolution.width == width && item.resolution.height == height;
            });
        };
        const auto hasSupported = [](const auto& collection, const std::uint32_t width, const std::uint32_t height) {
            return std::any_of(collection.begin(), collection.end(), [width, height](const DisplayResolution item) {
                return item.width == width && item.height == height;
            });
        };

        if (!hasSupported(profile.supportedResolutions, 1920, 1080) || !hasSupported(profile.supportedResolutions, 1024, 768))
        {
            validation.errors.push_back(L"Supported resolutions must be 1920x1080 and 1024x768.");
        }
        if (!hasResolution(profile.profiles, 1920, 1080) || !hasResolution(profile.profiles, 1024, 768))
        {
            validation.errors.push_back(L"Resolution profiles must be 1920x1080 and 1024x768.");
        }
        for (const auto& resolutionProfile : profile.profiles)
        {
            if (!DisplayResolutionCatalog::IsSupported(resolutionProfile.resolution.width, resolutionProfile.resolution.height))
            {
                validation.errors.push_back(L"Unsupported resolution profile found.");
            }
            if (!resolutionProfile.swapChainOnly || resolutionProfile.recreateRendererState)
            {
                validation.errors.push_back(L"Resolution profiles must update swap chain and layout without recreating renderer state.");
            }
        }

        validation.valid = validation.errors.empty();
        return validation;
    }
}
