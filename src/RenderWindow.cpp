// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <algorithm>
#include <map>
#include <string>

#include <GL/glew.h>

#if defined( _WIN32 )
    #include <GL/wglew.h>
    #include <dwmapi.h>
    #include <mmsystem.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Common/defs.h"
#include "Common/Timer.h"
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

#if defined( __linux__ ) || defined( __LINUX__ )
    // Include after project headers so X11 macros (e.g. None, Bool, Status) never pollute C++ enums.
    #include <GL/glxew.h>
#endif


DEFINE_LOCAL_EVENT_TYPE( wxEVT_WND_INITIALIZED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_LAUNCH_PRESSED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_NEW_ROUND_STARTED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_STAGE_FINISHED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_RESET )


// clang-format off
BEGIN_EVENT_TABLE( RenderWindow, wxGLCanvas )
    EVT_PAINT( RenderWindow::onPaint )
    EVT_KEY_DOWN( RenderWindow::onKeyPressed )
    EVT_KEY_UP( RenderWindow::onKeyReleased )
    EVT_KILL_FOCUS( RenderWindow::onKillFocus )
    EVT_MOTION( RenderWindow::onMouseMove )
    EVT_SIZE( RenderWindow::onSize )
    EVT_COMMAND( wxID_ANY, wxEVT_CURRENT_SCORE_INCREASED, RenderWindow::onScoreIncreased )
    EVT_COMMAND( wxID_ANY, wxEVT_ROUND_COMPLETED, RenderWindow::onRoundCompleted )
    EVT_COMMAND( wxID_ANY, wxEVT_BALL_LOST, RenderWindow::onBallLost )
    EVT_COMMAND( wxID_ANY, wxEVT_PING, RenderWindow::onPaddleContact )
    EVT_COMMAND( wxID_ANY, wxEVT_PONG, RenderWindow::onPaddleContact )
    EVT_COMMAND( wxID_ANY, wxEVT_ARMOR_HIT, RenderWindow::onArmorHit )
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

    m_context = std::make_unique<wxGLContext>( this );
    if ( !m_context->IsOK() )
    {
        throw std::runtime_error( "Unable to create wxGLContext" );
    }

    ResourceManager::LoadResources();

    SetBackgroundStyle( wxBG_STYLE_PAINT );
    SetExtraStyle( wxWS_EX_PROCESS_IDLE );
    wxIdleEvent::SetMode( wxIDLE_PROCESS_SPECIFIED );

#if defined( _WIN32 )
    timeBeginPeriod( 1 );
#endif

    m_timer = std::make_shared<Timer>( false );

    Bind( wxEVT_IDLE, &RenderWindow::onIdle, this );
}

RenderWindow::~RenderWindow()
{
    ensureContextCurrent();
    if ( m_mediaManager )
        m_mediaManager->reset();
    ResourceManager::Clear();
#if defined( _WIN32 )
    timeEndPeriod( 1 );
#endif
}

void RenderWindow::ensureContextCurrent()
{
    if ( !m_context )
        return;

#if defined( _WIN32 )
    if ( wglGetCurrentContext() == m_context->GetGLRC() && wglGetCurrentDC() == GetHDC() )
        return;
#endif
    SetCurrent( *m_context );
}

