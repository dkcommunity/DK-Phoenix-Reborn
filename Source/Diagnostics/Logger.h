#pragma once

#include <filesystem>
#include <fstream>
#include <chrono>
#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>

namespace phoenix::diagnostics
{
#if defined(PHOENIX_ENABLE_DIAGNOSTICS)
    class Logger final
    {
    public:
        explicit Logger(const std::filesystem::path& logPath);
        ~Logger();

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        void Info(std::wstring_view message);
        void Warning(std::wstring_view message);
        void Error(std::wstring_view message);
        void Critical(std::wstring_view message);
        void Diagnostic(std::wstring_view category, std::wstring_view message, double durationMs = 0.0);
        void SetDiagnosticEnabled(bool enabled) noexcept { diagnosticEnabled_.store(enabled, std::memory_order_relaxed); }
        void SetCrashContext(std::wstring context);
        void InstallUnhandledExceptionHandler();
        void UninstallUnhandledExceptionHandler();
        void WriteCrashReport(unsigned long exceptionCode, const void* faultAddress, unsigned long threadId);
        void Flush();

        [[nodiscard]] const std::filesystem::path& Path() const noexcept { return logPath_; }
        [[nodiscard]] const std::filesystem::path& DumpPath() const noexcept { return dumpPath_; }

    private:
        void Write(std::wstring_view level, std::wstring_view message, bool flushImmediately);
        void WriteUtf8Unlocked(std::wstring_view text);
        std::filesystem::path logPath_;
        std::filesystem::path dumpPath_;
        std::ofstream stream_;
        std::mutex mutex_;
        std::deque<std::wstring> recentEvents_;
        std::wstring crashContext_;
        std::chrono::steady_clock::time_point monotonicStart_;
        std::atomic_bool diagnosticEnabled_{ true };
    };
#else
    class Logger final
    {
    public:
        explicit Logger(const std::filesystem::path&) noexcept {}
        void Info(std::wstring_view) noexcept {}
        void Warning(std::wstring_view) noexcept {}
        void Error(std::wstring_view) noexcept {}
        void Critical(std::wstring_view) noexcept {}
        void Diagnostic(std::wstring_view, std::wstring_view, double = 0.0) noexcept {}
        void SetDiagnosticEnabled(bool) noexcept {}
        void SetCrashContext(std::wstring) noexcept {}
        void InstallUnhandledExceptionHandler() noexcept {}
        void UninstallUnhandledExceptionHandler() noexcept {}
        void WriteCrashReport(unsigned long, const void*, unsigned long) noexcept {}
        void Flush() noexcept {}
        [[nodiscard]] const std::filesystem::path& Path() const noexcept { return emptyPath_; }
        [[nodiscard]] const std::filesystem::path& DumpPath() const noexcept { return emptyPath_; }
    private:
        std::filesystem::path emptyPath_;
    };
#endif
}
