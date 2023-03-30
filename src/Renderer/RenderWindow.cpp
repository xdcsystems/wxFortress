// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
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
#include "Common/Tools.h"
#include "Common/Rect.hpp"
#include "Shader.h"
#include "SpriteRenderer.h"
#include "Sounds/SoundManager.h"
#include "Shapes/ShapesManager.h"
#include "Shapes/ParticleGenerator.h"
#include "ResourceManager.h"
#include "Overlay.h"
#include "TextRenderer.h"
#include "RenderWindow.h"

#if defined( _MSC_VER )
    #pragma comment( lib, "../external/directX/x86/dwmapi.lib" )
#endif

DEFINE_LOCAL_EVENT_TYPE( wxEVT_LAUNCH_PRESSED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_NEW_ROUND_STARTED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_STAGE_FINISHED )

BEGIN_EVENT_TABLE( RenderWindow, wxGLCanvas )
    EVT_PAINT( RenderWindow::onPaint )
    EVT_KEY_DOWN( RenderWindow::onKeyPressed )
    EVT_SIZE( RenderWindow::onSize )
    EVT_COMMAND( wxID_ANY, wxEVT_CURRENT_SCORE_INCREASED, RenderWindow::onScoreIncreased )
    EVT_COMMAND( wxID_ANY, wxEVT_ROUND_COMLETED, RenderWindow::onRoundCompleted )
    EVT_COMMAND( wxID_ANY, wxEVT_BALL_LOST, RenderWindow::onBallLost )
    EVT_COMMAND( wxID_ANY, wxEVT_PING, RenderWindow::onPaddleContact )
    EVT_COMMAND( wxID_ANY, wxEVT_PONG, RenderWindow::onPaddleContact )
    EVT_COMMAND( wxID_ANY, wxEVT_STAGE_FINISHED, RenderWindow::onStageFinished )
END_EVENT_TABLE()

RenderWindow::RenderWindow(
    wxWindow *parent,
    wxWindowID id,
    const int *attribList,
    const wxPoint &pos,
    const wxSize &size,
    long style,
    const wxString &name,
    const wxPalette &palette )
  : wxGLCanvas( parent, id, attribList, pos, size, style, name, palette )
{
    init();
}

void RenderWindow::init()
{
    m_context = std::make_unique<wxGLContext>( this );
    SetCurrent( *m_context );  // TODO move to resize

    initializeGLEW();
    setupGraphics();

    m_soundManager = std::make_shared<SoundManager>();
    m_soundManager->init();

    SetExtraStyle( wxWS_EX_PROCESS_IDLE );
    wxIdleEvent::SetMode( wxIDLE_PROCESS_SPECIFIED );

    m_timer = std::make_shared<Timer>( false );
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
        throw std::exception( reinterpret_cast<const char *>( msg ) );
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

    const auto size = GetClientSize();

    // load shaders
    ResourceManager::LoadShader( "/../data/shaders/Sprite.vs",   "/../data/shaders/Sprite.fraq",   "", "sprite" );
    ResourceManager::LoadShader( "/../data/shaders/Particle.vs", "/../data/shaders/Particle.frag", "", "particle" );
    ResourceManager::LoadShader( "/../data/shaders/Text.vs",     "/../data/shaders/Text.frag",     "", "text" );

    // configure shaders
    glm::mat4 projection =
      glm::ortho( 0.0f, static_cast<float>( size.GetWidth() ), 0.0f, static_cast<float>( size.GetHeight() ), -1.0f, 1.0f );

    ResourceManager::GetShader( "sprite" )->use().setInteger( "image", 0 );
    ResourceManager::GetShader( "sprite" )->setMatrix4( "projection", projection );
    ResourceManager::GetShader( "particle" )->use().setInteger( "sprite", 0 );
    ResourceManager::GetShader( "particle" )->setMatrix4( "projection", projection );
    ResourceManager::GetShader( "text" )->use().setInteger( "charImage", 0 );
    ResourceManager::GetShader( "text" )->setMatrix4( "projection", projection );

    GL_CHECK( glClearColor( 0.0, 0.0, 0.0, 1.0 ) );
    GL_CHECK( glEnable( GL_TEXTURE_2D ) );
    GL_CHECK( glEnable( GL_COLOR_MATERIAL ) );
    GL_CHECK( glEnable( GL_BLEND ) );
    GL_CHECK( glDisable( GL_DEPTH_TEST ) );
    GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );

    // set render-specific controls
    m_spriteRenderer = std::make_shared<SpriteRenderer>( ResourceManager::GetShader( "sprite" ) );
    m_shapesManager = std::make_shared<Shapes::ShapesManager>( this );
}

void RenderWindow::start()
{
    Bind( wxEVT_IDLE, &RenderWindow::onIdle, this );
    m_isRunning = true;
}

void RenderWindow::stop()
{
    m_isRunning = false;
    Unbind( wxEVT_IDLE, &RenderWindow::onIdle, this );
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

    m_state = m_shapesManager->switchRun( m_state == State::NEWROUND ) ? State::RUN : State::PAUSE;
    //wxCommandEvent eventStageFinished( wxEVT_STAGE_FINISHED );
    //AddPendingEvent( eventStageFinished );
}

void RenderWindow::resize( const wxSize &size )
{
    if ( size.x < 1 || size.y < 1 )
        return;

    //GL_CHECK( glViewport( 0, 0, ( GLint )size.GetWidth(), ( GLint )size.GetHeight() ) );

    m_overlay = std::make_shared<Overlay>( size );
    m_shapesManager->resize( size );
}

void RenderWindow::onPaint( wxPaintEvent & )
{
    wxPaintDC dc( this );
    render();
}

void RenderWindow::onIdle( wxIdleEvent &event )
{
    if ( m_isRunning )
    {
        m_elapsedTime = m_timer->getElapsedTimeInMilliSec();
        if ( m_elapsedTime >= 16 )
        {
            m_timer->start();
            render();
        }
    }
    event.RequestMore();
}

void RenderWindow::clearScreen()
{
    SetCurrent( *m_context );
    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );
}

void RenderWindow::render()
{
    if ( !m_isRunning || !IsShown() )
        return;

    clearScreen();

    switch ( m_state )
    {
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

        case State::FINISHED:
            m_textRender->renderFrame();
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
}

void RenderWindow::onKeyPressed( wxKeyEvent &event )
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
            //... give help ...
        break;

        case WXK_ESCAPE:
            m_parent->Close();
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

void RenderWindow::onStageFinished( wxCommandEvent& )
{
    m_textRender = std::make_shared<TextRenderer>();
    m_textRender->init();
    
    m_state = State::FINISHED;
}