void RenderWindow::configureSwapAndPacing()
{
#if defined( _WIN32 )
    BOOL dwmEnabled = FALSE;
    m_useDwmPacing = SUCCEEDED( DwmIsCompositionEnabled( &dwmEnabled ) ) && dwmEnabled == TRUE;
    if ( m_useDwmPacing )
    {
        DWM_TIMING_INFO ti {};
        ti.cbSize = sizeof( ti );
        LARGE_INTEGER freq {};
        if ( SUCCEEDED( DwmGetCompositionTimingInfo( nullptr, &ti ) ) &&
             QueryPerformanceFrequency( &freq ) &&
             ti.qpcRefreshPeriod > 0 && freq.QuadPart > 0 )
        {
            FramePacing::setNominalPeriodSec(
                m_framePacer,
                static_cast<double>( ti.qpcRefreshPeriod ) / static_cast<double>( freq.QuadPart ) );
        }

        // Let DwmFlush() own VBlank pacing to avoid ATI WDDM 1.1 SwapBuffers(interval=1) stalls.
        if ( wglSwapIntervalEXT )
            wglSwapIntervalEXT( 0 );

        m_framePacer.paceExternally = true;
    }
    else if ( wglSwapIntervalEXT )
    {
        wglSwapIntervalEXT( 1 );
    }
#elif defined( __linux__ ) || defined( __LINUX__ )
    // Prefer GLX_EXT_swap_control, then MESA, then SGI (positive interval = 1).
    if ( GLXEW_EXT_swap_control && glXSwapIntervalEXT )
    {
        Display* dpy = glXGetCurrentDisplay();
        GLXDrawable drawable = glXGetCurrentDrawable();
        if ( dpy && drawable )
            glXSwapIntervalEXT( dpy, drawable, 1 );
    }
    else if ( GLXEW_MESA_swap_control && glXSwapIntervalMESA )
    {
        glXSwapIntervalMESA( 1 );
    }
    else if ( GLXEW_SGI_swap_control && glXSwapIntervalSGI )
    {
        glXSwapIntervalSGI( 1 );
    }
#endif
}

bool RenderWindow::waitForVBlank()
{
#if defined( _WIN32 )
    if ( m_useDwmPacing )
    {
        if ( SUCCEEDED( DwmFlush() ) )
            return true;

        m_useDwmPacing = false;
        if ( wglSwapIntervalEXT )
            wglSwapIntervalEXT( 1 );
        return false;
    }
    return true;
#else
    return true;
#endif
}

void RenderWindow::initializeGLEW()
{
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if ( err != GLEW_OK )
    {
        const GLubyte *msg = glewGetErrorString( err );
        throw std::runtime_error( reinterpret_cast<const char *>( msg ) );
    }
    // Drain any benign GL_INVALID_ENUM emitted by glewInit() on modern drivers (bounded).
    for ( int i = 0; i < 64 && glGetError() != GL_NO_ERROR; ++i ) {}
}

void RenderWindow::setupGraphics()
{
    configureSwapAndPacing();

    GL_CHECK( glClearColor( 0.0, 0.0, 0.0, 1.0 ) );
    GL_CHECK( glEnable( GL_BLEND ) );
    GL_CHECK( glDisable( GL_DEPTH_TEST ) );
    GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );
}

void RenderWindow::init()
{
    m_spriteRenderer = std::make_shared<SpriteRenderer>();
    m_shapesManager = std::make_shared<Shapes::ShapesManager>( this, m_spriteRenderer );
    m_overlay = std::make_shared<Overlay>( m_spriteRenderer );

    m_soundManager->init();
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
        m_shapesManager->loadLevel( level, m_stage );
        m_currentLevel = level;
        if ( m_soundManager && m_state != State::PLAY )
            m_soundManager->playMusic( level );
    }
    catch ( const stage_complete_exception& )
    {
        QueueEvent( new wxCommandEvent( wxEVT_STAGE_FINISHED ) );
    }
}

void RenderWindow::resetBallSpeed()
{
    if ( m_shapesManager )
        m_shapesManager->resetBallSpeed();
    m_stageIncidents = 0;
    m_impactFeedback.reset();
    m_hudStatus.clearTransient();
}

void RenderWindow::resetStage()
{
    m_stage = 0;
}

void RenderWindow::syncScoreTelemetry( uint32_t currentScore, uint32_t hiScore, bool newRecord )
{
    m_currentScore = currentScore;
    m_hiScore = hiScore;
    if ( newRecord )
    {
        m_hudStatus.showTransient( "NEW RECORD", 2.6 );
    }
}

