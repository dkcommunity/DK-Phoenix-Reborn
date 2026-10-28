#include "LegacyPacket/Bridge/LegacyPacketBridge.h"

#include "Client_PCH.h"

#include "Cpackets/CGConnect.h"
#include "Cpackets/CGGlobalChat.h"
#include "Cpackets/CGGuildChat.h"
#include "Cpackets/CGMove.h"
#include "Cpackets/CGPartySay.h"
#include "Cpackets/CGReady.h"
#include "Cpackets/CGUseBonusPoint.h"
#include "Cpackets/CGRangerSay.h"
#include "Cpackets/CGSay.h"
#include "Cpackets/CGWhisper.h"
#include "Cpackets/CLLogin.h"
#include "Cpackets/CLSelectPC.h"
#include "Gpackets/GCGlobalChat.h"
#include "Gpackets/GCGuildChat.h"
#include "Gpackets/GCPartySay.h"
#include "Gpackets/GCPartyJoined.h"
#include "Gpackets/GCPartyLeave.h"
#include "Gpackets/GCSay.h"
#include "Gpackets/GCSystemMessage.h"
#include "Gpackets/GCWhisper.h"
#include "Gpackets/GCWhisperFailed.h"
#include "Gpackets/GCMove.h"
#include "Gpackets/GCMoveError.h"
#include "Gpackets/GCMoveOK.h"
#include "Gpackets/GCAddVampireCorpse.h"
#include "Packet.h"
#include "SocketInputStream.h"
#include "SocketOutputStream.h"

#include <limits>

namespace phoenix::legacy_packet
{
    namespace
    {
        [[nodiscard]] PCType ToLegacyPcType(const protocol::PcType pcType) noexcept
        {
            switch (pcType)
            {
            case protocol::PcType::Vampire:
                return PC_VAMPIRE;
            case protocol::PcType::Ousters:
                return PC_OUSTERS;
            case protocol::PcType::Slayer:
            default:
                return PC_SLAYER;
            }
        }

