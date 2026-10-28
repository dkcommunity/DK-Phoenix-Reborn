#include "Diagnostics/PhoenixDiagnostics.h"

#if defined(PHOENIX_ENABLE_DIAGNOSTICS)

namespace phoenix::diagnostics
{
    namespace
    {
        std::atomic<Logger*> gLogger{ nullptr };
        const auto gStart = std::chrono::steady_clock::now();
    }

    void SetPhoenixDiagnosticsLogger(Logger* logger) noexcept
    {
        gLogger.store(logger, std::memory_order_release);
    }

    Logger* PhoenixDiagnosticsLogger() noexcept
    {
        return gLogger.load(std::memory_order_acquire);
    }

    std::uint64_t PhoenixDiagnosticsNowMs() noexcept
    {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - gStart).count());
    }

    void PhoenixDiagnosticsEmit(const PhoenixLogLevel level, const std::wstring_view message)
    {
        auto* logger = PhoenixDiagnosticsLogger();
        if (logger == nullptr)
        {
            return;
        }
        switch (level)
        {
        case PhoenixLogLevel::Warning: logger->Warning(message); break;
        case PhoenixLogLevel::Error: logger->Error(message); break;
        default: logger->Info(message); break;
        }
    }

    PhoenixProfileScope::PhoenixProfileScope(const std::wstring_view name) noexcept
        : name_(name), start_(std::chrono::steady_clock::now())
    {
    }

    PhoenixProfileScope::~PhoenixProfileScope()
    {
        const auto elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start_).count();
        if (elapsed >= 5.0)
        {
            PhoenixDiagnosticsEmit(PhoenixLogLevel::Warning,
                std::wstring(L"[PHX_PROFILE] scope=") + std::wstring(name_) +
                L" duration_ms=" + std::to_wstring(elapsed));
        }
    }
}

#endif
