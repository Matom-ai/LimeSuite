#pragma once
#include "USBGeneric.h"

#include <vector>
#include <set>
#include <string>
#include <atomic>
#include <memory>
#include <thread>
#include <ciso646>

namespace lime {

/** @brief A class for communicating with devices using the Cypress USB 3.0 CYUSB3014-BZXC USB controller. */
class FX3 : public USBGeneric
{
  public:
    /**
      @brief Constructs the class for communicating with the FX3 controller.
      @param usbContext The USB context to use for the communication.
     */
    FX3(void* usbContext = nullptr);
    virtual ~FX3();

    virtual bool Connect(uint16_t vid, uint16_t pid, const std::string& serial = "") override;
    virtual void Disconnect() override;

#ifndef __unix__
    virtual int BeginDataXfer(uint8_t* buffer, uint32_t length, uint8_t endPointAddr) override;
    virtual bool WaitForXfer(int contextHandle, int32_t timeout_ms) override;
    virtual int FinishDataXfer(uint8_t* buffer, uint32_t length, int contextHandle) override;
    virtual void AbortEndpointXfers(uint8_t endPointAddr) override;
#endif
};

} // namespace lime
