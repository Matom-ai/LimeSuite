#include "LimeSDR.h"

#include "USBGeneric.h"
#include "LMSBoards.h"
#include "limesuite/LMS7002M.h"
#include "Si5351C/Si5351C.h"
#include "LMS64CProtocol.h"
#include "Logger.h"
#include "FPGA_common.h"
#include "TRXLooper_USB.h"
#include "limesuite/LMS7002M_parameters.h"
#include "lms7002m/LMS7002M_validation.h"
#include "protocols/LMS64CProtocol.h"
#include "limesuite/DeviceNode.h"
#include "FX3/FX3.h"

#include <array>
#include <cassert>
#include <cmath>
#include <memory>
#include <set>
#include <stdexcept>

#ifdef __unix__
    #ifdef __GNUC__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
    #endif
    #include <libusb.h>
    #ifdef __GNUC__
        #pragma GCC diagnostic pop
    #endif
#endif

using namespace lime;

static const uint8_t SPI_LMS7002M = 0;
static const uint8_t SPI_FPGA = 1;
static const uint8_t SPI_ADF4002 = 2;

static const SDRDevice::CustomParameter CP_VCTCXO_DAC = { "VCTCXO DAC (volatile)", 0, 0, 65535, false };
static const SDRDevice::CustomParameter CP_TEMPERATURE = { "Board Temperature", 1, 0, 65535, true };

static const std::vector<std::pair<uint16_t, uint16_t>> lms7002defaultsOverrides = { //
    { 0x0022, 0x0FFF },
    { 0x0023, 0x5550 },
    { 0x002B, 0x0038 },
    { 0x002C, 0x0000 },
    { 0x002D, 0x0641 },
    { 0x0086, 0x4101 },
    { 0x0087, 0x5555 },
    { 0x0088, 0x0525 },
    { 0x0089, 0x1078 },
    { 0x008B, 0x218C },
    { 0x008C, 0x267B },
    { 0x00A6, 0x000F },
    { 0x00A9, 0x8000 },
    { 0x00AC, 0x2000 },
    { 0x0108, 0x218C },
    { 0x0109, 0x57C1 },
    { 0x010A, 0x154C },
    { 0x010B, 0x0001 },
    { 0x010C, 0x8865 },
    { 0x010D, 0x011A },
    { 0x010E, 0x0000 },
    { 0x010F, 0x3142 },
    { 0x0110, 0x2B14 },
    { 0x0111, 0x0000 },
    { 0x0112, 0x000C },
    { 0x0113, 0x03C2 },
    { 0x0114, 0x01F0 },
    { 0x0115, 0x000D },
    { 0x0118, 0x418C },
    { 0x0119, 0x5292 },
    { 0x011A, 0x3001 },
    { 0x011C, 0x8941 },
    { 0x011D, 0x0000 },
    { 0x011E, 0x0984 },
    { 0x0120, 0xE6C0 },
    { 0x0121, 0x3638 },
    { 0x0122, 0x0514 },
    { 0x0123, 0x200F },
    { 0x0200, 0x00E1 },
    { 0x0208, 0x017B },
    { 0x020B, 0x4000 },
    { 0x020C, 0x8000 },
    { 0x0400, 0x8081 },
    { 0x0404, 0x0006 },
    { 0x040B, 0x1020 },
    { 0x040C, 0x00FB }
};

static inline void ValidateChannel(uint8_t channel)
{
    if (channel > 1)
        throw std::logic_error("invalid channel index");
}

