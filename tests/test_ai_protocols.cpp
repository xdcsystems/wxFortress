#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "AiProtocols.hpp"
#include "BallPhysics.hpp"
#include "Resonance.hpp"

using namespace Shapes::Physics;

TEST( AiProtocolsTest, SplitCreatesAngledClonesUpToMaxBallsWithValidSlopeAndGuards )
{
    ArenaConfig cfg {};
    BallState lead {};
    lead.center = { 400.0f, 300.0f };
    lead.radius = 8.0f;
    lead.vel = { 120.0f, 240.0f };
    const float leadSpeed = length( lead.vel );

    const auto spawned = AiProtocols::createSplitBalls( lead, 1u, cfg );
    ASSERT_EQ( spawned.size(), 2u );

    for ( const auto& b : spawned )
    {
        EXPECT_NEAR( length( b.vel ), leadSpeed, 1e-2f );
        EXPECT_GE( std::fabs( b.vel.y ) / length( b.vel ), cfg.minAbsSinAngle - 1e-4f );
    }
    EXPECT_GT( spawned[ 0 ].vel.x, lead.vel.x );
    EXPECT_LT( spawned[ 1 ].vel.x, lead.vel.x );

    // From 2 balls -> spawns exactly 1 clone
    const auto oneMore = AiProtocols::createSplitBalls( lead, 2u, cfg );
    ASSERT_EQ( oneMore.size(), 1u );

    // Shallow locked ball with ensureUpward = true -> never spawns downward into paddle
    BallState shallowLocked = lead;
    shallowLocked.vel = { 280.0f, 60.0f };
    const auto upwardClones = AiProtocols::createSplitBalls( shallowLocked, 1u, cfg, AiProtocols::kMaxBalls, true );
    ASSERT_EQ( upwardClones.size(), 2u );
    for ( const auto& b : upwardClones )
        EXPECT_GT( b.vel.y, 0.0f );

    // Non-finite velocity is handled safely without NaN
    BallState nanBall = lead;
    nanBall.vel = { std::numeric_limits<float>::quiet_NaN(), 200.0f };
    const auto safeClones = AiProtocols::createSplitBalls( nanBall, 1u, cfg );
    ASSERT_EQ( safeClones.size(), 2u );
    for ( const auto& b : safeClones )
    {
        EXPECT_TRUE( std::isfinite( b.vel.x ) );
        EXPECT_TRUE( std::isfinite( b.vel.y ) );
    }

    // Already at kMaxBalls -> spawns nothing
    const auto none = AiProtocols::createSplitBalls( lead, AiProtocols::kMaxBalls, cfg );
    EXPECT_TRUE( none.empty() );
}

TEST( AiProtocolsTest, OverheatPierceTransitionsFromPierceToOverheatPenalty )
{
    AiProtocols::ActiveState state {};
    state.activate( AiProtocols::ProtocolKind::OverheatPierce );
    EXPECT_TRUE( state.isPierceActive() );
    EXPECT_FALSE( state.isOverheatActive() );
    EXPECT_FLOAT_EQ( state.paddleSpeedScale(), 1.0f );
    EXPECT_EQ( state.hudStatusText(), "PIERCE 5S" );

    // Advance 5.05 seconds in 0.05s steps -> Pierce expires and triggers 3s Overheat penalty
    for ( int i = 0; i < 101; ++i )
        state.step( 0.05f );

    EXPECT_FALSE( state.isPierceActive() );
    EXPECT_TRUE( state.isOverheatActive() );
    EXPECT_FLOAT_EQ( state.paddleSpeedScale(), AiProtocols::kOverheatPaddleSpeedScale );
    EXPECT_EQ( state.hudStatusText(), "OVERHEAT 3S" );

    // Advance 3.1 seconds -> Overheat clears
    for ( int i = 0; i < 62; ++i )
        state.step( 0.05f );

    EXPECT_FALSE( state.isOverheatActive() );
    EXPECT_FLOAT_EQ( state.paddleSpeedScale(), 1.0f );
    EXPECT_TRUE( state.hudStatusText().empty() );
}

TEST( AiProtocolsTest, PiercePassesThroughMultipleBricksAndStillBouncesOffWalls )
{
    ArenaConfig cfg {};
    cfg.height = 310.0f;
    BallState ball {};
    ball.center = { 200.0f, 200.0f };
    ball.radius = 8.0f;
    ball.vel = { 0.0f, 300.0f };

    // Two stacked bricks (3-HP armor + 1-HP standard) below top wall at y=310
    std::vector<BrickItem> bricks {
        BrickItem { Aabb { 170.0f, 220.0f, 60.0f, 20.0f }, true, 3u },
        BrickItem { Aabb { 170.0f, 250.0f, 60.0f, 20.0f }, true, 1u }
    };
    const Aabb paddle { 350.0f, 15.0f, 100.0f, 18.0f };

    bool wallBounced = false;
    for ( int i = 0; i < 60; ++i )
    {
        const StepEvents ev = stepBall( ball, paddle, bricks, 0.01f, cfg, 0.0f, 0.0f, /*pierceBricks=*/true );
        if ( ev.hitWall )
            wallBounced = true;
    }

    EXPECT_FALSE( bricks[ 0 ].alive );
    EXPECT_FALSE( bricks[ 1 ].alive );
    EXPECT_EQ( bricks[ 0 ].hp, 0u );
    EXPECT_TRUE( wallBounced );
    // Reflected downward off the top wall
    EXPECT_LT( ball.vel.y, -250.0f );
}

