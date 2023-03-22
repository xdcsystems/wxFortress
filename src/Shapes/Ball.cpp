// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "Common/defs.h"
#include "Common/Tools.h"
#include "Common/Rect.hpp"
#include "Renderer/ResourceManager.h"
#include "Base.h"
#include "Ball.h"

using namespace Shapes;

Ball::Ball()
{
    load( ResourceManager::LoadTexture( "/../resources/images/ball.png", true, "simple_ball" ) );

    m_radius = m_size.x / 2.0f;
    m_radiusSquared = m_radius * m_radius;

    m_velocity = { BEGIN_BALL_VELOCITY, BEGIN_BALL_VELOCITY };
}

ContactPosition Ball::intersect( const Rect& rect ) const
{
    const glm::vec2 cBall( m_position.x + m_radius, m_position.y + m_radius ); // center Ball

    float x = cBall.x;
    float y = cBall.y;

    if ( cBall.x < rect.left() )
    {
        x = rect.left();
    }
    else if ( cBall.x > rect.right() )
    {
        x = rect.right();
    }

    if ( cBall.y < rect.top() )
    {
        y = rect.top();
    }
    else if ( cBall.y > rect.bottom() )
    {
        y = rect.bottom();
    }

    if ( ( cBall.x - x ) * ( cBall.x - x ) + ( cBall.y - y ) * ( cBall.y - y ) <= m_radiusSquared )
    {
        // has collision 
        const glm::vec2 points[] = {
            { rect.m_x,    rect.m_y },        // points[ 0 ] : A = left bottom corner
            { rect.m_x,    rect.bottom() },  // points[ 1 ] : E = left top corner
            { rect.right(), rect.bottom() },  // points[ 2 ] : B = right top corner
            { rect.right(), rect.m_y },        // points[ 3 ] : F = right bottom corner
        };

        const auto d2 = ( cBall.x - points[ 0 ].x ) * ( points[ 2 ].y - points[ 0 ].y ) - ( cBall.y - points[ 0 ].y ) * ( points[ 2 ].x - points[ 0 ].x );
        const auto d1 = ( cBall.x - points[ 3 ].x ) * ( points[ 1 ].y - points[ 3 ].y ) - ( cBall.y - points[ 3 ].y ) * ( points[ 1 ].x - points[ 3 ].x );

        if ( d1 < 0 && d2 > 0 )
        {
            return ContactPosition::ContactBottom;
        }
        
        if ( d1 < 0 && d2 < 0 )
        {
            return ContactPosition::ContactRight;
        }

        if ( d1 > 0 && d2 < 0 )
        {
            return ContactPosition::ContactTop;
        }

        if ( d1 > 0 && d2 > 0 )
        {
            return ContactPosition::ContactLeft;
        }
    }

    return ContactPosition::ContactNull;
}
