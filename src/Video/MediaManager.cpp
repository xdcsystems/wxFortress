// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <condition_variable>

#include <GL/glew.h>
#include <wx/glcanvas.h>

#include "Common/defs.h"
#include "Common/Tools.h"
#include "Renderer/TextRenderer.h"
#include "Renderer/VideoRenderer.h"
#include "Movie.h"
#include "MediaManager.h"

DEFINE_LOCAL_EVENT_TYPE( wxEVT_VIDEO_PLAY )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_VIDEO_FINISHED )

MediaManager::MediaManager( wxGLCanvas* canvas, const textRedererPtr& textRenderer )
  : m_textRenderer( textRenderer )
  , m_canvas( canvas )
  , m_eventHandler( canvas->GetEventHandler() )
  , m_eventMediaPlay( wxEVT_VIDEO_PLAY )
  , m_eventMediaFinished( wxEVT_VIDEO_FINISHED )
{
    m_textRenderer->selectFontType( TextRendererFont::NORMAL );
    m_movie = std::make_shared<Movie>();
}

MediaManager::~MediaManager()
{
    reset();
}

void MediaManager::reset()
{
    m_isPlaying = false;
    m_currentFrame = { nullptr, std::numeric_limits<int64_t>::min() };
    m_duration = -1;
    if ( m_movie )
    {
        m_movie->close();
        m_videoRenderer.reset();
        m_movie.reset();
    }
}

void MediaManager::playIntro()
{
    open( Tools::Instance().getFullFileName( ( "/" + wxTheApp->GetAppName() + ".vid" ).ToStdString() ) );
    m_eventHandler->AddPendingEvent( m_eventMediaPlay );
}

void MediaManager::showSkipMessage()
{
    m_textRenderer->print( "Press R or ESC to skip",
        m_videoRenderer->viewWidth() - 300,
        31,
        glm::vec2( 14.f, 16.f ),
        glm::vec3( .6f, .6f, .6f ) );
}

void MediaManager::open( std::string filename )
{
    if ( !m_movie )
    {
        m_movie = std::make_shared<Movie>();
    }
    m_currentFrame = { nullptr, std::numeric_limits<int64_t>::min() };
    m_duration = -1;
    m_isPlaying = true;
    m_movie->open( std::move( filename ) );
}

void MediaManager::stop()
{
    close();
}

void MediaManager::close()
{
    if ( !m_isPlaying || !m_movie )
        return;

    reset();
    m_eventHandler->AddPendingEvent( m_eventMediaFinished );
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

bool MediaManager::needRefresh()
{
    if ( !m_isPlaying || !m_movie )
        return false;

    if ( m_movie->isFinished() )
    {
        close();
        return false;
    }

    if ( m_duration < 0 )
    {
        m_duration = m_movie->duration();
    }

    const auto [ frame, pts ] = m_movie->currentFrame();
    if ( frame && m_currentFrame.second != pts )
    {
        m_currentFrame.first = frame;
        m_currentFrame.second = pts;

        return true;
    }
    return false;
}

void MediaManager::renderFrame()
{
    if ( m_currentFrame.first && m_videoRenderer )
    {
        const AVFrame* f = m_currentFrame.first;
        switch ( f->format )
        {
            case AV_PIX_FMT_NV12:
                if ( f->data[ 0 ] && f->data[ 1 ] )
                    m_videoRenderer->drawNV12( f->width, f->height, const_cast<uint8_t**>( f->data ), f->linesize );
            break;

            case AV_PIX_FMT_YUV420P:
            case AV_PIX_FMT_YUVJ420P:
                if ( f->data[ 0 ] && f->data[ 1 ] && f->data[ 2 ] )
                    m_videoRenderer->draw( f->width, f->height, const_cast<uint8_t**>( f->data ), f->linesize );
            break;

            default:
                // Skip non-CPU or unsupported pixel formats rather than uploading null/invalid pointers.
            break;
        }
    }

    if ( m_currentFrame.second > s_timerInterval && 
         m_currentFrame.second < m_duration - s_timerInterval )
        showSkipMessage();
}

