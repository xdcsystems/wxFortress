// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <map>

#include <GL/glew.h>

#if defined( _MSC_VER )
    #include <GL/wglew.h>
    #include <dwmapi.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Common/defs.h"
#include "Common/Timer.h"
#include "Common/Rect.hpp"
#include "Common/Tools.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/Shader.h"
#include "Renderer/SpriteRenderer.h"
#include "Renderer/Overlay.h"
#include "Renderer/TextRenderer.h"
#include "Sounds/SoundManager.h"
#include "Shapes/ShapesManager.h"
#include "Shapes/ParticleGenerator.h"
#include "Video/MediaManager.h"

#include "RenderWindow.h"

#if defined( _MSC_VER )
    #pragma comment( lib, "../external/directX/x86/dwmapi.lib" )
#endif

DEFINE_LOCAL_EVENT_TYPE( wxEVT_LAUNCH_PRESSED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_NEW_ROUND_STARTED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_STAGE_FINISHED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_RESET )

// clang-format off
BEGIN_EVENT_TABLE( RenderWindow, wxGLCanvas )
    EVT_PAINT( RenderWindow::onPaint )
    EVT_KEY_DOWN( RenderWindow::onKeyPressed )
    EVT_SIZE( RenderWindow::onSize )
    EVT_HELP( wxID_ANY, RenderWindow::onHelp )
    EVT_COMMAND( wxID_ANY, wxEVT_CURRENT_SCORE_INCREASED, RenderWindow::onScoreIncreased )
    EVT_COMMAND( wxID_ANY, wxEVT_ROUND_COMLETED, RenderWindow::onRoundCompleted )
    EVT_COMMAND( wxID_ANY, wxEVT_BALL_LOST, RenderWindow::onBallLost )
    EVT_COMMAND( wxID_ANY, wxEVT_PING, RenderWindow::onPaddleContact )
    EVT_COMMAND( wxID_ANY, wxEVT_PONG, RenderWindow::onPaddleContact )
    EVT_COMMAND( wxID_ANY, wxEVT_STAGE_FINISHED, RenderWindow::onStageFinished )
    EVT_COMMAND( wxID_ANY, wxEVT_CHAR_SHOW, RenderWindow::onTextCharShow )
END_EVENT_TABLE()
// clang-format on

RenderWindow::RenderWindow(
    wxWindow *parent,
    wxWindowID id,
    const int *attribList,
    const wxSize &size,
    long style,
    const wxString &name,
    const wxPalette &palette )
    : wxGLCanvas( parent, id, attribList, { 0, 0 }, { 1, 1 }, style, name, palette )
{
    SetMinSize( size );
    init();
}

RenderWindow::~RenderWindow()
{
    SetCurrent( *m_context );
    ResourceManager::Clear();
}

void RenderWindow::initializeGLEW()
{
    glewExperimental = true;
    GLenum err = glewInit();
    if ( err != GLEW_OK )
    {
        const GLubyte *msg = glewGetErrorString( err );
        throw std::runtime_error( reinterpret_cast<const char *>( msg ) );
    }
}

void RenderWindow::setupGraphics()
{
#if defined( _MSC_VER )
    wglSwapIntervalEXT( -1 );
#elif defined( _POSIX_VER )
    // NOTE check for GLX_SGI_swap_control extension : http://www.opengl.org/wiki/Swap_Interval#In_Linux_.2F_GLXw
    glXSwapIntervalSGI( -1 );  
#elif defined( _MACOSX_VER )
    // aglSetInteger (AGL_SWAP_INTERVAL, 0);
    wglSwapIntervalEXT( GetContext()->GetWXGLContext() );
#endif

    // load shaders
    ResourceManager::LoadShader( "/../data/shaders/Sprite.vs",   "/../data/shaders/Sprite.fraq",   "", "sprite" );
    ResourceManager::LoadShader( "/../data/shaders/Particle.vs", "/../data/shaders/Particle.frag", "", "particle" );
    ResourceManager::LoadShader( "/../data/shaders/Text.vs", "/../data/shaders/Text.frag", "", "text" );

    ResourceManager::GetShader( "sprite" )->setInteger( "image", 0, true );
    ResourceManager::GetShader( "particle" )->setInteger( "sprite", 0, true );
    ResourceManager::GetShader( "text" )->setInteger( "charImage", 0, true );

    GL_CHECK( glClearColor( 0.0, 0.0, 0.0, 1.0 ) );
    GL_CHECK( glEnable( GL_TEXTURE_2D ) );
    GL_CHECK( glEnable( GL_COLOR_MATERIAL ) );
    GL_CHECK( glEnable( GL_BLEND ) );
    GL_CHECK( glDisable( GL_DEPTH_TEST ) );
    GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );
}

