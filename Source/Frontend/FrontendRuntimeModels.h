#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace phoenix::frontend
{
    struct ServerEntry
    {
        std::wstring name;
        std::wstring status;
        std::uint8_t id{ 0 };
    };

    struct ChannelEntry
    {
        std::wstring name;
        std::wstring status;
        std::uint8_t id{ 0 };
        std::uint32_t slayerCount{ 0 };
        std::uint32_t vampireCount{ 0 };
        std::uint32_t oustersCount{ 0 };
    };

    struct ServerSelectModel
    {
        std::array<ServerEntry, 2> servers{
            ServerEntry{ L"Phoenix", L"Online" },
            ServerEntry{ L"Test", L"Closed" }
        };
        std::array<ChannelEntry, 3> channels{
            ChannelEntry{ L"Channel 1", L"Normal", 120, 55, 25 },
            ChannelEntry{ L"Channel 2", L"Busy", 40, 95, 20 },
            ChannelEntry{ L"Channel 3", L"Normal", 20, 35, 80 }
        };
        std::uint32_t selectedServer{ 0 };
        std::uint32_t selectedChannel{ 0 };
        std::uint32_t serverCount{ 2 };
        std::uint32_t channelCount{ 3 };
    };

    enum class RuntimePcType
    {
        Slayer,
        Vampire,
        Ousters
    };

    struct CharacterSlot
    {
        std::wstring name;
        std::wstring race;
        std::wstring job;
        RuntimePcType pcType{ RuntimePcType::Slayer };
        std::uint16_t level{ 1 };
        bool occupied{ false };
    };

    struct CharacterSelectModel
    {
        std::array<CharacterSlot, 3> slots{
            CharacterSlot{ L"Slayer", L"Slayer", L"Blade", RuntimePcType::Slayer, 1, true },
            CharacterSlot{ L"Vampire", L"Vampire", L"Blood", RuntimePcType::Vampire, 1, true },
            CharacterSlot{ L"Empty", L"", L"", RuntimePcType::Slayer, 1, false }
        };
        std::uint32_t selectedSlot{ 0 };
    };

    enum class CreateRace
    {
        Slayer,
        Vampire,
        Ousters
    };

    enum class CreateGender
    {
        Male,
        Female
    };

    enum class CreateClass
    {
        Blade,
        Sword,
        Gun,
        Heal,
        Enchant,
        Vampire,
        OustersCombat
    };

    struct CharacterCreateModel
    {
        std::wstring name;
        CreateRace race{ CreateRace::Slayer };
        CreateGender gender{ CreateGender::Male };
        CreateClass selectedClass{ CreateClass::Blade };
        std::uint32_t direction{ 2 };
        std::uint32_t hairColor{ 0 };
        std::uint32_t skinColor{ 0 };
        std::uint32_t bonusStatPoints{ 0 };
    };
}
