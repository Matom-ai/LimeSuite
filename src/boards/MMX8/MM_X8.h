#ifndef LIME_LIMESDR_5G_H
#define LIME_LIMESDR_5G_H

#include "ADF4002/ADF4002.h"
#include "CDCM6208/CDCM6208_Dev.h"
#include "limesuite/IComms.h"
#include "limesuite/SDRDevice.h"
#include "PacketsFIFO.h"
#include "protocols/LMS64CProtocol.h"

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace lime {

class LitePCIe;
class LimeSDR_XTRX;

/** @brief Class for managing the LimeSDR-MMX8 device and its subdevices. */
class LimeSDR_MMX8 : public SDRDevice
{
  public:
    LimeSDR_MMX8() = delete;
    LimeSDR_MMX8(std::vector<std::shared_ptr<IComms>>& spiLMS7002M,
        std::vector<std::shared_ptr<IComms>>& spiFPGA,
        std::vector<std::shared_ptr<LitePCIe>> trxStreams,
        std::shared_ptr<ISerialPort> control,
        std::shared_ptr<ISPI> adfComms);
    virtual ~LimeSDR_MMX8();

    virtual OpStatus Configure(const SDRConfig& config, uint8_t socIndex) override;
    virtual const Descriptor& GetDescriptor() const override;

    virtual OpStatus Init() override;
    virtual OpStatus Reset() override;
    virtual OpStatus GetGPSLock(GPS_Lock* status) override;

    virtual OpStatus EnableChannel(uint8_t moduleIndex, TRXDir trx, uint8_t channel, bool enable) override;

    virtual double GetFrequency(uint8_t moduleIndex, TRXDir trx, uint8_t channel) override;
    virtual OpStatus SetFrequency(uint8_t moduleIndex, TRXDir trx, uint8_t channel, double frequency) override;

    virtual double GetNCOFrequency(uint8_t moduleIndex, TRXDir trx, uint8_t channel, uint8_t index) override;
    virtual OpStatus SetNCOFrequency(
        uint8_t moduleIndex, TRXDir trx, uint8_t channel, uint8_t index, double frequency, double phaseOffset = -1.0) override;

    virtual double GetNCOOffset(uint8_t moduleIndex, TRXDir trx, uint8_t channel) override;

    virtual double GetSampleRate(uint8_t moduleIndex, TRXDir trx, uint8_t channel) override;
    virtual OpStatus SetSampleRate(
        uint8_t moduleIndex, TRXDir trx, uint8_t channel, double sampleRate, uint8_t oversample) override;

    virtual double GetLowPassFilter(uint8_t moduleIndex, TRXDir trx, uint8_t channel) override;
    virtual OpStatus SetLowPassFilter(uint8_t moduleIndex, TRXDir trx, uint8_t channel, double lpf) override;

    virtual uint8_t GetAntenna(uint8_t moduleIndex, TRXDir trx, uint8_t channel) override;
    virtual OpStatus SetAntenna(uint8_t moduleIndex, TRXDir trx, uint8_t channel, uint8_t path) override;

    virtual OpStatus SetTestSignal(uint8_t moduleIndex,
        TRXDir direction,
        uint8_t channel,
        ChannelConfig::Direction::TestSignal signalConfiguration,
        int16_t dc_i = 0,
        int16_t dc_q = 0) override;
    virtual ChannelConfig::Direction::TestSignal GetTestSignal(uint8_t moduleIndex, TRXDir direction, uint8_t channel) override;

    virtual double GetClockFreq(uint8_t clk_id, uint8_t channel) override;
    virtual OpStatus SetClockFreq(uint8_t clk_id, double freq, uint8_t channel) override;

    virtual OpStatus SetGain(uint8_t moduleIndex, TRXDir direction, uint8_t channel, eGainTypes gain, double value) override;
    virtual OpStatus GetGain(uint8_t moduleIndex, TRXDir direction, uint8_t channel, eGainTypes gain, double& value) override;

    virtual bool GetDCOffsetMode(uint8_t moduleIndex, TRXDir trx, uint8_t channel) override;
    virtual OpStatus SetDCOffsetMode(uint8_t moduleIndex, TRXDir trx, uint8_t channel, bool isAutomatic) override;

    virtual complex64f_t GetDCOffset(uint8_t moduleIndex, TRXDir trx, uint8_t channel) override;
    virtual OpStatus SetDCOffset(uint8_t moduleIndex, TRXDir trx, uint8_t channel, const complex64f_t& offset) override;

    virtual complex64f_t GetIQBalance(uint8_t moduleIndex, TRXDir trx, uint8_t channel) override;
    virtual OpStatus SetIQBalance(uint8_t moduleIndex, TRXDir trx, uint8_t channel, const complex64f_t& balance) override;

    virtual bool GetCGENLocked(uint8_t moduleIndex) override;
    virtual double GetTemperature(uint8_t moduleIndex) override;

    virtual bool GetSXLocked(uint8_t moduleIndex, TRXDir trx) override;

    virtual unsigned int ReadRegister(uint8_t moduleIndex, unsigned int address, bool useFPGA = false) override;
    virtual OpStatus WriteRegister(uint8_t moduleIndex, unsigned int address, unsigned int value, bool useFPGA = false) override;

    virtual OpStatus LoadConfig(uint8_t moduleIndex, const std::string& filename) override;
    virtual OpStatus SaveConfig(uint8_t moduleIndex, const std::string& filename) override;

    virtual uint16_t GetParameter(uint8_t moduleIndex, uint8_t channel, const std::string& parameterKey) override;
    virtual OpStatus SetParameter(uint8_t moduleIndex, uint8_t channel, const std::string& parameterKey, uint16_t value) override;

    virtual uint16_t GetParameter(uint8_t moduleIndex, uint8_t channel, uint16_t address, uint8_t msb, uint8_t lsb) override;
    virtual OpStatus SetParameter(
        uint8_t moduleIndex, uint8_t channel, uint16_t address, uint8_t msb, uint8_t lsb, uint16_t value) override;

    virtual OpStatus Synchronize(bool toChip) override;
    virtual void EnableCache(bool enable) override;

    virtual OpStatus Calibrate(uint8_t moduleIndex, TRXDir trx, uint8_t channel, double bandwidth) override;
    virtual OpStatus ConfigureGFIR(
        uint8_t moduleIndex, TRXDir trx, uint8_t channel, ChannelConfig::Direction::GFIRFilter settings) override;

    virtual std::vector<double> GetGFIRCoefficients(uint8_t moduleIndex, TRXDir trx, uint8_t channel, uint8_t gfirID) override;
    virtual OpStatus SetGFIRCoefficients(
        uint8_t moduleIndex, TRXDir trx, uint8_t channel, uint8_t gfirID, std::vector<double> coefficients) override;
    virtual OpStatus SetGFIR(uint8_t moduleIndex, TRXDir trx, uint8_t channel, uint8_t gfirID, bool enabled) override;

    virtual uint64_t GetHardwareTimestamp(uint8_t moduleIndex) override;
    virtual OpStatus SetHardwareTimestamp(uint8_t moduleIndex, const uint64_t now) override;

    virtual OpStatus StreamSetup(const StreamConfig& config, uint8_t moduleIndex) override;
    virtual void StreamStart(uint8_t moduleIndex) override;
    virtual void StreamStart(const std::vector<uint8_t> moduleIndexes) override;
    virtual void StreamStop(uint8_t moduleIndex) override;
    virtual void StreamStop(const std::vector<uint8_t> moduleIndexes) override;

    virtual uint32_t StreamRx(uint8_t moduleIndex, lime::complex32f_t* const* samples, uint32_t count, StreamMeta* meta) override;
    virtual uint32_t StreamRx(uint8_t moduleIndex, lime::complex16_t* const* samples, uint32_t count, StreamMeta* meta) override;
    virtual uint32_t StreamRx(uint8_t moduleIndex, lime::complex12_t* const* samples, uint32_t count, StreamMeta* meta) override;
    virtual uint32_t StreamTx(
        uint8_t moduleIndex, const lime::complex32f_t* const* samples, uint32_t count, const StreamMeta* meta) override;
    virtual uint32_t StreamTx(
        uint8_t moduleIndex, const lime::complex16_t* const* samples, uint32_t count, const StreamMeta* meta) override;
    virtual uint32_t StreamTx(
        uint8_t moduleIndex, const lime::complex12_t* const* samples, uint32_t count, const StreamMeta* meta) override;
    virtual void StreamStatus(uint8_t moduleIndex, SDRDevice::StreamStats* rx, SDRDevice::StreamStats* tx) override;

    virtual OpStatus SPI(uint32_t chipSelect, const uint32_t* MOSI, uint32_t* MISO, uint32_t count) override;

    virtual OpStatus CustomParameterWrite(const std::vector<CustomParameterIO>& parameters) override;
    virtual OpStatus CustomParameterRead(std::vector<CustomParameterIO>& parameters) override;

    virtual void SetDataLogCallback(DataCallbackType callback){};
    virtual void SetMessageLogCallback(LogCallbackType callback) override;

    virtual void* GetInternalChip(uint32_t index) override;

    virtual OpStatus UploadMemory(
        eMemoryDevice device, uint8_t moduleIndex, const char* data, size_t length, UploadMemoryCallback callback) override;
    virtual OpStatus MemoryWrite(std::shared_ptr<DataStorage> storage, Region region, const void* data) override;
    virtual OpStatus MemoryRead(std::shared_ptr<DataStorage> storage, Region region, void* data) override;
    virtual OpStatus UploadTxWaveform(
        const StreamConfig& config, uint8_t moduleIndex, const void** samples, uint32_t count) override;

  private:
    std::shared_ptr<IComms> mMainFPGAcomms;
    Descriptor mDeviceDescriptor;
    std::vector<std::shared_ptr<LitePCIe>> mTRXStreamPorts;
    std::vector<LimeSDR_XTRX*> mSubDevices;
    std::map<uint32_t, LimeSDR_XTRX*> chipSelectToDevice;
    std::map<uint32_t, LimeSDR_XTRX*> customParameterToDevice;
    lime::ADF4002* mADF;
};

} // namespace lime

#endif // LIME_LIMESDR_5G_H
