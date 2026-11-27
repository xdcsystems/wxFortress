// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

#include "Common/defs.h"
#include "Common/ScopeExit.hpp"
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
DEFINE_LOCAL_EVENT_TYPE( wxEVT_ROUND_COMPLETED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_BALL_LOST )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_PING )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_PONG )

using namespace Shapes;

namespace
{
    constexpr float kInitialBallSpeedPxPerSec =
        ( BEGIN_BALL_VELOCITY * 1000.0f ) / static_cast<float>( DEFAULT_DELTATIME ); // 187.5 px/s (3 px / 16 ms)
}

ShapesManager::ShapesManager( wxWindow* parent, const rendererPtr& spriteRenderer )
    : m_eventHandler( parent->GetEventHandler() )
    , m_eventCurrentScoreInc( wxEVT_CURRENT_SCORE_INCREASED )
    , m_eventRoundCompleted( wxEVT_ROUND_COMPLETED )
    , m_eventPing( wxEVT_PING )
    , m_eventPong( wxEVT_PONG )
    , m_eventBallLost( wxEVT_BALL_LOST )
    , m_renderer( spriteRenderer )
{
    m_currentBallSpeed = kInitialBallSpeedPxPerSec;
    m_ball = std::make_shared<Ball>();
    m_board = std::make_shared<Board>();
    m_bricks = std::make_shared<Bricks>();
    m_explosions = std::make_shared<Explosions>();

    // set render-specific controls
    m_particles = std::make_shared<ParticleGenerator>( 500 );

    m_ballState.radius = m_ball->radius();

    if ( wxTheApp && wxTheApp->argc > 1 )
    {
        const std::string param { wxTheApp->argv[ 1 ] };
        m_isRobot = param == "--robot";
    }
}

void ShapesManager::loadLevel( unsigned short level ) const
{
    // let's throw some bricks
    m_bricks->loadLevel( level );
}

void ShapesManager::resetBallSpeed()
{
    m_currentBallSpeed = kInitialBallSpeedPxPerSec;
}

Physics::Aabb ShapesManager::updateBallPosition( const Physics::Aabb& boardBounds, bool syncPrev )
{
    if ( m_size.x < 1 || m_size.y < 1 )
        return {};

    // set ball on board
    const auto ballBounds = m_ball->bounds();

    m_ball->moveTo(
        boardBounds.x + ( boardBounds.w - ballBounds.w ) / 2.0f,
        boardBounds.top() );

    m_ballState.center = m_ball->center();
    m_ballState.radius = m_ball->radius();
    m_ballState.vel = { 0.0f, 0.0f };
    if ( syncPrev )
        syncPreviousState();

    return m_ball->bounds();
}

void ShapesManager::resize( const wxSize& size, const glm::mat4& projection )
{
    m_size = size;
    m_arenaCfg.width = static_cast<float>( size.x );
    m_arenaCfg.height = static_cast<float>( size.y );
    m_particles->resize( projection );

    if ( m_state == State::RUN || m_state == State::PAUSED )
        return;

    // set board position
    auto boardBounds = m_board->bounds();
    m_board->moveTo( ( m_size.x - boardBounds.w ) / 2.0f, 15.0f );

    // set ball on board
    boardBounds = m_board->bounds();
    m_paddleState.box = boardBounds;
    m_paddleState.speed = 0.0f;

    updateBallPosition( boardBounds, true );
}

void ShapesManager::initBallVelocity()
{
    static thread_local std::mt19937 rng( std::random_device {}() );
    std::uniform_int_distribution<int> distAngle(
        static_cast<int>( BEGIN_ANGLE ),
        static_cast<int>( END_ANGLE ) ); // [30..150]

    int angleDeg = 90;
    do
    {
        angleDeg = distAngle( rng );
    } while ( angleDeg > static_cast<int>( END_RIGHT_ANGLE ) &&
              angleDeg < static_cast<int>( BEGIN_LEFT_ANGLE ) ); // [30..70] or [110..150]

    const float launchSpeed = std::clamp( m_currentBallSpeed, m_arenaCfg.minSpeed, m_arenaCfg.maxSpeed );
    const float rad = glm::radians( static_cast<float>( angleDeg ) );
    m_ballState.vel = {
        launchSpeed * std::cos( rad ),
        launchSpeed * std::sin( rad )
    };
    m_ball->m_velocity = m_ballState.vel;
}

