// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
    #include <wx/dc.h>
#endif

#include "../Tools.h"
#include "Base.h"

using namespace Shapes;

void Base::load( const std::wstring& filename )
{
    m_bitmap = Tools::Instance().loadBitmapFromFile( filename );
    m_size = m_bitmap->GetSize();
}

void Base::moveTo( int x, int y )
{
    m_position.x = x;
    m_position.y = y;
}

void Base::moveTo( const wxPoint& position )
{
    moveTo( position.x, position.y );
}

void Base::draw( wxDC& dc ) const
{
    if ( !m_bitmap )
        return;
    
    dc.DrawBitmap( *m_bitmap, m_position, true );
}

void Base::clear( wxDC& dc ) const
{
    dc.DrawRectangle( bounds() );
}
