#include "pch.h"

#include "Exports.h"

#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <locale>
#include <codecvt>

std::atomic<bool> isRunning(false);
std::wstring folderPath;
std::vector<std::wstring>extensions;
HANDLE watchedFolderHandle = NULL;
std::thread monitorThread;

bool isExtensionMatch(const std::wstring& fileName)
{
    std::wstring fileExt = std::filesystem::path(fileName).extension().wstring();
    std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);
    return std::find(extensions.begin(), extensions.end(), fileExt) != extensions.end();
}

bool isFileReady(const std::wstring& filePath, int maxRetries = 50, int retryDelay = 500)
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

        if (WaitForSingleObject(hEvent, 1000) == WAIT_OBJECT_0)
        {
            FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)buffer;

            do {
                std::wstring fileName(event->FileName, event->FileNameLength / sizeof(WCHAR));
                std::wstring fullPath = folderPath + L"\\" + fileName;

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


bool Start(const wchar_t* folderPath, const wchar_t* fileExtension, const bool ftpServerEnabled)
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
    monitorThread = std::thread(MonitorThreadFunc, watchedFolderHandle);

    return true;
}

void Stop()
{
    if (!isRunning) return;

    isRunning = false;

    monitorThread.join();

    CloseHandle(watchedFolderHandle);
}
