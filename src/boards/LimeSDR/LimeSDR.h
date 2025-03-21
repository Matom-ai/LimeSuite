#ifndef LIME_LIMESDR_H
#define LIME_LIMESDR_H

#include "LMS7002M_SDRDevice.h"
#include "protocols/LMS64CProtocol.h"

#include <vector>
#include <memory>

namespace lime {

class USBGeneric;

/** @brief Class for managing the LimeSDR-USB device. */
class LimeSDR : public LMS7002M_SDRDevice
{
  public:
    LimeSDR(std::shared_ptr<IComms> spiLMS,
        std::shared_ptr<IComms> spiFPGA,
        std::shared_ptr<USBGeneric> mStreamPort,
        std::shared_ptr<ISerialPort> commsPort);
    virtual ~LimeSDR();

    virtual OpStatus Configure(const SDRConfig& config, uint8_t moduleIndex) override;

    virtual OpStatus Init() override;
    virtual OpStatus Reset() override;

    virtual double GetClockFreq(uint8_t clk_id, uint8_t channel) override;
    virtual OpStatus SetClockFreq(uint8_t clk_id, double freq, uint8_t channel) override;

    virtual OpStatus SetSampleRate(
        uint8_t moduleIndex, TRXDir trx, uint8_t channel, double sampleRate, uint8_t oversample) override;

    virtual OpStatus SPI(uint32_t chipSelect, const uint32_t* MOSI, uint32_t* MISO, uint32_t count) override;

    virtual OpStatus StreamSetup(const StreamConfig& config, uint8_t moduleIndex) override;

    virtual void StreamStart(uint8_t moduleIndex) override;
    virtual void StreamStop(uint8_t moduleIndex) override;

    virtual void* GetInternalChip(uint32_t index) override;

    virtual OpStatus GPIODirRead(uint8_t* buffer, const size_t bufLength) override;
    virtual OpStatus GPIORead(uint8_t* buffer, const size_t bufLength) override;
    virtual OpStatus GPIODirWrite(const uint8_t* buffer, const size_t bufLength) override;
    virtual OpStatus GPIOWrite(const uint8_t* buffer, const size_t bufLength) override;

    virtual OpStatus CustomParameterWrite(const std::vector<CustomParameterIO>& parameters) override;
    virtual OpStatus CustomParameterRead(std::vector<CustomParameterIO>& parameters) override;

    virtual OpStatus UploadMemory(
        eMemoryDevice device, uint8_t moduleIndex, const char* data, size_t length, UploadMemoryCallback callback) override;
    virtual OpStatus MemoryWrite(std::shared_ptr<DataStorage> storage, Region region, const void* data) override;
    virtual OpStatus MemoryRead(std::shared_ptr<DataStorage> storage, Region region, void* data) override;

  protected:
    OpStatus EnableChannel(TRXDir dir, uint8_t channel, bool enabled);
    SDRDevice::Descriptor GetDeviceInfo();
    void ResetUSBFIFO();
    static OpStatus UpdateFPGAInterface(void* userData);

  private:
    std::shared_ptr<USBGeneric> mStreamPort;
    std::shared_ptr<ISerialPort> mSerialPort;
    std::shared_ptr<IComms> mlms7002mPort;
    std::shared_ptr<IComms> mfpgaPort;
    bool mConfigInProgress;
};

} // namespace lime

#endif /* LIME_LIMESDR_H */