void RenderWindow::init()
{
    m_context = std::make_unique<wxGLContext>( this );
    SetCurrent( *m_context );  // TODO move to resize

    initializeGLEW();
    setupGraphics();

    // set render-specific controls
    m_spriteRenderer = std::make_shared<SpriteRenderer>( ResourceManager::GetShader( "sprite" ) );
    m_shapesManager = std::make_shared<Shapes::ShapesManager>( this );
    m_overlay = std::make_shared<Overlay>();
    m_textRenderer = std::make_shared<TextRenderer>( this );

    m_soundManager = std::make_shared<SoundManager>();
    m_soundManager->init();

    m_mediaManager = std::make_shared<MediaManager>( this, m_textRenderer );

    SetExtraStyle( wxWS_EX_PROCESS_IDLE );
    wxIdleEvent::SetMode( wxIDLE_PROCESS_SPECIFIED );

    m_timer = std::make_shared<Timer>( false );

    Bind( wxEVT_IDLE, &RenderWindow::onIdle, this );
}


void RenderWindow::playIntro()
{
    m_state = State::PLAY;
    m_mediaManager->playIntro();
}

void RenderWindow::loadLevel( unsigned short level )
{
    try
    {
        m_shapesManager->loadLevel( level );
    }
    catch ( const stage_complete_exception& )
    {
        wxCommandEvent eventStageFinished( wxEVT_STAGE_FINISHED );
        AddPendingEvent( eventStageFinished );
    }
}

void RenderWindow::start()
{
    m_state = State::NEWROUND;
    m_isRunning = true;
}

void RenderWindow::stop()
{
    m_isRunning = false;
    Unbind( wxEVT_IDLE, &RenderWindow::onIdle, this );
}

void RenderWindow::switchRun()
{
    if ( !m_isRunning )
        return;

    if ( m_state == State::NEWROUND )
    {
        wxCommandEvent lounchPressedEvent( wxEVT_LAUNCH_PRESSED );
        ProcessEvent( lounchPressedEvent );

        m_soundManager->playSiren();

        wxCommandEvent roundStartedEvent( wxEVT_NEW_ROUND_STARTED );
        ProcessEvent( roundStartedEvent );

        m_state = State::COUNTDOWN;
        for ( m_countDown = 3; m_countDown >= 1; --m_countDown )
        {
            render();
            m_soundManager->playCountdown();
        }
        m_state = State::NEWROUND;

        m_soundManager->playRocket();
    }

    if ( m_shapesManager->isRunning() )
    {
        m_shapesManager->pause();
        m_state = State::PAUSE;
    }
    else
    {
        m_shapesManager->run( m_state == State::NEWROUND );
        m_state = State::RUN;
    }

    //wxCommandEvent eventStageFinished( wxEVT_STAGE_FINISHED );
    //AddPendingEvent( eventStageFinished );
}

void RenderWindow::resize( const wxSize &size )
{
    if ( size.x < 1 || size.y < 1 )
        return;

    GL_CHECK( glViewport( 0, 0, ( GLint )size.GetWidth(), ( GLint )size.GetHeight() ) );

    // configure shaders
    const auto &projection =
        glm::ortho( 0.0f, static_cast< float >( size.GetWidth() ), 0.0f, static_cast< float >( size.GetHeight() ), -1.0f, 1.0f );

    ResourceManager::GetShader( "sprite" )->setMatrix4( "projection", projection, true );
    ResourceManager::GetShader( "particle" )->setMatrix4( "projection", projection, true );
    ResourceManager::GetShader( "text" )->setMatrix4( "projection", projection, true );

    m_overlay->resize( size );
    m_shapesManager->resize( size );
    m_mediaManager->resize( size );
}

void RenderWindow::onPaint( wxPaintEvent & )
{
    wxPaintDC dc( this );
    render();
}

