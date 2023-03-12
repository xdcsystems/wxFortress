// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <random>

#include "Common/Tools.h"
#include "Base.h"
#include "Ball.h"
#include "Brick.h"
#include "Bricks.h"
#include "Board.h"
#include "ParticleGenerator.h"
#include "ShapesManager.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/SpriteRenderer.h"


DEFINE_LOCAL_EVENT_TYPE( wxEVT_CURRENT_SCORE_INCREASED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_ROUND_COMLETED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_BALL_LOST )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_PING )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_PONG )

using namespace Shapes;
using enum ShapesManager::MoveDirection;

ShapesManager::ShapesManager( wxWindow* parent )
    : m_parent( parent )
    , m_eventCurrentScoreInc( wxEVT_CURRENT_SCORE_INCREASED )
    , m_eventRoundCompleted( wxEVT_ROUND_COMLETED )
    , m_eventPing( wxEVT_PING )
    , m_eventPong( wxEVT_PONG )
    , m_eventBallLost( wxEVT_BALL_LOST )
{
    m_ball = std::make_shared<Ball>();
    m_board = std::make_shared<Board>();
    m_bricks = std::make_shared<Bricks>();

    // set render-specific controls
    m_particles = std::make_shared<ParticleGenerator>( 500 );
}

bool ShapesManager::switchRun( bool bNewRound )
{ 
    m_bRun = !m_bRun; 
    
    if ( m_bRun && bNewRound )
    {
        std::random_device dev;
        std::mt19937 rng( dev() );
        std::uniform_int_distribution<std::mt19937::result_type> dist160( 30, 150 ); // distribution in range [30, 150]
        
        do {
            m_angle = dist160( rng );
        } while ( m_angle > 70 && m_angle < 110 );
        m_moveDirection = m_angle <= 90 ? DirectionTopRight : DirectionTopLeft;

        calculateTrajectory();
    }

    return m_bRun;
};

wxRect2DDouble ShapesManager::updateBallPosition( const wxRect2DDouble &boardBounds ) const
{
    if ( m_size.x < 1 || m_size.y < 1 || !m_trajectory.empty() )
        return {};

    // set ball on board
    const auto &ballBounds = m_ball->bounds();
    
    m_ball->moveTo( 
        boardBounds.m_x + ( boardBounds.m_width - ballBounds.m_width ) / 2,
        boardBounds.m_y + boardBounds.m_height );

    return m_ball->bounds();
}

void ShapesManager::loadLevel( unsigned short level )
{
    // let's throw some bricks
    m_bricks->loadLevel( level );
}

void ShapesManager::resize( const wxSize& size )
{
    m_size = size;
    m_diagonal = sqrt( pow( m_size.x, 2 ) + pow( m_size.y, 2 ) );
    m_trajectory.reserve( m_diagonal );

    // set board position 
    auto boardBounds = std::move( m_board->bounds() );
    m_board->moveTo( ( m_size.x - boardBounds.m_width ) / 2, 15 );

    // set ball on board
    boardBounds = std::move( m_board->bounds() );
    const auto &ballBounds = updateBallPosition( boardBounds );

    m_ballTopLimit = m_size.y - ballBounds.m_height;
    m_ballBottomLimit = boardBounds.m_y + boardBounds.m_height;

    m_eventHandler = m_parent->GetEventHandler();
    m_eventCurrentScoreInc.SetEventObject( this );
    m_eventPing.SetEventObject( this );
    m_eventPong.SetEventObject( this );
}

void ShapesManager::calculateTrajectory()
{
    const auto &ballBounds = m_ball->bounds();
    const auto radAngle = glm::radians( m_angle );

    const wxPoint2DDouble delta( m_diagonal * cos( radAngle ), m_diagonal * sin( radAngle ));

    m_trajectory.clear();
    
    const double Rab = delta.GetVectorLength();
    for ( double Rac = 1; Rac <= Rab; ++Rac )
    {
        const auto k = Rac / Rab;
        const auto Xc = ballBounds.m_x + delta.m_x * k;
        const auto Yc = ballBounds.m_y + delta.m_y * k;

        m_trajectory.push_back( { Xc, Yc } );

        if ( Xc < 0 || Yc < 0 || Xc > m_size.x || Yc > m_size.y )
            break;
    }

    if ( m_isRobot && ( m_moveDirection == DirectionLeftDown || m_moveDirection == DirectionRightDown ) )
    {
        const auto &boardBounds = m_board->bounds();
        m_currentTrajPoint = std::find_if( m_trajectory.begin(), m_trajectory.end(), [ &boardBounds ]( auto& point ) {
            return point.m_y <= boardBounds.GetBottom();
        } );

        if ( m_currentTrajPoint != m_trajectory.end() )
            m_boardMove = m_currentTrajPoint->m_x + ( ballBounds.m_width - boardBounds.m_width ) / 2 - boardBounds.m_x;
    }

    m_currentTrajPoint = m_trajectory.begin();
}

void ShapesManager::stop()
{
    m_bRun = false;
    m_trajectory.clear();
}

template <>
void ShapesManager::checkDirection<DirectionTopRight>( ContactPosition contactPosition )
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
void ShapesManager::checkDirection<DirectionTopLeft>( ContactPosition contactPosition )
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
void ShapesManager::checkDirection<DirectionRightDown>( ContactPosition contactPosition )
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
void ShapesManager::checkDirection<DirectionLeftDown>( ContactPosition contactPosition )
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