void RenderWindow::resetSimulationTiming()
{
    if ( m_timer )
        m_timer->start();
    FramePacing::resetFrameTiming( m_framePacer );
    FramePacing::resetFixedStep( m_fixedStep );
    if ( m_shapesManager )
    {
        m_shapesManager->syncPreviousState();
        m_shapesManager->setRenderAlpha( 1.0f );
    }
}

void RenderWindow::start()
{
    m_state = State::NEWROUND;
    m_isRunning = true;
    m_impactFeedback.reset();
    resetSimulationTiming();
    if ( m_soundManager )
        m_soundManager->playMusic( m_currentLevel );
}

void RenderWindow::triggerLaunch()
{
    if ( m_state == State::LAUNCHING || m_state == State::COUNTDOWN )
        return;

    if ( m_state == State::RUN )
    {
        // Ignore stray key-repeat / queued Space right after the countdown finishes.
        const auto sinceLaunchMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_lastLaunchTime ).count();
        if ( sinceLaunchMs >= 0 && sinceLaunchMs < 350 )
            return;
    }

    if ( m_state == State::NEWROUND || m_state == State::RUN || m_state == State::PAUSE )
    {
        switchRun();
        resetSimulationTiming();
        wxWakeUpIdle();
    }
}

void RenderWindow::stop()
{
    m_isRunning = false;
    m_impactFeedback.reset();
    m_hudStatus.clearTransient();
    if ( m_mediaManager )
    {
        ensureContextCurrent();
        m_mediaManager->reset();
    }
    if ( m_soundManager )
    {
        m_soundManager->stopLaunchAudio();
        m_soundManager->stopMusic();
    }
    Unbind( wxEVT_IDLE, &RenderWindow::onIdle, this );
}

void RenderWindow::switchRun()
{
    if ( !m_isRunning || m_state == State::LAUNCHING || m_state == State::COUNTDOWN )
        return;

    if ( m_state == State::NEWROUND )
    {
        wxCommandEvent lounchPressedEvent( wxEVT_LAUNCH_PRESSED );
        ProcessWindowEvent( lounchPressedEvent );

        if ( m_soundManager )
            m_soundManager->playSiren();

        m_state = State::LAUNCHING;
        return;
    }

    if ( m_shapesManager->isRunning() )
    {
        m_shapesManager->pause();
        m_state = State::PAUSE;
    }
    else
    {
        m_shapesManager->run( false );
        m_state = State::RUN;
    }
}

void RenderWindow::resize( const wxSize &size )
{
    if ( size.x < 1 || size.y < 1 )
        return;

    if ( m_isInitialized )
        ensureContextCurrent();

    const wxSize physSize = size * GetContentScaleFactor();
    const GLsizei vpW = std::max<GLsizei>( 1, static_cast<GLsizei>( physSize.GetWidth() ) );
    const GLsizei vpH = std::max<GLsizei>( 1, static_cast<GLsizei>( physSize.GetHeight() ) );
    GL_CHECK( glViewport( 0, 0, vpW, vpH ) );

    const auto &projection =
        glm::ortho( 0.0f, static_cast< float >( size.GetWidth() ), 0.0f, static_cast< float >( size.GetHeight() ), -1.0f, 1.0f );

    if ( m_textRenderer )
        m_textRenderer->resize( projection );

    if ( m_spriteRenderer )
        m_spriteRenderer->resize( projection );

    if ( m_overlay )
        m_overlay->resize( size );

    if ( m_shapesManager )
        m_shapesManager->resize( size, projection );

    if ( m_mediaManager )
        m_mediaManager->resize( size );
}

void RenderWindow::onPaint( wxPaintEvent & )
{
    wxPaintDC dc( this );
    render();
}

