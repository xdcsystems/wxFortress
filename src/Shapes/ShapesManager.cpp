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
#include "BrickMechanics.hpp"
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
DEFINE_LOCAL_EVENT_TYPE( wxEVT_ARMOR_HIT )

using namespace Shapes;

namespace
{
    constexpr float kInitialBallSpeedPxPerSec =
        ( BEGIN_BALL_VELOCITY * 1000.0f ) / static_cast<float>( DEFAULT_DELTATIME ); // 187.5 px/s (3 px / 16 ms)
}

ShapesManager::ShapesManager( wxWindow* parent, const rendererPtr& spriteRenderer )
    : m_eventHandler( parent->GetEventHandler() )
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

void ShapesManager::loadLevel( unsigned short level, unsigned short stage )
{
    m_resonance.reset();
    m_paddleRecoil.reset();
    m_smoothedPaddleVelX = 0.0f;
    m_paddleSkillFlashSec = 0.0f;
    m_pendingImpact = false;
    m_pendingImpactMaxOrFinal = false;

    // Tension sawtooth: keep level-entry speed intense yet fair after EM-heavy levels
    m_currentBallSpeed = ( std::min )( m_currentBallSpeed, BrickMechanics::kLevelStartMaxSpeed );

    // let's throw some bricks
    m_bricks->loadLevel( level, stage );
}

void ShapesManager::resetBallSpeed()
{
    m_currentBallSpeed = kInitialBallSpeedPxPerSec;
}

bool ShapesManager::toggleRobotMode() noexcept
{
    m_isRobot = !m_isRobot;
    m_robotDryBounces = 0;
    return m_isRobot;
}

bool ShapesManager::consumePendingImpact( bool& outMaxOrFinal ) noexcept
{
    if ( !m_pendingImpact )
        return false;
    outMaxOrFinal = m_pendingImpactMaxOrFinal;
    m_pendingImpact = false;
    m_pendingImpactMaxOrFinal = false;
    return true;
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
    m_ball->setVelocity( m_ballState.vel );
}

