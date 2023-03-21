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

#include "Common/defs.h"
#include "Common/xRect.hpp"
#include "Base.h"
#include "Ball.h"
#include "Brick.h"
#include "Bricks.h"
#include "Board.h"
#include "ParticleGenerator.h"
#include "Renderer/SpriteRenderer.h"
#include "ShapesManager.h"


DEFINE_LOCAL_EVENT_TYPE( wxEVT_CURRENT_SCORE_INCREASED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_ROUND_COMLETED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_BALL_LOST )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_PING )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_PONG )

using namespace Shapes;
using enum ContactPosition;

ShapesManager::ShapesManager( wxWindow* parent )
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

    // the scale division value 
    m_scaleDivisionValue = ( END_ANGLE - BEGIN_ANGLE ) / m_board->size().x;
}

ShapesManager::~ShapesManager()
{
    m_keepGoing.store( false );
    resumeWorker();
}

void ShapesManager::loadLevel( unsigned short level )
{
    // let's throw some bricks
    m_bricks->loadLevel( level );
}

xRect ShapesManager::updateBallPosition( const xRect& boardBounds ) const
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

void ShapesManager::resize( const wxSize& size )
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
                update( DEFAULT_DELTATIME );
                checkWorkerPaused( true );
            }
         } );
    else
        resumeWorker();
}

void ShapesManager::stop()
{
    m_bRun = false;
    m_particles->clear();
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
            // took away the power of the brick
            m_ball->increaseVelocity( INCREASE_VELOCITY_STEP );
        break;

        case PaddleContact:
            m_eventHandler->AddPendingEvent( m_eventPing );
            changeDirection( m_ball->center(), m_board->center() );
            return;
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

    static std::map<MoveDirection, void ( ShapesManager::* )( ContactPosition )> s_handlers = {
        { DirectionTopRight, &ShapesManager::changeDirection<DirectionTopRight> },
        { DirectionTopLeft, &ShapesManager::changeDirection<DirectionTopLeft> },
        { DirectionRightDown, &ShapesManager::changeDirection<DirectionRightDown> },
        { DirectionLeftDown, &ShapesManager::changeDirection<DirectionLeftDown> },
    };

    ( this->*( s_handlers[ m_moveDirection ] ) )( contactPosition );
}

void ShapesManager::moveBoard()
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

void ShapesManager::checkPaddleContact()
{
    const auto& ballBounds = m_ball->bounds();

    switch ( m_moveDirection )
    {
        case DirectionTopRight:
            if ( ballBounds.GetRight() >= m_size.x )
                changeMoveDirection( ContactRight );
            else if ( ballBounds.m_y >= m_ballTopLimit )
                changeMoveDirection( ContactBottom );
        break;

        case DirectionTopLeft:
            if ( ballBounds.m_x <= 0 )
                changeMoveDirection( ContactLeft );
            else if ( ballBounds.m_y >= m_ballTopLimit )
                changeMoveDirection( ContactBottom );
        break;

        case DirectionRightDown:
            if ( ballBounds.GetRight() >= m_size.x )
                changeMoveDirection( ContactRight );
            else if ( ballBounds.m_y < m_ballBottomLimit )
                changeMoveDirection( ContactTop,
                    m_ball->intersect( m_board->admissibleBounds( ballBounds ) ) == ContactNull ? BallLost : PaddleContact );
        break;

        case DirectionLeftDown:
            if ( ballBounds.m_y < m_ballBottomLimit )
                changeMoveDirection( ContactBottom,
                    m_ball->intersect( m_board->admissibleBounds( ballBounds ) ) == ContactNull ? BallLost : PaddleContact );
            else if ( ballBounds.m_x <= 0 )
                changeMoveDirection( ContactLeft );
        break;
    }
}

