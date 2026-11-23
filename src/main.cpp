// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#if !wxUSE_EXCEPTIONS
    #error "This programm only works with wxUSE_EXCEPTIONS == 1"
#endif // !wxUSE_EXCEPTIONS

#if !wxUSE_GLCANVAS
    #error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#if defined( __LINUX__ )
    #include <X11/Xlib.h> 
#endif

#include <iostream>
#include <memory>

#include "App.h"
#include "Common/Version.h"
#include "MainFrame.h"

#include "resources/sample.xpm"

#if !defined( WXFORTRESS_TESTING )
wxIMPLEMENT_APP( App );
#else
wxIMPLEMENT_APP_NO_MAIN( App );
#endif

App::App()
{
#if defined( __LINUX__ )
    XInitThreads();
#endif
}

bool App::OnInit()
{
    for ( int i = 1; i < argc; ++i )
    {
        const wxString arg = argv[ i ];
        if ( arg == "--version" || arg == "-v" )
        {
            std::cout << "wxFortress " << WXFORTRESS_VERSION_STRING << std::endl;
            return false;
        }
    }

    try
    {
        m_mainFrame = new MainFrame(); // will delete by wxApp at exit as top level window
        const wxString windowTitle = wxString::Format( "wxFortress %s", WXFORTRESS_VERSION_STRING );
        if ( m_mainFrame->Create( nullptr, wxID_ANY, windowTitle ) )
        {
            // Give it an icon
            m_mainFrame->SetIcon( sample_xpm );
            m_mainFrame->Show();

            return true;
        }
    }
    catch ( const std::exception& e )
    {
        wxMessageBox( e.what(), "Exception Caught", wxOK );
    }
  
    // Something went wrong ...
    return false;
}

bool App::OnExceptionInMainLoop()
{
    try 
    { 
        throw; 
    }
    catch ( const std::exception& e )
    {
        m_mainFrame->stop();
        wxMessageBox( e.what(), "Exception Caught", wxOK );
    }
    return false;
}

