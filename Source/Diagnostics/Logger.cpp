#include "Diagnostics/Logger.h"

#if defined(PHOENIX_ENABLE_DIAGNOSTICS)

#include <Windows.h>
#include <DbgHelp.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>

namespace phoenix::diagnostics
{
    namespace
    {
        Logger* gCrashLogger = nullptr;

        std::wstring CurrentTimestamp()
        {
            const auto now = std::chrono::system_clock::now();
            const auto time = std::chrono::system_clock::to_time_t(now);

            std::tm localTime{};
            localtime_s(&localTime, &time);

            std::wstringstream text;
            text << std::put_time(&localTime, L"%Y-%m-%d %H:%M:%S");
            return text.str();
        }

        std::string WideToUtf8(const std::wstring_view value)
        {
            if (value.empty())
            {
                return {};
            }
            const int required = WideCharToMultiByte(
                CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                nullptr, 0, nullptr, nullptr);
            if (required <= 0)
            {
                std::string fallback;
                fallback.reserve(value.size());
                for (const wchar_t ch : value)
                {
                    fallback.push_back(ch >= 0x20 && ch <= 0x7e ? static_cast<char>(ch) : '?');
                }
                return fallback;
            }
            std::string result(static_cast<std::size_t>(required), '\0');
            (void)WideCharToMultiByte(
                CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                result.data(), required, nullptr, nullptr);
            return result;
        }

        LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* exception)
        {
            if (gCrashLogger == nullptr || exception == nullptr || exception->ExceptionRecord == nullptr)
            {
                return EXCEPTION_CONTINUE_SEARCH;
            }

            gCrashLogger->WriteCrashReport(
                exception->ExceptionRecord->ExceptionCode,
                exception->ExceptionRecord->ExceptionAddress,
                GetCurrentThreadId());

            const auto dumpPath = gCrashLogger->DumpPath();
            const HANDLE file = CreateFileW(
                dumpPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file != INVALID_HANDLE_VALUE)
            {
                MINIDUMP_EXCEPTION_INFORMATION info{};
                info.ThreadId = GetCurrentThreadId();
                info.ExceptionPointers = exception;
                info.ClientPointers = FALSE;
                MiniDumpWriteDump(
                    GetCurrentProcess(), GetCurrentProcessId(), file,
                    MiniDumpWithThreadInfo, &info, nullptr, nullptr);
                CloseHandle(file);
            }
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }

    Logger::Logger(const std::filesystem::path& logPath)
        : logPath_(logPath),
          dumpPath_(logPath.parent_path() / (logPath.stem().wstring() + L".dmp")),
          stream_(logPath, std::ios::out | std::ios::trunc | std::ios::binary),
          monotonicStart_(std::chrono::steady_clock::now())
    {
        Info(L"Logger initialized.");
    }

    Logger::~Logger()
    {
        Info(L"Logger shutdown.");
        Flush();
        UninstallUnhandledExceptionHandler();
    }

    void Logger::Info(std::wstring_view message)
    {
        Write(L"INFO", message, false);
    }

    void Logger::Error(std::wstring_view message)
    {
        Write(L"ERROR", message, true);
    }

    void Logger::Warning(const std::wstring_view message)
    {
        Write(L"WARNING", message, false);
    }

    void Logger::Critical(const std::wstring_view message)
    {
        Write(L"CRITICAL", message, true);
    }

    void Logger::Diagnostic(const std::wstring_view category, const std::wstring_view message, const double durationMs)
    {
        if (!diagnosticEnabled_.load(std::memory_order_relaxed))
        {
            return;
        }
        std::wstring full;
        full.reserve(category.size() + message.size() + 48U);
        full.append(L"[").append(category).append(L"] ").append(message);
        if (durationMs > 0.0)
        {
            std::wstringstream duration;
            duration << L" duration_ms=" << std::fixed << std::setprecision(3) << durationMs;
            full.append(duration.str());
        }
        if (durationMs >= 100.0)
        {
            Critical(full);
        }
        else if (durationMs >= 20.0)
        {
            Warning(full);
        }
        else
        {
            Info(full);
        }
    }

    void Logger::SetCrashContext(std::wstring context)
    {
        std::lock_guard lock(mutex_);
        crashContext_ = std::move(context);
    }

    void Logger::InstallUnhandledExceptionHandler()
    {
        gCrashLogger = this;
        SetUnhandledExceptionFilter(UnhandledExceptionFilter);
    }

    void Logger::UninstallUnhandledExceptionHandler()
    {
        if (gCrashLogger == this)
        {
            gCrashLogger = nullptr;
            SetUnhandledExceptionFilter(nullptr);
        }
    }

    void Logger::Flush()
    {
        std::lock_guard lock(mutex_);
        if (stream_.is_open())
        {
            stream_.flush();
        }
    }

    void Logger::Write(std::wstring_view level, std::wstring_view message, const bool flushImmediately)
    {
        std::lock_guard lock(mutex_);
        if (!stream_.is_open())
        {
            return;
        }

        const auto monotonicUs = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - monotonicStart_).count();
        std::wstringstream line;
        line << L'[' << CurrentTimestamp() << L"] [mono_ms=" << std::fixed << std::setprecision(3)
             << (static_cast<double>(monotonicUs) / 1000.0) << L"] [tid=" << GetCurrentThreadId()
             << L"] [" << level << L"] " << message;
        const auto formatted = line.str();
        WriteUtf8Unlocked(formatted);
        recentEvents_.push_back(formatted);
        if (recentEvents_.size() > 100U)
        {
            recentEvents_.pop_front();
        }
        if (flushImmediately)
        {
            stream_.flush();
        }
    }

    void Logger::WriteUtf8Unlocked(const std::wstring_view text)
    {
        const auto encoded = WideToUtf8(text);
        stream_.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
        stream_.put('\n');
    }

    void Logger::WriteCrashReport(
        const unsigned long exceptionCode,
        const void* faultAddress,
        const unsigned long threadId)
    {
        std::unique_lock lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock())
        {
            return;
        }
        if (!stream_.is_open())
        {
            return;
        }
        std::wstringstream header;
        header << L"[CRASH] code=0x" << std::hex << exceptionCode << L" address=" << faultAddress
               << std::dec << L" tid=" << threadId << L" context={" << crashContext_ << L"}";
        WriteUtf8Unlocked(header.str());
        WriteUtf8Unlocked(L"[CRASH] last_diagnostic_events_begin");
        for (const auto& event : recentEvents_)
        {
            WriteUtf8Unlocked(event);
        }
        WriteUtf8Unlocked(L"[CRASH] last_diagnostic_events_end dump=" + dumpPath_.wstring());
        stream_.flush();
    }
}

#endif