/// @brief Constructs a new LimeSDR object
/// @param spiLMS The communications port to the LMS7002M chip.
/// @param spiFPGA The communications port to the device's FPGA.
/// @param streamPort The communications port to send and receive sample data.
/// @param commsPort The communications port for direct communications with the device.
LimeSDR::LimeSDR(std::shared_ptr<IComms> spiLMS,
    std::shared_ptr<IComms> spiFPGA,
    std::shared_ptr<USBGeneric> streamPort,
    std::shared_ptr<ISerialPort> commsPort)
    : mStreamPort(streamPort)
    , mSerialPort(commsPort)
    , mlms7002mPort(spiLMS)
    , mfpgaPort(spiFPGA)
    , mConfigInProgress(false)
{
    SDRDevice::Descriptor descriptor = GetDeviceInfo();

    LMS7002M* chip = new LMS7002M(mlms7002mPort);
    chip->ModifyRegistersDefaults(lms7002defaultsOverrides);
    chip->SetConnection(mlms7002mPort);
    chip->SetOnCGENChangeCallback(UpdateFPGAInterface, this);
    mLMSChips.push_back(chip);

    mFPGA = new FPGA(spiFPGA, spiLMS);
    FPGA::GatewareInfo gw = mFPGA->GetGatewareInfo();
    FPGA::GatewareToDescriptor(gw, descriptor);

    mStreamers.resize(1, nullptr);

    descriptor.customParameters.push_back(CP_VCTCXO_DAC);
    descriptor.customParameters.push_back(CP_TEMPERATURE);

    descriptor.spiSlaveIds = { { "LMS7002M", SPI_LMS7002M }, { "FPGA", SPI_FPGA } };

    RFSOCDescriptor soc;
    soc.name = "LMS";
    soc.channelCount = 2;
    soc.pathNames[TRXDir::Rx] = { "None", "LNAH", "LNAL", "LNAW", "LB1", "LB2" };
    soc.pathNames[TRXDir::Tx] = { "None", "Band1", "Band2" };
    soc.samplingRateRange = { 100e3, 61.44e6, 0 };
    soc.frequencyRange = { 100e3, 3.8e9, 0 };

    soc.lowPassFilterRange[TRXDir::Rx] = { 1.4001e6, 130e6 };
    soc.lowPassFilterRange[TRXDir::Tx] = { 5e6, 130e6 };

    soc.antennaRange[TRXDir::Rx]["LNAH"] = { 2e9, 2.6e9 };
    soc.antennaRange[TRXDir::Rx]["LNAL"] = { 700e6, 900e6 };
    soc.antennaRange[TRXDir::Rx]["LNAW"] = { 700e6, 2.6e9 };
    soc.antennaRange[TRXDir::Rx]["LB1"] = soc.antennaRange[TRXDir::Rx]["LNAL"];
    soc.antennaRange[TRXDir::Rx]["LB2"] = soc.antennaRange[TRXDir::Rx]["LNAW"];
    soc.antennaRange[TRXDir::Tx]["Band1"] = { 30e6, 1.9e9 };
    soc.antennaRange[TRXDir::Tx]["Band2"] = { 2e9, 2.6e9 };

    SetGainInformationInDescriptor(soc);

    descriptor.rfSOC.push_back(soc);

    auto fpgaNode = std::make_shared<DeviceNode>("FPGA", eDeviceNodeClass::FPGA, mFPGA);
    fpgaNode->children.push_back(std::make_shared<DeviceNode>("LMS", eDeviceNodeClass::LMS7002M, mLMSChips[0]));
    descriptor.socTree = std::make_shared<DeviceNode>("SDR-USB", eDeviceNodeClass::SDRDevice, this);
    descriptor.socTree->children.push_back(fpgaNode);

    const std::unordered_map<eMemoryRegion, Region> eepromMap = { { eMemoryRegion::VCTCXO_DAC, { 16, 2 } } };
    descriptor.memoryDevices[MEMORY_DEVICES_TEXT.at(eMemoryDevice::FPGA_FLASH)] =
        std::make_shared<DataStorage>(this, eMemoryDevice::FPGA_FLASH);
    descriptor.memoryDevices[MEMORY_DEVICES_TEXT.at(eMemoryDevice::EEPROM)] =
        std::make_shared<DataStorage>(this, eMemoryDevice::EEPROM, eepromMap);

    mDeviceDescriptor = descriptor;

    //must configure synthesizer before using LimeSDR
    /*if (info.device == LMS_DEV_LIMESDR && info.hardware < 4)
    {
        auto si5351module = std::make_shared<Si5351C>();
        si5351module->Initialize(conn);
        si5351module->SetPLL(0, 25000000, 0);
        si5351module->SetPLL(1, 25000000, 0);
        si5351module->SetClock(0, 27000000, true, false);
        si5351module->SetClock(1, 27000000, true, false);
        for (int i = 2; i < 8; ++i)
            si5351module->SetClock(i, 27000000, false, false);
        Si5351C::Status status = si5351module->ConfigureClocks();
        if (status != Si5351C::SUCCESS)
        {
            lime::warning("Failed to configure Si5351C");
            return;
        }
        status = si5351module->UploadConfiguration();
        if (status != Si5351C::SUCCESS)
            lime::warning("Failed to upload Si5351C configuration");
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); //some settle time
    }*/
}