TEST( AiProtocolsTest, MagLockLastChargeKeepsTimerForAutoReleaseAndHud )
{
    AiProtocols::ActiveState state {};
    state.activate( AiProtocols::ProtocolKind::MagLock );
    EXPECT_TRUE( state.isMagLockActive() );
    EXPECT_EQ( state.magLockCharges, AiProtocols::kMagLockMaxCharges );

    // Consume all 3 charges
    for ( unsigned int c = 0u; c < AiProtocols::kMagLockMaxCharges; ++c )
        EXPECT_TRUE( state.consumeMagLockCharge() );

    // No further captures allowed, but timer stays active for the 3rd held ball
    EXPECT_FALSE( state.isMagLockActive() );
    EXPECT_FALSE( state.consumeMagLockCharge() );
    EXPECT_TRUE( state.isMagLockTimerActive() );
    EXPECT_EQ( state.hudStatusText( /*hasLockedBalls=*/true ), "MAG-LOCK 10S" );

    // Step until timer expires -> must still fire magLockExpired = true!
    bool expired = false;
    for ( int i = 0; i < 250; ++i )
    {
        if ( state.step( 0.05f ).magLockExpired )
            expired = true;
    }
    EXPECT_TRUE( expired );
    EXPECT_FALSE( state.isMagLockTimerActive() );
}

TEST( AiProtocolsTest, LimiterOffBoostsRestoresOnExpiryAndDoublesScore )
{
    AiProtocols::ActiveState state {};
    EXPECT_EQ( state.scoreMultiplier(), 1u );
    state.activate( AiProtocols::ProtocolKind::LimiterOff );
    EXPECT_TRUE( state.isLimiterActive() );
    EXPECT_EQ( state.scoreMultiplier(), 2u );

    const float boosted = AiProtocols::applyLimiterBoost( 300.0f, 60.0f, 1200.0f );
    EXPECT_NEAR( boosted, 354.0f, 1e-2f );

    const float restored = AiProtocols::restoreLimiterSpeed( boosted, 300.0f, 60.0f, 1200.0f );
    EXPECT_NEAR( restored, 300.0f, 1e-2f );

    const float capped = AiProtocols::applyLimiterBoost( 540.0f, 60.0f, 1200.0f );
    EXPECT_LE( capped, AiProtocols::kLimiterMaxSpeed + 1e-3f );

    // Already faster than limiter cap -> never slows down
    const float alreadyFast = AiProtocols::applyLimiterBoost( 640.0f, 60.0f, 1200.0f );
    EXPECT_FLOAT_EQ( alreadyFast, 640.0f );

    // Non-finite input safe
    const float safe = AiProtocols::applyLimiterBoost( std::numeric_limits<float>::quiet_NaN(), 60.0f, 1200.0f );
    EXPECT_FLOAT_EQ( safe, 60.0f );

    // Advance 8.1s -> limiter expires and scoreMultiplier returns to 1x
    bool limiterExpired = false;
    for ( int i = 0; i < 165; ++i )
    {
        if ( state.step( 0.05f ).limiterExpired )
            limiterExpired = true;
    }
    EXPECT_TRUE( limiterExpired );
    EXPECT_FALSE( state.isLimiterActive() );
    EXPECT_EQ( state.scoreMultiplier(), 1u );
}

TEST( AiProtocolsTest, CapsuleFallingCatchAndOffScreenPruning )
{
    std::vector<AiProtocols::ProtocolCapsule> capsules {
        AiProtocols::ProtocolCapsule { AiProtocols::ProtocolKind::Split, Aabb { 100.0f, 38.0f, 28.0f, 10.0f }, 150.0f, true },
        AiProtocols::ProtocolCapsule { AiProtocols::ProtocolKind::MagLock, Aabb { 500.0f, 2.0f, 28.0f, 10.0f }, 300.0f, true },
    };
    const Aabb paddle { 80.0f, 15.0f, 100.0f, 18.0f };

    const auto caught = AiProtocols::stepCapsules( capsules, paddle, 0.05f );
    ASSERT_EQ( caught.size(), 1u );
    EXPECT_EQ( caught[ 0 ], AiProtocols::ProtocolKind::Split );
    EXPECT_TRUE( capsules.empty() );
}

