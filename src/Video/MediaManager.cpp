// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <GL/glew.h>
#include <wx/dcbuffer.h>
#include <wx/glcanvas.h>

#if defined( _MSC_VER )
    #include <dwmapi.h>
#endif

#include "Common/defs.h"
#include "Common/Tools.h"
#include "Renderer/TextRenderer.h"
#include "Renderer/VideoRenderer.h"
#include "Movie.h"

#include "MediaManager.h"

#if defined( _MSC_VER )
    #pragma comment( lib, "../external/directX/x86/dwmapi.lib" )
#endif

DEFINE_LOCAL_EVENT_TYPE( wxEVT_VIDEO_PLAY )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_VIDEO_FINISHED )

MediaManager::MediaManager( wxGLCanvas* canvas, const textRedererPtr& textRenderer )
  : m_textRenderer( textRenderer )
  , m_canvas( canvas )
  , m_eventHandler( canvas->GetEventHandler() )
  , m_eventMediaPlay( wxEVT_VIDEO_PLAY )
  , m_eventMediaFinised( wxEVT_VIDEO_FINISHED )
{
    m_textRenderer->selectFontType( TextRendererFont::NORMAL );
    m_renderTimer.Bind( wxEVT_TIMER, &MediaManager::OnRenderTimer, this );
    m_movie = std::make_shared<Movie>();
}

MediaManager::~MediaManager()
{
    reset();
}

void MediaManager::reset()
{
    if ( m_movie )
    {
        m_movie->close();
        m_videoRenderer.reset();
        m_movie.reset();
    }
}

void MediaManager::playIntro()
{
    open( Tools::Instance().getFullFileName( "/../resources/video/Intro.mp4" ) );
    m_eventHandler->AddPendingEvent( m_eventMediaPlay );
}

void MediaManager::showSkipMessage( bool show )
{
    m_textRenderer->print( "Press R key for skip",
        m_videoRenderer->viewWidth() - 300,
        31,
        glm::vec2( 14.f, 16.f ) );
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

void MediaManager::open( std::string filename )
{
    m_movie->open( std::move( filename ) );
    m_pts = std::numeric_limits<int64_t>::min();
    m_renderTimer.Start( 10 );
}

void MediaManager::stop()
{
    m_renderTimer.Stop();
    close();
}

void MediaManager::close()
{
    reset();
    m_eventHandler->AddPendingEvent( m_eventMediaFinised );
}

void MediaManager::resize( const wxSize& size )
{
    if ( !m_canvas->IsShownOnScreen() || !m_movie )
        return;

    if ( !m_videoRenderer )
    {
        m_videoRenderer = std::make_shared<VideoRenderer>();
    }

    if ( !m_videoRenderer->ok() )
        return;

    wxSize currentSize = size * m_canvas->GetContentScaleFactor();
    m_videoRenderer->setViewport( 0, 0, currentSize.x, currentSize.y );
}

void MediaManager::OnRenderTimer( wxTimerEvent& event )
{
    if ( m_movie->isFinished() )
    {
        stop();
        return;
    }

    auto [frame, pts] = m_movie->currentFrame();
    if ( frame && m_pts != pts )
    {
        m_pts = pts;
        m_videoRenderer->draw( frame->width, frame->height, frame->data, frame->linesize, true );
        
        if ( m_pts > s_timerInterval )
            showSkipMessage();

        m_canvas->SwapBuffers();

#if defined( _MSC_VER )
        DwmFlush();
#endif
    }
}

