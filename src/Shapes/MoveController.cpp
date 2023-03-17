// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "Common/xRect.hpp"
#include "Base.h"
#include "Ball.h"
#include "MoveController.h"


using namespace Shapes;
using enum MoveController::MoveDirection;

template <>
void MoveController::checkDirection<DirectionTopRight>( ContactPosition contactPosition )
{
    if ( ( contactPosition == Ball::ContactRight ) ||
        ( contactPosition == Ball::ContactLeft ) )
    {
        m_angle += ( 90 - m_angle ) * 2;
        m_moveDirection = DirectionTopLeft;
    }
    else if ( ( contactPosition == Ball::ContactBottom ) ||
        ( contactPosition == Ball::ContactTop ) )
    {
        m_angle *= -1;
        m_moveDirection = DirectionRightDown;
    }
}

template <>
void MoveController::checkDirection<DirectionTopLeft>( ContactPosition contactPosition )
{
    if ( ( contactPosition == Ball::ContactLeft ) ||
        ( contactPosition == Ball::ContactRight ) )
    {
        m_angle = 90 - ( m_angle - 90 );
        m_moveDirection = DirectionTopRight;
    }
    else if ( ( contactPosition == Ball::ContactBottom ) ||
        ( contactPosition == Ball::ContactTop ) )
    {
        m_angle += ( 180 - m_angle ) * 2;
        m_moveDirection = DirectionLeftDown;
    }
}

template <>
void MoveController::checkDirection<DirectionRightDown>( ContactPosition contactPosition )
{
    if ( ( contactPosition == Ball::ContactRight ) ||
        ( contactPosition == Ball::ContactLeft ) ||
        ( contactPosition == Ball::ContactBottom ) )
    {
        m_angle -= ( 90 - fabs( m_angle ) ) * 2;
        m_moveDirection = DirectionLeftDown;
    }
    else if ( contactPosition == Ball::ContactTop )
    {
        m_angle *= -1;
        m_moveDirection = DirectionTopRight;
    }
}

template <>
void MoveController::checkDirection<DirectionLeftDown>( ContactPosition contactPosition )
{
    if ( ( contactPosition == Ball::ContactBottom ) ||
        ( contactPosition == Ball::ContactTop ) )
    {
        m_angle *= -1;
        m_moveDirection = DirectionTopLeft;
    }
    else if ( ( contactPosition == Ball::ContactLeft ) ||
        ( contactPosition == Ball::ContactRight ) )
    {
        m_angle -= ( 90 + m_angle ) * 2;
        m_moveDirection = DirectionRightDown;
    }
}
