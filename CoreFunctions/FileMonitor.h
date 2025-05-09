#pragma once

#include <atomic>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

class FileMonitor
{
private:
    std::atomic<bool> m_isRunning;

    std::wstring m_path;
    std::wstring m_execPath;
    std::vector<std::wstring> m_fileExtension;
    
    HANDLE m_hFolder{};
    HANDLE m_hEvent{};

    std::thread m_thread;
    
    bool IsExtensionMatch(const std::wstring& fileName);
    bool IsFileReady(const std::wstring& filePath, int maxRetries, int retryDelay);

    void ThreadImpl();

public:
    FileMonitor(const wchar_t* path, const wchar_t* execPath, const wchar_t* m_fileExtension, bool& succeed);
    ~FileMonitor();
};