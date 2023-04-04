#pragma once

#include <wx/glcanvas.h>

// Forward declarations
class VideoRenderer;
class Movie;

class wxFFmpegView : public wxGLCanvas
{
    public:
        wxFFmpegView( 
            wxWindow* parent, 
            const wxGLAttributes& dispAttrs,
            wxWindowID winid = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = 0,
            const wxString& name = wxGLCanvasName, 
            const wxPalette& palette = wxNullPalette );

        ~wxFFmpegView();

        void open( std::string filename );
        void start();
        void stop();
        void close();

    private:
        void OnPaint( wxPaintEvent& event );
        void OnSize( wxSizeEvent& event );
        void OnRenderTimer( wxTimerEvent& event );

        wxGLContext* m_glContext;
        VideoRenderer* m_videoRenderer { nullptr };
        wxTimer m_renderTimer;
        Movie* m_movie { nullptr };
        int64_t m_pts;

        wxDECLARE_EVENT_TABLE();
};