void ShapesManager::run( bool bNewRound )
{
    m_state = State::RUN;

    if ( bNewRound )
    {
        m_isRoundCompleted = false;
        m_robotDryBounces = 0;
        m_ballState.center = m_ball->center();
        m_ballState.radius = m_ball->radius();
        initBallVelocity();
    }
    syncPreviousState();
}

void ShapesManager::stop()
{
    m_state = State::STOPPED;
    m_isRoundCompleted = false;
    m_robotDryBounces = 0;
    m_hasMouseTarget = false;
    m_particleSpawnAccum = 0.0f;
    m_paddleState.speed = 0.0f;
    m_ballState.vel = { 0.0f, 0.0f };
    m_particles->clear();
    m_explosions->clear();
    syncPreviousState();
}

void ShapesManager::setPaddleMouseX( float mouseX )
{
    m_mouseTargetX = mouseX;
    m_hasMouseTarget = true;
}

void ShapesManager::setRenderAlpha( float alpha )
{
    m_renderAlpha = std::isfinite( alpha ) ? std::clamp( alpha, 0.0f, 1.0f ) : 1.0f;
}

void ShapesManager::syncPreviousState()
{
    m_prevBallCenter = m_ballState.center;
    m_prevPaddleX = m_paddleState.box.x;
}

void ShapesManager::updatePaddleInput( float dtSec, int remainingSteps )
{
    int moveDir = 0;
    if ( wxGetKeyState( WXK_LEFT ) )
        moveDir = -1;
    else if ( wxGetKeyState( WXK_RIGHT ) )
        moveDir = +1;

    if ( moveDir != 0 )
    {
        m_hasMouseTarget = false;
        Physics::updatePaddle( m_paddleState, moveDir, dtSec, m_arenaCfg );
    }
    else if ( m_hasMouseTarget )
    {
        if ( remainingSteps <= 1 )
        {
            Physics::setPaddleTargetX( m_paddleState, m_mouseTargetX, m_arenaCfg );
            m_hasMouseTarget = false;
        }
        else if ( std::isfinite( m_mouseTargetX ) )
        {
            const float maxX = std::max( 0.0f, m_arenaCfg.width - m_paddleState.box.w );
            const float clampedTargetX = std::clamp(
                m_mouseTargetX - 0.5f * m_paddleState.box.w, 0.0f, maxX );
            const float stepX = m_paddleState.box.x +
                ( clampedTargetX - m_paddleState.box.x ) / static_cast<float>( remainingSteps );
            m_paddleState.box.x = std::clamp( stepX, 0.0f, maxX );
            m_paddleState.speed = 0.0f;
        }
        else
        {
            m_hasMouseTarget = false;
        }
    }
    else
    {
        Physics::updatePaddle( m_paddleState, 0, dtSec, m_arenaCfg );
    }

    if ( isRunning() && m_isRobot && moveDir == 0 )
    {
        Physics::updateRobotPaddle(
            m_paddleState,
            m_ballState,
            m_physBricks,
            dtSec,
            m_arenaCfg,
            m_robotDryBounces );
    }

    m_board->moveTo( m_paddleState.box.x, m_paddleState.box.y );
}