TEST( AiProtocolsTest, HitPaddleTopDistinguishesTopFromSideAndRobotRespectsOverheat )
{
    ArenaConfig cfg {};
    std::vector<BrickItem> bricks;
    const Aabb paddle { 300.0f, 15.0f, 100.0f, 18.0f };

    // Top hit sets hitPaddleTop = true
    BallState topBall { { 350.0f, 44.0f }, 8.0f, { 0.0f, -250.0f } };
    const StepEvents topEv = stepBall( topBall, paddle, bricks, 0.05f, cfg );
    EXPECT_TRUE( topEv.hitPaddle );
    EXPECT_TRUE( topEv.hitPaddleTop );

    // Side graze sets hitPaddle = true, hitPaddleTop = false
    BallState sideBall { { 286.0f, 24.0f }, 8.0f, { 260.0f, 20.0f } };
    const StepEvents sideEv = stepBall( sideBall, paddle, bricks, 0.05f, cfg );
    EXPECT_TRUE( sideEv.hitPaddle );
    EXPECT_FALSE( sideEv.hitPaddleTop );

    // Robot with allowEmergencyCatch = false (Overheat) cannot teleport to imminent landing X
    PaddleState robotPaddle { Aabb { 50.0f, 15.0f, 100.0f, 18.0f }, 0.0f };
    ArenaConfig overheatCfg = cfg;
    overheatCfg.paddleMaxSpeed *= AiProtocols::kOverheatPaddleSpeedScale;
    BallState urgentBall { { 600.0f, 50.0f }, 8.0f, { 0.0f, -400.0f } }; // tti < 0.08s
    updateRobotPaddle( robotPaddle, urgentBall, bricks, 0.01f, overheatCfg, 0u, /*allowEmergencyCatch=*/false );
    EXPECT_LT( robotPaddle.box.x, 60.0f ); // bounded step (~4.88 px), no teleport to x=550
}

TEST( AiProtocolsTest, AdaptiveSelectorHudPriorityAndRobotPrimaryFallback )
{
    // At max balls, Split roll redirects to LimiterOff
    EXPECT_EQ( AiProtocols::selectNextProtocol( AiProtocols::kMaxBalls, 0u, 10u ), AiProtocols::ProtocolKind::LimiterOff );
    // High breach count boosts MagLock stabilization
    EXPECT_EQ( AiProtocols::selectNextProtocol( 1u, 2u, 20u ), AiProtocols::ProtocolKind::MagLock );

    // Empty vector safe
    std::vector<BallState> emptyBalls;
    EXPECT_EQ( AiProtocols::selectPrimaryBallIndex( emptyBalls, 33.0f ), 0u );

    // All ascending -> selects lowest Y ball
    std::vector<BallState> ascending {
        BallState { { 200.0f, 350.0f }, 8.0f, { 0.0f, +200.0f } },
        BallState { { 400.0f, 180.0f }, 8.0f, { 0.0f, +200.0f } },
    };
    EXPECT_EQ( AiProtocols::selectPrimaryBallIndex( ascending, 33.0f ), 1u );

    // Descending -> selects smallest TTI
    std::vector<BallState> mixed {
        BallState { { 200.0f, 400.0f }, 8.0f, { 0.0f, -150.0f } },
        BallState { { 500.0f, 200.0f }, 8.0f, { 0.0f, -300.0f } },
        BallState { { 300.0f, 150.0f }, 8.0f, { 0.0f, +250.0f } },
    };
    EXPECT_EQ( AiProtocols::selectPrimaryBallIndex( mixed, 33.0f ), 1u );

    // Protocol HUD priority: OVERHEAT > PIERCE > LIMITER x2 > MAG-LOCK
    AiProtocols::ActiveState p {};
    p.activate( AiProtocols::ProtocolKind::MagLock );
    EXPECT_EQ( p.hudStatusText(), "MAG-LOCK 10S" );
    p.activate( AiProtocols::ProtocolKind::LimiterOff );
    EXPECT_EQ( p.hudStatusText(), "LIMITER x2 8S" );
    p.activate( AiProtocols::ProtocolKind::OverheatPierce );
    EXPECT_EQ( p.hudStatusText(), "PIERCE 5S" );

    // Zone 2 HUD priority: transient > AUTO MODE > activeProtocol > RECORD -N
    Resonance::HudStatus hud {};
    EXPECT_EQ( hud.resolveZone2( false, 50u, 100u, "" ).text, "RECORD -50" );
    EXPECT_EQ( hud.resolveZone2( false, 50u, 100u, "PIERCE 4S" ).text, "PIERCE 4S" );
    EXPECT_EQ( hud.resolveZone2( true, 50u, 100u, "PIERCE 4S" ).text, "AUTO MODE" );
    hud.showTransient( "PROTOCOL: PIERCE", 2.0 );
    EXPECT_EQ( hud.resolveZone2( true, 50u, 100u, "PIERCE 4S" ).text, "PROTOCOL: PIERCE" );
}
