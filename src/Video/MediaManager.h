#pragma once

#include "Common/NonCopyable.hpp"

#include "IntroSkipHold.hpp"

// Forward declarations
class wxGLCanvas;
class TextRenderer;
class VideoRenderer;
class Movie;
struct AVFrame;

class MediaManager final : private NonCopyable
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
        void onSkipKeyDown();
        void onSkipKeyUp();
        void cancelSkipHold();
        bool needRefresh();
        void renderFrame();

   private:
        // Helper functions
        void showSkipMessage();
        void open( std::string filename );
        void close();

        // Private data
        static const int64_t s_timerInterval { 15000000000 }; // 15 sec

        textRedererPtr m_textRenderer;
        videoRedererPtr m_videoRenderer;
        moviePtr m_movie;

        int64_t m_duration { -1 };

        wxGLCanvas* m_canvas { nullptr };

        wxEvtHandler* m_eventHandler { nullptr };
        wxCommandEvent m_eventMediaPlay;
        wxCommandEvent m_eventMediaFinished;

        bool m_isPlaying { false };
        bool m_skipOverlayDirty { false };
        int m_logicalWidth { 0 };
        double m_lastOverlayRefreshSec { -1.0 };
        IntroSkipHold::State m_skipHold {};
        std::pair<AVFrame*, int64_t> m_currentFrame { nullptr, 0 };
};