void ShapesManager::run( bool bNewRound )
{
    m_state = State::RUN;
    m_smoothedPaddleVelX = 0.0f;

    if ( bNewRound )
    {
        m_isRoundCompleted = false;
        m_robotDryBounces = 0;
        m_resonance.reset();
        m_paddleRecoil.reset();
        m_smoothedPaddleVelX = 0.0f;
        m_paddleSkillFlashSec = 0.0f;
        m_pendingImpact = false;
        m_pendingImpactMaxOrFinal = false;
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
    m_resonance.reset();
    m_paddleRecoil.reset();
    m_smoothedPaddleVelX = 0.0f;
    m_paddleSkillFlashSec = 0.0f;
    m_pendingImpact = false;
    m_pendingImpactMaxOrFinal = false;
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
        queueCommandEvent( m_eventHandler, wxEVT_ROUND_COMPLETED );
        return;
    }

    const float dtSec = std::clamp( static_cast<float>( deltaTimeSec ), 0.0f, 0.05f );
    m_paddleRecoil.step( dtSec );
    if ( m_paddleSkillFlashSec > 0.0f )
        m_paddleSkillFlashSec = ( std::max )( 0.0f, m_paddleSkillFlashSec - dtSec );
    m_bricks->updateVisuals( dtSec );

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
            m_physBricks.push_back( Physics::BrickItem { brick->bounds(), brick->isAlive(), brick->hp() } );
        }
    }

    // Snapshot previous authoritative physics state before advancing this fixed tick
    m_prevBallCenter = m_ballState.center;
    m_prevPaddleX = m_paddleState.box.x;

    const float prevPaddleX = m_paddleState.box.x;
    updatePaddleInput( dtSec, remainingSteps );
    const float paddleDeltaX = m_paddleState.box.x - prevPaddleX;
    m_smoothedPaddleVelX = Physics::smoothPaddleVelX(
        m_smoothedPaddleVelX, paddleDeltaX, dtSec, m_arenaCfg.paddleMaxSpeed );

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
                paddleDeltaX,
                m_smoothedPaddleVelX );

            const float r = m_ballState.radius;
            m_ball->moveTo( m_ballState.center - glm::vec2( r ) );
            m_ball->setVelocity( m_ballState.vel );

            if ( ev.hitWall )
            {
                const float wallPitch = Resonance::impactPitchForSpeed( ev.wallImpactSpeed, false );
                queueCommandEvent( m_eventHandler, wxEVT_PONG, static_cast<int>( std::lround( wallPitch * 100.0f ) ) );
            }

            if ( ev.hitPaddle )
            {
                ++m_robotDryBounces;
                m_resonance.onPaddleHit();
                if ( ev.skillShot )
                {
                    m_paddleRecoil.trigger( Resonance::PaddleRecoil::kMaxOffsetPx );
                    m_paddleSkillFlashSec = 0.16f;
                    m_currentBallSpeed = std::clamp(
                        Physics::length( m_ballState.vel ),
                        m_arenaCfg.minSpeed,
                        m_arenaCfg.maxSpeed );
                    m_particles->spawnBurst(
                        ev.skillShotPos,
                        ev.skillShotDir,
                        18u,
                        glm::vec4( 1.0f, 0.88f, 0.45f, 1.0f ) );
                }
                else
                {
                    m_paddleRecoil.trigger();
                }
                const float paddlePitch = Resonance::impactPitchForSpeed( ev.paddleImpactSpeed, ev.skillShot );
                queueCommandEvent( m_eventHandler, wxEVT_PING, static_cast<int>( std::lround( paddlePitch * 100.0f ) ) );
            }

            if ( !ev.hitBricks.empty() )
            {
                m_robotDryBounces = 0;
                bool hitThreshold = false;
                bool hitMax = false;
                bool armorHitOccurred = false;
                unsigned int emBoostCount = 0u;
                std::vector<std::size_t> directRedDetonators;

                auto awardDestruction = [ & ]( const brickPtr& destroyedBrick )
                {
                    m_explosions->add( destroyedBrick );
                    const auto role = BrickMechanics::roleForType( static_cast<unsigned char>( destroyedBrick->type() ) );
                    Resonance::HitResult hit = m_resonance.onBrickHit();
                    unsigned int scoreAward = hit.scoreDelta;
                    if ( role == BrickMechanics::BrickRole::ProtocolContainer )
                    {
                        // YELLOW protocol container: bonus resonance step + 2x score reward
                        const Resonance::HitResult bonus = m_resonance.onBrickHit();
                        hit.crossedHitStopThreshold = hit.crossedHitStopThreshold || bonus.crossedHitStopThreshold;
                        hit.multiplier = bonus.multiplier;
                        hit.streak = bonus.streak;
                        scoreAward = bonus.scoreDelta * 2u;
                    }
                    if ( hit.crossedHitStopThreshold )
                    {
                        hitThreshold = true;
                        if ( hit.multiplier >= Resonance::kMaxMultiplier )
                            hitMax = true;
                    }
                    queueCommandEvent(
                        m_eventHandler,
                        wxEVT_CURRENT_SCORE_INCREASED,
                        static_cast<int>( scoreAward ),
                        static_cast<long>( hit.streak ) );
                };

                for ( const std::size_t idx : ev.hitBricks )
                {
                    if ( idx < brickList.size() )
                    {
                        const auto& brick = brickList[ idx ];
                        if ( !brick->isAlive() )
                            continue;

                        const bool destroyed = brick->takeDamage( 1u );
                        if ( !destroyed )
                        {
                            armorHitOccurred = true;
                            continue;
                        }

                        const auto role = BrickMechanics::roleForType( static_cast<unsigned char>( brick->type() ) );
                        if ( role == BrickMechanics::BrickRole::EmAccelerator )
                            ++emBoostCount;
                        else if ( role == BrickMechanics::BrickRole::ReactiveCell )
                            directRedDetonators.push_back( idx );

                        awardDestruction( brick );
                    }
                }

                // RED reactive energy cells: depth-capped 3x3 chain detonation
                if ( !directRedDetonators.empty() )
                {
                    std::vector<BrickMechanics::BlastBrick> blastGrid;
                    blastGrid.reserve( brickList.size() );
                    for ( const auto& b : brickList )
                    {
                        const auto box = b->bounds();
                        blastGrid.push_back( BrickMechanics::BlastBrick {
                            { box.x + 0.5f * box.w, box.y + 0.5f * box.h },
                            { box.w, box.h },
                            static_cast<unsigned char>( b->type() ),
                            b->hp(),
                            b->isAlive()
                        } );
                    }

                    const BrickMechanics::BlastResult blast = BrickMechanics::resolveBlastChain(
                        blastGrid,
                        directRedDetonators,
                        BrickMechanics::kMaxChainDepth );

                    for ( const auto& [ idx, dmg ] : blast.survivedDamaged )
                    {
                        if ( idx < brickList.size() && brickList[ idx ]->isAlive() )
                        {
                            brickList[ idx ]->takeDamage( dmg );
                            armorHitOccurred = true;
                        }
                    }

                    for ( const std::size_t idx : blast.destroyedIndices )
                    {
                        if ( idx < brickList.size() && brickList[ idx ]->isAlive() )
                        {
                            brickList[ idx ]->kill();
                            awardDestruction( brickList[ idx ] );
                        }
                    }

                    if ( !blast.destroyedIndices.empty() || !blast.survivedDamaged.empty() )
                    {
                        hitThreshold = true;
                        hitMax = true;
                    }
                }

                // PURPLE EM accelerator: direct-hit velocity surge (strictly clamped to arena maxSpeed)
                if ( emBoostCount > 0u )
                {
                    float spd = Physics::length( m_ballState.vel );
                    for ( unsigned int i = 0u; i < emBoostCount; ++i )
                        spd = BrickMechanics::applyEmBoost( spd, m_arenaCfg.minSpeed, m_arenaCfg.maxSpeed );
                    m_ballState.vel = Physics::normalizeSafe( m_ballState.vel, { 0.0f, 1.0f } ) * spd;
                    m_ball->setVelocity( m_ballState.vel );
                }

                m_currentBallSpeed = std::clamp(
                    Physics::length( m_ballState.vel ),
                    m_arenaCfg.minSpeed,
                    m_arenaCfg.maxSpeed );

                if ( armorHitOccurred )
                    queueCommandEvent( m_eventHandler, wxEVT_ARMOR_HIT );

                const bool justCompleted = m_bricks->empty();
                m_isRoundCompleted = justCompleted;
                if ( hitThreshold || justCompleted )
                {
                    m_pendingImpact = true;
                    m_pendingImpactMaxOrFinal = m_pendingImpactMaxOrFinal || hitMax || justCompleted;
                }
            }

            if ( ev.ballLost )
            {
                stop();
                updateBallPosition( m_board->bounds(), true );
                queueCommandEvent( m_eventHandler, wxEVT_BALL_LOST );
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
    const float visualPaddleY = ( std::max )( 0.0f, authPaddleY - m_paddleRecoil.offsetY );

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
        m_board->moveTo( interpPaddleX, visualPaddleY );
    if ( r > 0.0f )
        m_ball->moveTo( interpBallCenter - glm::vec2( r ) );

    m_renderer->selectShader();

    if ( m_paddleSkillFlashSec > 0.0f )
    {
        const float t = std::clamp( m_paddleSkillFlashSec / 0.16f, 0.0f, 1.0f );
        const glm::vec3 tint = glm::mix( glm::vec3( 1.0f ), glm::vec3( 1.50f, 1.38f, 1.08f ), t );
        m_board->drawTinted( m_renderer, tint );
    }
    else
    {
        m_board->draw( m_renderer );
    }

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
