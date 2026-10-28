#include "Content/LegacyQuestCatalog.h"

#include <Windows.h>
#include <Shlwapi.h>
#include <XmlLite.h>
#include <wrl/client.h>

#include <charconv>
#include <algorithm>
#include <cwchar>
#include <string_view>
#include <system_error>

namespace phoenix::content
{
    namespace
    {
        [[nodiscard]] std::string Utf8(const wchar_t* value, const std::uint32_t length)
        {
            if (value == nullptr || length == 0U)
            {
                return {};
            }
            const auto size = WideCharToMultiByte(CP_UTF8, 0, value, static_cast<int>(length), nullptr, 0, nullptr, nullptr);
            std::string result(static_cast<std::size_t>(std::max(0, size)), '\0');
            if (size > 0)
            {
                (void)WideCharToMultiByte(CP_UTF8, 0, value, static_cast<int>(length), result.data(), size, nullptr, nullptr);
            }
            return result;
        }

        [[nodiscard]] std::optional<std::uint32_t> ParseUnsigned(const wchar_t* value)
        {
            if (value == nullptr)
            {
                return std::nullopt;
            }
            const auto narrow = Utf8(value, static_cast<std::uint32_t>(wcslen(value)));
            std::uint32_t result = 0;
            const auto parsed = std::from_chars(narrow.data(), narrow.data() + narrow.size(), result);
            return parsed.ec == std::errc{} && parsed.ptr == narrow.data() + narrow.size()
                ? std::optional<std::uint32_t>{ result }
                : std::nullopt;
        }
    }

    bool LegacyQuestCatalog::Load(const std::filesystem::path& dataRoot)
    {
        quests_.clear();
        const auto textRoot = dataRoot / L"txt";
        const auto eventLoaded = LoadXml(textRoot / L"EventGQuest.xml");
        const auto simpleLoaded = LoadXml(textRoot / L"SimpleGQuest.xml");
        loaded_ = eventLoaded || simpleLoaded;
        return loaded_;
    }

    const LegacyQuestText* LegacyQuestCatalog::Find(const std::uint32_t questId) const noexcept
    {
        const auto found = quests_.find(questId);
        return found == quests_.end() ? nullptr : &found->second;
    }

    bool LegacyQuestCatalog::LoadXml(const std::filesystem::path& path)
    {
        Microsoft::WRL::ComPtr<IStream> stream;
        if (FAILED(SHCreateStreamOnFileEx(path.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &stream)))
        {
            return false;
        }
        Microsoft::WRL::ComPtr<IXmlReader> reader;
        if (FAILED(CreateXmlReader(__uuidof(IXmlReader), reinterpret_cast<void**>(reader.GetAddressOf()), nullptr)) ||
            FAILED(reader->SetInput(stream.Get())))
        {
            return false;
        }

        std::uint32_t currentQuestId = 0;
        std::uint8_t condition = 0xFFU;
        std::uint16_t missionIndex = 0;
        enum class Capture { None, Title, Description, Mission } capture = Capture::None;
        XmlNodeType nodeType = XmlNodeType_None;
        while (reader->Read(&nodeType) == S_OK)
        {
            const wchar_t* localName = nullptr;
            std::uint32_t localNameLength = 0;
            if (nodeType == XmlNodeType_Element)
            {
                if (FAILED(reader->GetLocalName(&localName, &localNameLength)))
                {
                    continue;
                }
                const std::wstring_view name(localName, localNameLength);
                if (name == L"Quest")
                {
                    const wchar_t* id = nullptr;
                    if (reader->MoveToAttributeByName(L"id", nullptr) == S_OK)
                    {
                        (void)reader->GetValue(&id, nullptr);
                        currentQuestId = ParseUnsigned(id).value_or(0U);
                        (void)reader->MoveToElement();
                    }
                }
                else if (name == L"Title" && currentQuestId != 0U)
                {
                    capture = Capture::Title;
                }
                else if (name == L"Happen") condition = 0U;
                else if (name == L"Complete") condition = 1U;
                else if (name == L"Fail") condition = 2U;
                else if (name == L"Reward") condition = 3U;
                else if (name == L"Script" && currentQuestId != 0U && condition == 0xFFU)
                {
                    capture = Capture::Description;
                }
                else if (currentQuestId != 0U && condition != 0xFFU)
                {
                    const wchar_t* index = nullptr;
                    if (reader->MoveToAttributeByName(L"index", nullptr) == S_OK)
                    {
                        (void)reader->GetValue(&index, nullptr);
                        missionIndex = static_cast<std::uint16_t>(ParseUnsigned(index).value_or(0U));
                        capture = missionIndex == 0U ? Capture::None : Capture::Mission;
                        (void)reader->MoveToElement();
                    }
                }
            }
            else if (nodeType == XmlNodeType_Text || nodeType == XmlNodeType_CDATA)
            {
                const wchar_t* value = nullptr;
                std::uint32_t valueLength = 0;
                if (currentQuestId == 0U || FAILED(reader->GetValue(&value, &valueLength)))
                {
                    continue;
                }
                auto& quest = quests_[currentQuestId];
                const auto text = Utf8(value, valueLength);
                if (capture == Capture::Title) quest.title = text;
                else if (capture == Capture::Description && quest.description.empty()) quest.description = text;
                else if (capture == Capture::Mission) quest.missions[{ condition, missionIndex }] = text;
                capture = Capture::None;
            }
            else if (nodeType == XmlNodeType_EndElement)
            {
                if (FAILED(reader->GetLocalName(&localName, &localNameLength)))
                {
                    continue;
                }
                const std::wstring_view name(localName, localNameLength);
                if (name == L"Quest")
                {
                    currentQuestId = 0U;
                    condition = 0xFFU;
                }
                else if (name == L"Happen" || name == L"Complete" || name == L"Fail" || name == L"Reward")
                {
                    condition = 0xFFU;
                }
            }
        }
        return true;
    }
}