LimeSDR::~LimeSDR()
{
    auto& streamer = mStreamers.at(0);
    if (streamer != nullptr && streamer->IsStreamRunning())
    {
        streamer->Stop();
    }
}

OpStatus LimeSDR::Configure(const SDRConfig& cfg, uint8_t moduleIndex = 0)
{
    OpStatus status = OpStatus::SUCCESS;
    std::vector<std::string> errors;
    bool isValidConfig = LMS7002M_Validate(cfg, errors);

    if (!isValidConfig)
    {
        std::stringstream ss;
        for (const auto& err : errors)
            ss << err << std::endl;
        return lime::ReportError(OpStatus::ERROR, "LimeSDR: %s.", ss.str().c_str());
    }

    bool rxUsed = false;
    bool txUsed = false;
    for (int i = 0; i < 2; ++i)
    {
        const ChannelConfig& ch = cfg.channel[i];
        rxUsed |= ch.rx.enabled;
        txUsed |= ch.tx.enabled;
    }

    try
    {
        mConfigInProgress = true;
        LMS7002M* chip = mLMSChips.at(0);
        if (!cfg.skipDefaults)
        {
            const bool skipTune = true;
            // TODO: skip tune
            status = Init();
            if (status != OpStatus::SUCCESS)
                return status;
        }

        status = LMS7002LOConfigure(chip, cfg);
        if (status != OpStatus::SUCCESS)
            return lime::ReportError(OpStatus::ERROR, "LimeSDR: LO configuration failed.");
        for (int i = 0; i < 2; ++i)
        {
            status = LMS7002ChannelConfigure(chip, cfg.channel[i], i);
            if (status != OpStatus::SUCCESS)
                return lime::ReportError(OpStatus::ERROR, "LimeSDR: channel%i configuration failed.");
            LMS7002TestSignalConfigure(chip, cfg.channel[i], i);
        }

        // enabled ADC/DAC is required for FPGA to work
        chip->Modify_SPI_Reg_bits(LMS7_PD_RX_AFE1, 0);
        chip->Modify_SPI_Reg_bits(LMS7_PD_TX_AFE1, 0);
        chip->SetActiveChannel(LMS7002M::Channel::ChA);

        double sampleRate;
        if (rxUsed)
            sampleRate = cfg.channel[0].rx.sampleRate;
        else
            sampleRate = cfg.channel[0].tx.sampleRate;
        if (sampleRate > 0)
        {
            status = SetSampleRate(0, TRXDir::Rx, 0, sampleRate, cfg.channel[0].rx.oversample);
            if (status != OpStatus::SUCCESS)
                return lime::ReportError(OpStatus::ERROR, "LimeSDR: failed to set sampling rate.");
        }

        for (int i = 0; i < 2; ++i)
        {
            const SDRDevice::ChannelConfig& ch = cfg.channel[i];
            LMS7002ChannelCalibration(chip, ch, i);
            // TODO: should report calibration failure, but configuration can
            // still work after failed calibration.
        }
        chip->SetActiveChannel(LMS7002M::Channel::ChA);

        // Workaround: Toggle LimeLights transmit port to flush residual value from data interface
        // uint16_t txMux = chip->Get_SPI_Reg_bits(LMS7param(TX_MUX));
        // chip->Modify_SPI_Reg_bits(LMS7param(TX_MUX), 2);
        // chip->Modify_SPI_Reg_bits(LMS7param(TX_MUX), txMux);

        mConfigInProgress = false;
        if (sampleRate > 0)
        {
            status = UpdateFPGAInterface(this);
            if (status != OpStatus::SUCCESS)
                return lime::ReportError(OpStatus::ERROR, "LimeSDR: failed to update FPGA interface frequency.");
        }
    } //try
    catch (std::logic_error& e)
    {
        lime::error("LimeSDR_USB config: %s\n", e.what());
        return OpStatus::ERROR;
    } catch (std::runtime_error& e)
    {
        lime::error("LimeSDR_USB config: %s\n", e.what());
        return OpStatus::ERROR;
    }
    return OpStatus::SUCCESS;
}

