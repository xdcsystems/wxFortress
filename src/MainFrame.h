#pragma once

// Forward declarations
class Timer;
class RenderWindow;
class MediaManager;

namespace ControlPanel
{
    class Panel;
}

class MainFrame final : public wxFrame
{
    public:
        MainFrame() = default;

        MainFrame(
            wxWindow* parent,
            int id = wxID_ANY,
            const wxString &title = wxTheApp->GetAppName(),
            wxPoint pos = wxDefaultPosition,
            wxSize size = { 1024, 768 },
            int style = ( wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL | wxCLIP_CHILDREN ) & ~( wxRESIZE_BORDER | wxMAXIMIZE_BOX ) );

        bool Create(
            wxWindow* parent,
            int id = wxID_ANY,
            const wxString &title = wxTheApp->GetAppName(),
            wxPoint pos = wxDefaultPosition,
            wxSize size = { 1024, 768 },
            int style = ( wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL | wxCLIP_CHILDREN ) & ~( wxRESIZE_BORDER | wxMAXIMIZE_BOX ),
            const wxString& name = wxASCII_STR( wxFrameNameStr ) );

        bool isRunning() const { return m_isRunning; };
        void start();
        void stop();

    private:
        // Event Handlers
        void onClose( wxCloseEvent& );
        void onLaunchPressed( wxCommandEvent& );
        void onRoundStarted( wxCommandEvent& );
        void onRoundCompleted( wxCommandEvent& );
        void onBallLost( wxCommandEvent& );
        void onReset( wxCommandEvent& );
        void onScoreIncreased( wxCommandEvent& );
        void onVideoFinished( wxCommandEvent& );

        // Helper functions
        void init();

        // Private data
        std::shared_ptr<RenderWindow> m_renderSurface;
        std::shared_ptr<ControlPanel::Panel> m_controlPanel;
        std::shared_ptr <MediaManager> m_mediaManager;

#if defined( wxUSE_LOGWINDOW ) && defined( USE_LOGGER )
        wxLogWindow* m_logWindow = nullptr;
#endif // wxUSE_LOGWINDOW

        bool m_isRunning = false;

        wxDECLARE_EVENT_TABLE();
};
