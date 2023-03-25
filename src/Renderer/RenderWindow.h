#pragma once

#include <memory> 
#include "wx/glcanvas.h" 

DECLARE_LOCAL_EVENT_TYPE( wxEVT_LAUNCH_PRESSED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_NEW_ROUND_STARTED, wxID_ANY )

// Forward declarations
class Overlay;
class SoundManager;
class SpriteRenderer;
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
            RUN,
            PAUSE,
            COUNTDOWN,
            NEWROUND,
        };

        RenderWindow(
            wxWindow* parent,
            wxWindowID id = wxID_ANY,
            const int* attribList = nullptr,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxBORDER_NONE,
            const wxString& name = wxASCII_STR( wxPanelNameStr ),
            const wxPalette& palette = wxNullPalette );

        ~RenderWindow();

        RenderWindow( const RenderWindow& ) = delete;
        RenderWindow( RenderWindow&& ) = delete;
        RenderWindow& operator= ( const RenderWindow& ) = delete;
        RenderWindow& operator= ( RenderWindow&& ) = delete;

        void stop();
        void start();
        void loadLevel( unsigned short level );

    private:
         // Event Handlers
        void onPaint( wxPaintEvent& );
        void onSize( wxSizeEvent& );
        void onKeyPressed( wxKeyEvent& );
        void onScoreIncreased( wxCommandEvent& );
        void onPaddleContact( wxCommandEvent& );
        void onRoundCompleted( wxCommandEvent& );
        void onBallLost( wxCommandEvent& );
        void onIdle( wxIdleEvent& );

        // Helper functions
        void init();
        void resize( const wxSize& size );
        void switchRun();
        void render();

        void initializeGLEW();
        void setupGraphics();

        // Private data
        std::unique_ptr<wxGLContext> m_context;
        std::shared_ptr<SpriteRenderer> m_spriteRenderer;
        std::shared_ptr<Overlay> m_overlay;

        std::shared_ptr<Shapes::ShapesManager> m_shapesManager;
        std::shared_ptr <SoundManager> m_soundManager;

        std::shared_ptr<Timer> m_timer;

        double m_elapsedTime = 0;
        
        bool m_isRunning = false;
        unsigned char m_countDown = 0;

        State m_state = State::NEWROUND;

        wxDECLARE_EVENT_TABLE();
};
