#pragma once

#include <memory>

#include "Counter.hpp"

DECLARE_LOCAL_EVENT_TYPE( wxEVT_LAUNCH_CLICKED, -1 )

namespace ControlPanel
{
    class Panel final : public wxWindow
    {
        private:
            using bitmapPtr = std::shared_ptr<wxBitmap>;
            using memoryDCPtr = std::shared_ptr<wxMemoryDC>;

        public:
            Panel(
                wxWindow* parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxBORDER_NONE,
                const wxString& name = wxASCII_STR( wxPanelNameStr ) );

            bool AcceptsFocus() const override { return false; }
            bool AcceptsFocusFromKeyboard() const override { return false; }

            void paintLaunched();
            void activate();
            void reset();
            void increaseScore();
            unsigned char decreaseLives();
            unsigned short increaseLevel();
            unsigned short currentLevel() const;

        private:
            // Event Handlers
            void onPaint( wxPaintEvent& );
            void onLeftDown( wxMouseEvent& );
            void onMouseMove( wxMouseEvent& );
            bool isPointInLaunchButton( const wxPoint& pt ) const;
            bool m_hoverLaunch { false };
            bool m_isLaunched { false };

            // Helper functions
            void init();
            void render( wxDC& dc, const bitmapPtr& panel );
            void refreshDisplayValue( const Counter& counter );

            // Private data
            memoryDCPtr m_mdc;
            memoryDCPtr m_numbersDC;

            bitmapPtr m_bitmapPanelBuffer;
            bitmapPtr m_bitmapPanelLaunchedBuffer;
            bitmapPtr m_bitmapControlBgBuffer;
            bitmapPtr m_numbers;

            Counter m_hiScore { 67, 181 };
            Counter m_score { 67, 267 };
            Counter m_lives { 67, 353 };
            Counter m_level { 67, 439 };

            wxDECLARE_EVENT_TABLE();
    };
}
