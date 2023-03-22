// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <random>

#include "Common/defs.h"
#include "Common/Rect.hpp"
#include "Base.h"
#include "MoveController.h"


using namespace Shapes;
using enum MoveController::MoveDirection;
using enum ContactPosition;

void MoveController::initDirection()
{
    std::random_device dev;
    std::mt19937 rng( dev() );
    std::uniform_int_distribution<std::mt19937::result_type> dist160( BEGIN_ANGLE, END_ANGLE ); // distribution in range [30, 150]

    do {
        m_angle = dist160( rng );
    } while ( m_angle > END_RIGHT_ANGLE && m_angle < BEGIN_LEFT_ANGLE ); // > 70 && < 110

    m_moveDirection = m_angle <= 90 ? DirectionTopRight : DirectionTopLeft;
}

template <>
void MoveController::changeDirection<DirectionTopRight>( ContactPosition contactPosition )
{
    if ( contactPosition == ContactRight )
    {
        m_angle += ( 90 - m_angle ) * 2;
        m_moveDirection = DirectionTopLeft;
        return;
    }
    m_angle *= -1;
    m_moveDirection = DirectionRightDown;
}

template <>
void MoveController::changeDirection<DirectionTopLeft>( ContactPosition contactPosition )
{
    if ( contactPosition == ContactLeft )
    {
        m_angle = 90 - ( m_angle - 90 );
        m_moveDirection = DirectionTopRight;
        return;
    }
    m_angle *= -1;
    m_moveDirection = DirectionLeftDown;
}

template <>
void MoveController::changeDirection<DirectionRightDown>( ContactPosition contactPosition )
{
    if ( contactPosition == ContactRight )
    {
        m_angle -= ( 90 - fabs( m_angle ) ) * 2;
        m_moveDirection = DirectionLeftDown;
        return;
    }
    m_angle *= -1;
    m_moveDirection = DirectionTopRight;
}

template <>
void MoveController::changeDirection<DirectionLeftDown>( ContactPosition contactPosition )
{
    if ( contactPosition == ContactLeft ) 
    {
        m_angle -= ( 90 + m_angle ) * 2;
        m_moveDirection = DirectionRightDown;
        return;
    }
    m_angle *= -1;
    m_moveDirection = DirectionTopLeft;
}

// Paddle contact
void MoveController::changeDirection( const glm::vec2 &ballCenter, const glm::vec2 &paddleCenter )
{
    m_angle *= -1;
    m_angle += m_scaleDivisionValue * ( paddleCenter.x - ballCenter.x );

    switch ( m_moveDirection )
    {
        case DirectionRightDown:
            if ( m_angle > END_RIGHT_ANGLE )
                m_angle = END_RIGHT_ANGLE;
            else if ( m_angle < BEGIN_ANGLE )
                m_angle = BEGIN_ANGLE;

            m_moveDirection = DirectionTopRight;
        break;
        
        case DirectionLeftDown:
            if ( m_angle > END_ANGLE )
                m_angle = END_ANGLE;
            else if ( m_angle < BEGIN_LEFT_ANGLE )
                m_angle = BEGIN_LEFT_ANGLE;

            m_moveDirection = DirectionTopLeft;
        break;

        default:
            // wrong direction, error
            throw std::runtime_error( "Error: Wrong ball direction" );
        break;
    }
}

void MoveController::changeMoveDirection( ContactPosition contactPosition )
{
    static std::map<MoveDirection, void ( MoveController::* )( ContactPosition )> s_handlers = {
        { DirectionTopRight, &MoveController::changeDirection<DirectionTopRight> },
        { DirectionTopLeft, &MoveController::changeDirection<DirectionTopLeft> },
        { DirectionRightDown, &MoveController::changeDirection<DirectionRightDown> },
        { DirectionLeftDown, &MoveController::changeDirection<DirectionLeftDown> },
    };

    ( this->*( s_handlers[ m_moveDirection ] ) )( contactPosition );
}