void RenderWindow::onIdle( wxIdleEvent &event )
{
    const auto* topLevel = wxDynamicCast( wxGetTopLevelParent( this ), wxTopLevelWindow );
    if ( !IsShownOnScreen() || ( topLevel && topLevel->IsIconized() ) )
    {
        if ( m_mediaManager )
            m_mediaManager->cancelSkipHold();
        resetSimulationTiming();
        wxMilliSleep( 25 );
        event.RequestMore();
        return;
    }

    if ( m_state == State::PLAY )
    {
        const bool presented = render();
        if ( m_useDwmPacing )
        {
            if ( !waitForVBlank() )
                wxMilliSleep( 4 );
        }
        else if ( !presented )
        {
            wxMilliSleep( 4 );
        }

        event.RequestMore();
        return;
    }

    const double elapsedSec = m_timer->getElapsedTimeInSec();
    m_elapsedTime = elapsedSec * 1000.0;
    if ( FramePacing::shouldStep( m_framePacer, elapsedSec ) )
    {
        m_timer->start();
        const double dtSec = FramePacing::computeStepDt( m_framePacer, elapsedSec );

        m_impactFeedback.step( dtSec );
        m_hudStatus.step( dtSec );

        if ( m_soundManager )
        {
            const bool shouldDuck = ( m_state == State::PAUSE );
            m_soundManager->setMusicDucked( shouldDuck );
            const float resScale = ( m_shapesManager && m_state == State::RUN )
                ? m_shapesManager->resonanceMusicDuckScale()
                : 1.0f;
            m_soundManager->setResonanceDuckScale( resScale );
            m_soundManager->updateMusic( dtSec );
        }
        if ( m_state == State::LAUNCHING )
        {
            if ( !m_soundManager || !m_soundManager->isLaunchAudioPlaying() )
            {
                wxCommandEvent roundStartedEvent( wxEVT_NEW_ROUND_STARTED );
                ProcessWindowEvent( roundStartedEvent );

                m_state = State::COUNTDOWN;
                m_countDown = 3;
                if ( m_soundManager )
                    m_soundManager->playCountdown();
            }
        }
        else if ( m_state == State::COUNTDOWN )
        {
            if ( !m_soundManager || !m_soundManager->isLaunchAudioPlaying() )
            {
                if ( m_countDown > 1 )
                {
                    --m_countDown;
                    if ( m_soundManager )
                        m_soundManager->playCountdown();
                }
                else
                {
                    m_countDown = 0;
                    if ( m_soundManager )
                    {
                        m_soundManager->stopLaunchAudio();
                        m_soundManager->playRocket();
                    }
                    if ( m_shapesManager )
                        m_shapesManager->run( true );
                    m_state = State::RUN;
                    m_lastLaunchTime = std::chrono::steady_clock::now();
                    FramePacing::resetFixedStep( m_fixedStep );
                    if ( m_shapesManager )
                        m_shapesManager->setRenderAlpha( 1.0f );
                }
            }
        }
        if ( m_shapesManager &&
             !m_impactFeedback.isHitStopActive() &&
             ( m_state == State::NEWROUND || m_state == State::LAUNCHING ||
               m_state == State::COUNTDOWN || m_state == State::RUN ) )
        {
            constexpr FramePacing::FixedStepConfig kFixedCfg {};
            const int fixedSteps = FramePacing::consumeFixedSteps( m_fixedStep, dtSec, kFixedCfg );
            const bool wasRunning = m_shapesManager->isRunning();
            for ( int step = 0; step < fixedSteps; ++step )
            {
                m_shapesManager->update( kFixedCfg.fixedDtSec, fixedSteps - step );
                bool isMaxOrFinal = false;
                if ( m_shapesManager->consumePendingImpact( isMaxOrFinal ) )
                {
                    m_impactFeedback.triggerThresholdImpact( isMaxOrFinal );
                    FramePacing::resetFixedStep( m_fixedStep );
                    m_shapesManager->syncPreviousState();
                    break;
                }
                if ( wasRunning && !m_shapesManager->isRunning() )
                {
                    FramePacing::resetFixedStep( m_fixedStep );
                    break;
                }
            }
            m_shapesManager->setRenderAlpha( static_cast<float>( m_fixedStep.alpha ) );
        }
        if ( m_useDwmPacing )
        {
            render();
            const auto syncStart = std::chrono::steady_clock::now();
            const bool syncOk = waitForVBlank();
            const double syncMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - syncStart ).count();
            FramePacing::recordSyncDuration( m_framePacer, syncOk, syncMs );
            if ( !m_framePacer.paceExternally )
            {
                m_useDwmPacing = false;
#if defined( _WIN32 )
                if ( wglSwapIntervalEXT )
                    wglSwapIntervalEXT( 1 );
#endif
            }
        }
        else
        {
            const auto presentStart = std::chrono::steady_clock::now();
            render();
            const double presentMs = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - presentStart ).count();
            FramePacing::recordSyncDuration( m_framePacer, true, presentMs );
        }

        const bool isStaticState = ( m_state == State::PAUSE || m_state == State::HELP );
        const bool musicTransitioning = m_soundManager && m_soundManager->isMusicGainTransitioning();
        const bool hudOrFeedbackActive = m_impactFeedback.isActive() || m_hudStatus.hasTransient();
        if ( isStaticState && !musicTransitioning && !hudOrFeedbackActive )
            return;
    }
    else
    {
        wxMilliSleep( FramePacing::computeSleepMs( m_framePacer, elapsedSec ) );
    }

    event.RequestMore();
}