// Callback for updating FPGA's interface clocks when LMS7002M CGEN is manually modified
OpStatus LimeSDR::UpdateFPGAInterface(void* userData)
{
    constexpr int chipIndex = 0;
    assert(userData != nullptr);
    LimeSDR* pthis = static_cast<LimeSDR*>(userData);
    // don't care about cgen changes while doing Config(), to avoid unnecessary fpga updates
    if (pthis->mConfigInProgress)
        return OpStatus::SUCCESS;
    LMS7002M* soc = pthis->mLMSChips[chipIndex];
    return UpdateFPGAInterfaceFrequency(*soc, *pthis->mFPGA, chipIndex);
}

OpStatus LimeSDR::SetSampleRate(uint8_t moduleIndex, TRXDir trx, uint8_t channel, double sampleRate, uint8_t oversample)
{
    const bool bypass = (oversample == 1) || (oversample == 0 && sampleRate > 62e6);
    uint8_t decimation = 7; // HBD_OVR_RXTSP=7 - bypass
    uint8_t interpolation = 7; // HBI_OVR_TXTSP=7 - bypass
    double cgenFreq = sampleRate * 4; // AI AQ BI BQ
    // TODO:
    // for (uint8_t i = 0; i < GetNumChannels(false) ;i++)
    // {
    //     if (rx_channels[i].cF_offset_nco != 0.0 || tx_channels[i].cF_offset_nco != 0.0)
    //     {
    //         bypass = false;
    //         break;
    //     }
    // }
    if (!bypass)
    {
        if (oversample == 0)
        {
            const int n = lime::LMS7002M::CGEN_MAX_FREQ / (cgenFreq);
            oversample = (n >= 32) ? 32 : (n >= 16) ? 16 : (n >= 8) ? 8 : (n >= 4) ? 4 : 2;
        }

        decimation = 4;
        if (oversample <= 16)
        {
            constexpr std::array<int, 17> decimationTable{ 0, 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3 };
            decimation = decimationTable.at(oversample);
        }
        interpolation = decimation;
        cgenFreq *= 2 << decimation;
    }

    if (bypass)
    {
        lime::info("Sampling rate set(%.3f MHz): CGEN:%.3f MHz, Decim: bypass, Interp: bypass", sampleRate / 1e6, cgenFreq / 1e6);
    }
    else
    {
        lime::info("Sampling rate set(%.3f MHz): CGEN:%.3f MHz, Decim: 2^%i, Interp: 2^%i",
            sampleRate / 1e6,
            cgenFreq / 1e6,
            decimation + 1,
            interpolation + 1); // dec/inter ratio is 2^(value+1)
    }
    lime::LMS7002M* lms = mLMSChips.at(moduleIndex);

    lms->Modify_SPI_Reg_bits(LMS7param(EN_ADCCLKH_CLKGN), 0);
    lms->Modify_SPI_Reg_bits(LMS7param(CLKH_OV_CLKL_CGEN), 2);
    lms->Modify_SPI_Reg_bits(LMS7param(MAC), 2);
    lms->Modify_SPI_Reg_bits(LMS7param(HBD_OVR_RXTSP), decimation);
    lms->Modify_SPI_Reg_bits(LMS7param(HBI_OVR_TXTSP), interpolation);
    lms->Modify_SPI_Reg_bits(LMS7param(MAC), 1);
    return lms->SetInterfaceFrequency(cgenFreq, interpolation, decimation);
}

