#pragma once

// Forward eclarations
class wxMediaCtrl;
class wxMediaEvent;

#define PLAYER_CONTROLS_PANEL_HEIGHT 20

DECLARE_LOCAL_EVENT_TYPE( wxEVT_VIDEO_FINISHED, wxID_ANY )

class MediaManager final : public wxWindow
{
    public:
        MediaManager(
            wxWindow* parent,
            wxWindowID id,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxBORDER_NONE,
            const wxString& name = wxASCII_STR( wxPanelNameStr ) );
        
        ~MediaManager();

        void reset();
        void playIntro();

    private:
        // Event Handlers
        void OnTimer( wxTimerEvent& );
        void OnCheckEnd( wxTimerEvent& );
        void OnKeyPressed( wxKeyEvent& event );
        void OnMediaLoaded( wxMediaEvent& event );
        void OnMediaFinished( wxMediaEvent& event );
        void OnMediaPlayStarted( wxMediaEvent& event );

        // Helper functions
        void createMediaControl();
        void destroyMediaContol();
        void showSkipMessage( bool show = true );
        
    private:
        // Private data
        static const int s_timerInterval = 15000; // 15 sec
        static const int s_timerCheckEndInterval = 1000; // 1 sec

        wxTimer   m_timer;
        wxTimer   m_timerCheckEnd;

        const wxFont m_font;

        wxMediaCtrl* m_mediaControl = nullptr;
        bool m_isOK = false;

        DECLARE_EVENT_TABLE()
};