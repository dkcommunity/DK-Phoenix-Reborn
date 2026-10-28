#include "Platform/FileWatcher.h"
#include <Diagnostics/PhoenixDiagnostics.h>

namespace phoenix::platform
{

void FileWatcher::Start(const std::filesystem::path& path, Callback callback)
{
    if (running_.load()) return;

    // path can be a file (interface.ini) or directory
    // If it's a file, watch its parent directory but filter for that specific file
    std::filesystem::path watchDir = path;
    std::filesystem::path targetFile;
    if (std::filesystem::exists(path) && !std::filesystem::is_directory(path)) {
        targetFile = path.filename();
        watchDir = path.parent_path();
    }

    watchPath_ = watchDir;
    targetFile_ = targetFile;
    callback_ = std::move(callback);
    running_.store(true);

#if defined(_WIN32)
    // Klasör handle'ı aç
    dirHandle_ = CreateFileW(
        watchPath_.wstring().c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (dirHandle_ == INVALID_HANDLE_VALUE) {
        PHX_LOG_ERROR(L"[FILEWATCHER] Failed to open directory: " + watchPath_.wstring());
        running_.store(false);
        return;
    }

    eventHandle_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!eventHandle_) {
        CloseHandle(dirHandle_);
        dirHandle_ = nullptr;
        running_.store(false);
        return;
    }

    ZeroMemory(&overlapped_, sizeof(overlapped_));
    overlapped_.hEvent = eventHandle_;

    // İlk read işlemini başlat - alt klasörleri IZLEME (bWatchSubtree=FALSE)
    ReadDirectoryChangesW(
        dirHandle_, buffer_, sizeof(buffer_), FALSE,
        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
        nullptr, &overlapped_, nullptr);
#endif

    thread_ = std::thread(&FileWatcher::ThreadFunc, this);
    PHX_LOG_INFO(L"[FILEWATCHER] Started watching: " + watchPath_.wstring() + 
        (targetFile_.empty() ? L"" : L" (filter: " + targetFile_.wstring() + L")"));
}

void FileWatcher::Stop()
{
    if (!running_.exchange(false)) return;

#if defined(_WIN32)
    if (dirHandle_) {
        CancelIoEx(dirHandle_, &overlapped_);
        CloseHandle(dirHandle_);
        dirHandle_ = nullptr;
    }
    if (eventHandle_) {
        SetEvent(eventHandle_);
        CloseHandle(eventHandle_);
        eventHandle_ = nullptr;
    }
#endif

    if (thread_.joinable()) thread_.join();
    PHX_LOG_INFO(L"[FILEWATCHER] Stopped");
}

void FileWatcher::Poll()
{
    std::vector<std::filesystem::path> events;
    {
        std::lock_guard lock(queueMutex_);
        while (!eventQueue_.empty()) {
            events.push_back(std::move(eventQueue_.front()));
            eventQueue_.pop();
        }
    }

    // Debounce: aynı dosya için son event'ten 300ms geçmişse tetikle
    auto now = std::chrono::steady_clock::now();
    for (const auto& path : events) {
        // Eğer hedef dosya belirtilmişse, sadece o dosya için callback çağır
        if (!targetFile_.empty() && path.filename() != targetFile_) {
            continue;  // Bu dosyayı atla
        }
        if (now - lastEventTime_ >= std::chrono::milliseconds(DEBOUNCE_MS)) {
            lastEventTime_ = now;
            if (callback_) callback_(path);
        }
    }
}

void FileWatcher::ThreadFunc()
{
#if defined(_WIN32)
    while (running_.load()) {
        DWORD bytes = 0;
        BOOL result = GetOverlappedResult(dirHandle_, &overlapped_, &bytes, TRUE);

        if (!running_.load()) break;

        if (result && bytes > 0) {
            FILE_NOTIFY_INFORMATION* notify = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer_);
            while (true) {
                std::wstring fileName(notify->FileName, notify->FileNameLength / sizeof(wchar_t));
                std::filesystem::path fullPath = watchPath_ / fileName;

                // Eğer hedef dosya belirtilmişse, sadece o dosyayı kuyruğa al
                if (targetFile_.empty() || fileName == targetFile_.wstring()) {
                    {
                        std::lock_guard lock(queueMutex_);
                        eventQueue_.push(std::move(fullPath));
                    }
                    cv_.notify_one();
                }

                if (notify->NextEntryOffset == 0) break;
                notify = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                    reinterpret_cast<char*>(notify) + notify->NextEntryOffset);
            }

            // Yeni read işlemi başlat
            ZeroMemory(&overlapped_, sizeof(overlapped_));
            overlapped_.hEvent = eventHandle_;
            ReadDirectoryChangesW(
                dirHandle_, buffer_, sizeof(buffer_), FALSE,
                FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
                nullptr, &overlapped_, nullptr);
        } else {
            // Hata veya iptal
            if (!running_.load()) break;
            Sleep(100);
        }
    }
#endif
}

} // namespace phoenix::platform