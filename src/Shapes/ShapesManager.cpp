// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <thread>
#include <future>

#include <glm/glm.hpp>

#include "Common/defs.h"
#include "Common/Rect.hpp"
#include "Base.h"
#include "Ball.h"
#include "Brick.h"
#include "Bricks.h"
#include "Board.h"
#include "Explosions.h"
#include "ParticleGenerator.h"
#include "Renderer/SpriteRenderer.h"
#include "ShapesManager.h"


DEFINE_LOCAL_EVENT_TYPE( wxEVT_CURRENT_SCORE_INCREASED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_ROUND_COMLETED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_BALL_LOST )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_PING )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_PONG )

using namespace Shapes;

ShapesManager::ShapesManager( wxWindow* parent )
    : m_eventHandler( parent->GetEventHandler() )
    , m_eventCurrentScoreInc( wxEVT_CURRENT_SCORE_INCREASED )
    , m_eventRoundCompleted( wxEVT_ROUND_COMLETED )
    , m_eventPing( wxEVT_PING )
    , m_eventPong( wxEVT_PONG )
    , m_eventBallLost( wxEVT_BALL_LOST )
{
    m_ball = std::make_shared<Ball>();
    m_board = std::make_shared<Board>();
    m_bricks = std::make_shared<Bricks>();
    m_explosions = std::make_shared<Explosions>();

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

void ShapesManager::loadLevel( unsigned short level ) const
{
    // let's throw some bricks
    m_bricks->loadLevel( level );
}

Rect ShapesManager::updateBallPosition( const Rect& boardBounds ) const
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

    // set board position 
    auto boardBounds = m_board->bounds();
    m_board->moveTo( ( m_size.x - boardBounds.m_width ) / 2, 15 );

    // set ball on board
    boardBounds = m_board->bounds();
    const auto& ballBounds = updateBallPosition( boardBounds );

    m_ballTopLimit = m_size.y - ballBounds.m_height;
    m_ballBottomLimit = boardBounds.m_y + boardBounds.m_height;

    // calc worker
    if ( !m_asyncWorker.valid() )
    {
        m_asyncWorker = std::async( std::launch::async, [ this ]() {
            while ( m_keepGoing.load() )
            {
                try
                {
                    update( DEFAULT_DELTATIME );
                }
                catch ( const std::exception& e )
                {
                    wxMessageBox( e.what(), "Exception Caught", MB_OK );
                }
                checkWorkerPaused( true );
            }
         } );
    }
    else
        resumeWorker();
}

void ShapesManager::run( bool bNewRound )
{
    m_state = State::RUN;

    if ( bNewRound )
    {
        initDirection();
        calculateDelta();
    }
}

void ShapesManager::stop()
{
    m_state = State::STOPPED;
    m_isRoundCompleted = false;
    m_particles->clear();
    m_explosions->clear();
}

void ShapesManager::calculateDelta()
{
    const auto rab = m_ball->velocity().x;
    const auto radAngle = glm::radians( m_angle );
    m_delta.x = rab * cos( radAngle );
    m_delta.y = rab * sin( radAngle );
}

void ShapesManager::changeMoveDirection( ContactPosition contactPosition, TypeContact typeContact )
{
    switch ( typeContact )
    {
        case TypeContact::BrickContact:
            m_eventHandler->AddPendingEvent( m_eventCurrentScoreInc );
            m_isRoundCompleted = m_bricks->empty();

            if ( m_isRoundCompleted )
                return;

            // took away the power of the brick
            m_ball->increaseVelocity( INCREASE_VELOCITY_STEP );
        break;

        case TypeContact::PaddleContact:
            m_eventHandler->AddPendingEvent( m_eventPing );
            changeDirection( m_ball->center(), m_board->center() );
            calculateDelta();
            return;
        break;

        case TypeContact::WallContact:
            m_eventHandler->AddPendingEvent( m_eventPong );
        break;

        case TypeContact::BallLost:
            stop();
            m_eventHandler->AddPendingEvent( m_eventBallLost );
            return;
        break;
    }

    MoveController::changeMoveDirection( contactPosition );
    calculateDelta();
}

void ShapesManager::moveBoard( float value ) const
{
    auto boardBounds = m_board->bounds();
    boardBounds.m_x += value;

    if ( value > 0 && !( boardBounds.right() < m_size.x ) ) // move to right
    {
        m_board->moveTo( m_size.x - boardBounds.m_width, boardBounds.m_y );
    }
    else if ( value < 0 && !( boardBounds.m_x > 0 ) ) // move to left
    {
        m_board->moveTo( 0, boardBounds.m_y );
    }
    else
    {
        m_board->moveTo( boardBounds.m_x, boardBounds.m_y );
    }
}

