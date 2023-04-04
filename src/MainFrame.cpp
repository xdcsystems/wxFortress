// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/mediactrl.h>

#include "Common/defs.h"
#include "Video/MediaManager.h"
#include "ControlPanel/Panel.h"
#include "RenderWindow.h"
#include "MainFrame.h"


// clang-format off
BEGIN_EVENT_TABLE( MainFrame, wxFrame )
    EVT_CLOSE( MainFrame::onClose )
    EVT_COMMAND( wxID_ANY, wxEVT_CURRENT_SCORE_INCREASED, MainFrame::onScoreIncreased )
    EVT_COMMAND( wxID_ANY, wxEVT_ROUND_COMLETED, MainFrame::onRoundCompleted )
    EVT_COMMAND( wxID_ANY, wxEVT_MEDIA_PLAY, MainFrame::onRoundCompleted )
    EVT_COMMAND( wxID_ANY, wxEVT_MEDIA_FINISHED, MainFrame::onVideoFinished )
    EVT_COMMAND( wxID_ANY, wxEVT_LAUNCH_PRESSED, MainFrame::onLaunchPressed )
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
#if defined( wxUSE_LOGWINDOW ) && defined( USE_LOGGER )
    m_logWindow = new wxLogWindow( nullptr, wxT( "Log" ), true, false );
    m_logWindow->SetVerbose( TRUE );
    wxLog::SetActiveTarget( m_logWindow );

    m_logWindow->GetFrame()->SetFocus();
#endif

    SetBackgroundColour( *wxBLACK );

    wxInitAllImageHandlers();

    const auto &size = GetSize();

    //m_mediaManager = std::make_shared<MediaManager>( this, wxID_ANY, wxDefaultPosition, size );
    //m_mediaManager->playIntro();

    m_renderSurface = std::make_shared<RenderWindow>( this, wxID_ANY, nullptr, wxPoint( 0, 0 ), wxSize( 800, size.y ) );
    m_controlPanel = std::make_shared<ControlPanel::Panel>( this, wxID_ANY, wxPoint( 800, 0 ), wxSize( size.x - 800, size.y ) );

    auto *bSizer = new wxBoxSizer( wxHORIZONTAL );
    bSizer->Add( m_renderSurface.get(), 1, wxEXPAND | wxALL );
    bSizer->Add( m_controlPanel.get(), 1, wxEXPAND | wxALL );

    SetSizer( bSizer );
}

void MainFrame::start()
{
    m_controlPanel->Show();

    m_renderSurface->start();
    m_isRunning = true;

    m_renderSurface->Raise();
    m_renderSurface->SetFocus();

    GetSizer()->Layout();

    m_mediaManager->Hide();
    m_mediaManager->reset();
}

void MainFrame::onVideoFinished( wxCommandEvent& )
{
    start();
}

void MainFrame::stop()
{
    m_isRunning = false;
    m_renderSurface->stop();
};

void MainFrame::onClose( wxCloseEvent &event )
{
    stop();
    event.Skip();
}

void MainFrame::onScoreIncreased( wxCommandEvent& )
{
    m_controlPanel->increaseScore();
}

void MainFrame::onLaunchPressed( wxCommandEvent& )
{
    m_controlPanel->paintLaunched();
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

void MainFrame::onBallLost( wxCommandEvent& )
{
    if ( m_controlPanel->decreaseLives() > 0 )
    {
        m_renderSurface->loadLevel( m_controlPanel->currentLevel() );
    }
    else
    {
        m_controlPanel->reset();
        m_renderSurface->loadLevel( m_controlPanel->increaseLevel() );
    }
}

void MainFrame::onReset( wxCommandEvent& )
{
    m_controlPanel->reset();
    m_renderSurface->loadLevel( m_controlPanel->increaseLevel() );
}