OpStatus LimeSDR::Init()
{
    OpStatus status;
    lime::LMS7002M* lms = mLMSChips[0];
    // TODO: write GPIO to hard reset the chip
    status = lms->ResetChip();
    if (status != OpStatus::SUCCESS)
        return status;

    lms->Modify_SPI_Reg_bits(LMS7param(MAC), 1);

    // TODO:
    // if(lms->CalibrateTxGain(0,nullptr) != OpStatus::SUCCESS)
    //     return -1;

    EnableChannel(TRXDir::Rx, 0, false);
    EnableChannel(TRXDir::Tx, 0, false);
    lms->Modify_SPI_Reg_bits(LMS7param(MAC), 2);

    // if(lms->CalibrateTxGain(0,nullptr) != OpStatus::SUCCESS)
    //     return -1;

    EnableChannel(TRXDir::Rx, 1, false);
    EnableChannel(TRXDir::Tx, 1, false);

    lms->Modify_SPI_Reg_bits(LMS7param(MAC), 1);

    // if(lms->SetFrequency(SDRDevice::Dir::Tx, 0, lms->GetFrequency(SDRDevice::Dir::Tx, 0)) != OpStatus::SUCCESS)
    //     return -1;
    // if(lms->SetFrequency(SDRDevice::Dir::Rx, 0, lms->GetFrequency(SDRDevice::Dir::Rx, 0)) != OpStatus::SUCCESS)
    //     return -1;

    // if (SetRate(10e6,2)!=0)
    //     return -1;
    return status;
}

SDRDevice::Descriptor LimeSDR::GetDeviceInfo(void)
{
    assert(mSerialPort);
    SDRDevice::Descriptor deviceDescriptor;

    LMS64CProtocol::FirmwareInfo info;
    OpStatus returnCode = LMS64CProtocol::GetFirmwareInfo(*mSerialPort, info);

    if (returnCode != OpStatus::SUCCESS)
    {
        deviceDescriptor.name = GetDeviceName(LMS_DEV_UNKNOWN);
        deviceDescriptor.expansionName = GetExpansionBoardName(EXP_BOARD_UNKNOWN);

        return deviceDescriptor;
    }

    deviceDescriptor.name = GetDeviceName(static_cast<eLMS_DEV>(info.deviceId));
    deviceDescriptor.expansionName = GetExpansionBoardName(static_cast<eEXP_BOARD>(info.expansionBoardId));
    deviceDescriptor.firmwareVersion = std::to_string(info.firmware);
    deviceDescriptor.hardwareVersion = std::to_string(info.hardware);
    deviceDescriptor.protocolVersion = std::to_string(info.protocol);
    deviceDescriptor.serialNumber = info.boardSerialNumber;

    const uint32_t addrs[] = { 0x0000, 0x0001, 0x0002, 0x0003 };
    uint32_t data[4];
    SPI(SPI_FPGA, addrs, data, 4);
    auto boardID = static_cast<eLMS_DEV>(data[0]); //(pkt.inBuffer[2] << 8) | pkt.inBuffer[3];
    auto gatewareVersion = data[1]; //(pkt.inBuffer[6] << 8) | pkt.inBuffer[7];
    auto gatewareRevision = data[2]; //(pkt.inBuffer[10] << 8) | pkt.inBuffer[11];
    auto hwVersion = data[3] & 0x7F; //pkt.inBuffer[15]&0x7F;

    deviceDescriptor.gatewareTargetBoard = GetDeviceName(boardID);
    deviceDescriptor.gatewareVersion = std::to_string(gatewareVersion);
    deviceDescriptor.gatewareRevision = std::to_string(gatewareRevision);
    deviceDescriptor.hardwareVersion = std::to_string(hwVersion);

    return deviceDescriptor;
}

OpStatus LimeSDR::Reset()
{
    return LMS64CProtocol::DeviceReset(*mSerialPort, 0);
}

