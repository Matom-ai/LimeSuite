#include "FPGA_XTRX.h"
#include "Logger.h"
#include <ciso646>
#include <vector>
#include <map>
#include <math.h>
#include <iostream>

#include "Register.h"

using namespace std::literals::string_literals;

namespace lime {

FPGA_XTRX::FPGA_XTRX(std::shared_ptr<ISPI> fpgaSPI, std::shared_ptr<ISPI> lms7002mSPI)
    : FPGA(fpgaSPI, lms7002mSPI)
{
}

OpStatus FPGA_XTRX::SetInterfaceFreq(double txRate_Hz, double rxRate_Hz, double txPhase, double rxPhase, int chipIndex)
{
    lime::debug("FPGA_XTRX"s);
    lime::info("Phases: tx phase %f rx phase %f", txPhase, rxPhase);
    const int txPLLindex = 0;
    const int rxPLLindex = 1;

    std::vector<FPGA_PLL_clock> rxClocks(2);
    rxClocks[0].index = 0;
    rxClocks[0].outFrequency = rxRate_Hz;
    rxClocks[1].index = 1;
    rxClocks[1].outFrequency = rxRate_Hz;
    rxClocks[1].phaseShift_deg = rxPhase;
    if (FPGA_XTRX::SetPllFrequency(rxPLLindex, rxRate_Hz, rxClocks) != OpStatus::SUCCESS)
        return OpStatus::ERROR;

    std::vector<FPGA_PLL_clock> txClocks(2);
    txClocks[0].index = 0;
    txClocks[0].outFrequency = txRate_Hz;
    txClocks[1].index = 1;
    txClocks[1].outFrequency = txRate_Hz;
    txClocks[1].phaseShift_deg = txPhase;
    if (FPGA_XTRX::SetPllFrequency(txPLLindex, txRate_Hz, txClocks) != OpStatus::SUCCESS)
        return OpStatus::ERROR;

    return OpStatus::SUCCESS;
}

OpStatus FPGA_XTRX::SetPllFrequency(const uint8_t pllIndex, const double inputFreq, std::vector<FPGA_PLL_clock>& clocks)
{
    //Xilinx boards have different phase control mechanism
    double phase = clocks.at(1).phaseShift_deg;
    WriteRegister(0x0020, phase);
    return FPGA::SetPllFrequency(pllIndex, inputFreq, clocks);
}

} //namespace lime