void RenderWindow::renderHud()
{
    if ( !m_textRenderer )
        return;

    const float flashAlpha = m_impactFeedback.flashAlpha();
    if ( flashAlpha > 0.001f )
    {
        m_textRenderer->drawSensorFlash( m_impactFeedback.flashColor, flashAlpha );
    }

    const wxSize clientSize = GetClientSize();
    if ( clientSize.x < 100 || clientSize.y < 100 )
        return;

    // Top free black strip above bricks (max brick top is 540 px on 800x729) -> <= 2 active HUD zones.
    // Matches the Help page and post-5th-level report style verbatim: Font.png (OLD), 11x15.25 native cell size.
    constexpr glm::vec2 kHudCharSize { 11.f, 15.25f };
    constexpr float kMarginX = 18.0f;
    const float hudY = static_cast<float>( clientSize.y ) - 28.0f;

    const unsigned int mult = m_shapesManager ? m_shapesManager->resonanceMultiplier() : 1u;
    const bool showZone1 = ( mult >= 2u );
    const bool isRobot = m_shapesManager && m_shapesManager->isRobotMode();
    const Resonance::HudStatus::DisplayLine zone2 = m_hudStatus.resolveZone2( isRobot, m_currentScore, m_hiScore );
    const bool showZone2 = zone2.visible && !zone2.text.empty();
    if ( !showZone1 && !showZone2 )
        return;

    m_textRenderer->selectFontType( TextRendererFont::OLD );

    // Font.png has opaque black glyph cells (A=255): additive blending is pixel-exact identical on black
    // and prevents an opaque black cutout box if the ball or particles pass through the top strip.
    GL_CHECK( glBlendFunc( GL_ONE, GL_ONE ) );

    // Zone 1 (top-left, priority): active plasma resonance multiplier x2..x8
    float zone1RightX = 0.0f;
    if ( showZone1 )
    {
        const std::string resText = "RESONANCE x" + std::to_string( mult );
        m_textRenderer->print( resText, { kMarginX, hudY }, kHudCharSize );
        zone1RightX = kMarginX + static_cast<float>( resText.size() ) * kHudCharSize.x;
    }

    // Zone 2 (top-right, contextual): transient event (2-3s) overrides persistent status
    if ( showZone2 )
    {
        const float textWidth = static_cast<float>( zone2.text.size() ) * kHudCharSize.x;
        const float posX = ( std::max )( kMarginX, static_cast<float>( clientSize.x ) - textWidth - kMarginX );
        if ( zone1RightX <= 0.0f || posX >= zone1RightX + 16.0f )
            m_textRenderer->print( zone2.text, { posX, hudY }, kHudCharSize );
    }

    GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );
}