OpStatus LimeSDR::EnableChannel(TRXDir dir, uint8_t channel, bool enabled)
{
    OpStatus status = mLMSChips[0]->EnableChannel(dir, channel, enabled);
    if (dir == TRXDir::Tx) //always enable DAC1, otherwise sample rates <2.5MHz do not work
        mLMSChips[0]->Modify_SPI_Reg_bits(LMS7_PD_TX_AFE1, 0);
    return status;
}
/*
uint8_t LimeSDR::GetPath(SDRDevice::Dir dir, uint8_t channel) const
{
    ValidateChannel(channel);
    mLMSChips[0]->SetActiveChannel(channel == 1 ? LMS7002M::ChB : LMS7002M::ChA);
    if (dir == SDRDevice::Dir::Tx)
        return mLMSChips[0]->GetBandTRF();
    return mLMSChips[0]->GetPathRFE();
}
*/

double LimeSDR::GetClockFreq(uint8_t clk_id, uint8_t channel)
{
    return mLMSChips[0]->GetClockFreq(static_cast<LMS7002M::ClockID>(clk_id));
}

OpStatus LimeSDR::SetClockFreq(uint8_t clk_id, double freq, uint8_t channel)
{
    return mLMSChips[0]->SetClockFreq(static_cast<LMS7002M::ClockID>(clk_id), freq);
}

OpStatus LimeSDR::SPI(uint32_t chipSelect, const uint32_t* MOSI, uint32_t* MISO, uint32_t count)
{
    assert(mSerialPort);
    assert(MOSI);
    LMS64CPacket pkt;

    size_t srcIndex = 0;
    size_t destIndex = 0;
    constexpr int maxBlocks = LMS64CPacket::payloadSize / (sizeof(uint32_t) / sizeof(uint8_t)); // = 14

    while (srcIndex < count)
    {
        pkt.status = LMS64CProtocol::STATUS_UNDEFINED;
        pkt.blockCount = 0;
        pkt.periphID = chipSelect;

        // fill packet with same direction operations
        const bool willDoWrite = MOSI[srcIndex] & (1 << 31);
        for (int i = 0; i < maxBlocks && srcIndex < count; ++i)
        {
            bool isWrite = MOSI[srcIndex] & (1 << 31);
            if (isWrite != willDoWrite)
                break; // change between write/read, flush packet

            if (chipSelect == SPI_ADF4002)
                isWrite = true;

            if (isWrite)
            {
                switch (chipSelect)
                {
                case SPI_LMS7002M:
                    pkt.cmd = LMS64CProtocol::CMD_LMS7002_WR;
                    break;
                case SPI_FPGA:
                    pkt.cmd = LMS64CProtocol::CMD_BRDSPI_WR;
                    break;
                case SPI_ADF4002:
                    pkt.cmd = LMS64CProtocol::CMD_ADF4002_WR;
                    break;
                default:
                    throw std::logic_error("LimeSDR SPI invalid SPI chip select");
                }
                int payloadOffset = pkt.blockCount * 4;
                pkt.payload[payloadOffset + 0] = MOSI[srcIndex] >> 24;
                pkt.payload[payloadOffset + 1] = MOSI[srcIndex] >> 16;
                pkt.payload[payloadOffset + 2] = MOSI[srcIndex] >> 8;
                pkt.payload[payloadOffset + 3] = MOSI[srcIndex];
            }
            else
            {
                switch (chipSelect)
                {
                case SPI_LMS7002M:
                    pkt.cmd = LMS64CProtocol::CMD_LMS7002_RD;
                    break;
                case SPI_FPGA:
                    pkt.cmd = LMS64CProtocol::CMD_BRDSPI_RD;
                    break;
                case SPI_ADF4002:
                    throw std::logic_error("LimeSDR ADF4002 registers reading not supported");
                default:
                    throw std::logic_error("LimeSDR SPI invalid SPI chip select");
                }
                int payloadOffset = pkt.blockCount * 2;
                pkt.payload[payloadOffset + 0] = MOSI[srcIndex] >> 8;
                pkt.payload[payloadOffset + 1] = MOSI[srcIndex];
            }
            ++pkt.blockCount;
            ++srcIndex;
        }

        // flush packet
        //printPacket(pkt, 4, "Wr:");
        int sent = mSerialPort->Write(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), 100);
        if (sent != sizeof(pkt))
            throw std::runtime_error("SPI failed");

        int recv = mSerialPort->Read(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), 100);
        //printPacket(pkt, 4, "Rd:");

        if (recv >= pkt.headerSize + 4 * pkt.blockCount && pkt.status == LMS64CProtocol::STATUS_COMPLETED_CMD)
        {
            for (int i = 0; MISO && i < pkt.blockCount && destIndex < count; ++i)
            {
                //MISO[destIndex] = 0;
                //MISO[destIndex] = pkt.payload[0] << 24;
                //MISO[destIndex] |= pkt.payload[1] << 16;
                MISO[destIndex] = (pkt.payload[i * 4 + 2] << 8) | pkt.payload[i * 4 + 3];
                ++destIndex;
            }
        }
        else
        {
            throw std::runtime_error("SPI failed");
        }
    }

    return OpStatus::SUCCESS;
}