void ShapesManager::changeMoveDirection( ContactPosition contactPosition, TypeContact typeContact )
{
    switch ( typeContact )
    {
        case BrickContact:
            m_eventHandler->AddPendingEvent( m_eventCurrentScoreInc );
            if ( m_bricks->empty() )
            {
                stop();
                m_eventHandler->AddPendingEvent( m_eventRoundCompleted );
                return;
            }
        break;

        case PaddleContact:
            m_eventHandler->AddPendingEvent( m_eventPing );
        break;

        case WallContact:
            m_eventHandler->AddPendingEvent( m_eventPong );
        break;

        case BallLost:
            stop();
            m_eventHandler->AddPendingEvent( m_eventBallLost );
            return;
        break;
    }

    static std::map<MoveDirection, void ( ShapesManager:: *)( ContactPosition )> s_handlers = {
        { DirectionTopRight, &ShapesManager::checkDirection<DirectionTopRight> },
        { DirectionTopLeft, &ShapesManager::checkDirection<DirectionTopLeft> },
        { DirectionRightDown, &ShapesManager::checkDirection<DirectionRightDown> },
        { DirectionLeftDown, &ShapesManager::checkDirection<DirectionLeftDown> },
    };

    ( this->*( s_handlers[ m_moveDirection ] ) )( contactPosition );

    calculateTrajectory();
}

void ShapesManager::update( double deltaTime )
{
    m_particles->update( deltaTime, m_ball, 1, glm::vec2( m_ball->radius() / 2.0f ) );
}

void ShapesManager::renderFrame( rendererPtr spriteRenderer )
{
    ContactPosition contactPosition = Ball::ContactNull;
    m_bricks->render( m_bRun, spriteRenderer, [this, &contactPosition]( brickPtr brick ) {
        contactPosition = m_ball->intersect( brick->bounds() );
        if ( contactPosition == Ball::ContactNull )
            return false;

        brick->kill();
        changeMoveDirection( contactPosition, BrickContact );

        return true;
    } );

    if ( m_boardMove )
        offsetBoard();

    m_board->draw( spriteRenderer );

    if ( m_bRun )
    {
        offsetBall();
        m_particles->draw();
    }
    else
        updateBallPosition( m_board->bounds() );

    spriteRenderer->selectShader();
    m_ball->draw( spriteRenderer );
}

void ShapesManager::offsetBoard()
{
    auto boardBounds = std::move( m_board->bounds() );
    boardBounds.m_x += m_boardMove;

    if ( m_boardMove > 0 && !( boardBounds.GetRight() < m_size.x ) ) // move to right
    {
        m_board->moveTo( m_size.x - boardBounds.m_width, boardBounds.m_y );
    }
    else if ( m_boardMove < 0 && !( boardBounds.m_x > 0 ) ) // move to left
    {
        m_board->moveTo( 0, boardBounds.m_y );
    }
    else
    {
        m_board->moveTo( boardBounds.m_x, boardBounds.m_y );
    }

    if ( m_isRobot )
        m_boardMove = 0;
}

void ShapesManager::offsetBall()
{
    if ( m_currentTrajPoint != m_trajectory.end() )
    {
        //auto currentSpeed = 2;
        m_ball->moveTo( *m_currentTrajPoint++ );
        /*if ( std::distance<std::vector<wxPoint2DDouble>::const_iterator>( m_currentTrajPoint, m_trajectory.end() ) > currentSpeed )
            m_currentTrajPoint += currentSpeed;
        else
            ++m_currentTrajPoint;*/

        //m_ball->moveTo( *m_currentTrajPoint );
    }

    const auto &ballBounds = m_ball->bounds();

    switch ( m_moveDirection )
    {
        case DirectionTopRight:
            if ( ballBounds.GetRight() >= m_size.x )
                changeMoveDirection( Ball::ContactRight );
            else if ( ballBounds.m_y >= m_ballTopLimit )
                changeMoveDirection( Ball::ContactBottom );
        break;

        case DirectionTopLeft:
            if ( ballBounds.m_x <= 0 )
                changeMoveDirection( Ball::ContactLeft );
            else if ( ballBounds.m_y >= m_ballTopLimit )
                changeMoveDirection( Ball::ContactBottom );
        break;

        case DirectionRightDown:
            if ( ballBounds.GetRight() >= m_size.x )
                changeMoveDirection( Ball::ContactRight );
            else if ( ballBounds.m_y < m_ballBottomLimit )
                changeMoveDirection( Ball::ContactTop,
                    m_ball->intersect( m_board->admissibleBounds( ballBounds ) ) == Ball::ContactNull ? BallLost : PaddleContact );
        break;

        case DirectionLeftDown:
            if ( ballBounds.m_y < m_ballBottomLimit )
                changeMoveDirection( Ball::ContactBottom,
                    m_ball->intersect( m_board->admissibleBounds( ballBounds ) ) == Ball::ContactNull ? BallLost : PaddleContact );
            else if ( ballBounds.m_x <= 0 )
                changeMoveDirection( Ball::ContactLeft );
        break;
    }
}