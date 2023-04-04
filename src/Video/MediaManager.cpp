// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/dcbuffer.h>

#include "Common/defs.h"
#include "Common/Tools.h"
#include "wxFFmpegView.h"
#include "MediaManager.h"

DEFINE_LOCAL_EVENT_TYPE( wxEVT_VIDEO_FINISHED )

// clang-format off
//BEGIN_EVENT_TABLE( MediaManager, wxWindow )
//    EVT_KEY_DOWN( MediaManager::onKeyPressed )
//END_EVENT_TABLE()
// clang-format on

MediaManager::MediaManager( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name )
    : wxWindow( parent, id, pos, size, style, name )
    , m_font( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL )
{
    SetBackgroundColour( *wxBLACK );

    m_timer.Bind( wxEVT_TIMER, &MediaManager::onTimer, this );
    //m_timerCheckEnd.Bind( wxEVT_TIMER, &MediaManager::onCheckEnd, this );
}

MediaManager::~MediaManager()
{
    /*if ( IsShown() && m_mediaControl )
    {
        wxClientDC cDC( this );
        cDC.Clear();

        m_mediaControl->Hide();
        m_mediaControl->Stop();
        reset();

        Hide();
    }*/
}

void MediaManager::createMediaControl()
{
    wxGLAttributes attrs;
    attrs.PlatformDefaults().Defaults().EndList();

    m_mediaControl = new wxFFmpegView( this, attrs );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    sizer->Add( m_mediaControl, 1, wxEXPAND );
    SetSizer( sizer );

    Layout();

    m_mediaControl->SetBackgroundColour( *wxBLACK );
    
    //m_isOK = m_mediaControl->Create(
    //    this,
    //    wxID_ANY,
    //    wxEmptyString,
    //    wxDefaultPosition,
    //    wxDefaultSize,
    //    wxMC_NO_AUTORESIZE | wxMEDIACTRLPLAYERCONTROLS_NONE );
    //    //wxMEDIABACKEND_WMP10 );

    //wxASSERT_MSG( m_isOK, "Could not create media control!" );

    //m_mediaControl->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

    //m_mediaControl->Bind( wxEVT_MEDIA_LOADED, &MediaManager::onMediaLoaded, this );
    //m_mediaControl->Bind( wxEVT_MEDIA_PLAY, &MediaManager::onMediaPlayStarted, this );
    //m_mediaControl->Bind( wxEVT_MEDIA_FINISHED, &MediaManager::onMediaFinished, this );
}

void MediaManager::reset()
{
    m_timer.Stop();
    m_timerCheckEnd.Stop();

    //m_mediaControl->Unbind( wxEVT_MEDIA_LOADED, &MediaManager::onMediaLoaded, this );
    //m_mediaControl->Unbind( wxEVT_MEDIA_PLAY, &MediaManager::onMediaPlayStarted, this );
    //m_mediaControl->Unbind( wxEVT_MEDIA_FINISHED, &MediaManager::onMediaFinished, this );

    m_mediaControl->Destroy();
    m_mediaControl = nullptr;
}

//void MediaManager::onMediaLoaded( wxMediaEvent& )
//{
//    const auto &cleentSize = GetClientSize();
//    const auto &videoSize = m_mediaControl->GetBestSize();
//    const auto controlHeight = ( double )videoSize.GetHeight() / videoSize.GetWidth() * cleentSize.GetWidth();
//    const wxPoint controlPosition( 0, ( cleentSize.GetHeight() - controlHeight ) / 2 - PLAYER_CONTROLS_PANEL_HEIGHT );
//    
//    m_mediaControl->SetSize( cleentSize.GetWidth(), controlHeight );
//    m_mediaControl->SetPosition( controlPosition );
//
//    if ( m_isOK )
//    {
//        m_mediaControl->Play();
//    }
//
//    m_timer.Start( s_timerInterval );
//}

//void MediaManager::onMediaPlayStarted( wxMediaEvent& event )
//{
//    AddPendingEvent( event );
//}
//
//void MediaManager::onMediaFinished( wxMediaEvent& event )
//{
//    AddPendingEvent( event );
//}


//void MediaManager::close()
//{
//    m_mediaControl->close();
//}


void MediaManager::playIntro()
{
    createMediaControl();
    m_mediaControl->open( Tools::Instance().getFullFileName( "/../resources/video/Intro.mp4" ) );
}

void MediaManager::showSkipMessage( bool show )
{
    wxClientDC cDC( this );

    if ( show )
    {
        const auto &clientSize = GetClientSize();
        cDC.SetFont( m_font );
        cDC.SetTextForeground( wxColor( 128, 128, 128 ) );
        cDC.DrawText( wxT( "Press R key for skip" ), clientSize.GetWidth() - 200, clientSize.GetHeight() - 90 );

        return;
    }

    cDC.Clear();
}

void MediaManager::onTimer( wxTimerEvent& )
{
    m_timer.Stop();
    showSkipMessage();
    m_timerCheckEnd.Start( s_timerCheckEndInterval );
}

//void MediaManager::onCheckEnd( wxTimerEvent& )
//{
//    static const auto s_videoEnd = m_mediaControl->Length() * 90.0 / 100;
//
//    if ( m_mediaControl->Tell() >= s_videoEnd )
//    {
//        m_timerCheckEnd.Stop();
//        showSkipMessage( false );
//    }
//}
//
//void MediaManager::onKeyPressed( wxKeyEvent& event )
//{
//    switch ( event.GetKeyCode() )
//    {
//        case 'R':
//        case 'r':
//            m_mediaControl->Stop();
//
//            wxCommandEvent eventVideoFinished( wxEVT_MEDIA_FINISHED );
//            AddPendingEvent( eventVideoFinished );
//            break;
//    }
//}
