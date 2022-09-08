/**
    @file ConnectionRemote.cpp
    @author Lime Microsystems
    @brief Implementation of EVB7 connection of serial COM port.
*/

#include "ConnectionXTRX.h"
#include <fstream>
#include <iostream>

using namespace std;
using namespace lime;


//! make a static-initialized entry in the registry
void __loadConnectionXTRXEntry(void) //TODO fixme replace with LoadLibrary/dlopen
{
    static ConnectionXTRXEntry EVB7COMEntry;
}

ConnectionXTRXEntry::ConnectionXTRXEntry(void):
    ConnectionRegistryEntry("Z_Remote") //Z just to appear last on the list
{
    return;
}

std::vector<ConnectionHandle> ConnectionXTRXEntry::enumerate(const ConnectionHandle &hint)
{
    std::vector<ConnectionHandle> result;

printf ("ENTRY TO REGISTER\n");
    ConnectionHandle handle;
    handle.media = "PCIe";
    handle.name = "XTRX";
    handle.addr = hint.addr;
    result.push_back(handle);

    return result;
}

IConnection *ConnectionXTRXEntry::make(const ConnectionHandle &handle)
{
    return new ConnectionXTRX(handle.addr.c_str());
}
