#include "ILMS7002MTab.h"

#include "lms7002_gui_utilities.h"

#include "limesuite/LMS7002M.h"
#include "Logger.h"

using namespace lime;
using namespace std::literals::string_literals;

ILMS7002MTab::ILMS7002MTab(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : wxPanel(parent, id, pos, size, style)
    , lmsControl(nullptr)
    , mChannel(0)
{
}

void ILMS7002MTab::Initialize(ControllerType* pControl)
{
    lmsControl = pControl;
}

void ILMS7002MTab::UpdateGUI()
{
    if (lmsControl == nullptr)
        return;
    LMS7002_WXGUI::UpdateControlsByMap(this, lmsControl, wndId2Enum, mChannel);
}

void ILMS7002MTab::ParameterChangeHandler(wxCommandEvent& event)
{
    assert(lmsControl != nullptr);
    LMS7Parameter parameter;
    try
    {
        parameter = wndId2Enum.at(reinterpret_cast<wxWindow*>(event.GetEventObject()));
    } catch (std::exception& e)
    {
        lime::error("Control element(ID = "s + std::to_string(event.GetId()) + ") don't have assigned LMS parameter."s);
        return;
    }
    WriteParam(parameter, event.GetInt());
}

void ILMS7002MTab::SpinParameterChangeHandler(wxSpinEvent& event)
{
    wxCommandEvent evt;
    evt.SetInt(event.GetInt());
    evt.SetId(event.GetId());
    evt.SetEventObject(event.GetEventObject());
    ParameterChangeHandler(evt);
}

void ILMS7002MTab::SetChannel(uint8_t channel)
{
    mChannel = channel;
}

void ILMS7002MTab::WriteParam(const LMS7Parameter& param, uint16_t val)
{
    lmsControl->SetActiveChannel(mChannel == 0 ? LMS7002M::Channel::ChA : LMS7002M::Channel::ChB);
    lmsControl->Modify_SPI_Reg_bits(param, val);
}

int ILMS7002MTab::ReadParam(const LMS7Parameter& param)
{
    lmsControl->SetActiveChannel(mChannel == 0 ? LMS7002M::Channel::ChA : LMS7002M::Channel::ChB);
    return lmsControl->Get_SPI_Reg_bits(param);
}

int ILMS7002MTab::LMS_ReadLMSReg(ControllerType* lms, uint16_t address, uint16_t* value)
{
    *value = lms->SPI_read(address);
    return 0;
}
int ILMS7002MTab::LMS_WriteLMSReg(ControllerType* lms, uint16_t address, uint16_t value)
{
    return lms->SPI_write(address, value) == OpStatus::SUCCESS ? 0 : -1;
}

int ILMS7002MTab::LMS_ReadParam(ControllerType* lmsControl, const LMS7Parameter& param, uint16_t* value)
{
    *value = ReadParam(param);
    return 0;
}
