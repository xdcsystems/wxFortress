// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <GL/glew.h>

#if defined( _MSC_VER )
    #include <GL/wglew.h>
    #include <dwmapi.h>
#endif

#include "Renderer/VideoRenderer.h"
#include "Movie.h"

#include "wxFFmpegView.h"

#if defined( _MSC_VER )
    #pragma comment( lib, "../external/directX/x86/dwmapi.lib" )
#endif

// clang-format off
wxBEGIN_EVENT_TABLE( wxFFmpegView, wxGLCanvas )
    EVT_PAINT( wxFFmpegView::OnPaint )
    EVT_SIZE( wxFFmpegView::OnSize )
wxEND_EVENT_TABLE();
// clang-format on

wxFFmpegView::wxFFmpegView(
    wxWindow *parent,
    const wxGLAttributes &dispAttrs,
    wxWindowID winid,
    const wxPoint &pos,
    const wxSize &size,
    long style,
    const wxString &name,
    const wxPalette &palette )
  : wxGLCanvas( parent, dispAttrs, winid, pos, size, style, name, palette )
{
    m_glContext = new wxGLContext( this );
    if ( !m_glContext->IsOK() )
    {
        delete m_glContext;
        m_glContext = nullptr;
        return;
    }

    m_renderTimer.Bind( wxEVT_TIMER, &wxFFmpegView::OnRenderTimer, this );
    m_movie = new Movie;
}

wxFFmpegView::~wxFFmpegView()
{
    if ( m_glContext )
    {
        SetCurrent( *m_glContext );
    }

    if ( m_movie )
    {
        m_movie->close();
        delete m_movie;
    }

    if ( m_videoRenderer )
    {
        delete m_videoRenderer;
    }

    if ( m_glContext )
    {
        delete m_glContext;
    }
}

void wxFFmpegView::open( std::string filename )
{
    m_movie->open( std::move( filename ) );
    m_pts = std::numeric_limits<int64_t>::min();
    m_renderTimer.Start( 10 );
}

void wxFFmpegView::start()
{}

void wxFFmpegView::stop()
{}

void wxFFmpegView::close()
{
    m_movie->close();
}

void wxFFmpegView::OnPaint( wxPaintEvent &event )
{
    wxPaintDC dc( this );

    if ( !m_videoRenderer || !m_videoRenderer->ok() )
        return;
}

void wxFFmpegView::OnSize( wxSizeEvent &event )
{
    event.Skip();

    if ( !IsShownOnScreen() )
        return;

    if ( !m_glContext )
        return;

    SetCurrent( *m_glContext );

    if ( !m_videoRenderer )
    {
        m_videoRenderer = new VideoRenderer;
    }

    if ( !m_videoRenderer->ok() )
        return;

    wxSize size = event.GetSize() * GetContentScaleFactor();
    m_videoRenderer->setViewport( 0, 0, size.x, size.y );
}

void wxFFmpegView::OnRenderTimer( wxTimerEvent &event )
{
    auto [frame, pts] = m_movie->currentFrame();
    if ( frame && m_pts != pts )
    {
        m_pts = pts;
        SetCurrent( *m_glContext );
        m_videoRenderer->draw( frame->width, frame->height, frame->data, frame->linesize );
        SwapBuffers();

#if defined( _MSC_VER )
        DwmFlush();
#endif
    }
}

