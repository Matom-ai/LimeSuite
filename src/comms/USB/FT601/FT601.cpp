#include "FT601.h"
#include "dataTypes.h"
#include "DeviceExceptions.h"
#include "USBTransferContext_FT601.h"

using namespace lime;

static const int STREAM_BULK_WRITE_ADDRESS = 0x03;
static const int STREAM_BULK_READ_ADDRESS = 0x83;

static const int CONTROL_BULK_WRITE_ADDRESS = 0x02;
static const int CONTROL_BULK_READ_ADDRESS = 0x82;

FT601::FT601(void* usbContext)
    : USBGeneric(usbContext)
#ifdef __unix
    , mUsbCounter(0)
#endif
{
}

FT601::~FT601()
{
    Disconnect();
}

bool FT601::Connect(uint16_t vid, uint16_t pid, const std::string& serial)
{
    Disconnect();
#ifndef __unix__
    DWORD devCount;
    FT_STATUS ftStatus = FT_OK;
    DWORD dwNumDevices = 0;
    // Open a device
    ftStatus = FT_Create(reinterpret_cast<void*>(const_cast<char*>(serial.c_str())), FT_OPEN_BY_SERIAL_NUMBER, &mFTHandle);

    if (FT_FAILED(ftStatus))
    {
        ReportError(ENODEV, "Failed to list USB Devices");
        return -1;
    }

    FT_AbortPipe(mFTHandle, STREAM_BULK_READ_ADDRESS);
    FT_AbortPipe(mFTHandle, CONTROL_BULK_READ_ADDRESS);
    FT_AbortPipe(mFTHandle, CONTROL_BULK_WRITE_ADDRESS);
    FT_AbortPipe(mFTHandle, STREAM_BULK_WRITE_ADDRESS);
    FT_SetStreamPipe(mFTHandle, FALSE, FALSE, CONTROL_BULK_READ_ADDRESS, 64);
    FT_SetStreamPipe(mFTHandle, FALSE, FALSE, CONTROL_BULK_WRITE_ADDRESS, 64);
    FT_SetStreamPipe(mFTHandle, FALSE, FALSE, STREAM_BULK_READ_ADDRESS, sizeof(FPGA_DataPacket));
    FT_SetStreamPipe(mFTHandle, FALSE, FALSE, STREAM_BULK_WRITE_ADDRESS, sizeof(FPGA_DataPacket));
    FT_SetPipeTimeout(mFTHandle, CONTROL_BULK_WRITE_ADDRESS, 500);
    FT_SetPipeTimeout(mFTHandle, CONTROL_BULK_READ_ADDRESS, 500);
    FT_SetPipeTimeout(mFTHandle, STREAM_BULK_READ_ADDRESS, 0);
    FT_SetPipeTimeout(mFTHandle, STREAM_BULK_WRITE_ADDRESS, 0);

    isConnected = true;
#else
    bool isSuccessful = USBGeneric::Connect(vid, pid, serial);
    if (!isSuccessful)
    {
        return false;
    }

    FT_FlushPipe(CONTROL_BULK_READ_ADDRESS); // Clear control endpoint rx buffer
    FT_SetStreamPipe(CONTROL_BULK_READ_ADDRESS, 64);
    FT_SetStreamPipe(CONTROL_BULK_WRITE_ADDRESS, 64);
#endif
    contexts = new USBTransferContext_FT601[USB_MAX_CONTEXTS];
    return true;
}

void FT601::Disconnect()
{
    USBGeneric::Disconnect();
#ifndef __unix__
    FT_Close(mFTHandle);
#else
    if (dev_handle != nullptr)
    {
        FT_FlushPipe(STREAM_BULK_READ_ADDRESS);
        FT_FlushPipe(CONTROL_BULK_READ_ADDRESS);
        libusb_release_interface(dev_handle, 0);
        libusb_release_interface(dev_handle, 1);
        libusb_close(dev_handle);
        dev_handle = nullptr;
    }
#endif
    isConnected = false;
}

