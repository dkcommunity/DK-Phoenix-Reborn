#include "Network/TcpClient.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <mmsystem.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <limits>

namespace phoenix::network
{
    namespace
    {
        constexpr std::uint32_t kMaxPacketBodySize = 4U * 1024U * 1024U;

        [[nodiscard]] bool EnsureWinsock()
        {
            static const bool initialized = [] {
                WSADATA data{};
                return WSAStartup(MAKEWORD(2, 2), &data) == 0;
            }();
            return initialized;
        }

        [[nodiscard]] std::wstring WsaErrorMessage(const wchar_t* prefix)
        {
            return std::wstring(prefix) + L" WSA error " + std::to_wstring(WSAGetLastError()) + L".";
        }

        [[nodiscard]] std::uint16_t ReadU16LE(const std::uint8_t* bytes) noexcept
        {
            return static_cast<std::uint16_t>(bytes[0]) |
                static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[1]) << 8U);
        }

        [[nodiscard]] std::uint32_t ReadU32LE(const std::uint8_t* bytes) noexcept
        {
            return static_cast<std::uint32_t>(bytes[0]) |
                (static_cast<std::uint32_t>(bytes[1]) << 8U) |
                (static_cast<std::uint32_t>(bytes[2]) << 16U) |
                (static_cast<std::uint32_t>(bytes[3]) << 24U);
        }

        void AppendU32LE(std::vector<std::uint8_t>& bytes, const std::uint32_t value)
        {
            bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
        }

        void AppendU16LE(std::vector<std::uint8_t>& bytes, const std::uint16_t value)
        {
            bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
        }

        constexpr std::size_t kLegacyOutgoingPacketHeaderSize = 6U;
        constexpr std::size_t kLegacyIncomingPacketHeaderSize = 6U;
    }

    TcpClient::~TcpClient()
    {
        Stop();
    }

    bool TcpClient::Start(std::string host, const std::uint16_t port)
    {
        Stop();
        if (!EnsureWinsock())
        {
            PushEvent(TcpEvent{ TcpEventType::Error, protocol::PacketId::CLLogin, {}, L"WSAStartup failed." });
            return false;
        }

        running_ = true;
        worker_ = std::thread(&TcpClient::Worker, this, std::move(host), port);
        return true;
    }

    void TcpClient::Stop() noexcept
    {
        running_ = false;
        connected_ = false;
        const auto rawSocket = static_cast<SOCKET>(socket_.load());
        if (rawSocket != INVALID_SOCKET && rawSocket != 0)
        {
            shutdown(rawSocket, SD_BOTH);
            closesocket(rawSocket);
            socket_.store(0);
        }

        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    bool TcpClient::Send(const std::vector<std::uint8_t>& packet)
    {
        if (!connected_ || packet.size() < kLegacyOutgoingPacketHeaderSize)
        {
            return false;
        }

        std::lock_guard lock(sendMutex_);
        const auto encryptionCode = encryptionCode_.load();
        const auto includeTimestamp = includeTimestamp_.load();
        const auto packetId = static_cast<protocol::PacketId>(ReadU16LE(packet.data()));
        const auto reportedPacketSize = ReadU32LE(packet.data() + 2U);
        std::vector<std::uint8_t> body(
            packet.begin() + static_cast<std::ptrdiff_t>(kLegacyOutgoingPacketHeaderSize),
            packet.end());

        if (encryptionCode >= 0)
        {
            if (!protocol::ApplyLegacyOutgoingEncryption(packetId, body, static_cast<std::uint8_t>(encryptionCode & 0xFF)))
            {
                PushEvent(TcpEvent{ TcpEventType::Error, packetId, {}, L"Legacy outgoing packet encryption failed." });
                return false;
            }
        }

        std::vector<std::uint8_t> outgoing;
        outgoing.reserve(kLegacyOutgoingPacketHeaderSize + (includeTimestamp ? sizeof(DWORD) : 0U) + body.size());
        AppendU16LE(outgoing, static_cast<std::uint16_t>(packetId));
        AppendU32LE(outgoing, reportedPacketSize);
        if (includeTimestamp)
        {
            AppendU32LE(outgoing, static_cast<std::uint32_t>(timeGetTime()));
        }
        outgoing.insert(outgoing.end(), body.begin(), body.end());

        const auto rawSocket = static_cast<SOCKET>(socket_.load());
        std::size_t sent = 0;
        while (sent < outgoing.size())
        {
            const auto chunk = static_cast<int>(std::min<std::size_t>(outgoing.size() - sent, static_cast<std::size_t>(std::numeric_limits<int>::max())));
            const auto result = send(rawSocket, reinterpret_cast<const char*>(outgoing.data() + sent), chunk, 0);
            if (result <= 0)
            {
                PushEvent(TcpEvent{ TcpEventType::Error, protocol::PacketId::CLLogin, {}, WsaErrorMessage(L"Socket send failed.") });
                return false;
            }
            sent += static_cast<std::size_t>(result);
        }
        PushEvent(TcpEvent{
            TcpEventType::PacketSent,
            packetId,
            {},
            {},
            reportedPacketSize,
            encryptionCode >= 0
        });
        return true;
    }

    void TcpClient::SetEncryptionCode(const std::optional<std::uint8_t> code) noexcept
    {
        encryptionCode_.store(code ? static_cast<int>(*code) : -1);
    }

    void TcpClient::SetLegacyTransportConfig(const LegacyTransportConfig config) noexcept
    {
        includeTimestamp_.store(config.includeTimestamp);
    }

    std::optional<TcpEvent> TcpClient::PollEvent()
    {
        std::lock_guard lock(eventMutex_);
        if (events_.empty())
        {
            return std::nullopt;
        }

        auto event = std::move(events_.front());
        events_.pop();
        return event;
    }

    bool TcpClient::IsRunning() const noexcept
    {
        return running_;
    }

    void TcpClient::Worker(std::string host, const std::uint16_t port)
    {
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* addresses = nullptr;
        const auto service = std::to_string(port);
        if (getaddrinfo(host.c_str(), service.c_str(), &hints, &addresses) != 0)
        {
            PushEvent(TcpEvent{ TcpEventType::Error, protocol::PacketId::CLLogin, {}, L"Login endpoint could not be resolved." });
            running_ = false;
            return;
        }

        SOCKET connectedSocket = INVALID_SOCKET;
        for (auto* address = addresses; address != nullptr && running_; address = address->ai_next)
        {
            const auto candidate = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
            if (candidate == INVALID_SOCKET)
            {
                continue;
            }

            u_long nonBlocking = 1;
            (void)ioctlsocket(candidate, FIONBIO, &nonBlocking);
            const auto connectResult = connect(candidate, address->ai_addr, static_cast<int>(address->ai_addrlen));
            if (connectResult == 0 || WSAGetLastError() == WSAEWOULDBLOCK)
            {
                fd_set writeSet;
                FD_ZERO(&writeSet);
                FD_SET(candidate, &writeSet);
                timeval timeout{};
                timeout.tv_sec = 5;
                timeout.tv_usec = 0;
                const auto selectResult = select(0, nullptr, &writeSet, nullptr, &timeout);
                if (selectResult > 0)
                {
                    int socketError = 0;
                    int socketErrorSize = sizeof(socketError);
                    if (getsockopt(candidate, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&socketError), &socketErrorSize) == 0 && socketError == 0)
                    {
                        connectedSocket = candidate;
                        break;
                    }
                }
            }

            closesocket(candidate);
        }
        freeaddrinfo(addresses);

        if (connectedSocket == INVALID_SOCKET)
        {
            PushEvent(TcpEvent{ TcpEventType::Error, protocol::PacketId::CLLogin, {}, L"Server connection timed out or was refused." });
            running_ = false;
            return;
        }

        u_long blocking = 0;
        (void)ioctlsocket(connectedSocket, FIONBIO, &blocking);
        socket_.store(static_cast<std::uintptr_t>(connectedSocket));
        connected_ = true;
        PushEvent(TcpEvent{ TcpEventType::Connected });

        while (running_)
        {
            std::array<std::uint8_t, kLegacyIncomingPacketHeaderSize> header{};
            std::wstring receiveError;
            if (!ReceiveExact(header.data(), header.size(), &receiveError))
            {
                if (running_)
                {
                    PushEvent(TcpEvent{
                        TcpEventType::Error,
                        protocol::PacketId::CLLogin,
                        {},
                        receiveError.empty() ? L"Socket receive failed while reading packet header." : receiveError
                    });
                }
                break;
            }

            const auto packetId = static_cast<protocol::PacketId>(ReadU16LE(header.data()));
            const auto packetSize = ReadU32LE(header.data() + 2U);
            const auto wireBodySize = protocol::LegacyIncomingWireBodySize(packetId, packetSize);
            if (wireBodySize > kMaxPacketBodySize)
            {
                PushEvent(TcpEvent{ TcpEventType::Error, packetId, {}, L"Malformed packet size received." });
                break;
            }

            std::vector<std::uint8_t> body(wireBodySize);
            if (wireBodySize > 0U && !ReceiveExact(body.data(), body.size(), &receiveError))
            {
                if (running_)
                {
                    PushEvent(TcpEvent{
                        TcpEventType::Error,
                        packetId,
                        {},
                        (receiveError.empty() ? L"Socket receive failed while reading packet body." : receiveError) +
                            L" Packet id=" + std::to_wstring(static_cast<std::uint16_t>(packetId)) +
                            L" size=" + std::to_wstring(wireBodySize) + L"."
                    });
                }
                break;
            }

            bool malformedBodySize = false;
            while (true)
            {
                const auto requiredBodySize = protocol::ProbeLegacyIncomingRequiredBodySize(packetId, body);
                if (!requiredBodySize || *requiredBodySize > kMaxPacketBodySize)
                {
                    PushEvent(TcpEvent{ TcpEventType::Error, packetId, {}, L"Malformed legacy packet body extent." });
                    malformedBodySize = true;
                    break;
                }
                if (*requiredBodySize <= body.size())
                {
                    break;
                }

                const auto previousSize = body.size();
                body.resize(*requiredBodySize);
                if (!ReceiveExact(body.data() + previousSize, body.size() - previousSize, &receiveError))
                {
                    PushEvent(TcpEvent{
                        TcpEventType::Error,
                        packetId,
                        {},
                        (receiveError.empty() ? L"Socket receive failed while extending legacy packet body." : receiveError) +
                            L" Packet id=" + std::to_wstring(static_cast<std::uint16_t>(packetId)) +
                            L" size=" + std::to_wstring(body.size()) + L"."
                    });
                    malformedBodySize = true;
                    break;
                }
            }
            if (malformedBodySize)
            {
                break;
            }

            const auto encryptionCode = encryptionCode_.load();
            if (encryptionCode >= 0 &&
                !protocol::ApplyLegacyIncomingEncryption(packetId, body, static_cast<std::uint8_t>(encryptionCode & 0xFF)))
            {
                PushEvent(TcpEvent{ TcpEventType::Error, packetId, {}, L"Legacy incoming packet encryption failed." });
                break;
            }

            const auto receivedBodySize = static_cast<std::uint32_t>(body.size());
            PushEvent(TcpEvent{
                TcpEventType::Packet,
                packetId,
                std::move(body),
                {},
                receivedBodySize,
                encryptionCode >= 0,
                packetSize
            });
        }

        connected_ = false;
        running_ = false;
        const auto rawSocket = static_cast<SOCKET>(socket_.load());
        if (rawSocket != INVALID_SOCKET && rawSocket != 0)
        {
            closesocket(rawSocket);
            socket_.store(0);
        }
        PushEvent(TcpEvent{ TcpEventType::Disconnected });
    }

    void TcpClient::PushEvent(TcpEvent event)
    {
        std::lock_guard lock(eventMutex_);
        events_.push(std::move(event));
    }

    bool TcpClient::ReceiveExact(std::uint8_t* buffer, const std::size_t size, std::wstring* errorMessage)
    {
        std::size_t received = 0;
        const auto rawSocket = static_cast<SOCKET>(socket_.load());
        while (running_ && received < size)
        {
            const auto chunk = static_cast<int>(std::min<std::size_t>(size - received, static_cast<std::size_t>(std::numeric_limits<int>::max())));
            const auto result = recv(rawSocket, reinterpret_cast<char*>(buffer + received), chunk, 0);
            if (result == 0)
            {
                if (errorMessage != nullptr)
                {
                    *errorMessage = L"Socket closed by remote peer.";
                }
                return false;
            }
            if (result < 0)
            {
                const auto error = WSAGetLastError();
                if (error == WSAEINTR)
                {
                    continue;
                }
                if (errorMessage != nullptr)
                {
                    *errorMessage = WsaErrorMessage(L"Socket receive failed.");
                }
                return false;
            }
            received += static_cast<std::size_t>(result);
        }
        if (received != size && errorMessage != nullptr)
        {
            *errorMessage = L"Socket receive stopped before requested bytes were read.";
        }
        return received == size;
    }
}
