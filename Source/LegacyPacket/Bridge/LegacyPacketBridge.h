#pragma once

#include "Protocol/LegacyProtocol.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace phoenix::legacy_packet
{
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCLLogin(
        const std::string& username,
        const std::string& password,
        const std::array<std::uint8_t, 6>& macAddress,
        std::uint8_t loginMode,
        std::uint32_t realIpAddress);

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCLSelectPC(
        const std::string& pcName,
        protocol::PcType pcType);

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCGConnect(
        std::uint32_t authKey,
        protocol::PcType pcType,
        const std::string& pcName,
        const std::array<std::uint8_t, 6>& macAddress);

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCGMove(
        std::uint8_t x,
        std::uint8_t y,
        std::uint8_t direction);

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCGReady();

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCGUseBonusPoint(
        std::uint8_t which);

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCGSay(
        std::uint32_t color,
        const std::string& message);

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCGGlobalChat(
        std::uint32_t color,
        const std::string& message);

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCGWhisper(
        const std::string& name,
        std::uint32_t color,
        const std::string& message);

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCGPartySay(
        std::uint32_t color,
        const std::string& message);

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCGGuildChat(
        std::uint8_t type,
        std::uint32_t color,
        const std::string& message);

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> EncodeCGRangerSay(
        const std::string& message);

    [[nodiscard]] std::optional<protocol::MoveInfo> DecodeGCMove(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<protocol::SetPosition> DecodeGCMoveOK(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<protocol::SetPosition> DecodeGCMoveError(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<protocol::WorldObjectInfo> DecodeGCAddVampireCorpse(
        const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<protocol::ChatMessage> DecodeGCSay(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<protocol::ChatMessage> DecodeGCGlobalChat(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<protocol::ChatMessage> DecodeGCWhisper(const std::vector<std::uint8_t>& body);
    [[nodiscard]] bool DecodeGCWhisperFailed(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<protocol::ChatMessage> DecodeGCPartySay(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<protocol::ChatMessage> DecodeGCGuildChat(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<protocol::SystemMessage> DecodeGCSystemMessage(const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<protocol::PartyJoinedInfo> DecodeGCPartyJoined(
        const std::vector<std::uint8_t>& body);
    [[nodiscard]] std::optional<protocol::PartyLeaveInfo> DecodeGCPartyLeave(
        const std::vector<std::uint8_t>& body);
}