#ifndef __unix__
int32_t FT601::BulkTransfer(uint8_t endPointAddr, uint8_t* data, int length, int32_t timeout_ms)
{
    ULONG ulBytesTransferred = 0;
    FT_STATUS ftStatus = FT_OK;
    OVERLAPPED vOverlapped = { 0 };
    FT_InitializeOverlapped(mFTHandle, &vOverlapped);

    if (endPointAddr == CONTROL_BULK_WRITE_ADDRESS)
    {
        ftStatus = FT_WritePipe(mFTHandle, CONTROL_BULK_WRITE_ADDRESS, data, length, &ulBytesTransferred, &vOverlapped);
    }
    else
    {
        ftStatus = FT_ReadPipe(mFTHandle, CONTROL_BULK_READ_ADDRESS, data, length, &ulBytesTransferred, &vOverlapped);
    }

    if (ftStatus != FT_IO_PENDING)
    {
        FT_ReleaseOverlapped(mFTHandle, &vOverlapped);
        ReinitPipe(endPointAddr);
        return -1;
    }

    DWORD dwRet = WaitForSingleObject(vOverlapped.hEvent, timeout_ms);
    if (dwRet == WAIT_OBJECT_0 || dwRet == WAIT_TIMEOUT)
    {
        if (FT_GetOverlappedResult(mFTHandle, &vOverlapped, &ulBytesTransferred, FALSE) == FALSE)
        {
            ReinitPipe(endPointAddr);
            ulBytesTransferred = -1;
        }
    }
    else
    {
        ReinitPipe(endPointAddr);
        ulBytesTransferred = -1;
    }

    FT_ReleaseOverlapped(mFTHandle, &vOverlapped);
    return ulBytesTransferred;
}
#endif

int32_t FT601::ControlTransfer(int requestType, int request, int value, int index, uint8_t* data, uint32_t length, int32_t timeout)
{
    throw(OperationNotSupported("ControlTransfer not supported on FT601 connections."));
}

#ifndef __unix__
int FT601::BeginDataXfer(uint8_t* buffer, uint32_t length, uint8_t endPointAddr)
{
    int index = GetUSBContextIndex();

    if (index < 0)
    {
        return -1;
    }

    ULONG ulActual;
    FT_STATUS ftStatus = FT_OK;
    FT_InitializeOverlapped(mFTHandle, &contexts[index].inOvLap);

    if (endPointAddr == STREAM_BULK_READ_ADDRESS)
    {
        ftStatus = FT_ReadPipe(mFTHandle, STREAM_BULK_READ_ADDRESS, buffer, length, &ulActual, &contexts[index].inOvLap);
    }
    else
    {
        ftStatus = FT_WritePipe(mFTHandle, STREAM_BULK_WRITE_ADDRESS, buffer, length, &ulActual, &contexts[index].inOvLap);
    }

    contexts[index].endPointAddr = endPointAddr;

    if (ftStatus != FT_IO_PENDING)
    {
        lime::error("ERROR BEGIN DATA TRANSFER %d", ftStatus);
        contexts[index].used = false;
        return -1;
    }

    return index;
}

bool FT601::WaitForXfer(int contextHandle, uint32_t timeout_ms)
{
    if (contextHandle >= 0 && contexts[contextHandle].used == true)
    {
        DWORD dwRet = WaitForSingleObject(contexts[contextHandle].inOvLap.hEvent, timeout_ms);

        if (dwRet == WAIT_OBJECT_0)
        {
            return true;
        }
    }

    return true; //there is nothing to wait for (signal wait finished)
}

int FT601::FinishDataXfer(uint8_t* buffer, uint32_t length, int contextHandle)
{
    if (contextHandle >= 0 && contexts[contextHandle].used == true)
    {
        ULONG ulActualBytesTransferred;
        FT_STATUS ftStatus = FT_OK;

        ftStatus = FT_GetOverlappedResult(mFTHandle, &contexts[contextHandle].inOvLap, &ulActualBytesTransferred, FALSE);

        if (ftStatus != FT_OK)
        {
            length = 0;
        }
        else
        {
            length = ulActualBytesTransferred;
        }

        FT_ReleaseOverlapped(mFTHandle, &contexts[contextHandle].inOvLap);
        contexts[contextHandle].used = false;
        return length;
    }

    return 0;
}

void FT601::AbortEndpointXfers(uint8_t endPointAddr)
{
    FT_AbortPipe(mFTHandle, endPointAddr);

    for (int i = 0; i < USB_MAX_CONTEXTS; ++i)
    {
        if (contexts[i].used == true && contexts[i].endPointAddr == endPointAddr)
        {
            FT_ReleaseOverlapped(mFTHandle, &contexts[i].inOvLap);
            contexts[i].used = false;
        }
    }

    if (endPointAddr == STREAM_BULK_READ_ADDRESS)
    {
        FT_FlushPipe(mFTHandle, STREAM_BULK_READ_ADDRESS);
    }

    FT_SetStreamPipe(mFTHandle, FALSE, FALSE, endPointAddr, sizeof(FPGA_DataPacket));

    WaitForXfers(endPointAddr);
}
#endif

