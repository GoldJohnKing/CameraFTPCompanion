#include "pch.h"
#include "FtpService.h"
#include "Utils.h"
#include <filesystem>
#include <string>

FtpServer::FtpServer(const int port, const std::string& rootPath, bool& succeed) :
    m_port(port),
    m_rootPath(rootPath)
{
    if (m_rootPath.back() != '\\')
        m_rootPath += '\\';

    m_ftpServerInstance = std::make_unique<fineftp::FtpServer>(m_port);

    (*m_ftpServerInstance).addUserAnonymous(m_rootPath, fineftp::Permission::All);

    succeed = (*m_ftpServerInstance).start(4);
}

FtpServer::~FtpServer()
{
    (*m_ftpServerInstance).stop();
}
