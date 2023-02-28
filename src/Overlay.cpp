// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <map>

#include "Tools.h"
#include "Overlay.h"

Overlay::Overlay( const wxSize& size )
 : m_font( 36, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL )
{
    m_bitmapOverlay = std::make_shared<wxBitmap>( size, 32 );

    m_bitmapPause = Tools::Instance().loadBitmapFromFile(
        wxT( "/../resources/images/Pause.png" )
    );

    for ( unsigned char count = 3; count > 0; --count )
        m_bitmapsCountDown.push_back(
            Tools::Instance().loadBitmapFromFile(
                wxT( "/../resources/images/Countdown/" + s_countdownMap.at( count ) )
            )
        );
}

void Overlay::showPause( wxDC* source, wxDC *dest )
{
    const wxSize& clientSize = m_bitmapOverlay->GetSize();

    m_overlayDC.SelectObject( *m_bitmapOverlay );
    m_overlayDC.Blit( 0, 0, clientSize.x, clientSize.y, source, 0, -clientSize.y );

    m_overlayDC.DrawBitmap( *m_bitmapPause, 0, 0 );

    dest->Blit( 0, 0, clientSize.x, clientSize.y, &m_overlayDC, 0, 0 );
    m_overlayDC.SelectObject( wxNullBitmap );
}

void Overlay::showCountDown( wxDC* source, wxDC* dest, unsigned char count )
{
    const wxSize& clientSize = m_bitmapOverlay->GetSize();

    m_overlayDC.SelectObject( *m_bitmapOverlay );
    m_overlayDC.Blit( 0, 0, clientSize.x, clientSize.y, source, 0, -clientSize.y );

    m_overlayDC.DrawBitmap( *m_bitmapsCountDown[ count ], 0, 0 );

    dest->Blit( 0, 0, clientSize.x, clientSize.y, &m_overlayDC, 0, 0 );
    m_overlayDC.SelectObject( wxNullBitmap );
}