void RenderWindow::onIdle( wxIdleEvent &event )
{
    m_elapsedTime = m_timer->getElapsedTimeInMilliSec();
    if ( m_elapsedTime >= 16 )
    {
        m_timer->start();
        render();
    }

    event.RequestMore();
}

void RenderWindow::render()
{
    if ( !IsShown() )
        return;

    SetCurrent( *m_context );
    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );

    switch ( m_state )
    {
        case State::PLAY:
            m_mediaManager->renderFrame();
        break;

        case State::NEWROUND:
        case State::RUN:
            m_shapesManager->renderFrame( m_spriteRenderer );
        break;

        case State::PAUSE:
            m_shapesManager->renderFrame( m_spriteRenderer );
            m_overlay->showPause( m_spriteRenderer );
        break;

        case State::COUNTDOWN:
            m_shapesManager->renderFrame( m_spriteRenderer );
            m_overlay->showCountDown( m_spriteRenderer, m_countDown );
        break;

        case State::HELP:
        case State::FINISHED:
            m_textRenderer->renderFrame();
        break;

        default :
        break;
    }

    SwapBuffers();

#if defined( _MSC_VER )
    DwmFlush();
#endif
}

void RenderWindow::onSize( wxSizeEvent &event )
{
    resize( event.GetSize() );
    event.Skip();
}

void RenderWindow::onKeyPressed( wxKeyEvent &event )
{
    switch ( m_state )
    {
        case State::PLAY:
            if ( event.GetKeyCode() == 'R' ||
                event.GetKeyCode() == 'r' )
            {
                m_mediaManager->stop();
            }
        break;

        case State::HELP:
            if ( event.GetKeyCode() == WXK_ESCAPE )
                  m_state = m_prevState;
        break;

        case State::RUN:
        case State::PAUSE:
        case State::NEWROUND:
        {
            switch ( event.GetKeyCode() )
            {
                case WXK_SPACE:
                {
                    switchRun();
                    m_timer->stop();
                    m_timer->start();
                }
                break;

                case WXK_F1:
                {
                    if ( m_state == State::RUN )
                    {
                        switchRun();
                        m_timer->stop();
                        m_timer->start();
                    }
                    m_prevState = m_state;
                }
                break;

                case WXK_ESCAPE:
                    m_parent->Close();
                break;
            }
        }
        break;

        case State::FINISHED:
        {
            if ( event.GetKeyCode() == 'Y' ||
                event.GetKeyCode() == 'y' )
            {
                m_state = State::NEWROUND;
                wxCommandEvent eventReset( wxEVT_RESET );
                AddPendingEvent( eventReset );

                break;
            }

            if ( event.GetKeyCode() == 'N' ||
                event.GetKeyCode() == 'n' )
                m_parent->Close();
        }
        break;
    }
}

void RenderWindow::onScoreIncreased( wxCommandEvent &event )
{
    m_soundManager->playDestroyBrick();
    event.Skip();
}

void RenderWindow::onPaddleContact( wxCommandEvent &event )
{
    static std::map<wxEventType, void ( SoundManager::* )()> s_contactSound = {
      { wxEVT_PING,  &SoundManager::playPing },
      { wxEVT_PONG, &SoundManager::playPong }
    };

    ( m_soundManager.get()->*( s_contactSound[ event.GetEventType() ] ) )();
}

void RenderWindow::onRoundCompleted( wxCommandEvent &event )
{
    m_soundManager->playLevelComplete();
    m_state = State::NEWROUND;

    render();

    event.Skip();
}

void RenderWindow::onBallLost( wxCommandEvent &event )
{
    m_soundManager->playBallLost();
    m_state = State::NEWROUND;

    render();

    event.Skip();
}

void RenderWindow::onTextCharShow( wxCommandEvent& )
{
    m_soundManager->playCharShow();
}

void RenderWindow::onStageFinished( wxCommandEvent& )
{
    static unsigned short s_stage = 0;

    m_textRenderer->selectFontType( TextRendererFont::OLD );
    m_textRenderer->switchToFinishState( ++s_stage );
    m_state = State::FINISHED;
}

void RenderWindow::onHelp( wxHelpEvent& )
{
    m_textRenderer->selectFontType( TextRendererFont::OLD );
    m_textRenderer->switchToHelpState();
    m_state = State::HELP;
}

