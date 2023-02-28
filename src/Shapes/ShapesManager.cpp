// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <random>

#include <wx/dcbuffer.h>


#include "../Tools.h"
#include "Base.h"
#include "Ball.h"
#include "Brick.h"
#include "Bricks.h"
#include "Board.h"
#include "ShapesManager.h"


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

wxRect ShapesManager::updateBallPosition( const wxRect &boardBounds ) const
{
    if ( m_size.x < 1 || m_size.y < 1 || !m_trajectory.empty() )
        return {};

    // set ball on board
    const auto &ballBounds = m_ball->bounds();
    m_ball->moveTo( boardBounds.x + ( ( boardBounds.width - ballBounds.width ) >> 1 ), boardBounds.y + boardBounds.height );
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
    m_board->moveTo( ( ( m_size.x - boardBounds.width ) >> 1 ), 15 );

    // set ball on board
    boardBounds = std::move( m_board->bounds() );
    const auto &ballBounds = updateBallPosition( boardBounds );

    m_ballTopLimit = m_size.y - ballBounds.height;
    m_ballBottomLimit = boardBounds.y + boardBounds.height;

    m_eventHandler = m_parent->GetEventHandler();
    m_eventCurrentScoreInc.SetEventObject( this );
    m_eventPing.SetEventObject( this );
    m_eventPong.SetEventObject( this );
}

void ShapesManager::calculateTrajectory()
{
    static auto d2r = []( double d ) { return ( d / 180.0 ) * ( ( double )M_PI ); };
    const auto &bounds = m_ball->bounds();

    int x = bounds.x + m_diagonal * cos( d2r( m_angle ) );
    int y = bounds.y + m_diagonal * sin( d2r( m_angle ) );

    //////////////////////// TODO

    //const double deltaX = x - bounds.x;
    //const double deltaY = y - bounds.y;

    //m_trajectory.clear();
    //const double Rab = sqrt( pow( deltaX, 2 ) + pow( deltaY, 2 ) );
    //for ( double Rac = 1; Rac <= Rab; ++Rac )
    //{
    //    const auto k = Rac / Rab;
    //    const auto Xc = bounds.x + deltaX * k;
    //    const auto Yc = bounds.y + deltaY * k;

    //    if ( Xc < 0 || Yc < 0 || Xc > m_size.x || Yc > m_size.y )
    //        break;

    //    m_trajectory.push_back( { Xc, Yc } );
    //}

    Tools::Instance().bhmLine( m_trajectory, bounds.x, bounds.y, x, y, m_size );
    m_currentTrajectoryPoint = m_trajectory.begin();

    if ( m_isRobot && ( m_moveDirection == DirectionLeftDown || m_moveDirection == DirectionRightDown ) )
    {
        const auto &boardBounds = m_board->bounds();
        const auto pointItor = std::find_if( m_trajectory.begin(), m_trajectory.end(), [&boardBounds]( auto& point ) {
                return point.y <= boardBounds.y + boardBounds.height;
            } );

        if ( pointItor != m_trajectory.end() )
            m_boardMove = pointItor->x + ( bounds.width >> 1 ) - boardBounds.x - ( boardBounds.width >> 1 );
    }

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

void ShapesManager::renderFrame( wxMemoryDC &dc )
{
    ContactPosition contactPosition = Ball::ContactNull;
    m_bricks->render( m_bRun, dc,  [ this, &dc, &contactPosition ]( std::shared_ptr<Brick> brick ) {
        contactPosition = m_ball->intersect( brick->bounds() );
        if ( contactPosition == Ball::ContactNull )
            return false;
        
        brick->kill( dc );
        changeMoveDirection( contactPosition, BrickContact );

        return true;
    } );

    m_board->clear( dc );

    if ( m_boardMove )
        offsetBoard();
    
    m_board->draw( dc );
    
    m_ball->clear( dc );

    if ( m_bRun )
        offsetBall();
    else
        updateBallPosition( m_board->bounds() );

    m_ball->draw( dc );
}

void ShapesManager::offsetBoard()
{
    auto boardBounds = std::move( m_board->bounds() );
    boardBounds.x += m_boardMove;

    if ( m_boardMove > 0 && !( boardBounds.x + boardBounds.width < m_size.x ) ) // move to right
    {
        m_board->moveTo( m_size.x - boardBounds.width, boardBounds.y );
    }
    else if ( m_boardMove < 0 && !( boardBounds.x > 0 ) ) // move to left
    {
        m_board->moveTo( 0, boardBounds.y );
    }
    else
    {
        m_board->moveTo( boardBounds.x, boardBounds.y );
    }

    if ( m_isRobot )
        m_boardMove = 0;
}

void ShapesManager::offsetBall()
{
    if ( m_currentTrajectoryPoint != m_trajectory.end() )
    {
        //auto CurrentSpeed = 1;
        m_ball->moveTo( *m_currentTrajectoryPoint++ );
        /*if ( std::distance<std::vector<wxPoint>::const_iterator>( m_currentTrajectoryPoint, m_trajectory.end() ) > CurrentSpeed )
            m_currentTrajectoryPoint += CurrentSpeed;
        else
            m_currentTrajectoryPoint++;*/
    }

    const auto &ballBounds = m_ball->bounds();

    switch ( m_moveDirection )
    {
        case DirectionTopRight:
            if ( ballBounds.GetRight() >= m_size.x )
                changeMoveDirection( Ball::ContactRight );
            else if ( ballBounds.y >= m_ballTopLimit )
                changeMoveDirection( Ball::ContactBottom );
        break;

        case DirectionTopLeft:
            if ( ballBounds.x <= 0 )
                changeMoveDirection( Ball::ContactLeft );
            else if ( ballBounds.y >= m_ballTopLimit )
                changeMoveDirection( Ball::ContactBottom );
        break;

        case DirectionRightDown:
            if ( ballBounds.GetRight() >= m_size.x )
                changeMoveDirection( Ball::ContactRight );
            else if ( ballBounds.y < m_ballBottomLimit )
                changeMoveDirection( Ball::ContactTop,
                    m_ball->intersect( m_board->admissibleBounds( ballBounds ) ) == Ball::ContactNull ? BallLost : PaddleContact );
        break;

        case DirectionLeftDown:
            if ( ballBounds.y < m_ballBottomLimit )
                changeMoveDirection( Ball::ContactBottom,
                    m_ball->intersect( m_board->admissibleBounds( ballBounds ) ) == Ball::ContactNull ? BallLost : PaddleContact );
            else if ( ballBounds.x <= 0 )
                changeMoveDirection( Ball::ContactLeft );
        break;
    }
}