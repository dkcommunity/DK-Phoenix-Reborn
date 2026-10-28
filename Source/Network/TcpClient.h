#pragma once

#include "Protocol/LegacyProtocol.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace phoenix::network
{
    enum class TcpEventType
    {
        Connected,
        Packet,
        PacketSent,
        Disconnected,
        Error
    };

    struct LegacyTransportConfig
    {
        bool includeTimestamp{ true };
    };

    struct TcpEvent
    {
        TcpEventType type{ TcpEventType::Disconnected };
        protocol::PacketId packetId{ protocol::PacketId::CLLogin };
        std::vector<std::uint8_t> body;
        std::wstring message;
        std::uint32_t bodySize{ 0 };
        bool encrypted{ false };
        std::uint32_t reportedBodySize{ 0 };
    };

    class TcpClient final
    {
    public:
        TcpClient() = default;
        TcpClient(const TcpClient&) = delete;
        TcpClient& operator=(const TcpClient&) = delete;
        ~TcpClient();

        [[nodiscard]] bool Start(std::string host, std::uint16_t port);
        void Stop() noexcept;
        [[nodiscard]] bool Send(const std::vector<std::uint8_t>& packet);
        void SetEncryptionCode(std::optional<std::uint8_t> code) noexcept;
        void SetLegacyTransportConfig(LegacyTransportConfig config) noexcept;
        [[nodiscard]] std::optional<TcpEvent> PollEvent();
        [[nodiscard]] bool IsRunning() const noexcept;

    private:
        void Worker(std::string host, std::uint16_t port);
        void PushEvent(TcpEvent event);
        [[nodiscard]] bool ReceiveExact(std::uint8_t* buffer, std::size_t size, std::wstring* errorMessage = nullptr);

        std::thread worker_;
        std::mutex eventMutex_;
        std::mutex sendMutex_;
        std::queue<TcpEvent> events_;
        std::atomic_bool running_{ false };
        std::atomic_bool connected_{ false };
        std::atomic_int encryptionCode_{ -1 };
        std::atomic_bool includeTimestamp_{ true };
        std::atomic<std::uintptr_t> socket_{ 0 };
    };
}
