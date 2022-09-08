/**
    @file ConnectionRemote.cpp
    @author Lime Microsystems
    @brief Implementation of EVB7 connection of serial COM port.
*/

#include "ConnectionXTRX.h"
#include "xtrx_api.h"
#include <fstream>
#include <iostream>

#define	MAX_DEVS	8

using namespace std;
using namespace lime;

//! make a static-initialized entry in the registry
void __loadConnectionXTRXEntry(void) //TODO fixme replace with LoadLibrary/dlopen
{
    static ConnectionXTRXEntry EVB7COMEntry;
}

ConnectionXTRXEntry::ConnectionXTRXEntry(void):
    ConnectionRegistryEntry("Z_Connection") //Z just to appear last on the list
{
    return;
}

std::vector<ConnectionHandle> ConnectionXTRXEntry::enumerate(const ConnectionHandle &hint)
{
    std::vector<ConnectionHandle> result;
    ConnectionHandle handle;
    xtrx_device_info_t	di[MAX_DEVS];
    int	res,c;

    res = xtrx_discovery (di,MAX_DEVS);
    for	(c = 0;c < res;c++)	{
	handle.media = "PCIe";
	handle.name = di[c].uniqname;
	handle.module = "XTRX";
	handle.addr = hint.addr;
	result.push_back(handle);
    }
    return result;
}

IConnection *ConnectionXTRXEntry::make(const ConnectionHandle &handle)
{
    return new ConnectionXTRX(handle.addr.c_str());
}
