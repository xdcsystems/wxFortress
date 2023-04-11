#pragma once

#include <memory> 
#include <wx/glcanvas.h>


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

        void playIntro();
        void loadLevel( unsigned short level );
        void start();
        void stop();

    private:
         // Event Handlers
        void onPaint( wxPaintEvent& );
        void onSize( wxSizeEvent& );
        void onHelp( wxHelpEvent& );
        void onKeyPressed( wxKeyEvent& );
        void onScoreIncreased( wxCommandEvent& );
        void onPaddleContact( wxCommandEvent& );
        void onRoundCompleted( wxCommandEvent& );
        void onBallLost( wxCommandEvent& );
        void onTextCharShow( wxCommandEvent& );
        void onStageFinished( wxCommandEvent& );
        void onIdle( wxIdleEvent& );

        // Helper functions
        void init();
        void resize( const wxSize& size );
        void switchRun();
        void clearScreen();
        void render();

        void initializeGLEW();
        void setupGraphics();

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
        
        bool m_isRunning = false;
        unsigned char m_countDown { 0 };

        State m_state = State::PLAY;
        State m_prevState = State::NEWROUND;

        wxDECLARE_EVENT_TABLE();
};