bool RenderWindow::render()
{
    if ( !IsShown() || !m_mediaManager )
        return false;

    if ( m_state == State::PLAY )
    {
        if ( m_mediaManager->needRefresh() )
        {
            ensureContextCurrent();
            GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );

            m_mediaManager->renderFrame();

            SwapBuffers();
            return true;
        }

        return false;
    }

    ensureContextCurrent();
    GL_CHECK( glClear( GL_COLOR_BUFFER_BIT ) );

    switch ( m_state )
    {
        case State::NEWROUND:
        case State::LAUNCHING:
        case State::RUN:
            m_shapesManager->renderFrame();
            renderHud();
        break;

        case State::PAUSE:
            m_shapesManager->renderFrame();
            m_overlay->showPause();
            renderHud();
        break;

        case State::COUNTDOWN:
            m_shapesManager->renderFrame();
            m_overlay->showCountDown( m_countDown );
            renderHud();
        break;

        case State::HELP:
        case State::FINISHED:
            m_textRenderer->renderFrame();
        break;

        default :
        break;
    }

    SwapBuffers();
    return true;
}

void RenderWindow::onSize( wxSizeEvent &event )
{
    event.Skip();
    
    // If this window is not fully initialized, dismiss this event
    if ( !IsShownOnScreen() )
        return;

    if ( !m_isInitialized )
    {
        if ( SetCurrent( *m_context ) )
        {
            initializeGLEW();
            setupGraphics();

            // set render-specific controls
            m_textRenderer = std::make_shared<TextRenderer>( this );
            m_soundManager = std::make_shared<SoundManager>();
            m_mediaManager = std::make_shared<MediaManager>( this, m_textRenderer );

            m_isInitialized = true;

            resize( event.GetSize() );
            
            QueueEvent( new wxCommandEvent( wxEVT_WND_INITIALIZED ) );

            return;
        }
        throw std::runtime_error( "Unable to set context current." );
    }

    resize( event.GetSize() );
}

void RenderWindow::onMouseMove( wxMouseEvent &event )
{
    if ( m_shapesManager &&
         ( m_state == State::RUN || m_state == State::NEWROUND ||
           m_state == State::LAUNCHING || m_state == State::COUNTDOWN ) )
    {
        m_shapesManager->setPaddleMouseX( static_cast<float>( event.GetX() ) );
    }
    event.Skip();
}

