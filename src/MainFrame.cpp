// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "Common/defs.h"
#include "ControlPanel/Panel.h"
#include "RenderWindow.h"
#include "MainFrame.h"


// clang-format off
BEGIN_EVENT_TABLE( MainFrame, wxFrame )
    EVT_CLOSE( MainFrame::onClose )
    EVT_ACTIVATE( MainFrame::onActivate )
    EVT_COMMAND( wxID_ANY, wxEVT_CURRENT_SCORE_INCREASED, MainFrame::onScoreIncreased )
    EVT_COMMAND( wxID_ANY, wxEVT_ROUND_COMPLETED, MainFrame::onRoundCompleted )
    EVT_COMMAND( wxID_ANY, wxEVT_VIDEO_PLAY, MainFrame::onVideoStarted )
    EVT_COMMAND( wxID_ANY, wxEVT_VIDEO_FINISHED, MainFrame::onVideoFinished )
    EVT_COMMAND( wxID_ANY, wxEVT_WND_INITIALIZED, MainFrame::onChildWndInitialized )
    EVT_COMMAND( wxID_ANY, wxEVT_LAUNCH_PRESSED, MainFrame::onLaunchPressed )
    EVT_COMMAND( wxID_ANY, wxEVT_LAUNCH_CLICKED, MainFrame::onLaunchClicked )
    EVT_COMMAND( wxID_ANY, wxEVT_NEW_ROUND_STARTED, MainFrame::onRoundStarted )
    EVT_COMMAND( wxID_ANY, wxEVT_BALL_LOST, MainFrame::onBallLost )
    EVT_COMMAND( wxID_ANY, wxEVT_RESET, MainFrame::onReset )
END_EVENT_TABLE()
// clang-format on

MainFrame::MainFrame( wxWindow *parent, int id, const wxString &title, wxPoint pos, wxSize size, int style )
  : wxFrame( parent, id, title, pos, size, style )
{
    init();
}

bool MainFrame::Create( wxWindow *parent, int id, const wxString &title, wxPoint pos, wxSize size, int style, const wxString &name )
{
    bool returnVal = wxFrame::Create( parent, id, title, pos, size, style, name );
    if ( returnVal )
    {
        init();
        CentreOnScreen();
    }

    return returnVal;
}

void MainFrame::init()
{
#if defined( wxUSE_LOGWINDOW ) && wxUSE_LOGWINDOW && defined( USE_LOGGER ) && USE_LOGGER == 1
    wxLogWindow::SetVerbose( true );
    // wxLogWindow inherits wxLogChain, whose ctor registers itself as active target and restores the previous one on destruction.
    m_logWindow = std::make_unique<wxLogWindow>( this, wxT( "Log" ), true, false );
#endif

    SetBackgroundColour( *wxBLACK );

    wxInitAllImageHandlers();
    
    const auto &size = GetSize();

    m_renderSurface = new RenderWindow( this, wxID_ANY, nullptr, wxSize( 800, size.y ), wxBORDER_NONE | wxWANTS_CHARS );
    m_controlPanel = new ControlPanel::Panel( this, wxID_ANY, wxPoint( 800, 0 ), wxSize( size.x - 800, size.y ) );

    auto *hSizer = new wxBoxSizer( wxHORIZONTAL );
    hSizer->Add( m_renderSurface, 1, wxEXPAND | wxALL );
    hSizer->Add( m_controlPanel, 1, wxEXPAND | wxALL );

    hSizer->SetMinSize( size );
    hSizer->SetSizeHints( this );

    SetSizer( hSizer );
}

void MainFrame::start()
{
    if ( !m_renderSurface || !m_controlPanel )
        return;

    m_controlPanel->Show();

    m_renderSurface->start();
    m_isRunning = true;

    m_renderSurface->Raise();
    m_renderSurface->SetFocus();

    Layout();
}

void MainFrame::onVideoStarted( wxCommandEvent& )
{
    if ( !m_renderSurface || !m_controlPanel )
        return;

    m_renderSurface->init();
    m_renderSurface->loadLevel( m_controlPanel->increaseLevel() );
}

void MainFrame::onVideoFinished( wxCommandEvent& )
{
    start();
}

void MainFrame::stop()
{
    m_isRunning = false;
    if ( m_renderSurface )
        m_renderSurface->stop();
}

void MainFrame::onClose( wxCloseEvent &event )
{
    stop();
    event.Skip();
}

void MainFrame::onActivate( wxActivateEvent &event )
{
    if ( event.GetActive() && m_renderSurface && m_renderSurface->IsShownOnScreen() )
        m_renderSurface->SetFocus();
    event.Skip();
}

void MainFrame::onScoreIncreased( wxCommandEvent& )
{
    m_controlPanel->increaseScore();
}

void MainFrame::onChildWndInitialized( wxCommandEvent& )
{
    if ( !m_renderSurface )
        return;

    m_renderSurface->SetFocus();
    m_renderSurface->playIntro();
}

void MainFrame::onLaunchPressed( wxCommandEvent& )
{
    m_controlPanel->paintLaunched();
}

void MainFrame::onLaunchClicked( wxCommandEvent& )
{
    if ( m_renderSurface )
    {
        m_renderSurface->triggerLaunch();
        m_renderSurface->SetFocus();
    }
}

void MainFrame::onRoundStarted( wxCommandEvent& )
{
    m_controlPanel->activate();
}

// Load next level
void MainFrame::onRoundCompleted( wxCommandEvent& )
{
    m_renderSurface->loadLevel( m_controlPanel->increaseLevel() );
}

void MainFrame::resetToFirstLevel()
{
    m_controlPanel->reset();
    m_renderSurface->resetBallSpeed();
    m_renderSurface->loadLevel( m_controlPanel->increaseLevel() );
}

void MainFrame::onBallLost( wxCommandEvent& )
{
    if ( m_controlPanel->decreaseLives() > 0 )
    {
        m_renderSurface->loadLevel( m_controlPanel->currentLevel() );
    }
    else
    {
        resetToFirstLevel();
    }
}

void MainFrame::onReset( wxCommandEvent& )
{
    resetToFirstLevel();
}