int FT601::ResetStreamBuffers()
{
#ifndef __unix__
    if (FT_AbortPipe(mFTHandle, STREAM_BULK_READ_ADDRESS) != FT_OK)
    {
        return -1;
    }

    if (FT_AbortPipe(mFTHandle, STREAM_BULK_WRITE_ADDRESS) != FT_OK)
    {
        return -1;
    }

    if (FT_FlushPipe(mFTHandle, STREAM_BULK_READ_ADDRESS) != FT_OK)
    {
        return -1;
    }

    if (FT_SetStreamPipe(mFTHandle, FALSE, FALSE, STREAM_BULK_READ_ADDRESS, sizeof(FPGA_DataPacket)) != 0)
    {
        return -1;
    }

    if (FT_SetStreamPipe(mFTHandle, FALSE, FALSE, STREAM_BULK_WRITE_ADDRESS, sizeof(FPGA_DataPacket)) != 0)
    {
        return -1;
    }
#else
    if (FT_FlushPipe(STREAM_BULK_WRITE_ADDRESS) != 0)
    {
        return -1;
    }

    if (FT_FlushPipe(STREAM_BULK_READ_ADDRESS) != 0)
    {
        return -1;
    }

    if (FT_SetStreamPipe(STREAM_BULK_WRITE_ADDRESS, sizeof(FPGA_DataPacket)) != 0)
    {
        return -1;
    }

    if (FT_SetStreamPipe(STREAM_BULK_READ_ADDRESS, sizeof(FPGA_DataPacket)) != 0)
    {
        return -1;
    }
#endif
    return 0;
}

#ifndef __unix__
int FT601::ReinitPipe(unsigned char ep)
{
    FT_AbortPipe(mFTHandle, ep);
    FT_FlushPipe(mFTHandle, ep);
    FT_SetStreamPipe(mFTHandle, FALSE, FALSE, ep, 64);
    return 0;
}
#endif

#ifdef __unix__
int FT601::FT_FlushPipe(unsigned char ep)
{
    unsigned char wbuffer[20]{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    mUsbCounter++;
    wbuffer[0] = (mUsbCounter)&0xFF;
    wbuffer[1] = (mUsbCounter >> 8) & 0xFF;
    wbuffer[2] = (mUsbCounter >> 16) & 0xFF;
    wbuffer[3] = (mUsbCounter >> 24) & 0xFF;
    wbuffer[4] = ep;

    int actual = BulkTransfer(0x01, wbuffer, 20, 1000);
    if (actual != 20)
    {
        return -1;
    }

    mUsbCounter++;
    wbuffer[0] = (mUsbCounter)&0xFF;
    wbuffer[1] = (mUsbCounter >> 8) & 0xFF;
    wbuffer[2] = (mUsbCounter >> 16) & 0xFF;
    wbuffer[3] = (mUsbCounter >> 24) & 0xFF;
    wbuffer[4] = ep;
    wbuffer[5] = 0x03;

    actual = BulkTransfer(0x01, wbuffer, 20, 1000);
    if (actual != 20)
    {
        return -1;
    }

    return 0;
}

int FT601::FT_SetStreamPipe(unsigned char ep, size_t size)
{
    unsigned char wbuffer[20] = { 0 };

    mUsbCounter++;
    wbuffer[0] = (mUsbCounter)&0xFF;
    wbuffer[1] = (mUsbCounter >> 8) & 0xFF;
    wbuffer[2] = (mUsbCounter >> 16) & 0xFF;
    wbuffer[3] = (mUsbCounter >> 24) & 0xFF;
    wbuffer[4] = ep;

    int actual = BulkTransfer(0x01, wbuffer, 20, 1000);
    if (actual != 20)
    {
        return -1;
    }

    mUsbCounter++;
    wbuffer[0] = (mUsbCounter)&0xFF;
    wbuffer[1] = (mUsbCounter >> 8) & 0xFF;
    wbuffer[2] = (mUsbCounter >> 16) & 0xFF;
    wbuffer[3] = (mUsbCounter >> 24) & 0xFF;
    wbuffer[5] = 0x02;
    wbuffer[8] = (size)&0xFF;
    wbuffer[9] = (size >> 8) & 0xFF;
    wbuffer[10] = (size >> 16) & 0xFF;
    wbuffer[11] = (size >> 24) & 0xFF;

    actual = BulkTransfer(0x01, wbuffer, 20, 1000);

    if (actual != 20)
    {
        return -1;
    }

    return 0;
}
#endif