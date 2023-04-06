#pragma once

// Forward declarations
class wxGLCanvas;
class TextRenderer;
class VideoRenderer;
class Movie;

class MediaManager final
{
    using textRedererPtr = std::shared_ptr<TextRenderer>;
    using videoRedererPtr = std::shared_ptr<VideoRenderer>;
    using moviePtr = std::shared_ptr<Movie>;

    public:
        MediaManager( wxGLCanvas* canvas, const textRedererPtr &textRenderer );
        ~MediaManager();

        void resize( const wxSize& size );
        void reset();
        void playIntro();
        void stop();

   private:
        // Event Handlers
        void OnRenderTimer( wxTimerEvent& );

        // Helper functions
        void showSkipMessage( bool show = true );
        void open( std::string filename );
        void close();
        
        // Private data
        static const int64_t s_timerInterval = 15000000000; // 15 sec
        static const int s_timerCheckEndInterval = 1000; // 1 sec

        wxTimer m_renderTimer;

        textRedererPtr m_textRenderer;
        videoRedererPtr m_videoRenderer;
        moviePtr m_movie;
        
        int64_t m_pts { 0 };
        bool m_isOK { false };

        wxGLCanvas* m_canvas { nullptr };

        wxEvtHandler* m_eventHandler { nullptr };
        wxCommandEvent m_eventMediaPlay;
        wxCommandEvent m_eventMediaFinised;

        wxDECLARE_EVENT_TABLE();
};