// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <map>
#include <random>
#include <memory>

#include "Common/defs.h"
#include "Common/Rect.hpp"
#include "Base.h"
#include "MoveController.h"


using namespace Shapes;

void MoveController::initDirection()
{
    std::random_device dev;
    std::mt19937 rng( dev() );
    std::uniform_int_distribution<std::mt19937::result_type> dist160( BEGIN_ANGLE, END_ANGLE ); // distribution in range [30, 150]

    do {
        m_angle = dist160( rng );
    } while ( m_angle > END_RIGHT_ANGLE && m_angle < BEGIN_LEFT_ANGLE ); // > 70 && < 110

    m_moveDirection = m_angle <= 90 ? MoveDirection::TopRight : MoveDirection::TopLeft;
}

template <>
void MoveController::changeDirection<MoveController::MoveDirection::TopRight>( ContactPosition contactPosition )
{
    if ( contactPosition == ContactPosition::Right )
    {
        m_angle += ( 90 - m_angle ) * 2;
        m_moveDirection = MoveDirection::TopLeft;
        return;
    }
    m_angle *= -1;
    m_moveDirection = MoveDirection::RightDown;
}

template <>
void MoveController::changeDirection<MoveController::MoveDirection::TopLeft>( ContactPosition contactPosition )
{
    if ( contactPosition == ContactPosition::Left )
    {
        m_angle = 90 - ( m_angle - 90 );
        m_moveDirection = MoveDirection::TopRight;
        return;
    }
    m_angle *= -1;
    m_moveDirection = MoveDirection::LeftDown;
}

template <>
void MoveController::changeDirection<MoveController::MoveDirection::RightDown>( ContactPosition contactPosition )
{
    if ( contactPosition == ContactPosition::Right )
    {
        m_angle -= ( 90 - fabs( m_angle ) ) * 2;
        m_moveDirection = MoveDirection::LeftDown;
        return;
    }
    m_angle *= -1;
    m_moveDirection = MoveDirection::TopRight;
}

template <>
void MoveController::changeDirection<MoveController::MoveDirection::LeftDown>( ContactPosition contactPosition )
{
    if ( contactPosition == ContactPosition::Left )
    {
        m_angle -= ( 90 + m_angle ) * 2;
        m_moveDirection = MoveDirection::RightDown;
        return;
    }
    m_angle *= -1;
    m_moveDirection = MoveDirection::TopLeft;
}

// Paddle contact
void MoveController::changeDirection( const glm::vec2 &ballCenter, const glm::vec2 &paddleCenter )
{
    m_angle *= -1;
    m_angle += m_scaleDivisionValue * ( paddleCenter.x - ballCenter.x );

    switch ( m_moveDirection )
    {
        case MoveDirection::RightDown:
            if ( m_angle > END_RIGHT_ANGLE )
            {
                m_angle = END_RIGHT_ANGLE;
            }
            else if ( m_angle < BEGIN_ANGLE )
            {
                m_angle = BEGIN_ANGLE;
            }
            m_moveDirection = MoveDirection::TopRight;
        break;
        
        case MoveDirection::LeftDown:
            if ( m_angle > END_ANGLE )
            {
                m_angle = END_ANGLE;
            }
            else if ( m_angle < BEGIN_LEFT_ANGLE )
            {
                m_angle = BEGIN_LEFT_ANGLE;
            }
            m_moveDirection = MoveDirection::TopLeft;
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
        { MoveDirection::TopRight, &MoveController::changeDirection<MoveDirection::TopRight> },
        { MoveDirection::TopLeft, &MoveController::changeDirection<MoveDirection::TopLeft> },
        { MoveDirection::RightDown, &MoveController::changeDirection<MoveDirection::RightDown> },
        { MoveDirection::LeftDown, &MoveController::changeDirection<MoveDirection::LeftDown> },
    };

    ( this->*( s_handlers[ m_moveDirection ] ) )( contactPosition );
}