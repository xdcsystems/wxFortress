#pragma once

class Overlay
{
        static inline const std::map<unsigned char, const std::wstring> s_countdownMap =
        {
            { 3, wxT( "3.png" ) },
            { 2, wxT( "2.png" ) },
            { 1, wxT( "1.png" ) },
        };

    public:
        Overlay( const wxSize& size );

        void showPause( wxDC* source, wxDC* dest );
        void showCountDown( wxDC* source, wxDC* dest, unsigned char count );

    private:
        using bitmapPtr = std::shared_ptr<wxBitmap>;
        
        bitmapPtr m_bitmapOverlay;
        bitmapPtr m_bitmapPause;

        std::vector<bitmapPtr> m_bitmapsCountDown;
        
        wxMemoryDC m_overlayDC;
        const wxFont m_font;
};