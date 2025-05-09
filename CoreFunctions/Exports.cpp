#include "pch.h"

#include "Exports.h"

#include "FtpService.h"
#include "FileMonitor.h"
#include "Utils.h"

#include <filesystem>
#include <windows.h>

std::unique_ptr<FtpServer> g_ftpServer;
std::unique_ptr<FileMonitor> g_fileMonitor;

bool DLL_EXPORT Start(const wchar_t* path, const bool enableFileMonitor, const wchar_t* autoExecProgram, const wchar_t* fileExtensionFilter, const int ftpPort)
{
    if (g_ftpServer || g_fileMonitor)
        return true;

    if (!std::filesystem::exists(path))
    {
        MessageBox(NULL, L"存储路径无效", NULL, NULL);
        return false;
    }

    bool ftpIsRunning = false;
    bool fileMonitorIsRunning = false;

    if (enableFileMonitor)
    {
        g_fileMonitor = std::make_unique<FileMonitor>(path, autoExecProgram, fileExtensionFilter, fileMonitorIsRunning);

        if (!fileMonitorIsRunning)
        {
            g_fileMonitor.reset();
            MessageBox(NULL, L"文件监控创建失败", NULL, NULL);
        }
    }

    bool enableFtpServer = ftpPort >= 0 && ftpPort <= 65535;

    if (enableFtpServer)
    {
        g_ftpServer = std::make_unique<FtpServer>(ftpPort, WideCharToUTF8(path), ftpIsRunning);

        if (!ftpIsRunning)
        {
            g_ftpServer.reset();
            MessageBox(NULL, L"FTP服务器启动失败", NULL, NULL);
        }
    }

    return !(enableFtpServer && !ftpIsRunning) && !(enableFileMonitor && !fileMonitorIsRunning);
}

void DLL_EXPORT Stop()
{
    g_ftpServer.reset();
    g_fileMonitor.reset();
}