/*int LimeSDR::I2CWrite(int address, const uint8_t *data, uint32_t length)
{
    assert(comms);
    LMS64CPacket pkt;
    int remainingBytes = length;
    const uint8_t* src = data;
    while (remainingBytes > 0)
    {
        pkt.cmd = LMS64CProtocol::CMD_I2C_WR;
        pkt.status = LMS64CProtocol::STATUS_UNDEFINED;
        pkt.blockCount = remainingBytes > pkt.payloadSize ? pkt.payloadSize : remainingBytes;
        pkt.periphID = address;
        memcpy(pkt.payload, src, pkt.blockCount);
        src += pkt.blockCount;
        remainingBytes -= pkt.blockCount;
        int sent = comms->BulkTransfer(CONTROL_BULK_OUT_ADDRESS, reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), 100);
        if (sent != sizeof(pkt))
            throw std::runtime_error("I2C write failed");
        int recv = comms->BulkTransfer(CONTROL_BULK_IN_ADDRESS, reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), 100);

        if (recv < pkt.headerSize || pkt.status != LMS64CProtocol::STATUS_COMPLETED_CMD)
            throw std::runtime_error("I2C write failed");
    }
    return 0;
}*/

/*int LimeSDR::I2CRead(int address, uint8_t *data, uint32_t length)
{
    assert(comms);
    LMS64CPacket pkt;
    int remainingBytes = length;
    uint8_t* dest = data;
    while (remainingBytes > 0)
    {
        pkt.cmd = LMS64CProtocol::CMD_I2C_RD;
        pkt.status = LMS64CProtocol::STATUS_UNDEFINED;
        pkt.blockCount = remainingBytes > pkt.payloadSize ? pkt.payloadSize : remainingBytes;
        pkt.periphID = address;

        int sent = comms->BulkTransfer(CONTROL_BULK_OUT_ADDRESS, reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), 100);
        if (sent != sizeof(pkt))
            throw std::runtime_error("I2C read failed");
        int recv = comms->BulkTransfer(CONTROL_BULK_IN_ADDRESS, reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), 100);

        memcpy(dest, pkt.payload, pkt.blockCount);
        dest += pkt.blockCount;
        remainingBytes -= pkt.blockCount;

        if (recv <= pkt.headerSize || pkt.status != LMS64CProtocol::STATUS_COMPLETED_CMD)
            throw std::runtime_error("I2C read failed");
    }
    return 0;
}*/

// There might be some leftover samples data still buffered in USB device
// clear the USB buffers before streaming samples to avoid old data
void LimeSDR::ResetUSBFIFO()
{
    LMS64CPacket pkt;
    pkt.cmd = LMS64CProtocol::CMD_USB_FIFO_RST;
    pkt.status = LMS64CProtocol::STATUS_UNDEFINED;
    pkt.blockCount = 1;
    pkt.payload[0] = 0;

    int sentBytes = mSerialPort->Write(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), 100);
    if (sentBytes != sizeof(pkt))
    {
        throw std::runtime_error("LimeSDR::ResetUSBFIFO write failed");
    }

    int gotBytes = mSerialPort->Read(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt), 100);
    if (gotBytes != sizeof(pkt))
    {
        throw std::runtime_error("LimeSDR::ResetUSBFIFO read failed");
    }
}

