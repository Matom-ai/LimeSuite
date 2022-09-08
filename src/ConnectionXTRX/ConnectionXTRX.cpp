/**
    @file ConnectionRemote.cpp
    @author Lime Microsystems
    @brief Implementation of EVB7 connection of serial COM port.
*/

// On some systems USB device can only be used by single application at a time.
// Remote connection is ONLY for debugging purposes to allow to forward communications
// from external application to inspect and adjust board configuration at runtime.
// It may cause performance issues (github #263)

#include "ConnectionXTRX.h"
#include <string>
#include "string.h"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include "FPGA_common.h"
#include "Logger.h"
#ifdef __unix__

#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <termios.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#else
#include <Winsock.h>
#endif // LINUX

#if defined(__FreeBSD__)
#include <sys/socket.h>
#endif

using namespace std;
using namespace lime;

ConnectionXTRX::ConnectionXTRX(const char *comName)
{
// try close before open
    remoteIP = std::string(comName);
    socketFd = -1;
}

ConnectionXTRX::~ConnectionXTRX(void)
{
    Close();
}

void ConnectionXTRX::Close(void)
{

}

bool ConnectionXTRX::IsOpen(void)
{
    return true;
}

int ConnectionXTRX::Open()
{
    if (socketFd < 0)
        return Connect(remoteIP.c_str(), 5000);
    return 0;
}

int ConnectionXTRX::Connect(const char* ip, uint16_t port)
{
    return 0;
}

int ConnectionXTRX::TransferPacket(GenericPacket &pkt)
{
    int status = 0;
    return status;
}

int ConnectionXTRX::Write(const unsigned char *data, int len, int timeout_ms)
{
    return len;
}

int ConnectionXTRX::Read(unsigned char *response, int len, int timeout_ms)
{
    return len;
}

int ConnectionXTRX::GetBuffersCount() const
{
    return 1;
}
int ConnectionXTRX::CheckStreamSize(int size) const
{
    return size;
}
