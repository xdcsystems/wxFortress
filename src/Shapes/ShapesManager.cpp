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
DEFINE_LOCAL_EVENT_TYPE( wxEVT_PROTOCOL_ACTIVATED )
DEFINE_LOCAL_EVENT_TYPE( wxEVT_TELEMETRY_SIGNAL )

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

    BallInstance primary {};
    primary.state.radius = m_ball->radius();
    m_balls.push_back( primary );

    if ( wxTheApp && wxTheApp->argc > 1 )
    {
        const std::string param { wxTheApp->argv[ 1 ] };
        m_isRobot = param == "--robot";
    }
}

void ShapesManager::loadLevel( unsigned short level, unsigned short stage )
{
    m_stage = stage;
    m_resonance.reset();
    m_paddleRecoil.reset();
    m_protocols.reset();
    m_capsules.clear();
    m_smoothedPaddleVelX = 0.0f;
    m_paddleSkillFlashSec = 0.0f;
    m_pendingImpact = false;
    m_pendingImpactMaxOrFinal = false;
    m_pendingHighSpeedCapture = false;
    m_pendingHighSpeedChainBlast = false;
    m_pendingHighSpeedResonance = 1u;
    m_plasmaPeakAnnounced = false;
    m_roundSkillShots = 0u;
    m_vectorCutAnnounced = false;

    // Progressive Stage 1 -> N escalation with bounded tension sawtooth ceiling
    const StageDirector::StageProfile stageProfile = StageDirector::profileForStage( m_stage );
    const float stageFloor = kInitialBallSpeedPxPerSec + stageProfile.startSpeedBonusPx;
    m_currentBallSpeed = std::clamp( m_currentBallSpeed, stageFloor, stageProfile.levelStartMaxSpeed );

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

bool ShapesManager::consumeRoundFinishTelemetry( bool& outLastKillChainBlast, unsigned int& outResonanceMult ) noexcept
{
    if ( !m_pendingHighSpeedCapture )
        return false;
    outLastKillChainBlast = m_pendingHighSpeedChainBlast;
    outResonanceMult = m_pendingHighSpeedResonance;
    m_pendingHighSpeedCapture = false;
    m_pendingHighSpeedChainBlast = false;
    m_pendingHighSpeedResonance = 1u;
    return true;
}

bool ShapesManager::hasMagLockedBalls() const noexcept
{
    if ( m_isRoundCompleted )
        return false;
    for ( const auto& b : m_balls )
    {
        if ( b.magLocked )
            return true;
    }
    return false;
}

bool ShapesManager::releaseMagLockedBalls() noexcept
{
    bool released = false;
    for ( auto& b : m_balls )
    {
        if ( !b.magLocked )
            continue;
        b.magLocked = false;
        b.robotHoldSec = 0.0f;
        const float spd = std::clamp(
            Physics::length( b.magLaunchVel ),
            m_arenaCfg.minSpeed,
            m_arenaCfg.maxSpeed );
        const Physics::Vec2 dir = Physics::normalizeSafe( b.magLaunchVel, { 0.35f, 0.93f } );
        b.state.vel = Physics::detail::enforceMinVerticalSlope( dir * spd, m_arenaCfg.minAbsSinAngle );
        if ( b.state.vel.y < 0.0f )
            b.state.vel.y = -b.state.vel.y;
        released = true;
    }
    return released;
}

void ShapesManager::syncPhysBricksFromBricks()
{
    const auto& brickList = m_bricks->items();
    m_physBricks.clear();
    m_physBricks.reserve( brickList.size() );
    for ( const auto& brick : brickList )
    {
        m_physBricks.push_back( Physics::BrickItem { brick->bounds(), brick->isAlive(), brick->hp() } );
    }
}

void ShapesManager::activateProtocol( AiProtocols::ProtocolKind kind )
{
    if ( kind == AiProtocols::ProtocolKind::None )
        return;

    const StageDirector::StageProfile stageProfile = StageDirector::profileForStage( m_stage );
    m_protocols.activate( kind, stageProfile.magLockDurationSec );

    if ( kind == AiProtocols::ProtocolKind::Split && !m_balls.empty() )
    {
        // Prefer an unlocked ball as split source; if all locked, use the first one.
        std::size_t srcIdx = 0u;
        for ( std::size_t i = 0u; i < m_balls.size(); ++i )
        {
            if ( !m_balls[ i ].magLocked )
            {
                srcIdx = i;
                break;
            }
        }
        const bool srcLocked = m_balls[ srcIdx ].magLocked;
        Physics::BallState srcState = m_balls[ srcIdx ].state;
        if ( srcLocked )
            srcState.vel = m_balls[ srcIdx ].magLaunchVel;

        const auto clones = AiProtocols::createSplitBalls( srcState, m_balls.size(), m_arenaCfg, AiProtocols::kMaxBalls, srcLocked );
        for ( const auto& cloneState : clones )
        {
            BallInstance inst {};
            inst.state = cloneState;
            inst.prevCenter = cloneState.center;
            inst.limiterSavedSpeed = m_balls[ srcIdx ].limiterSavedSpeed;
            m_balls.push_back( inst );
        }
    }
    else if ( kind == AiProtocols::ProtocolKind::LimiterOff )
    {
        for ( auto& b : m_balls )
        {
            Physics::Vec2& v = b.magLocked ? b.magLaunchVel : b.state.vel;
            const float curSpd = Physics::length( v );
            if ( b.limiterSavedSpeed <= 0.0f )
                b.limiterSavedSpeed = std::clamp( curSpd, m_arenaCfg.minSpeed, m_arenaCfg.maxSpeed );
            const float boosted = AiProtocols::applyLimiterBoost( b.limiterSavedSpeed, m_arenaCfg.minSpeed, m_arenaCfg.maxSpeed );
            v = Physics::normalizeSafe( v, { 0.35f, 0.93f } ) * boosted;
        }
    }

    const glm::vec3 tint = AiProtocols::capsuleTint( kind );
    m_particles->spawnBurst(
        { m_paddleState.box.center().x, m_paddleState.box.top() },
        1.0f,
        14u,
        glm::vec4( tint, 1.0f ) );

    queueCommandEvent( m_eventHandler, wxEVT_PROTOCOL_ACTIVATED, static_cast<int>( kind ) );
}

Physics::Aabb ShapesManager::updateBallPosition( const Physics::Aabb& boardBounds, bool syncPrev )
{
    if ( m_size.x < 1 || m_size.y < 1 )
        return {};

    // set single ball on board
    const auto ballBounds = m_ball->bounds();

    m_ball->moveTo(
        boardBounds.x + ( boardBounds.w - ballBounds.w ) / 2.0f,
        boardBounds.top() );

    m_balls.clear();
    BallInstance primary {};
    primary.state.center = m_ball->center();
    primary.state.radius = m_ball->radius();
    primary.state.vel = { 0.0f, 0.0f };
    primary.prevCenter = primary.state.center;
    m_balls.push_back( primary );

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
    const Physics::Vec2 vel {
        launchSpeed * std::cos( rad ),
        launchSpeed * std::sin( rad )
    };

    if ( m_balls.empty() )
    {
        BallInstance primary {};
        primary.state.center = m_ball->center();
        primary.state.radius = m_ball->radius();
        primary.prevCenter = primary.state.center;
        m_balls.push_back( primary );
    }
    m_balls.front().state.vel = vel;
    m_ball->setVelocity( vel );
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
        m_protocols.reset();
        m_capsules.clear();
        m_smoothedPaddleVelX = 0.0f;
        m_paddleSkillFlashSec = 0.0f;
        m_pendingImpact = false;
        m_pendingImpactMaxOrFinal = false;
        m_pendingHighSpeedCapture = false;
        m_pendingHighSpeedChainBlast = false;
        m_pendingHighSpeedResonance = 1u;
        m_plasmaPeakAnnounced = false;
        m_roundSkillShots = 0u;
        m_vectorCutAnnounced = false;

        m_balls.clear();
        BallInstance primary {};
        primary.state.center = m_ball->center();
        primary.state.radius = m_ball->radius();
        primary.prevCenter = primary.state.center;
        m_balls.push_back( primary );
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
    m_particleOrbCursor = 0u;
    m_paddleState.speed = 0.0f;
    m_resonance.reset();
    m_paddleRecoil.reset();
    m_protocols.reset();
    m_capsules.clear();
    m_smoothedPaddleVelX = 0.0f;
    m_paddleSkillFlashSec = 0.0f;
    m_pendingImpact = false;
    m_pendingImpactMaxOrFinal = false;
    m_pendingHighSpeedCapture = false;
    m_pendingHighSpeedChainBlast = false;
    m_pendingHighSpeedResonance = 1u;
    m_plasmaPeakAnnounced = false;
    m_roundSkillShots = 0u;
    m_vectorCutAnnounced = false;

    if ( m_balls.empty() )
    {
        BallInstance primary {};
        primary.state.center = m_ball->center();
        primary.state.radius = m_ball->radius();
        m_balls.push_back( primary );
    }
    else
    {
        m_balls.resize( 1u );
    }
    m_balls.front().state.vel = { 0.0f, 0.0f };
    m_balls.front().magLocked = false;
    m_balls.front().robotHoldSec = 0.0f;
    m_balls.front().limiterSavedSpeed = 0.0f;

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
    for ( auto& b : m_balls )
        b.prevCenter = b.state.center;
    m_prevPaddleX = m_paddleState.box.x;
}

void ShapesManager::updatePaddleInput( float dtSec, int remainingSteps )
{
    Physics::ArenaConfig paddleCfg = m_arenaCfg;
    const float speedScale = m_protocols.paddleSpeedScale();
    paddleCfg.paddleAccel *= speedScale;
    paddleCfg.paddleMaxSpeed *= speedScale;

    int moveDir = 0;
    if ( wxGetKeyState( WXK_LEFT ) )
        moveDir = -1;
    else if ( wxGetKeyState( WXK_RIGHT ) )
        moveDir = +1;

    if ( moveDir != 0 )
    {
        m_hasMouseTarget = false;
        Physics::updatePaddle( m_paddleState, moveDir, dtSec, paddleCfg );
    }
    else if ( m_hasMouseTarget )
    {
        if ( std::isfinite( m_mouseTargetX ) )
        {
            const float maxX = ( std::max )( 0.0f, paddleCfg.width - m_paddleState.box.w );
            const float clampedTargetX = std::clamp(
                m_mouseTargetX - 0.5f * m_paddleState.box.w, 0.0f, maxX );

            if ( m_protocols.isOverheatActive() )
            {
                // Apply real overheat speed limit to mouse control as well (risk-reward parity).
                const float maxStep = paddleCfg.paddleMaxSpeed * dtSec;
                const float diff = clampedTargetX - m_paddleState.box.x;
                if ( std::fabs( diff ) <= maxStep )
                {
                    m_paddleState.box.x = clampedTargetX;
                    if ( remainingSteps <= 1 )
                        m_hasMouseTarget = false;
                }
                else
                {
                    m_paddleState.box.x = std::clamp(
                        m_paddleState.box.x + ( diff > 0.0f ? maxStep : -maxStep ),
                        0.0f,
                        maxX );
                }
                m_paddleState.speed = 0.0f;
            }
            else if ( remainingSteps <= 1 )
            {
                Physics::setPaddleTargetX( m_paddleState, m_mouseTargetX, paddleCfg );
                m_hasMouseTarget = false;
            }
            else
            {
                const float stepX = m_paddleState.box.x +
                    ( clampedTargetX - m_paddleState.box.x ) / static_cast<float>( remainingSteps );
                m_paddleState.box.x = std::clamp( stepX, 0.0f, maxX );
                m_paddleState.speed = 0.0f;
            }
        }
        else
        {
            m_hasMouseTarget = false;
        }
    }
    else
    {
        Physics::updatePaddle( m_paddleState, 0, dtSec, paddleCfg );
    }

    if ( isRunning() && m_isRobot && moveDir == 0 && !m_balls.empty() )
    {
        m_robotBallScratch.clear();
        m_robotBallScratch.reserve( m_balls.size() );
        for ( const auto& b : m_balls )
            m_robotBallScratch.push_back( b.state );
        const std::size_t primaryIdx = AiProtocols::selectPrimaryBallIndex( m_robotBallScratch, m_paddleState.box.top() );
        Physics::updateRobotPaddle(
            m_paddleState,
            m_balls[ primaryIdx ].state,
            m_physBricks,
            dtSec,
            paddleCfg,
            m_robotDryBounces,
            !m_protocols.isOverheatActive() );
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
    m_explosions->update( dtSec );

    if ( m_paddleState.box.w <= 0.0f || m_paddleState.box.h <= 0.0f )
    {
        m_paddleState.box = m_board->bounds();
        syncPreviousState();
    }

    const auto& brickList = m_bricks->items();
    if ( isRunning() && !m_isRoundCompleted )
    {
        syncPhysBricksFromBricks();
    }

    // Snapshot previous authoritative physics state before advancing this fixed tick
    for ( auto& b : m_balls )
        b.prevCenter = b.state.center;
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
            const AiProtocols::StepOutcome protoStep = m_protocols.step( dtSec );
            if ( protoStep.magLockExpired && releaseMagLockedBalls() )
                queueCommandEvent( m_eventHandler, wxEVT_PING, 118 );
            if ( protoStep.limiterExpired )
            {
                for ( auto& b : m_balls )
                {
                    if ( b.limiterSavedSpeed <= 0.0f )
                        continue;
                    Physics::Vec2& v = b.magLocked ? b.magLaunchVel : b.state.vel;
                    const float curSpd = Physics::length( v );
                    const float restored = AiProtocols::restoreLimiterSpeed(
                        curSpd, b.limiterSavedSpeed, m_arenaCfg.minSpeed, m_arenaCfg.maxSpeed );
                    v = Physics::normalizeSafe( v, { 0.35f, 0.93f } ) * restored;
                    b.limiterSavedSpeed = 0.0f;
                }
            }

            // Advance falling protocol capsules and activate any caught by the manipulator
            const auto caughtProtocols = AiProtocols::stepCapsules( m_capsules, m_paddleState.box, dtSec );
            for ( const auto kind : caughtProtocols )
                activateProtocol( kind );

            static thread_local std::mt19937 protocolRng( std::random_device {}() );
            std::uniform_int_distribution<unsigned int> protocolRollDist( 0u, 99u );

            bool hitWallAny = false;
            float maxWallSpeed = 0.0f;
            bool hitPaddleAny = false;
            bool skillShotAny = false;
            float maxPaddleSpeed = 0.0f;
            bool armorHitOccurred = false;
            bool hitThreshold = false;
            bool hitMax = false;
            bool lastKillWasChainBlast = false;

            const float ballRadius = m_ball->radius();
            const unsigned int scoreMult = m_protocols.scoreMultiplier();
            const bool pierceActive = m_protocols.isPierceActive();
            std::vector<bool> lostFlags( m_balls.size(), false );

            for ( std::size_t ballIdx = 0u; ballIdx < m_balls.size(); ++ballIdx )
            {
                BallInstance& inst = m_balls[ ballIdx ];
                inst.state.radius = ballRadius;

                if ( inst.magLocked )
                {
                    const float minX = ballRadius;
                    const float maxX = ( std::max )( minX, m_arenaCfg.width - ballRadius );
                    inst.state.center.x = std::clamp( m_paddleState.box.x + inst.magOffsetX, minX, maxX );
                    inst.state.center.y = m_paddleState.box.top() + ballRadius + 1e-3f;

                    if ( m_isRobot )
                    {
                        inst.robotHoldSec += dtSec;
                        if ( inst.robotHoldSec >= AiProtocols::kRobotMagHoldSec )
                        {
                            if ( releaseMagLockedBalls() )
                                queueCommandEvent( m_eventHandler, wxEVT_PING, 118 );
                        }
                    }
                    continue;
                }

                const Physics::StepEvents ev = Physics::stepBall(
                    inst.state,
                    m_paddleState.box,
                    m_physBricks,
                    dtSec,
                    m_arenaCfg,
                    paddleDeltaX,
                    m_smoothedPaddleVelX,
                    pierceActive );

                if ( ev.hitWall )
                {
                    hitWallAny = true;
                    maxWallSpeed = ( std::max )( maxWallSpeed, ev.wallImpactSpeed );
                }

                if ( ev.hitPaddle )
                {
                    hitPaddleAny = true;
                    maxPaddleSpeed = ( std::max )( maxPaddleSpeed, ev.paddleImpactSpeed );
                    ++m_robotDryBounces;
                    m_resonance.onPaddleHit();

                    if ( ev.skillShot )
                    {
                        skillShotAny = true;
                        m_paddleRecoil.trigger( Resonance::PaddleRecoil::kMaxOffsetPx );
                        m_paddleSkillFlashSec = 0.16f;
                        m_currentBallSpeed = std::clamp(
                            Physics::length( inst.state.vel ),
                            m_arenaCfg.minSpeed,
                            m_arenaCfg.maxSpeed );
                        m_particles->spawnBurst(
                            ev.skillShotPos,
                            ev.skillShotDir,
                            18u,
                            glm::vec4( 1.0f, 0.88f, 0.45f, 1.0f ) );
                        ++m_roundSkillShots;
                        if ( !m_vectorCutAnnounced && m_roundSkillShots >= StageDirector::kSkillShotTelemetryThreshold )
                        {
                            m_vectorCutAnnounced = true;
                            queueCommandEvent(
                                m_eventHandler,
                                wxEVT_TELEMETRY_SIGNAL,
                                static_cast<int>( StageDirector::TelemetryEvent::VectorCut ) );
                        }
                    }
                    else
                    {
                        m_paddleRecoil.trigger();
                    }

                    if ( ev.hitPaddleTop && m_protocols.isMagLockActive() && m_protocols.consumeMagLockCharge() )
                    {
                        inst.magLocked = true;
                        inst.robotHoldSec = 0.0f;
                        inst.magLaunchVel = inst.state.vel;
                        inst.state.vel = { 0.0f, 0.0f };
                        const float maxOff = ( std::max )( ballRadius, m_paddleState.box.w - ballRadius );
                        inst.magOffsetX = std::clamp( inst.state.center.x - m_paddleState.box.x, ballRadius, maxOff );
                        inst.state.center.y = m_paddleState.box.top() + ballRadius + 1e-3f;
                    }
                }

                if ( !ev.hitBricks.empty() )
                {
                    m_robotDryBounces = 0;
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
                            // YELLOW protocol container: bonus resonance step + 2x score + protocol capsule drop
                            const Resonance::HitResult bonus = m_resonance.onBrickHit();
                            hit.crossedHitStopThreshold = hit.crossedHitStopThreshold || bonus.crossedHitStopThreshold;
                            hit.multiplier = bonus.multiplier;
                            hit.streak = bonus.streak;
                            scoreAward = bonus.scoreDelta * 2u;

                            if ( m_protocols.spawnCooldownSec <= 0.0f &&
                                 m_capsules.size() < AiProtocols::kMaxActiveCapsules )
                            {
                                const auto box = destroyedBrick->bounds();
                                const auto kind = AiProtocols::selectNextProtocol(
                                    m_balls.size(),
                                    m_stageIncidents,
                                    protocolRollDist( protocolRng ) );
                                AiProtocols::ProtocolCapsule cap {};
                                cap.kind = kind;
                                cap.box = {
                                    box.x + 0.5f * ( box.w - AiProtocols::kCapsuleWidth ),
                                    box.y + 0.5f * ( box.h - AiProtocols::kCapsuleHeight ),
                                    AiProtocols::kCapsuleWidth,
                                    AiProtocols::kCapsuleHeight
                                };
                                cap.fallSpeed = AiProtocols::kCapsuleFallSpeed * StageDirector::profileForStage( m_stage ).capsuleFallScale;
                                cap.active = true;
                                m_capsules.push_back( cap );
                                m_protocols.spawnCooldownSec = AiProtocols::kCapsuleSpawnCooldownSec;
                            }
                        }
                        scoreAward *= scoreMult;

                        if ( hit.multiplier >= Resonance::kMaxMultiplier && !m_plasmaPeakAnnounced )
                        {
                            m_plasmaPeakAnnounced = true;
                            queueCommandEvent(
                                m_eventHandler,
                                wxEVT_TELEMETRY_SIGNAL,
                                static_cast<int>( StageDirector::TelemetryEvent::PlasmaPeak ) );
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

                            bool destroyed = false;
                            if ( pierceActive )
                            {
                                brick->kill();
                                destroyed = true;
                            }
                            else
                            {
                                destroyed = brick->takeDamage( 1u );
                            }

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

                            lastKillWasChainBlast = false;
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
                                lastKillWasChainBlast = true;
                                awardDestruction( brickList[ idx ] );
                            }
                        }

                        if ( !blast.destroyedIndices.empty() || !blast.survivedDamaged.empty() )
                        {
                            hitThreshold = true;
                            hitMax = true;
                            syncPhysBricksFromBricks();
                        }
                    }

                    // PURPLE EM accelerator: direct-hit velocity surge (strictly clamped to arena maxSpeed)
                    if ( emBoostCount > 0u )
                    {
                        float spd = Physics::length( inst.state.vel );
                        for ( unsigned int i = 0u; i < emBoostCount; ++i )
                            spd = BrickMechanics::applyEmBoost( spd, m_arenaCfg.minSpeed, m_arenaCfg.maxSpeed );
                        inst.state.vel = Physics::normalizeSafe( inst.state.vel, { 0.0f, 1.0f } ) * spd;
                    }

                    const float rawSpd = Physics::length( inst.state.vel );
                    const float baseSpd = m_protocols.isLimiterActive()
                        ? AiProtocols::restoreLimiterSpeed( rawSpd, inst.limiterSavedSpeed, m_arenaCfg.minSpeed, m_arenaCfg.maxSpeed )
                        : rawSpd;
                    m_currentBallSpeed = std::clamp( baseSpd, m_arenaCfg.minSpeed, m_arenaCfg.maxSpeed );

                    if ( m_bricks->empty() )
                        break;
                }

                if ( ev.ballLost )
                    lostFlags[ ballIdx ] = true;
            }

            if ( hitWallAny )
            {
                const float wallPitch = Resonance::impactPitchForSpeed( maxWallSpeed, false );
                queueCommandEvent( m_eventHandler, wxEVT_PONG, static_cast<int>( std::lround( wallPitch * 100.0f ) ) );
            }

            if ( hitPaddleAny )
            {
                const float paddlePitch = Resonance::impactPitchForSpeed( maxPaddleSpeed, skillShotAny );
                queueCommandEvent( m_eventHandler, wxEVT_PING, static_cast<int>( std::lround( paddlePitch * 100.0f ) ) );
            }

            if ( armorHitOccurred )
                queueCommandEvent( m_eventHandler, wxEVT_ARMOR_HIT );

            const bool justCompleted = m_bricks->empty();
            if ( justCompleted )
            {
                m_isRoundCompleted = true;
                releaseMagLockedBalls();
                m_protocols.reset();
                m_capsules.clear();
                m_pendingImpact = true;
                m_pendingImpactMaxOrFinal = true;
                m_pendingHighSpeedCapture = true;
                m_pendingHighSpeedChainBlast = lastKillWasChainBlast;
                m_pendingHighSpeedResonance = m_resonance.multiplier();
            }
            else
            {
                // Prune balls that fell below the chamber floor
                for ( std::size_t i = m_balls.size(); i > 0u; --i )
                {
                    if ( lostFlags[ i - 1u ] )
                        m_balls.erase( m_balls.begin() + static_cast<std::ptrdiff_t>( i - 1u ) );
                }

                if ( m_balls.empty() )
                {
                    stop();
                    updateBallPosition( m_board->bounds(), true );
                    queueCommandEvent( m_eventHandler, wxEVT_BALL_LOST );
                    return;
                }

                // Keep legacy m_ball sprite synchronized with lead ball
                m_ball->moveTo( m_balls.front().state.center - glm::vec2( ballRadius ) );
                m_ball->setVelocity( m_balls.front().state.vel );

                if ( hitThreshold )
                {
                    m_pendingImpact = true;
                    m_pendingImpactMaxOrFinal = m_pendingImpactMaxOrFinal || hitMax;
                }
            }
        }

        // Spawn particles at a capped 120 particles/sec round-robin across all active orbs (0 extra GPU load)
        if ( !m_isRoundCompleted )
        {
            m_particleSpawnAccum += dtSec * 120.0f;
            const unsigned int totalSpawn = static_cast<unsigned int>( m_particleSpawnAccum );
            m_particleSpawnAccum -= static_cast<float>( totalSpawn );
            if ( totalSpawn > 0u && !m_balls.empty() )
            {
                const float r = m_ball->radius();
                const glm::vec2 offset( r / 2.0f );
                for ( unsigned int s = 0u; s < totalSpawn; ++s )
                {
                    const auto& b = m_balls[ ( m_particleOrbCursor++ ) % m_balls.size() ];
                    if ( !b.magLocked )
                        m_particles->spawnTrail( b.state.center - glm::vec2( r ), b.state.vel, 1u, offset );
                }
            }
        }
        m_particles->stepParticles( dtSec );
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

    const float r = m_ball->radius();
    const glm::vec2 leadCenter = m_balls.empty() ? m_ball->center() : m_balls.front().state.center;

    // Restore authoritative physics positions on scope exit so no caller can ever read interpolated coords.
    const auto restorer = makeScopeExit( [ this, authPaddleX, authPaddleY, r, leadCenter ]() {
        if ( m_paddleState.box.w > 0.0f )
            m_board->moveTo( authPaddleX, authPaddleY );
        if ( r > 0.0f )
            m_ball->moveTo( leadCenter - glm::vec2( r ) );
    } );

    if ( m_paddleState.box.w > 0.0f )
        m_board->moveTo( interpPaddleX, visualPaddleY );

    m_renderer->selectShader();

    // Manipulator visual state: Skill Shot flash -> Overheat penalty -> Mag-Lock ready -> normal
    if ( m_paddleSkillFlashSec > 0.0f )
    {
        const float t = std::clamp( m_paddleSkillFlashSec / 0.16f, 0.0f, 1.0f );
        const glm::vec3 tint = glm::mix( glm::vec3( 1.0f ), glm::vec3( 1.50f, 1.38f, 1.08f ), t );
        m_board->drawTinted( m_renderer, tint );
    }
    else if ( m_protocols.isOverheatActive() )
    {
        m_board->drawTinted( m_renderer, glm::vec3( 1.20f, 0.45f, 0.40f ) );
    }
    else if ( m_protocols.isMagLockActive() || hasMagLockedBalls() )
    {
        m_board->drawTinted( m_renderer, glm::vec3( 0.65f, 1.15f, 0.80f ) );
    }
    else
    {
        m_board->draw( m_renderer );
    }

    if ( isRunning() )
    {
        m_particles->draw();
        m_renderer->selectShader();
    }

    if ( !m_isRoundCompleted )
    {
        m_bricks->draw( m_renderer );

        // Falling AI protocol capsules (GPU-first: reuses SimpleBoard texture + single SpriteRenderer VBO)
        for ( const auto& cap : m_capsules )
        {
            if ( !cap.active )
                continue;
            m_board->drawRectTinted(
                m_renderer,
                { cap.box.x, cap.box.y },
                { cap.box.w, cap.box.h },
                AiProtocols::capsuleTint( cap.kind ) );
        }

        // Active plasma orbs (1..3 balls) with protocol tint
        glm::vec3 ballTint { 1.0f, 1.0f, 1.0f };
        if ( m_protocols.isPierceActive() )
            ballTint = { 1.35f, 0.72f, 0.35f };
        else if ( m_protocols.isLimiterActive() )
            ballTint = { 1.25f, 1.15f, 0.65f };

        for ( const auto& b : m_balls )
        {
            const glm::vec2 interpCenter = glm::mix( b.prevCenter, b.state.center, alpha );
            m_ball->moveTo( interpCenter - glm::vec2( r ) );
            m_ball->drawTinted( m_renderer, ballTint );
        }
    }

    m_explosions->draw( m_renderer );
}
