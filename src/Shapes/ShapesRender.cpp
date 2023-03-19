// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <map>
#include <thread>
#include <future>

#include <glm/glm.hpp>

#include "Common/xRect.hpp"
#include "Base.h"
#include "Ball.h"
#include "Brick.h"
#include "Bricks.h"
#include "Board.h"
#include "ParticleGenerator.h"
#include "ShapesRender.h"
#include "Renderer/SpriteRenderer.h"

DEFINE_LOCAL_EVENT_TYPE( wxEVT_CURRENT_SCORE_INCREASED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_ROUND_COMLETED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_BALL_LOST )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_PING )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_PONG )

using namespace Shapes;
using enum ShapesRender::MoveDirection;

ShapesRender::ShapesRender( wxWindow* parent )
    : m_eventHandler( parent->GetEventHandler() )
    , m_trajectory( NUM_OF_CALCULATED_POINTS, { .0f, .0f } )
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

ShapesRender::~ShapesRender()
{
    m_keepGoing.store( false );
    resumeWorker();
}

void ShapesRender::loadLevel( unsigned short level )
{
    // let's throw some bricks
    m_bricks->loadLevel( level );
}

xRect ShapesRender::updateBallPosition( const xRect& boardBounds ) const
{
    if ( m_size.x < 1 || m_size.y < 1 )
        return {};

    // set ball on board
    const auto& ballBounds = m_ball->bounds();

    m_ball->moveTo(
        boardBounds.m_x + ( boardBounds.m_width - ballBounds.m_width ) / 2,
        boardBounds.m_y + boardBounds.m_height );

    return m_ball->bounds();
}

void ShapesRender::resize( const wxSize& size )
{
    m_size = size;
    m_diagonal = sqrt( pow( m_size.x, 2 ) + pow( m_size.y, 2 ) );

    // set board position 
    auto boardBounds = std::move( m_board->bounds() );
    m_board->moveTo( ( m_size.x - boardBounds.m_width ) / 2, 15 );

    // set ball on board
    boardBounds = std::move( m_board->bounds() );
    const auto& ballBounds = updateBallPosition( boardBounds );

    m_ballTopLimit = m_size.y - ballBounds.m_height;
    m_ballBottomLimit = boardBounds.m_y + boardBounds.m_height;

    // calc worker
    if ( !m_asyncWorker.valid() )
        m_asyncWorker = std::async( std::launch::async, [this]() {
            while ( m_keepGoing.load() )
            {
                update( 16 );
                pause.store( true );
                checkWorkerPaused();
            }
        } );
    else
        resumeWorker();
}

void ShapesRender::calculateTrajectory()
{
    const auto& ballBounds = m_ball->bounds();
    const auto radAngle = glm::radians( m_angle );

    const glm::vec2 delta( m_diagonal * cos( radAngle ), m_diagonal * sin( radAngle ) );

    double k = 0.0;
    float Rac = .0f;

    std::generate( m_trajectory.begin(), m_trajectory.end(), [this, &ballBounds, &delta, &k, &Rac]() -> glm::vec2 {
        k = Rac / m_diagonal;
        Rac += m_ball->velocity().x;
        return { ballBounds.m_x + delta.x * k, ballBounds.m_y + delta.y * k };
     } );

    if ( m_isRobot && ( m_moveDirection == DirectionLeftDown || m_moveDirection == DirectionRightDown ) )
    {
        const auto& boardBounds = m_board->bounds();
        m_currentTrajPoint = std::find_if( m_trajectory.cbegin(), m_trajectory.cend(), [&boardBounds]( auto& point ) {
            return point.y <= boardBounds.GetBottom();
        } );

        if ( m_currentTrajPoint != m_trajectory.end() )
            m_boardMove = m_currentTrajPoint->x + ( ballBounds.m_width - boardBounds.m_width ) / 2 - boardBounds.m_x;
    }

    m_currentTrajPoint = m_trajectory.begin();
}

void ShapesRender::stop()
{
    m_bRun = false;
    m_particles->clear();
}

void ShapesRender::changeMoveDirection( ContactPosition contactPosition, TypeContact typeContact )
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

    static std::map<MoveDirection, void ( ShapesRender::* )( ContactPosition )> s_handlers = {
        { DirectionTopRight, &ShapesRender::checkDirection<DirectionTopRight> },
        { DirectionTopLeft, &ShapesRender::checkDirection<DirectionTopLeft> },
        { DirectionRightDown, &ShapesRender::checkDirection<DirectionRightDown> },
        { DirectionLeftDown, &ShapesRender::checkDirection<DirectionLeftDown> },
    };

    ( this->*( s_handlers[ m_moveDirection ] ) )( contactPosition );
}

void ShapesRender::update( double deltaTime )
{
    calculateTrajectory();
    m_particles->update( deltaTime * 0.001, m_ball, 1, glm::vec2( m_ball->radius() / 2.0f ) );

    if ( m_bRun )
    {
        if ( m_trajectory.empty() )
            return;

        /*if ( std::distance( m_currentTrajPoint, m_trajectory.cend() ) > step )
            m_currentTrajPoint += step;
        else*/
            ++m_currentTrajPoint;

        if ( m_currentTrajPoint != m_trajectory.end() )
            m_ball->moveTo( *m_currentTrajPoint );

        ContactPosition contactPosition = Ball::ContactNull;
        m_bricks->render( m_bRun, [this, &contactPosition]( brickPtr brick ) {
            contactPosition = m_ball->intersect( brick->bounds() );
            return contactPosition != Ball::ContactNull;
        } );

        if ( contactPosition == Ball::ContactNull )
            checkBallContact();
        else
            changeMoveDirection( contactPosition, BrickContact );
    }
    else
        updateBallPosition( m_board->bounds() );

    checkKeysState();

    if ( m_boardMove )
        moveBoard();
}

void ShapesRender::moveBoard()
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

void ShapesRender::checkBallContact()
{
    const auto& ballBounds = m_ball->bounds();

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

void ShapesRender::checkKeysState()
{
    if ( wxGetKeyState( WXK_LEFT ) )
    {
        m_accelerate += 0.32;
        m_boardMove = DirectionLeft - m_accelerate;
        return;
    }

    if ( wxGetKeyState( WXK_RIGHT ) )
    {
        m_accelerate += 0.32;
        m_boardMove= DirectionRight + m_accelerate;
        return;
    }

    if ( m_accelerate != 0 )
    {
        m_accelerate = 0;
        m_boardMove = 0;
    }
}