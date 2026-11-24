#pragma once

#include <chrono>
#include <memory> 
#include <wx/glcanvas.h>

#include "Common/FramePacer.hpp"


// Forward declarations
class Overlay;
class MediaManager;
class SoundManager;
class SpriteRenderer;
class TextRenderer;
class Timer;

namespace Shapes
{
    class ShapesManager;
}

class RenderWindow final : public wxGLCanvas
{
    public:
        enum class State : unsigned char
        {
            HELP,
            PLAY,
            RUN,
            PAUSE,
            LAUNCHING,
            COUNTDOWN,
            NEWROUND,
            FINISHED,
        };

        RenderWindow(
            wxWindow* parent,
            wxWindowID id = wxID_ANY,
            const int* attribList = nullptr,
            const wxSize& size = wxDefaultSize,
            long style = wxBORDER_NONE,
            const wxString& name = wxASCII_STR( wxPanelNameStr ),
            const wxPalette& palette = wxNullPalette );

        ~RenderWindow();

        RenderWindow( const RenderWindow& ) = delete;
        RenderWindow( RenderWindow&& ) = delete;
        RenderWindow& operator= ( const RenderWindow& ) = delete;
        RenderWindow& operator= ( RenderWindow&& ) = delete;

        void init();
        void playIntro();
        void loadLevel( unsigned short level );
        void resetBallSpeed();
        void start();
        void stop();
        void triggerLaunch();

    private:
         // Event Handlers
        void onPaint( wxPaintEvent& );
        void onSize( wxSizeEvent& );
        void onKeyPressed( wxKeyEvent& );
        void onMouseMove( wxMouseEvent& );
        void onWndInitialized( wxCommandEvent& );
        void onScoreIncreased( wxCommandEvent& );
        void onPaddleContact( wxCommandEvent& );
        void onRoundCompleted( wxCommandEvent& );
        void onBallLost( wxCommandEvent& );
        void onTextCharShow( wxCommandEvent& );
        void onStageFinished( wxCommandEvent& );
        void onIdle( wxIdleEvent& );

        // Helper functions
        void resize( const wxSize& size );
        void switchRun();
        bool render();
        void initializeGLEW();
        void setupGraphics();
        void ensureContextCurrent();
        void configureSwapAndPacing();
        bool waitForVBlank();

        // Private data
        std::unique_ptr<wxGLContext> m_context;
        std::shared_ptr<SpriteRenderer> m_spriteRenderer;
        std::shared_ptr<Overlay> m_overlay;
        std::shared_ptr <TextRenderer> m_textRenderer;

        std::shared_ptr <MediaManager> m_mediaManager;
        std::shared_ptr<Shapes::ShapesManager> m_shapesManager;
        std::shared_ptr <SoundManager> m_soundManager;

        std::shared_ptr<Timer> m_timer;

        double m_elapsedTime { 0 };
        FramePacing::State m_framePacer {};

        bool m_isInitialized { false };
        bool m_isRunning { false };
        bool m_useDwmPacing { false };
        std::chrono::steady_clock::time_point m_lastLaunchTime {};
        unsigned char m_countDown { 0 };
        unsigned short m_currentLevel { 1 };

        State m_state { State::PLAY };
        State m_prevState { State::NEWROUND };

        wxDECLARE_EVENT_TABLE();
};
