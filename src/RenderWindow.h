#pragma once

DECLARE_LOCAL_EVENT_TYPE( wxEVT_LAUNCH_PRESSED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_NEW_ROUND_STARTED, wxID_ANY )

class Overlay;
class SoundManager;

class RenderWindow final : public wxWindow
{
    public:

        enum class State : unsigned char
        {
            RUN,
            PAUSE,
            COUNTDOWN,
            NEWROUND,
        };
        using enum State;

        RenderWindow(
            wxWindow* parent,
            wxWindowID id,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxBORDER_NONE,
            const wxString& name = wxASCII_STR( wxPanelNameStr ) );

        void stop() { m_isRunning = false; };
        void start() { m_isRunning = true; };
        void loadLevel( unsigned short level );
        void paintNow();

    private:
        static const int TIMER_INTERVAL; // milliseconds

         // Event Handlers
        void OnPaint( wxPaintEvent& );
        void OnSize( wxSizeEvent& );
        void OnKeyPressed( wxKeyEvent& );
        void OnScoreIncreased( wxCommandEvent& );
        void OnPaddleContact( wxCommandEvent& );
        void OnRoundCompleted( wxCommandEvent& );
        void OnBallLost( wxCommandEvent& );

        // Helper functions
        void init();
        void resize( const wxSize& size );
        void switchRun();
        void checkKeysState();
        void render( wxDC& dc );

    private:
        // Private data
        std::shared_ptr<Shapes::ShapesManager> m_shapesManager;
        std::shared_ptr <SoundManager> m_soundManager;

        std::shared_ptr<wxBitmap> m_bitmapBuffer;
        std::shared_ptr<wxMemoryDC> m_mdc;
        
        std::shared_ptr<Overlay> m_overlay;

        double m_accelerate = 0;
        bool m_isRunning = false;
        unsigned char m_countDown = 0;

        State m_state = NEWROUND;

        DECLARE_EVENT_TABLE()
};