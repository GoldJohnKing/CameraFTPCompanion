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
std::wstring autoRunExecPath;
std::thread monitorThread;
fineftp::FtpServer* ftp_server;

bool isExtensionMatch(const std::wstring& fileName)
{
    if (extensions.empty()) return true; // ���δ������չ������������չ������Ч

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
                    if (autoRunExecPath.empty()) // ����û�δ���ô򿪷�ʽ����ʹ��ϵͳĬ�ϵĴ򿪷�ʽ
                    {
                        HINSTANCE result = ShellExecuteW(NULL, L"open", fullPath.c_str(), NULL, NULL, SW_SHOW);

                        if ((INT_PTR)result <= 32) // ShellExecuteʧ��ʱ����ֵС�ڵ���32
                        {
                            MessageBoxW(NULL, L"ʹ��Ĭ�Ϸ�ʽ���ļ�ʧ�ܣ�", NULL, NULL);
                        }
                    }
                    else // ����ʹ���û������ĳ�����ļ�
                    {
                        HINSTANCE result = ShellExecuteW(NULL, L"open", autoRunExecPath.c_str(), fullPath.c_str(), NULL, SW_SHOW);
                        
                        if ((INT_PTR)result <= 32)
                        {
                            MessageBoxW(NULL, L"�޷�ʹ��ָ��������ļ���", NULL, NULL);
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

    CloseHandle(hEvent);
}

std::string WideCharToUTF8(const wchar_t* wideStr)
{
    // ��ȡת��������ֽ�����UTF-8��
    int size = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) return "";

    // ���仺����
    std::string utf8Str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, utf8Str.data(), size, nullptr, nullptr);

    // �Ƴ�ĩβ�Ŀ��ַ�������У�
    if (!utf8Str.empty() && utf8Str.back() == '\0') {
        utf8Str.pop_back();
    }

    return utf8Str;
}

bool StartFtpServer(const int ftpPort)
{
    ftp_server = new fineftp::FtpServer(ftpPort); // Default listen to 0.0.0.0

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

void StopFtpServer()
{
    if (nullptr != ftp_server)
    {
        (*ftp_server).stop();
        delete ftp_server;
        ftp_server = nullptr;
    }
}

bool DLL_EXPORT Start(const wchar_t* folderPath, const bool execEnabled, const wchar_t* execPath, const wchar_t* fileExtension, const int ftpPort)
{
    if (isRunning) return true; // �������������

    if (!std::filesystem::exists(folderPath))
    {
        MessageBox(NULL, L"�洢·����Ч��", NULL, NULL);
        return false;
    }

    if (execEnabled)
    {
        // �ָ���չ���������ʽ��Ϊ".ext"��ʽ����ת��ΪСд
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

        // ��Ҫ��ص�·��
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
            watchedFolderHandle = NULL;
            MessageBox(NULL, L"�޷��򿪴洢·����", NULL, NULL);
            return false;
        }

        watchedFolderPath = folderPath;
        autoRunExecPath = execPath;

        monitorThread = std::thread(MonitorThreadFunc, watchedFolderHandle);
    }

    if (0 <= ftpPort && 65535 >= ftpPort && !StartFtpServer(ftpPort))
    {
        MessageBox(NULL, L"FTP����������ʧ��", NULL, NULL);
        StopFtpServer();
    }

    isRunning = true;

    return true;
}

void DLL_EXPORT Stop()
{
    if (!isRunning) return;

    isRunning = false;

    if (monitorThread.joinable())
    {
        monitorThread.join();
    }

    if (NULL != watchedFolderHandle)
    {
        CloseHandle(watchedFolderHandle);
        watchedFolderHandle = NULL;
    }

    watchedFolderPath.clear();
    autoRunExecPath.clear();
    extensions.clear();

    StopFtpServer();
}
