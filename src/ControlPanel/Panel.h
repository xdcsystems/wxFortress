#pragma once

#define INITIAL_LIVES_VALUE 9

namespace ControlPanel
{
    // Forward declarations
    template <typename T>
    class Counter;

    class Panel final : public wxWindow
    {
        private:
            using bitmapPtr = std::shared_ptr<wxBitmap>;
            using memoryDCPtr = std::shared_ptr<wxMemoryDC>;
            
            template <typename T>
            using counterPtr = std::shared_ptr<Counter <T>>;

        public:
            Panel(
                wxWindow* parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxBORDER_NONE,
                const wxString& name = wxASCII_STR( wxPanelNameStr ) );

            void paintLaunched();
            void activate();
            void reset();
            void increaseScore();
            unsigned char decreaseLives();
            unsigned short increaseLevel();
            unsigned short currentLevel() const;

        private:
             // Event Handlers
            void OnPaint( wxPaintEvent& event );

            // Helper functions
            void init();
            void render( wxDC& dc, const bitmapPtr panel );

            template< typename T>
            void refreshDisplayValue( counterPtr<T> counter );

        private:
            // Private data
            memoryDCPtr m_mdc;
            memoryDCPtr m_numbersDC;

            bitmapPtr m_bitmapPanelBuffer;
            bitmapPtr m_bitmapPanelLaunchedBuffer;
            bitmapPtr m_bitmapControlBgBuffer;
            bitmapPtr m_numbers;

            counterPtr<unsigned long> m_hiScore;
            counterPtr<unsigned long> m_score;
            counterPtr<unsigned char> m_lives;
            counterPtr<unsigned short> m_level;

            DECLARE_EVENT_TABLE()
    };
}