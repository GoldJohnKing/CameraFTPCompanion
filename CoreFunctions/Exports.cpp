#include "pch.h"

#include "Exports.h"

#include "fineftp/server.h"

#include <atomic>
#include <filesystem>
#include <shellapi.h>
#include <sstream>
#include <string>
#include <thread>
#include <windows.h>

std::atomic<bool> isRunning(false);
std::vector<std::wstring>extensions;
std::wstring watchedFolderPath;
HANDLE watchedFolderHandle = NULL;
std::thread monitorThread;
fineftp::FtpServer* ftp_server;

bool isExtensionMatch(const std::wstring& fileName)
{
    std::wstring fileExt = std::filesystem::path(fileName).extension().wstring();
    std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);
    return std::find(extensions.begin(), extensions.end(), fileExt) != extensions.end();
}

bool isFileReady(const std::wstring& filePath, int maxRetries = 250, int retryDelay = 100)
{
    for (int i = 0; i < maxRetries; i++)
    {
        HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER fileSize1;

            if (GetFileSizeEx(hFile, &fileSize1))
            {
                CloseHandle(hFile);
                Sleep(retryDelay);

                hFile = CreateFileW(filePath.c_str(), GENERIC_READ, 0, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                if (hFile != INVALID_HANDLE_VALUE)
                {
                    LARGE_INTEGER fileSize2;
                    if (GetFileSizeEx(hFile, &fileSize2))
                    {
                        CloseHandle(hFile);

                        if (fileSize1.QuadPart == fileSize2.QuadPart)
                        {
                            return true;
                        }
                    }
                }
            }

            CloseHandle(hFile);
        }

        Sleep(retryDelay);
    }

    return false;
}

void MonitorThreadFunc(HANDLE hDir)
{
    char buffer[4096]{};
    OVERLAPPED overlapped{};

    HANDLE hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    if (!hEvent)
    {
        return;
    }

    overlapped.hEvent = hEvent;

    while (isRunning)
    {
        DWORD bytesReturned;

        if (!ReadDirectoryChangesW(hDir, buffer, sizeof(buffer), TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned, &overlapped, NULL))
        {
            break;
        }

        if (WaitForSingleObject(hEvent, 100) == WAIT_OBJECT_0)
        {
            FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)buffer;

            do {
                std::wstring fileName(event->FileName, event->FileNameLength / sizeof(WCHAR));
                std::wstring fullPath = watchedFolderPath + L"\\" + fileName;

                if ((event->Action == FILE_ACTION_ADDED ||
                    event->Action == FILE_ACTION_MODIFIED ||
                    event->Action == FILE_ACTION_RENAMED_NEW_NAME) &&
                    isExtensionMatch(fileName) &&
                    isFileReady(fullPath))
                {
                    HINSTANCE result = ShellExecuteW(NULL, L"open", fullPath.c_str(), NULL, NULL, SW_SHOW);
                }

                if (!event->NextEntryOffset)
                {
                    break;
                }

                event = (FILE_NOTIFY_INFORMATION*)((BYTE*)event + event->NextEntryOffset);
            } while (true);
        }
    }

    CloseHandle(hEvent);
}

std::string WideCharToUTF8(const wchar_t* wideStr)
{
    // 获取转换所需的字节数（UTF-8）
    int size = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) return "";

    // 分配缓冲区
    std::string utf8Str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, utf8Str.data(), size, nullptr, nullptr);

    // 移除末尾的空字符（如果有）
    if (!utf8Str.empty() && utf8Str.back() == '\0') {
        utf8Str.pop_back();
    }

    return utf8Str;
}

bool StartFtpServer()
{
    ftp_server = new fineftp::FtpServer(21); // Default listen to "0.0.0.0:21"

    // Append a '\' to path's end, or fineftp won't handle the path correctly
    std::string ftpRootPath = WideCharToUTF8(watchedFolderPath.c_str());

    if (ftpRootPath[ftpRootPath.size() - 1] != '\\')
    {
        ftpRootPath = ftpRootPath + '\\';
    }

    // Add the well known anonymous user. Clients can log in using username
    // "anonymous" or "ftp" with any password. The user will be able to access
    // your drive and upload, download, create or delete files.
    (*ftp_server).addUserAnonymous(ftpRootPath, fineftp::Permission::All);

    // Start the FTP Server with a thread-pool size of 4.
    return (*ftp_server).start(4);
}

void StopStpServer()
{
    (*ftp_server).stop();

    if (nullptr != ftp_server)
    {
        delete ftp_server;
        ftp_server = nullptr;
    }
}

bool DLL_EXPORT Start(const wchar_t* folderPath, const wchar_t* fileExtension, const bool ftpServerEnabled)
{
    if (isRunning) return true; // 监控仍在运行中

    if (!std::filesystem::exists(folderPath))
    {
        return false;
    }

    // 分割扩展名，将其格式化为".ext"格式，并转换为小写
    std::wistringstream iss(fileExtension);
    std::wstring ext;

    while (iss >> ext)
    {
        if (ext[0] != L'.')
        {
            ext = L"." + ext;
        }

        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        extensions.push_back(ext);
    }

    if (extensions.empty())
    {
        return false;
    }

    // 打开要监控的路径
    HANDLE watchedFolderHandle = CreateFileW(
        folderPath,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (watchedFolderHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    isRunning = true;
    watchedFolderPath = folderPath;

    monitorThread = std::thread(MonitorThreadFunc, watchedFolderHandle);

    if (ftpServerEnabled && !StartFtpServer())
    {
        MessageBox(NULL, L"FTP服务器启动失败", NULL, NULL);
        StopStpServer();
    }

    return true;
}

void DLL_EXPORT Stop()
{
    if (!isRunning) return;

    isRunning = false;
    watchedFolderPath.clear();

    monitorThread.join();

    CloseHandle(watchedFolderHandle);

    StopStpServer();
}