ContactPosition ShapesManager::checkPaddleContact( bool checkOnly )
{
    const auto& ballBounds = m_ball->bounds();

    if ( ballBounds.m_y >= m_ballTopLimit )
    {
        if ( !checkOnly )
            changeMoveDirection( ContactPosition::Top );
        
        return ContactPosition::Top;
    }
    
    if ( ballBounds.m_y <= m_ballBottomLimit )
    {
        const auto& restrictions = m_board->admissibleBounds( ballBounds );
        const bool isLost = m_ball->intersect( restrictions ) == ContactPosition::Null;
        const auto typeContact = isLost ? TypeContact::BallLost : TypeContact::PaddleContact;

        if ( !checkOnly )
            changeMoveDirection( ContactPosition::Bottom, typeContact );

        return ContactPosition::Bottom;
    }
    
    if ( ballBounds.right() >= m_size.x )
    {
        if ( !checkOnly )
            changeMoveDirection( ContactPosition::Right );
    
        return ContactPosition::Right;
    }
    
    if ( ballBounds.m_x <= 0 )
    {
        if ( !checkOnly )
            changeMoveDirection( ContactPosition::Left );

        return ContactPosition::Left;
    }

    return ContactPosition::Null;
}

void ShapesManager::checkKeysState()
{
    if ( wxGetKeyState( WXK_LEFT ) )
    {
        m_accelerate += m_board->velocity().x;
        moveBoard( MoveDirection::Left - m_accelerate );
        return;
    }

    if ( wxGetKeyState( WXK_RIGHT ) )
    {
        m_accelerate += m_board->velocity().x;
        moveBoard( MoveDirection::Right + m_accelerate );
        return;
    }

    if ( m_accelerate != 0 )
        m_accelerate = 0;
}

ContactPosition ShapesManager::checkContact(
    const glm::vec2 &ballPosition,
    float beginValue,
    float endValue,
    float increment )
{
    TypeContact typeContact = TypeContact::Null;
    ContactPosition contactPosition = ContactPosition::Null;
    glm::vec2 prevPosition = ballPosition;
    const auto rab = m_ball->velocity().x;
    double k = 0.0;

    for ( float rac = beginValue; rac <= endValue; rac += increment )
    {
        k = rac / rab;
        prevPosition = m_ball->position();
        
        m_ball->moveTo( ballPosition.x + m_delta.x * k, ballPosition.y + m_delta.y * k );

        if ( typeContact == TypeContact::Null || typeContact == TypeContact::BrickContact )
        {
            m_bricks->checkContact( [ this, &contactPosition, &increment, &typeContact ]( const brickPtr& brick ) {
                contactPosition = m_ball->intersect( brick->bounds() );
                if ( contactPosition == ContactPosition::Null )
                    return false;

                if ( increment != INCREASE_DEFAULT_STEP )
                {
                    brick->kill();
                    m_explosions->add( brick );
                }

                typeContact = TypeContact::BrickContact;
                return true;
            } );
        }

        if ( typeContact != TypeContact::BrickContact )
            contactPosition = checkPaddleContact( increment == INCREASE_DEFAULT_STEP );

        if ( contactPosition != ContactPosition::Null ) // there is a contact, clarify the position of the contact
        {
            if ( increment != INCREASE_DEFAULT_STEP )
            {
                if ( typeContact == TypeContact::BrickContact )
                    changeMoveDirection( contactPosition, typeContact );
                break;
            }

            if ( typeContact == TypeContact::Null )
                typeContact = TypeContact::PaddleContact;

            m_ball->moveTo( prevPosition );
            rac -= increment;
            increment = INCREASE_VELOCITY_STEP;
            contactPosition = ContactPosition::Null;
        }
    }

    return contactPosition;
}

void ShapesManager::update( double deltaTime )
{
    if ( m_isRoundCompleted && m_explosions->empty() )
    {
        stop();
        m_eventHandler->AddPendingEvent( m_eventRoundCompleted );
        return;
    }

    if ( isRunning() )
    {
        const auto& ballPosition = m_ball->position();
        const auto rab = m_ball->velocity().x;
        float cc = { .0f };
        const auto dc = modff( rab, &cc );

        if ( checkContact( ballPosition, 1.f, cc, INCREASE_DEFAULT_STEP ) == ContactPosition::Null && dc > 0 )
            checkContact( ballPosition, cc, rab, INCREASE_VELOCITY_STEP );

        if ( m_isRobot )
        {
            const auto& ballBounds = m_ball->bounds();
            const auto& boardBounds = m_board->bounds();
            moveBoard( ballBounds.m_x + ( ballBounds.m_width - boardBounds.m_width ) / 2 - boardBounds.m_x );
        }

        // update the position of the particles according to the new position of the ball
        m_particles->update( deltaTime * 0.001, m_ball, 2, glm::vec2( m_ball->radius() / 2.0f ) );
    }
    else if ( isStopped() )
        updateBallPosition( m_board->bounds() );

    checkKeysState();
}

void ShapesManager::renderFrame( const rendererPtr &spriteRenderer )
{
    if ( pauseWorker() == std::cv_status::timeout )
    {
        wxLogDebug( "Can't pause Worker" );
        return;
    }

    spriteRenderer->selectShader();

    m_board->draw( spriteRenderer );

    if ( !m_isRoundCompleted )
    {
        m_bricks->draw( spriteRenderer );
        
        if ( isRunning() )
        {
            m_particles->draw();
            spriteRenderer->selectShader();
        }
        
        m_ball->draw( spriteRenderer );
    }

    m_explosions->draw( spriteRenderer );

    resumeWorker();
}
