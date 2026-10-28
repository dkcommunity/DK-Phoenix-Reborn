#pragma once

#if defined(PHOENIX_ENABLE_DIAGNOSTICS)

#include "Diagnostics/Logger.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace phoenix::diagnostics
{
    enum class PhoenixLogLevel : std::uint8_t
    {
        Info,
        Warning,
        Error
    };

    void SetPhoenixDiagnosticsLogger(Logger* logger) noexcept;
    [[nodiscard]] Logger* PhoenixDiagnosticsLogger() noexcept;
    [[nodiscard]] std::uint64_t PhoenixDiagnosticsNowMs() noexcept;
    void PhoenixDiagnosticsEmit(PhoenixLogLevel level, std::wstring_view message);

    class PhoenixProfileScope final
    {
    public:
        explicit PhoenixProfileScope(std::wstring_view name) noexcept;
        ~PhoenixProfileScope();

        PhoenixProfileScope(const PhoenixProfileScope&) = delete;
        PhoenixProfileScope& operator=(const PhoenixProfileScope&) = delete;

    private:
        std::wstring_view name_;
        std::chrono::steady_clock::time_point start_;
    };
}

#define PHX_DETAIL_JOIN_INNER(a, b) a##b
#define PHX_DETAIL_JOIN(a, b) PHX_DETAIL_JOIN_INNER(a, b)
#define PHX_LOG_INFO(message) ::phoenix::diagnostics::PhoenixDiagnosticsEmit(::phoenix::diagnostics::PhoenixLogLevel::Info, (message))
#define PHX_LOG_WARNING(message) ::phoenix::diagnostics::PhoenixDiagnosticsEmit(::phoenix::diagnostics::PhoenixLogLevel::Warning, (message))
#define PHX_LOG_ERROR(message) ::phoenix::diagnostics::PhoenixDiagnosticsEmit(::phoenix::diagnostics::PhoenixLogLevel::Error, (message))
#define PHX_LOG_ONCE(message) do { \
    static std::atomic_bool PHX_DETAIL_JOIN(phx_once_, __LINE__){ false }; \
    if (!PHX_DETAIL_JOIN(phx_once_, __LINE__).exchange(true, std::memory_order_relaxed)) { PHX_LOG_INFO(message); } \
} while (false)
#define PHX_LOG_EVERY_N(n, message) do { \
    static std::atomic_uint64_t PHX_DETAIL_JOIN(phx_every_n_, __LINE__){ 0 }; \
    const auto PHX_DETAIL_JOIN(phx_count_, __LINE__) = PHX_DETAIL_JOIN(phx_every_n_, __LINE__).fetch_add(1, std::memory_order_relaxed) + 1; \
    if ((n) != 0 && (PHX_DETAIL_JOIN(phx_count_, __LINE__) % static_cast<std::uint64_t>(n)) == 0) { PHX_LOG_INFO(message); } \
} while (false)
#define PHX_LOG_EVERY_MS(ms, message) do { \
    static std::atomic_uint64_t PHX_DETAIL_JOIN(phx_last_ms_, __LINE__){ 0 }; \
    const auto PHX_DETAIL_JOIN(phx_now_ms_, __LINE__) = ::phoenix::diagnostics::PhoenixDiagnosticsNowMs(); \
    auto PHX_DETAIL_JOIN(phx_expected_ms_, __LINE__) = PHX_DETAIL_JOIN(phx_last_ms_, __LINE__).load(std::memory_order_relaxed); \
    if (PHX_DETAIL_JOIN(phx_now_ms_, __LINE__) - PHX_DETAIL_JOIN(phx_expected_ms_, __LINE__) >= static_cast<std::uint64_t>(ms) && \
        PHX_DETAIL_JOIN(phx_last_ms_, __LINE__).compare_exchange_strong(PHX_DETAIL_JOIN(phx_expected_ms_, __LINE__), PHX_DETAIL_JOIN(phx_now_ms_, __LINE__), std::memory_order_relaxed)) { PHX_LOG_INFO(message); } \
} while (false)
#define PHX_LOG_POWER_OF_TWO(message) do { \
    static std::atomic_uint64_t PHX_DETAIL_JOIN(phx_power_, __LINE__){ 0 }; \
    const auto PHX_DETAIL_JOIN(phx_power_count_, __LINE__) = PHX_DETAIL_JOIN(phx_power_, __LINE__).fetch_add(1, std::memory_order_relaxed) + 1; \
    if ((PHX_DETAIL_JOIN(phx_power_count_, __LINE__) & (PHX_DETAIL_JOIN(phx_power_count_, __LINE__) - 1)) == 0) { PHX_LOG_WARNING(message); } \
} while (false)
#define PHX_COUNTER_ADD(counter, value) ((counter).fetch_add((value), std::memory_order_relaxed))
#define PHX_PROFILE_SCOPE(name) ::phoenix::diagnostics::PhoenixProfileScope PHX_DETAIL_JOIN(phx_profile_, __LINE__){ (name) }

#else

#define PHX_LOG_INFO(...) ((void)0)
#define PHX_LOG_WARNING(...) ((void)0)
#define PHX_LOG_ERROR(...) ((void)0)
#define PHX_LOG_ONCE(...) ((void)0)
#define PHX_LOG_EVERY_N(...) ((void)0)
#define PHX_LOG_EVERY_MS(...) ((void)0)
#define PHX_LOG_POWER_OF_TWO(...) ((void)0)
#define PHX_COUNTER_ADD(...) ((void)0)
#define PHX_PROFILE_SCOPE(...) ((void)0)

#endif