        [[nodiscard]] std::optional<std::vector<std::uint8_t>> BuildLegacyPacket(const Packet& packet)
        {
            try
            {
                SocketOutputStream stream(nullptr, 256);
                stream.write(&packet);
                const auto size = stream.length();
                if (size < szPacketID + szPacketSize)
                {
                    return std::nullopt;
                }

                const auto* begin = reinterpret_cast<const std::uint8_t*>(stream.data());
                return std::vector<std::uint8_t>(begin, begin + size);
            }
            catch (Throwable&)
            {
                return std::nullopt;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        template <typename PacketT>
        [[nodiscard]] std::optional<PacketT> ReadLegacyPacketBody(const std::vector<std::uint8_t>& body)
        {
            try
            {
                SocketInputStream stream(nullptr, static_cast<uint>(std::max<std::size_t>(body.size(), 16U)));
                stream.reset(reinterpret_cast<const char*>(body.data()), static_cast<uint>(body.size()));
                PacketT packet;
                packet.read(stream);
                if (!stream.isEmpty())
                {
                    return std::nullopt;
                }
                return packet;
            }
            catch (Throwable&)
            {
                return std::nullopt;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }
    }

    std::optional<std::vector<std::uint8_t>> EncodeCLLogin(
        const std::string& username,
        const std::string& password,
        const std::array<std::uint8_t, 6>& macAddress,
        const std::uint8_t loginMode,
        const std::uint32_t realIpAddress)
    {
        CLLogin packet;
        packet.setID(username);
        packet.setPassword(password);
        packet.setMacAddress(reinterpret_cast<const BYTE*>(macAddress.data()));
        packet.SetLoginMode(loginMode);
        packet.setRealIpaddr(static_cast<int>(std::min<std::uint32_t>(
            realIpAddress,
            static_cast<std::uint32_t>(std::numeric_limits<int>::max()))));
        return BuildLegacyPacket(packet);
    }

    std::optional<std::vector<std::uint8_t>> EncodeCLSelectPC(
        const std::string& pcName,
        const protocol::PcType pcType)
    {
        CLSelectPC packet;
        packet.setPCName(pcName);
        packet.setPCType(ToLegacyPcType(pcType));
        return BuildLegacyPacket(packet);
    }

    std::optional<std::vector<std::uint8_t>> EncodeCGConnect(
        const std::uint32_t authKey,
        const protocol::PcType pcType,
        const std::string& pcName,
        const std::array<std::uint8_t, 6>& macAddress)
    {
        CGConnect packet;
        packet.setKey(authKey);
        packet.setPCType(ToLegacyPcType(pcType));
        packet.setPCName(pcName);
        packet.setMacAddress(reinterpret_cast<const BYTE*>(macAddress.data()));
        return BuildLegacyPacket(packet);
    }

    std::optional<std::vector<std::uint8_t>> EncodeCGMove(
        const std::uint8_t x,
        const std::uint8_t y,
        const std::uint8_t direction)
    {
        CGMove packet;
        packet.setX(x);
        packet.setY(y);
        packet.setDir(direction);
        return BuildLegacyPacket(packet);
    }

    std::optional<std::vector<std::uint8_t>> EncodeCGReady()
    {
        CGReady packet;
        return BuildLegacyPacket(packet);
    }

    std::optional<std::vector<std::uint8_t>> EncodeCGUseBonusPoint(
        const std::uint8_t which)
    {
        CGUseBonusPoint packet;
        packet.setWhich(which);
        return BuildLegacyPacket(packet);
    }

    std::optional<std::vector<std::uint8_t>> EncodeCGSay(
        const std::uint32_t color,
        const std::string& message)
    {
        CGSay packet;
        packet.setColor(color);
        packet.setMessage(message);
        return BuildLegacyPacket(packet);
    }

    std::optional<std::vector<std::uint8_t>> EncodeCGGlobalChat(
        const std::uint32_t color,
        const std::string& message)
    {
        CGGlobalChat packet;
        packet.setColor(color);
        packet.setMessage(message);
        return BuildLegacyPacket(packet);
    }

    std::optional<std::vector<std::uint8_t>> EncodeCGWhisper(
        const std::string& name,
        const std::uint32_t color,
        const std::string& message)
    {
        CGWhisper packet;
        packet.setName(name);
        packet.setColor(color);
        packet.setMessage(message);
        return BuildLegacyPacket(packet);
    }

    std::optional<std::vector<std::uint8_t>> EncodeCGPartySay(
        const std::uint32_t color,
        const std::string& message)
    {
        CGPartySay packet;
        packet.setColor(color);
        packet.setMessage(message);
        return BuildLegacyPacket(packet);
    }

    std::optional<std::vector<std::uint8_t>> EncodeCGGuildChat(
        const std::uint8_t type,
        const std::uint32_t color,
        const std::string& message)
    {
        CGGuildChat packet;
        packet.SetType(type);
        packet.setColor(color);
        packet.setMessage(message);
        return BuildLegacyPacket(packet);
    }

    std::optional<std::vector<std::uint8_t>> EncodeCGRangerSay(const std::string& message)
    {
        CGRangerSay packet;
        packet.setMessage(message);
        return BuildLegacyPacket(packet);
    }

    std::optional<protocol::MoveInfo> DecodeGCMove(const std::vector<std::uint8_t>& body)
    {
        const auto packet = ReadLegacyPacketBody<GCMove>(body);
        if (!packet)
        {
            return std::nullopt;
        }

        protocol::MoveInfo move;
        move.objectId = packet->getObjectID();
        move.x = packet->getX();
        move.y = packet->getY();
        move.direction = packet->getDir();
        return move;
    }

    std::optional<protocol::SetPosition> DecodeGCMoveOK(const std::vector<std::uint8_t>& body)
    {
        const auto packet = ReadLegacyPacketBody<GCMoveOK>(body);
        if (!packet)
        {
            return std::nullopt;
        }

        protocol::SetPosition position;
        position.x = packet->getX();
        position.y = packet->getY();
        position.direction = packet->getDir();
        return position;
    }

    std::optional<protocol::SetPosition> DecodeGCMoveError(const std::vector<std::uint8_t>& body)
    {
        const auto packet = ReadLegacyPacketBody<GCMoveError>(body);
        if (!packet)
        {
            return std::nullopt;
        }

        protocol::SetPosition position;
        position.x = packet->getX();
        position.y = packet->getY();
        return position;
    }

    std::optional<protocol::WorldObjectInfo> DecodeGCAddVampireCorpse(const std::vector<std::uint8_t>& body)
    {
        auto packet = ReadLegacyPacketBody<GCAddVampireCorpse>(body);
        if (!packet)
        {
            return std::nullopt;
        }

        auto& vampire = packet->getVampireInfo();
        protocol::WorldObjectInfo info;
        info.kind = protocol::WorldObjectKind::Player;
        info.pcType = protocol::PcType::Vampire;
        info.objectId = vampire.getObjectID();
        info.name = vampire.getName();
        info.x = vampire.getX();
        info.y = vampire.getY();
        info.direction = vampire.getDir();
        info.sex = static_cast<std::uint8_t>(vampire.getSex());
        info.coatType = static_cast<std::uint8_t>(vampire.getCoatType());
        info.armType = static_cast<std::uint8_t>(vampire.getArmType());
        info.batType = vampire.GetBatType();
        info.vampireColors[0] = vampire.getBatColor();
        info.vampireColors[1] = vampire.getAdvanceBatColor();
        info.vampireColors[2] = vampire.getSkinColor();
        info.vampireColors[3] = vampire.getCoatColor(MAIN_COLOR);
        info.vampireColors[4] = vampire.getCoatColor(SUB_COLOR);
        info.vampireColors[5] = vampire.getArmColor();
        info.vampireColors[6] = vampire.getWingColor();
        info.vampireColors[7] = vampire.getWingEffectColor();
        info.masterEffectColor = vampire.getMasterEffectColor();
        info.born = vampire.getBorn();
        info.currentHp = vampire.getCurrentHP();
        info.maxHp = vampire.getMaxHP();
        info.attackSpeed = vampire.getAttackSpeed();
        info.alignment = vampire.getAlignment();
        info.shape = vampire.getShape();
        info.competence = vampire.getCompetence();
        info.guildId = vampire.getGuildID();
        info.unionId = vampire.getUnionID();
        info.rank = vampire.getRank();
        info.advancementLevel = vampire.getAdvancementLevel();
        info.contributePoint = vampire.getContributePoint();
        info.dead = true;
        info.raw = body;
        return info;
    }

    std::optional<protocol::ChatMessage> DecodeGCSay(const std::vector<std::uint8_t>& body)
    {
        const auto packet = ReadLegacyPacketBody<GCSay>(body);
        if (!packet)
        {
            return std::nullopt;
        }

        protocol::ChatMessage message;
        message.objectId = packet->getObjectID();
        message.color = packet->getColor();
        message.message = packet->getMessage();
        return message;
    }

    std::optional<protocol::ChatMessage> DecodeGCGlobalChat(const std::vector<std::uint8_t>& body)
    {
        const auto packet = ReadLegacyPacketBody<GCGlobalChat>(body);
        if (!packet)
        {
            return std::nullopt;
        }

        protocol::ChatMessage message;
        message.color = packet->getColor();
        message.message = packet->getMessage();
        message.race = static_cast<std::uint8_t>(packet->getRace());
        message.global = true;
        return message;
    }

    std::optional<protocol::ChatMessage> DecodeGCWhisper(const std::vector<std::uint8_t>& body)
    {
        const auto packet = ReadLegacyPacketBody<GCWhisper>(body);
        if (!packet)
        {
            return std::nullopt;
        }

        protocol::ChatMessage message;
        message.senderName = packet->getName();
        message.color = packet->getColor();
        message.message = packet->getMessage();
        message.race = static_cast<std::uint8_t>(packet->getRace());
        return message;
    }

    bool DecodeGCWhisperFailed(const std::vector<std::uint8_t>& body)
    {
        return ReadLegacyPacketBody<GCWhisperFailed>(body).has_value();
    }

    std::optional<protocol::ChatMessage> DecodeGCPartySay(const std::vector<std::uint8_t>& body)
    {
        const auto packet = ReadLegacyPacketBody<GCPartySay>(body);
        if (!packet)
        {
            return std::nullopt;
        }

        protocol::ChatMessage message;
        message.senderName = packet->getName();
        message.color = packet->getColor();
        message.message = packet->getMessage();
        return message;
    }

    std::optional<protocol::ChatMessage> DecodeGCGuildChat(const std::vector<std::uint8_t>& body)
    {
        const auto packet = ReadLegacyPacketBody<GCGuildChat>(body);
        if (!packet)
        {
            return std::nullopt;
        }

        protocol::ChatMessage message;
        message.guildChatType = packet->getType();
        message.guildName = packet->getSendGuildName();
        message.senderName = packet->getSender();
        message.color = packet->getColor();
        message.message = packet->getMessage();
        return message;
    }

    std::optional<protocol::SystemMessage> DecodeGCSystemMessage(const std::vector<std::uint8_t>& body)
    {
        const auto packet = ReadLegacyPacketBody<GCSystemMessage>(body);
        if (!packet)
        {
            return std::nullopt;
        }

        protocol::SystemMessage message;
        message.message = packet->getMessage();
        message.color = packet->getColor();
        message.type = static_cast<std::uint8_t>(packet->getType());
        return message;
    }

    std::optional<protocol::PartyJoinedInfo> DecodeGCPartyJoined(const std::vector<std::uint8_t>& body)
    {
        try
        {
            SocketInputStream stream(nullptr, static_cast<uint>(std::max<std::size_t>(body.size(), 16U)));
            stream.reset(reinterpret_cast<const char*>(body.data()), static_cast<uint>(body.size()));
            GCPartyJoined packet;
            packet.read(stream);
            if (!stream.isEmpty())
            {
                return std::nullopt;
            }

            protocol::PartyJoinedInfo result;
            result.members.reserve(packet.getMemberInfoCount());
            while (auto* member = packet.popMemberInfo())
            {
                result.members.push_back(protocol::PartyMemberInfo{
                    member->name,
                    member->sex,
                    member->hair_style,
                    member->ip
                });
                delete member;
            }
            return result;
        }
        catch (Throwable&)
        {
            return std::nullopt;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<protocol::PartyLeaveInfo> DecodeGCPartyLeave(const std::vector<std::uint8_t>& body)
    {
        const auto packet = ReadLegacyPacketBody<GCPartyLeave>(body);
        if (!packet)
        {
            return std::nullopt;
        }
        return protocol::PartyLeaveInfo{ packet->getExpeller(), packet->getExpellee() };
    }
}