OpStatus LimeSDR::StreamSetup(const StreamConfig& config, uint8_t moduleIndex)
{
    // Allow multiple setup calls
    if (mStreamers.at(moduleIndex) != nullptr)
    {
        delete mStreamers.at(moduleIndex);
    }

    mStreamers.at(moduleIndex) =
        new TRXLooper_USB(mStreamPort, mFPGA, mLMSChips.at(moduleIndex), FX3::STREAM_BULK_IN_ADDRESS, FX3::STREAM_BULK_OUT_ADDRESS);
    return mStreamers.at(moduleIndex)->Setup(config);
}

void LimeSDR::StreamStart(uint8_t moduleIndex)
{
    if (mStreamers[0])
    {
        ResetUSBFIFO();
        mStreamers[0]->Start();
    }
    else
        throw std::runtime_error("Stream not setup");
}

void LimeSDR::StreamStop(uint8_t moduleIndex)
{
    if (!mStreamers[0])
        return;

    mStreamers[0]->Stop();

    delete mStreamers[0];
    mStreamers[0] = nullptr;
}

void* LimeSDR::GetInternalChip(uint32_t index)
{
    return mLMSChips.at(index);
}

OpStatus LimeSDR::GPIODirRead(uint8_t* buffer, const size_t bufLength)
{
    return mfpgaPort->GPIODirRead(buffer, bufLength);
}

OpStatus LimeSDR::GPIORead(uint8_t* buffer, const size_t bufLength)
{
    return mfpgaPort->GPIORead(buffer, bufLength);
}

OpStatus LimeSDR::GPIODirWrite(const uint8_t* buffer, const size_t bufLength)
{
    return mfpgaPort->GPIODirWrite(buffer, bufLength);
}

OpStatus LimeSDR::GPIOWrite(const uint8_t* buffer, const size_t bufLength)
{
    return mfpgaPort->GPIOWrite(buffer, bufLength);
}

OpStatus LimeSDR::CustomParameterWrite(const std::vector<CustomParameterIO>& parameters)
{
    return mfpgaPort->CustomParameterWrite(parameters);
}

OpStatus LimeSDR::CustomParameterRead(std::vector<CustomParameterIO>& parameters)
{
    return mfpgaPort->CustomParameterRead(parameters);
}

OpStatus LimeSDR::UploadMemory(
    eMemoryDevice device, uint8_t moduleIndex, const char* data, size_t length, UploadMemoryCallback callback)
{
    int progMode;
    LMS64CProtocol::ProgramWriteTarget target = LMS64CProtocol::ProgramWriteTarget::FPGA;

    // TODO: add FX3 firmware flashing
    switch (device)
    {
    case eMemoryDevice::FPGA_RAM:
        progMode = 0;
        break;
    case eMemoryDevice::FPGA_FLASH:
        progMode = 1;
        break;
    default:
        return OpStatus::INVALID_VALUE;
    }

    return mfpgaPort->ProgramWrite(data, length, progMode, static_cast<int>(target), callback);
}

OpStatus LimeSDR::MemoryWrite(std::shared_ptr<DataStorage> storage, Region region, const void* data)
{
    if (storage == nullptr || storage->ownerDevice != this || storage->memoryDeviceType != eMemoryDevice::EEPROM)
        return OpStatus::ERROR;
    return mfpgaPort->MemoryWrite(region.address, data, region.size);
}

OpStatus LimeSDR::MemoryRead(std::shared_ptr<DataStorage> storage, Region region, void* data)
{
    if (storage == nullptr || storage->ownerDevice != this || storage->memoryDeviceType != eMemoryDevice::EEPROM)
        return OpStatus::ERROR;
    return mfpgaPort->MemoryRead(region.address, data, region.size);
}