void ShapesManager::update( double deltaTimeSec, int remainingSteps )
{
    if ( isPaused() || m_size.x < 1 || m_size.y < 1 || !std::isfinite( deltaTimeSec ) )
        return;

    if ( m_isRoundCompleted && m_explosions->empty() )
    {
        stop();
        updateBallPosition( m_board->bounds(), true );
        m_eventHandler->QueueEvent( m_eventRoundCompleted.Clone() );
        return;
    }

    const float dtSec = std::clamp( static_cast<float>( deltaTimeSec ), 0.0f, 0.05f );

    if ( m_paddleState.box.w <= 0.0f || m_paddleState.box.h <= 0.0f )
    {
        m_paddleState.box = m_board->bounds();
        syncPreviousState();
    }

    const auto& brickList = m_bricks->items();
    if ( isRunning() && !m_isRoundCompleted )
    {
        m_physBricks.clear();
        m_physBricks.reserve( brickList.size() );
        for ( const auto& brick : brickList )
        {
            m_physBricks.push_back( Physics::BrickItem { brick->bounds(), brick->isAlive() } );
        }
    }

    // Snapshot previous authoritative physics state before advancing this fixed tick
    m_prevBallCenter = m_ballState.center;
    m_prevPaddleX = m_paddleState.box.x;

    const float prevPaddleX = m_paddleState.box.x;
    updatePaddleInput( dtSec, remainingSteps );
    const float paddleDeltaX = m_paddleState.box.x - prevPaddleX;

    if ( isRunning() )
    {
        if ( !m_isRoundCompleted )
        {

            m_ballState.radius = m_ball->radius();
            const Physics::StepEvents ev = Physics::stepBall(
                m_ballState,
                m_paddleState.box,
                m_physBricks,
                dtSec,
                m_arenaCfg,
                paddleDeltaX );

            const float r = m_ballState.radius;
            m_ball->moveTo( m_ballState.center - glm::vec2( r ) );
            m_ball->m_velocity = m_ballState.vel;

            if ( ev.hitWall )
                m_eventHandler->QueueEvent( m_eventPong.Clone() );

            if ( ev.hitPaddle )
            {
                ++m_robotDryBounces;
                m_eventHandler->QueueEvent( m_eventPing.Clone() );
            }

            if ( !ev.hitBricks.empty() )
            {
                m_robotDryBounces = 0;
                m_currentBallSpeed = std::clamp(
                    Physics::length( m_ballState.vel ),
                    m_arenaCfg.minSpeed,
                    m_arenaCfg.maxSpeed );
                for ( const std::size_t idx : ev.hitBricks )
                {
                    if ( idx < brickList.size() )
                    {
                        const auto& brick = brickList[ idx ];
                        if ( brick->isAlive() )
                        {
                            brick->kill();
                            m_explosions->add( brick );
                            m_eventHandler->QueueEvent( m_eventCurrentScoreInc.Clone() );
                        }
                    }
                }
                m_isRoundCompleted = m_bricks->empty();
            }

            if ( ev.ballLost )
            {
                stop();
                updateBallPosition( m_board->bounds(), true );
                m_eventHandler->QueueEvent( m_eventBallLost.Clone() );
                return;
            }
        }

        // Spawn particles at a frame-rate-independent 120 particles/sec (= 2 per 60 Hz frame)
        m_particleSpawnAccum += dtSec * 120.0f;
        const unsigned int spawnCount = static_cast<unsigned int>( m_particleSpawnAccum );
        m_particleSpawnAccum -= static_cast<float>( spawnCount );
        m_particles->update( dtSec, m_ball, spawnCount, glm::vec2( m_ball->radius() / 2.0f ) );
    }
    else if ( isStopped() )
    {
        updateBallPosition( m_board->bounds(), false );
    }
}

void ShapesManager::renderFrame()
{
    const float alpha = m_renderAlpha;
    const float authPaddleX = m_paddleState.box.x;
    const float authPaddleY = m_paddleState.box.y;
    const float interpPaddleX = glm::mix( m_prevPaddleX, authPaddleX, alpha );

    const float r = m_ballState.radius;
    const glm::vec2 interpBallCenter = glm::mix( m_prevBallCenter, m_ballState.center, alpha );

    // Restore authoritative physics positions on scope exit so no caller can ever read interpolated coords.
    const auto restorer = makeScopeExit( [ this, authPaddleX, authPaddleY, r ]() {
        if ( m_paddleState.box.w > 0.0f )
            m_board->moveTo( authPaddleX, authPaddleY );
        if ( r > 0.0f )
            m_ball->moveTo( m_ballState.center - glm::vec2( r ) );
    } );

    if ( m_paddleState.box.w > 0.0f )
        m_board->moveTo( interpPaddleX, authPaddleY );
    if ( r > 0.0f )
        m_ball->moveTo( interpBallCenter - glm::vec2( r ) );

    m_renderer->selectShader();

    m_board->draw( m_renderer );

    if ( !m_isRoundCompleted )
    {
        m_bricks->draw( m_renderer );

        if ( isRunning() )
        {
            m_particles->draw();
            m_renderer->selectShader();
        }

        m_ball->draw( m_renderer );
    }

    m_explosions->draw( m_renderer );
}
