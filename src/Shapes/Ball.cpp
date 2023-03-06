// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "Common/Tools.h"
#include "Renderer/ResourceManager.h"
#include "Base.h"
#include "Ball.h"

using namespace Shapes;

Ball::Ball()
    : Base()
{
    load( ResourceManager::LoadTexture( "/../resources/images/ball.png", true, "simple_ball" ) );

    m_radius = m_size.x / 2.0f;
    m_radiusSquared = m_radius * m_radius;

    m_velocity = std::move( glm::vec2( 10.f, 10.f ) );
}

ContactPosition Ball::intersect( const wxRect2DDouble& rect ) const
{
    const double cx = m_position.x + m_radius,  // centerX
        cy = m_position.y + m_radius; // centerY

    double x = cx,
        y = cy;

    if ( cx < rect.GetLeft() )
        x = rect.GetLeft();
    else if ( cx > rect.GetRight() )
        x = rect.GetRight();

    if ( cy < rect.GetTop() )
        y = rect.GetTop();
    else if ( cy > rect.GetBottom() )
        y = rect.GetBottom();

    if ( ( cx - x ) * ( cx - x ) + ( cy - y ) * ( cy - y ) <= m_radiusSquared )
    {
        if ( round( m_position.x ) >= rect.GetRight() )
            return ContactPosition::ContactRight;
        else if ( round( m_position.x ) + m_size.x - 1 <= rect.GetLeft() )
            return ContactPosition::ContactLeft;
        else if ( round( m_position.y ) + m_size.y - 1 <= rect.GetTop() )
            return ContactPosition::ContactBottom;
        else if ( round( m_position.y ) >= rect.GetBottom() )
            return ContactPosition::ContactTop;
        else // corners
            return cy < rect.GetCentre().m_y ? ContactPosition::ContactBottom
            : ContactPosition::ContactTop;
    }

    return ContactPosition::ContactNull;
}
