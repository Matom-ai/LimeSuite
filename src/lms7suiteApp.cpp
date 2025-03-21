/***************************************************************
 * Name:      lms7projectApp.cpp
 * Purpose:   Code for Application Class
 * Author:    Lime Microsystems ()
 * Created:   2015-03-05
 * Copyright: Lime Microsystems (limemicro.com)
 * License:
 **************************************************************/

#ifdef WX_PRECOMP
    #include "wx_pch.h"
#endif

#ifdef __BORLANDC__
    #pragma hdrstop
#endif //__BORLANDC__

#include "lms7suiteApp.h"
#include "lms7suiteAppFrame.h"
#include <wx/time.h>
#include <wx/splash.h>
#include <wx/icon.h>
#include <iostream>
#if !defined(NDEBUG) & defined(__unix__)
IMPLEMENT_APP_CONSOLE(lms7suiteApp)
#else
IMPLEMENT_APP(lms7suiteApp)
#endif

#include "Logger.h"
#include "resources/splash.h"
#include "resources/LMS_ICO.xpm"

using namespace std::literals::string_literals;

bool lms7suiteApp::OnInit()
{
    wxInitAllImageHandlers();
    wxBitmap splashBitmap = wxBITMAP_PNG_FROM_DATA(splash);
    wxSplashScreen* splash = new wxSplashScreen(
        splashBitmap, wxSPLASH_CENTRE_ON_SCREEN, 6000, NULL, -1, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER | wxSTAY_ON_TOP);
    wxYield(); //linux needs this to load splash image
    LMS7SuiteAppFrame* frame = new LMS7SuiteAppFrame(0L);
    frame->SetIcon(wxICON(LMS_ICO));
#ifndef NDEBUG
    wxLongLong t1 = wxGetUTCTimeMillis();
    lime::debug( "Create time "s + (wxGetUTCTimeMillis() - t1).ToString() + " ms"s);
#endif
    splash->Destroy();
    frame->Show();
    return true;
}