void ShapesManager::checkKeysState()
{
    if ( wxGetKeyState( WXK_LEFT ) )
    {
        m_accelerate += m_board->velocity().x;
        m_boardMove = DirectionLeft - m_accelerate;
        return;
    }

    if ( wxGetKeyState( WXK_RIGHT ) )
    {
        m_accelerate += m_board->velocity().x;
        m_boardMove = DirectionRight + m_accelerate;
        return;
    }

    if ( m_accelerate != 0 )
    {
        m_accelerate = 0;
        m_boardMove = 0;
    }
}

bool ShapesManager::switchRun( bool bNewRound )
{
    m_bRun = !m_bRun;

    if ( m_bRun && bNewRound )
        initDirection();

    return m_bRun;
};

ContactPosition ShapesManager::checkBrickContact(
    const glm::vec2& ballPosition,
    const glm::vec2& delta,
    float beginValue,
    float endValue,
    float increment )
{
    ContactPosition contactPosition = ContactNull;
    glm::vec2 prevPosition = ballPosition;
    const auto Rab = m_ball->velocity().x;
    double k = 0.0;

    for ( float Rac = beginValue; Rac <= endValue; Rac += increment )
    {
        k = Rac / Rab;

        prevPosition = m_ball->position();
        m_ball->moveTo( { ballPosition.x + delta.x * k, ballPosition.y + delta.y * k } );

        m_bricks->checkContact( [ this, &contactPosition, &increment ]( brickPtr brick ) {
            contactPosition = m_ball->intersect( brick->bounds() );
            if ( contactPosition == ContactNull )
                return false;

            if ( increment != 1.f )
                brick->kill();

            return true;
        } );

        if ( contactPosition != ContactNull ) // there is a contact, clarify the position of the contact
        {
            if ( increment != 1.f )
                break;

            m_ball->moveTo( prevPosition );
            Rac -= increment;
            increment = INCREASE_VELOCITY_STEP;
        }
    }
    return contactPosition;
}

void ShapesManager::update( double deltaTime )
{
    if ( m_bRun )
    {
        const auto& ballPosition = m_ball->position();
        const auto Rab = m_ball->velocity().x;
        const auto radAngle = glm::radians( m_angle );
        const glm::vec2 delta( Rab * cos( radAngle ), Rab * sin( radAngle ) );
        float cc;
        const auto dc = modff( Rab, &cc );

        auto contactPosition = checkBrickContact( ballPosition, delta, 1.f, cc, 1.f );

        // if there is no contact and there is a remainder
        if ( contactPosition == ContactNull && dc > 0 )
            contactPosition= checkBrickContact( ballPosition, delta, cc, Rab, INCREASE_VELOCITY_STEP );

        // if there was no contact, we check the contact of the ball with the paddle
        if ( contactPosition == ContactNull )
        {
            if ( m_isRobot )
            {
                const auto& ballBounds = m_ball->bounds();
                const auto& boardBounds = m_board->bounds();
                m_boardMove = ballBounds.m_x + ( ballBounds.m_width - boardBounds.m_width ) / 2 - boardBounds.m_x;
            }
            checkPaddleContact();
        }
        else
            changeMoveDirection( contactPosition, BrickContact );

        // update the position of the particles according to the new position of the ball
        m_particles->update( deltaTime * 0.001, m_ball, 2, glm::vec2( m_ball->radius() / 2.0f ) );
    }
    else
        updateBallPosition( m_board->bounds() );

    checkKeysState();

    if ( m_boardMove )
        moveBoard();
}

void ShapesManager::renderFrame( rendererPtr spriteRenderer, double deltaTime )
{
    if ( pauseWorker() == std::cv_status::timeout )
    {
        wxLogDebug( "Can't pause Worker" );
        return;
    }

    spriteRenderer->selectShader();

    m_bricks->draw( spriteRenderer );
    m_board->draw( spriteRenderer );

    if ( m_bRun )
    {
        m_particles->draw();
        spriteRenderer->selectShader();
    }

    m_ball->draw( spriteRenderer );

    resumeWorker();
}