void RenderWindow::onKeyPressed( wxKeyEvent &event )
{
    if ( event.AltDown() )
    {
        event.Skip();
        return;
    }

    if ( m_state != State::PLAY && ( event.GetKeyCode() == 'M' || event.GetKeyCode() == 'm' ) )
    {
        if ( m_soundManager )
            m_soundManager->toggleMusicMute();
        wxWakeUpIdle();
        return;
    }

    if ( m_shapesManager &&
         ( m_state == State::RUN || m_state == State::PAUSE || m_state == State::NEWROUND ||
           m_state == State::LAUNCHING || m_state == State::COUNTDOWN ) &&
         ( event.GetKeyCode() == 'A' || event.GetKeyCode() == 'a' ) )
    {
        const bool enabled = m_shapesManager->toggleRobotMode();
        if ( m_soundManager )
            m_soundManager->playRobotToggle( enabled );
        if ( enabled )
            m_hudStatus.clearTransient();
        Refresh( false );
        wxWakeUpIdle();
        return;
    }

    switch ( m_state )
    {
        case State::PLAY:
            if ( event.GetKeyCode() == 'R' ||
                 event.GetKeyCode() == 'r' )
            {
                if ( m_mediaManager )
                    m_mediaManager->onSkipKeyDown();
            }
        break;

        case State::HELP:
            if ( event.GetKeyCode() == WXK_ESCAPE )
            {
                m_state = m_prevState;
                resetSimulationTiming();
                if ( m_soundManager )
                    m_soundManager->playMusic( m_currentLevel );
                Refresh( false );
                wxWakeUpIdle();
            }
        break;

        case State::RUN:
        case State::PAUSE:
        case State::NEWROUND:
        {
            switch ( event.GetKeyCode() )
            {
                case WXK_SPACE:
                {
                    triggerLaunch();
                }
                break;

                case WXK_F1:
                {
                    if ( m_state == State::RUN )
                    {
                        switchRun();
                        resetSimulationTiming();
                    }
                    m_prevState = m_state;

                    m_textRenderer->selectFontType( TextRendererFont::OLD );
                    m_textRenderer->switchToHelpState();
                    m_state = State::HELP;
                    if ( m_soundManager )
                        m_soundManager->playHelpMusic();
                    Refresh( false );
                    wxWakeUpIdle();
                }
                break;

                case WXK_ESCAPE:
                    m_parent->Close();
                break;
            }
        }
        break;

        case State::LAUNCHING:
        case State::COUNTDOWN:
            if ( event.GetKeyCode() == WXK_ESCAPE )
                m_parent->Close();
        break;

        case State::FINISHED:
        {
            if ( event.GetKeyCode() == 'Y' ||
                event.GetKeyCode() == 'y' )
            {
                m_stageIncidents = 0;
                m_state = State::NEWROUND;
                resetSimulationTiming();
                QueueEvent( new wxCommandEvent( wxEVT_RESET ) );
                wxWakeUpIdle();

                break;
            }

            if ( event.GetKeyCode() == 'N' ||
                event.GetKeyCode() == 'n' )
                m_parent->Close();
        }
        break;
    }
}

void RenderWindow::onKeyReleased( wxKeyEvent &event )
{
    if ( event.GetKeyCode() == 'R' || event.GetKeyCode() == 'r' )
    {
        if ( m_mediaManager )
            m_mediaManager->onSkipKeyUp();
    }
    event.Skip();
}

void RenderWindow::onKillFocus( wxFocusEvent &event )
{
    if ( m_mediaManager )
        m_mediaManager->cancelSkipHold();
    event.Skip();
}

void RenderWindow::onScoreIncreased( wxCommandEvent &event )
{
    const long streakRaw = event.GetExtraLong();
    const unsigned int streak = streakRaw > 0 ? static_cast<unsigned int>( streakRaw ) : 1u;
    const float pitch = Resonance::pitchForStreak( streak );
    if ( m_soundManager )
        m_soundManager->playDestroyBrick( pitch );
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

void RenderWindow::onArmorHit( wxCommandEvent& )
{
    if ( m_soundManager )
        m_soundManager->playArmorHit( 1.25f );
}

void RenderWindow::onRoundCompleted( wxCommandEvent &event )
{
    m_soundManager->playLevelComplete();
    m_state = State::NEWROUND;
    resetSimulationTiming();

    render();

    event.Skip();
}

void RenderWindow::onBallLost( wxCommandEvent &event )
{
    if ( m_shapesManager )
        m_shapesManager->stop();
    ++m_stageIncidents;
    m_impactFeedback.triggerBreachFlash();
    m_hudStatus.showTransient( "INCIDENT 1844/27: BREACH", 2.8 );
    m_soundManager->playBallLost();
    m_state = State::NEWROUND;
    resetSimulationTiming();

    render();

    event.Skip();
}

void RenderWindow::onTextCharShow( wxCommandEvent& )
{
    m_soundManager->playCharShow();
}

void RenderWindow::onStageFinished( wxCommandEvent& )
{
    m_textRenderer->selectFontType( TextRendererFont::OLD );
    m_textRenderer->switchToFinishState( ++m_stage, m_stageIncidents );
    m_state = State::FINISHED;
    resetSimulationTiming();
    if ( m_soundManager )
        m_soundManager->playVictoryMusic();
}
