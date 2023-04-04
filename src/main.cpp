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

#include "App.h"
#include "MainFrame.h"

#include "resources/sample.xpm"


bool App::OnInit()
{
    try
    {
        m_mainFrame = new MainFrame(); // delete by wxApp at exit as top level window
        if ( m_mainFrame->Create( nullptr ) )
        {
            // Give it an icon
            m_mainFrame->SetIcon( sample_xpm );
            m_mainFrame->Show();

            return true;
        }
    }
    catch ( const std::exception& e )
    {
        wxMessageBox( e.what(), "Exception Caught", MB_OK );
    }
  
    // Something went wrong ...
    return false;
}

wxIMPLEMENT_APP( App );

bool App::OnExceptionInMainLoop()
{
    try 
    { 
        throw; 
    }
    catch ( const std::exception& e )
    {
        m_mainFrame->stop();
        wxMessageBox( e.what(), "Exception Caught", MB_OK );
    }
    return false;
}

