#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace phoenix::platform
{

class FileWatcher final
{
public:
    using Callback = std::function<void(const std::filesystem::path&)>;

    FileWatcher() = default;
    ~FileWatcher() { Stop(); }

    // Başlat: path = izlenecek dosya/klasör, callback = değişiklik callback'i
    void Start(const std::filesystem::path& path, Callback callback);

    void Stop();

    // Ana thread'de çağrılmalı (game loop'ta)
    void Poll();

    // Değişiklik olduğunda çağrılacak callback'i ayarla
    void SetCallback(Callback callback) { callback_ = std::move(callback); }

private:
    void ThreadFunc();
    void ProcessEvents();

    std::filesystem::path watchPath_;
    std::filesystem::path targetFile_;
    Callback callback_;
    std::atomic<bool> running_{ false };
    std::thread thread_;
    std::mutex queueMutex_;
    std::queue<std::filesystem::path> eventQueue_;
    std::condition_variable cv_;
    std::chrono::steady_clock::time_point lastEventTime_;
    static constexpr auto DEBOUNCE_MS = 300;  // 300ms debounce

#if defined(_WIN32)
    HANDLE dirHandle_{ nullptr };
    HANDLE eventHandle_{ nullptr };
    OVERLAPPED overlapped_{};
    char buffer_[4096];
#endif
};

} // namespace phoenix::platform