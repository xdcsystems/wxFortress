// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
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
    load( ResourceManager::LoadTexture(
        "resources/images/ball.png",
        "simple_ball" ) );

    m_radius = m_size.x / 2.0f;
    m_radiusSquared = m_radius * m_radius;

    m_velocity = { BEGIN_BALL_VELOCITY, BEGIN_BALL_VELOCITY };
}

ContactPosition Ball::intersect( const Rect &rect ) const
{
    // get center point Ball first
    const glm::vec2 cBall( m_position + m_radius );
    
    // calculate AABB info (center, half-extents)
    const glm::vec2 halfExtentsAABB = rect.halfExtents();
    const glm::vec2 cAABB = rect.centre();

    // get difference vector between both centers
    const glm::vec2 difference = cBall - cAABB;
    const glm::vec2 clamped = glm::clamp( difference, -halfExtentsAABB, halfExtentsAABB );
    
    // add clamped value to AABB_center and we get the value of box closest to circle
    const glm::vec2 closest = cAABB + clamped;
    
    // retrieve vector between center circle and closest point AABB and check if length <= radius
    if ( glm::length( closest - cBall ) <= m_radius )
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
            return ContactPosition::Bottom;
        
        if ( d1 < 0 && d2 < 0 )
            return ContactPosition::Right;

        if ( d1 > 0 && d2 < 0 )
            return ContactPosition::Top;

        if ( d1 > 0 && d2 > 0 )
            return ContactPosition::Left;
    }

    return ContactPosition::Null;
}
