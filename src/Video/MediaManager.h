#pragma once

// Forward eclarations
class wxMediaCtrl;
class wxMediaEvent;

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
        void onTimer( wxTimerEvent& );
        void onCheckEnd( wxTimerEvent& );
        void onKeyPressed( wxKeyEvent& );
        void onMediaLoaded( wxMediaEvent& );
        void onMediaFinished( wxMediaEvent& );
        void onMediaPlayStarted( wxMediaEvent& );

        // Helper functions
        void createMediaControl();
        void showSkipMessage( bool show = true );
        
        // Private data
        static const int s_timerInterval = 15000; // 15 sec
        static const int s_timerCheckEndInterval = 1000; // 1 sec

        wxTimer   m_timer;
        wxTimer   m_timerCheckEnd;

        const wxFont m_font;

        wxMediaCtrl* m_mediaControl = nullptr;
        bool m_isOK = false;

        wxDECLARE_EVENT_TABLE();
};