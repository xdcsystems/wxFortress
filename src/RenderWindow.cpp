// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <map>

#include "Tools.h"
#include "SoundManager.h"
#include "shapes/ShapesManager.h"
#include "Overlay.h"
#include "RenderWindow.h"

DEFINE_LOCAL_EVENT_TYPE( wxEVT_LAUNCH_PRESSED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_NEW_ROUND_STARTED )

BEGIN_EVENT_TABLE( RenderWindow, wxWindow )
    EVT_PAINT( OnPaint )
    EVT_KEY_DOWN( OnKeyPressed )
    EVT_SIZE( OnSize )
    EVT_COMMAND( wxID_ANY, wxEVT_CURRENT_SCORE_INCREASED, OnScoreIncreased )
    EVT_COMMAND( wxID_ANY, wxEVT_ROUND_COMLETED, OnRoundCompleted )
    EVT_COMMAND( wxID_ANY, wxEVT_BALL_LOST, OnBallLost )
    EVT_COMMAND( wxID_ANY, wxEVT_PING, OnPaddleContact )
    EVT_COMMAND( wxID_ANY, wxEVT_PONG, OnPaddleContact )
END_EVENT_TABLE()


RenderWindow::RenderWindow( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
    :wxWindow( parent, id, pos, size, style, name )
{
    Hide();  // Initialy hide it
    init();
}

void RenderWindow::init()
{
    SetBackgroundStyle( wxBG_STYLE_PAINT );
    SetBackgroundColour( *wxBLACK );
    
    m_shapesManager = std::make_shared<Shapes::ShapesManager>( this );

    m_soundManager = std::make_shared<SoundManager>();
    m_soundManager->init();
}

void RenderWindow::loadLevel( unsigned short level )
{
    m_shapesManager->loadLevel( level );
}

void RenderWindow::switchRun( )
{
    if ( !m_isRunning )
        return;

    if ( m_state == NEWROUND )
    {
        
        wxCommandEvent lounchPressedEvent( wxEVT_LAUNCH_PRESSED );
        ProcessEvent( lounchPressedEvent );

        m_soundManager->playSiren();

        wxCommandEvent roundStartedEvent( wxEVT_NEW_ROUND_STARTED );
        ProcessEvent( roundStartedEvent );
        
        m_state = COUNTDOWN;
        for ( m_countDown = 0; m_countDown < 3; ++m_countDown )
        {
            paintNow();
            m_soundManager->playCountdown();
        }
        m_state = NEWROUND;

        m_soundManager->playRocket();
    }

    m_state = m_shapesManager->switchRun( m_state == NEWROUND ) ? RUN : PAUSE;
}

void RenderWindow::resize( const wxSize& size )
{
    if ( size.x < 1 || size.y < 1 )
        return;

    m_bitmapBuffer = std::make_shared<wxBitmap>( size, 32 );
    m_overlay = std::make_shared<Overlay>( size );
    m_shapesManager->resize( size );
}

void RenderWindow::OnPaint( wxPaintEvent& event )
{
    wxPaintDC dc( this );
    render( dc );
}

void RenderWindow::paintNow()
{
    wxClientDC dc( this );
    render( dc );
}

void RenderWindow::render( wxDC& dc )
{
    if ( !m_isRunning )
        return;

    const auto &clientSize = GetClientSize();
    if ( !m_mdc )
    {
        if ( !m_bitmapBuffer || clientSize.x < 1 || clientSize.y < 1 )
            return;

        m_mdc = std::make_shared<wxMemoryDC>();
        m_mdc->CopyAttributes( dc );
        m_mdc->SetDeviceOrigin( 0, -clientSize.y );
        m_mdc->SetAxisOrientation( 1, 1 );
        m_mdc->SetBrush( *wxBLACK_BRUSH );
    }
    m_mdc->SelectObject( *m_bitmapBuffer );

    checkKeysState();
    
    m_shapesManager->renderFrame( *m_mdc );

    switch ( m_state )
    {
        case PAUSE:
            m_overlay->showPause( m_mdc.get(), &dc );
            break;
        case COUNTDOWN:
            m_overlay->showCountDown( m_mdc.get(), &dc, m_countDown );
            break;
        default:
            dc.Blit( 0, 0, clientSize.x, clientSize.y, m_mdc.get(), 0, -clientSize.y );
            break;
    }

    m_mdc->SelectObject( wxNullBitmap );
}

void RenderWindow::checkKeysState()
{
    using namespace Shapes;

    if ( wxGetKeyState( WXK_LEFT ) )
    {
        m_accelerate += 0.03;
        m_shapesManager->moveBoard( ShapesManager::DirectionLeft - m_accelerate );
    } 
    else if ( wxGetKeyState( WXK_RIGHT ) )
    {
        m_accelerate += 0.03;
        m_shapesManager->moveBoard( ShapesManager::DirectionRight + m_accelerate );
    }
    else if ( m_accelerate != 0 )
    {
        m_accelerate = 0;
        m_shapesManager->moveBoard( 0 );
    }
}

void RenderWindow::OnSize( wxSizeEvent& event )
{
    resize( event.GetSize() );
}

void RenderWindow::OnKeyPressed( wxKeyEvent& event )
{
    switch ( event.GetKeyCode() )
    {
        case WXK_SPACE:
            switchRun();
            break;

        case WXK_F1:
            //... give help ...
            break;

        case WXK_ESCAPE:
            m_parent->Close();
            break;
    }
}

void RenderWindow::OnScoreIncreased( wxCommandEvent& event )
{
    m_soundManager->playDestroyBrick();
    event.Skip();
}

void RenderWindow::OnPaddleContact( wxCommandEvent& event )
{
    static std::map<wxEventType, void ( SoundManager::* )( )> s_contactSound = {
        { wxEVT_PING, &SoundManager::playPing },
        { wxEVT_PONG, &SoundManager::playPong }
    };

    ( m_soundManager.get()->*( s_contactSound[ event.GetEventType() ] ) )( );
}

void RenderWindow::OnRoundCompleted( wxCommandEvent& event ) 
{
    m_soundManager->playLevelComplete();
    m_state = NEWROUND;
    
    Refresh( false );
    Update();

    event.Skip();
} 

void RenderWindow::OnBallLost( wxCommandEvent& event )
{
    m_soundManager->playBallLost();
    m_state = NEWROUND;

    Refresh( false );
    Update();

    event.Skip();
}