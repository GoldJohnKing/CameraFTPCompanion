#pragma once

#include "fineftp/server.h"
#include <string>

class FtpServer
{
private:
    std::unique_ptr<fineftp::FtpServer> m_ftpServerInstance{};

    int m_port{};
    std::string m_rootPath{};

public:
    FtpServer(const int port, const std::string& rootPath, bool& succeed);
    ~FtpServer();
};
