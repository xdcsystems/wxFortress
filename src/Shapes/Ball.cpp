// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "../Tools.h"
#include "Base.h"
#include "Ball.h"

using namespace Shapes;

Ball::Ball()
 : Base()
{
    load( wxT( "/../resources/images/ball.png" ) );
    m_bitmapClear = Tools::Instance().loadBitmapFromFile( wxT( "/../resources/images/ball-clear.png" ) );

    m_radius = m_size.GetWidth() / 2;
    m_radiusSquared = m_radius * m_radius;
}

void Ball::clear( wxDC& dc ) const
{
    dc.DrawBitmap( *m_bitmapClear, m_position );
}

ContactPosition Ball::intersect( const wxRect& rect ) const
{
    const int cx = m_position.x + m_radius,  // centerX
                 cy = m_position.y + m_radius; // centerY
    
    int x = cx,
        y = cy;

    if ( cx < rect.x )
        x = rect.x;
    else if ( cx > rect.GetRight() )
        x = rect.GetRight();

    if ( cy < rect.y )
        y = rect.y;
    else if ( cy > rect.GetBottom() )
        y = rect.GetBottom();

    if ( ( cx - x ) * ( cx - x ) + ( cy - y ) * ( cy - y ) <= m_radiusSquared )
    {
        if ( m_position.x >= rect.GetRight()  )
            return ContactPosition::ContactRight;
        else if ( m_position.x + m_size.GetWidth() - 1 <= rect.GetLeft() )
            return ContactPosition::ContactLeft;
        else if ( m_position.y + m_size.GetHeight() - 1 <= rect.y )
            return ContactPosition::ContactBottom;
        else if ( m_position.y >= rect.GetBottom() )
            return ContactPosition::ContactTop;
        else // corners
            return cy < rect.x ? ContactPosition::ContactBottom
                                     : ContactPosition::ContactTop;
    }
 
    return ContactPosition::ContactNull;
}
