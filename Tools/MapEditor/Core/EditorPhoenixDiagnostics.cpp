#include "Diagnostics/PhoenixDiagnostics.h"

#if defined(PHOENIX_ENABLE_DIAGNOSTICS)

#include <windows.h>

namespace phoenix::diagnostics
{
    namespace
    {
        std::atomic<Logger*> gEditorLogger{ nullptr };
        const auto gEditorStart = std::chrono::steady_clock::now();
    }

    void SetPhoenixDiagnosticsLogger(Logger* logger) noexcept
    {
        gEditorLogger.store(logger, std::memory_order_release);
    }

    Logger* PhoenixDiagnosticsLogger() noexcept
    {
        return gEditorLogger.load(std::memory_order_acquire);
    }

    std::uint64_t PhoenixDiagnosticsNowMs() noexcept
    {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - gEditorStart).count());
    }

    void PhoenixDiagnosticsEmit(const PhoenixLogLevel level, const std::wstring_view message)
    {
        const wchar_t* prefix = level == PhoenixLogLevel::Error ? L"[MapEditor][Error] " :
            level == PhoenixLogLevel::Warning ? L"[MapEditor][Warning] " : L"[MapEditor] ";
        const std::wstring output = std::wstring(prefix) + std::wstring(message) + L"\n";
        OutputDebugStringW(output.c_str());
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
