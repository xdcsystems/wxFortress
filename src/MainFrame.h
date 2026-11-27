#pragma once

#include <memory>
#include "Common/defs.h"
#if defined( wxUSE_LOGWINDOW ) && wxUSE_LOGWINDOW && defined( USE_LOGGER ) && USE_LOGGER == 1
    #include <wx/log.h>
#endif

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
            wxSize size = { 1008, 729 }, // { 1024, 768 },
            int style = ( wxDEFAULT_FRAME_STYLE | wxCLIP_CHILDREN ) & ~( wxRESIZE_BORDER | wxMAXIMIZE_BOX ) );

        bool Create(
            wxWindow* parent,
            int id = wxID_ANY,
            const wxString &title = wxTheApp->GetAppName(),
            wxPoint pos = wxDefaultPosition,
            wxSize size = { 1008, 729 }, // { 1024, 768 },
            int style = ( wxDEFAULT_FRAME_STYLE | wxCLIP_CHILDREN ) & ~( wxRESIZE_BORDER | wxMAXIMIZE_BOX ),
            const wxString& name = wxASCII_STR( wxFrameNameStr ) );

        bool isRunning() const { return m_isRunning; };
        void start();
        void stop();

    private:
        // Event Handlers
        void onClose( wxCloseEvent& );
        void onActivate( wxActivateEvent& );
        void onChildWndInitialized( wxCommandEvent& );
        void onLaunchPressed( wxCommandEvent& );
        void onLaunchClicked( wxCommandEvent& );
        void onRoundStarted( wxCommandEvent& );
        void onRoundCompleted( wxCommandEvent& );
        void onBallLost( wxCommandEvent& );
        void onReset( wxCommandEvent& );
        void onScoreIncreased( wxCommandEvent& );
        void onVideoStarted( wxCommandEvent& );
        void onVideoFinished( wxCommandEvent& );

        // Helper functions
        void init();
        void resetToFirstLevel();

        // Private data (non-owning pointers; wxWindow parent owns child windows)
        RenderWindow* m_renderSurface { nullptr };
        ControlPanel::Panel* m_controlPanel { nullptr };
#if defined( wxUSE_LOGWINDOW ) && wxUSE_LOGWINDOW && defined( USE_LOGGER ) && USE_LOGGER == 1
        std::unique_ptr<wxLogWindow> m_logWindow;
#endif
        
        bool m_isRunning { false };

        wxDECLARE_EVENT_TABLE();
};
