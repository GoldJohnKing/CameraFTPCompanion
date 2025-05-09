#include "pch.h"

#include "FileMonitor.h"

#include <filesystem>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <windows.h>

void FileMonitor::ThreadImpl()
{
    char buffer[4096]{};
    OVERLAPPED overlapped{};
    overlapped.hEvent = m_hEvent;

    while (m_isRunning)
    {
        DWORD bytesReturned;

        if (!ReadDirectoryChangesW(m_hFolder, buffer, sizeof(buffer), TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned, &overlapped, NULL))
        {
            Sleep(100);
            continue;
        }

        if (WaitForSingleObject(m_hEvent, 100) == WAIT_OBJECT_0)
        {
            FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)buffer;

            do {
                std::wstring fileName(event->FileName, event->FileNameLength / sizeof(WCHAR));
                std::wstring fullPath = m_path + L"\\" + fileName;

                if ((event->Action == FILE_ACTION_ADDED ||
                    event->Action == FILE_ACTION_MODIFIED ||
                    event->Action == FILE_ACTION_RENAMED_NEW_NAME) &&
                    IsExtensionMatch(fileName) &&
                    IsFileReady(fullPath, 250, 100))
                {
                    if (m_execPath.empty()) // 如果用户未配置打开方式，则使用系统默认的打开方式
                    {
                        HINSTANCE result = ShellExecuteW(NULL, L"open", fullPath.c_str(), NULL, NULL, SW_SHOW);

                        if ((INT_PTR)result <= 32) // ShellExecute失败时返回值小于等于32
                        {
                            MessageBoxW(NULL, L"使用默认方式打开文件失败！", NULL, NULL);
                        }
                    }
                    else // 否则，使用用户给定的程序打开文件
                    {
                        HINSTANCE result = ShellExecuteW(NULL, L"open", m_execPath.c_str(), fullPath.c_str(), NULL, SW_SHOW);

                        if ((INT_PTR)result <= 32)
                        {
                            MessageBoxW(NULL, L"无法使用指定程序打开文件！", NULL, NULL);
                        }
                    }
                }

                if (!event->NextEntryOffset)
                {
                    break;
                }

                event = (FILE_NOTIFY_INFORMATION*)((BYTE*)event + event->NextEntryOffset);
            } while (true);
        }
    }
}

FileMonitor::FileMonitor(const wchar_t* path, const wchar_t* execPath, const wchar_t* fileExtension, bool& succeed) :
    m_path(path),
    m_execPath(execPath)
{
    succeed = false;

    std::wistringstream iss(fileExtension);
    std::wstring ext;

    while (iss >> ext)
    {
        if (ext[0] != L'.')
            ext = L"." + ext;

        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        m_fileExtension.push_back(ext);
    }

    m_hFolder = CreateFileW(
        m_path.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (m_hFolder == INVALID_HANDLE_VALUE) return;

    m_hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    if (!m_hEvent) return;

    m_isRunning = true;

    m_thread = std::thread(&FileMonitor::ThreadImpl, this);
    
    succeed = m_isRunning;
}

FileMonitor::~FileMonitor()
{
    m_isRunning = false;

    if (m_thread.joinable())
        m_thread.join();

    CloseHandle(m_hEvent);
    CloseHandle(m_hFolder);
}

bool FileMonitor::IsFileReady(const std::wstring& filePath, int maxRetries, int retryDelay)
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

bool FileMonitor::IsExtensionMatch(const std::wstring& fileName)
{
    if (m_fileExtension.empty()) return true;

    std::wstring fileExt = std::filesystem::path(fileName).extension().wstring();
    std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);
    return std::find(m_fileExtension.begin(), m_fileExtension.end(), fileExt) != m_fileExtension.end();
}