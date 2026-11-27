#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "BallPhysics.hpp"

using namespace Shapes::Physics;

namespace {

bool isFiniteVec( const Vec2& v )
{
    return std::isfinite( v.x ) && std::isfinite( v.y );
}

class BallPhysicsTest : public ::testing::Test
{
    protected:
        ArenaConfig cfg {};
        Aabb paddle { 350.0f, 15.0f, 100.0f, 16.0f };

        void SetUp() override
        {
            cfg.width = 800.0f;
            cfg.height = 600.0f;
        }
};

} // namespace

TEST_F( BallPhysicsTest, DiagonalHitElongatedBrick )
{
    std::vector<BrickItem> bricks { { Aabb { 200.0f, 300.0f, 100.0f, 20.0f }, true } };
    BallState ball { { 215.0f, 285.0f }, 8.0f, { 0.0f, 300.0f } };
    const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
    EXPECT_EQ( ev.hitBricks.size(), 1u );
    EXPECT_FALSE( bricks[ 0 ].alive );
    EXPECT_LT( ball.vel.y, 0.0f );
    EXPECT_NEAR( ball.vel.x, 0.0f, 1e-3f );
}

TEST_F( BallPhysicsTest, LeftFaceOrientation )
{
    std::vector<BrickItem> bricks { { Aabb { 200.0f, 300.0f, 100.0f, 20.0f }, true } };
    BallState ball { { 185.0f, 310.0f }, 8.0f, { 400.0f, 100.0f } };
    const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
    EXPECT_EQ( ev.hitBricks.size(), 1u );
    EXPECT_LT( ball.vel.x, 0.0f );
    EXPECT_GT( ball.vel.y, 0.0f );
}

TEST_F( BallPhysicsTest, PaddleLeftEdgeBounce )
{
    std::vector<BrickItem> bricks;
    BallState ball { { 360.0f, 45.0f }, 8.0f, { 200.0f, -300.0f } };
    const float spdBefore = length( ball.vel );
    const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
    EXPECT_TRUE( ev.hitPaddle );
    EXPECT_LT( ball.vel.x, 0.0f );
    EXPECT_GT( ball.vel.y, 0.0f );
    EXPECT_NEAR( length( ball.vel ), spdBefore, 1e-2f );
}

TEST_F( BallPhysicsTest, PaddleSideGrazeWhileMovingUp )
{
    std::vector<BrickItem> bricks;
    BallState ball { { 338.0f, 20.0f }, 8.0f, { 250.0f, 100.0f } };
    const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
    EXPECT_TRUE( ev.hitPaddle );
    EXPECT_LT( ball.vel.x, 0.0f );
    EXPECT_TRUE( isFiniteVec( ball.center ) );
    EXPECT_TRUE( isFiniteVec( ball.vel ) );
}

TEST_F( BallPhysicsTest, SimultaneousTopRightCornerHit )
{
    std::vector<BrickItem> bricks;
    BallState ball { { 785.0f, 585.0f }, 8.0f, { 600.0f, 600.0f } };
    const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
    EXPECT_TRUE( ev.hitWall );
    EXPECT_LT( ball.vel.x, 0.0f );
    EXPECT_LT( ball.vel.y, 0.0f );
    EXPECT_LE( ball.center.x, cfg.width - ball.radius + 1e-3f );
    EXPECT_LE( ball.center.y, cfg.height - ball.radius + 1e-3f );
}

TEST_F( BallPhysicsTest, SeamBetweenTwoBricks )
{
    std::vector<BrickItem> bricks {
        { Aabb { 200.0f, 300.0f, 50.0f, 20.0f }, true },
        { Aabb { 250.0f, 300.0f, 50.0f, 20.0f }, true },
    };
    BallState ball { { 250.0f, 285.0f }, 8.0f, { 0.0f, 400.0f } };
    const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
    EXPECT_EQ( ev.hitBricks.size(), 2u );
    EXPECT_FALSE( bricks[ 0 ].alive );
    EXPECT_FALSE( bricks[ 1 ].alive );
    EXPECT_LT( ball.vel.y, 0.0f );
}

TEST_F( BallPhysicsTest, AntiTunnelingAtExtremeSpeed )
{
    std::vector<BrickItem> bricks { { Aabb { 300.0f, 300.0f, 80.0f, 12.0f }, true } };
    BallState ball { { 340.0f, 285.0f }, 6.0f, { 0.0f, 1200.0f } };
    const auto ev = stepBall( ball, paddle, bricks, 0.03f, cfg ); // 36 px in 1 step
    EXPECT_EQ( ev.hitBricks.size(), 1u );
    EXPECT_FALSE( bricks[ 0 ].alive );
    EXPECT_LT( ball.vel.y, 0.0f );
    EXPECT_LT( ball.center.y, 300.0f );
}

TEST_F( BallPhysicsTest, ZeroVelocityAndNormalizeSafety )
{
    const Vec2 n = normalizeSafe( { 0.0f, 0.0f } );
    EXPECT_TRUE( isFiniteVec( n ) );
    EXPECT_NEAR( length( n ), 1.0f, 1e-4f );

    std::vector<BrickItem> bricks;
    BallState ball { { 400.0f, 300.0f }, 8.0f, { 0.0f, 0.0f } };
    (void)stepBall( ball, paddle, bricks, 0.016f, cfg );
    EXPECT_TRUE( isFiniteVec( ball.center ) );
    EXPECT_TRUE( isFiniteVec( ball.vel ) );
    const Vec2 defaultVec;
    EXPECT_FLOAT_EQ( defaultVec.x, 0.0f );
    EXPECT_FLOAT_EQ( defaultVec.y, 0.0f );

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const Vec2 nNan = normalizeSafe( { nan, 1.0f }, { 0.0f, 1.0f } );
    EXPECT_FLOAT_EQ( nNan.x, 0.0f );
    EXPECT_FLOAT_EQ( nNan.y, 1.0f );
    const Vec2 nInf = normalizeSafe( { inf, 1.0f }, { 0.0f, 1.0f } );
    EXPECT_FLOAT_EQ( nInf.x, 0.0f );
    EXPECT_FLOAT_EQ( nInf.y, 1.0f );
    // On SSE2 float32, 1e30f^2 overflows to +Inf -> fallback {0, 1}; on 32-bit x87
    // 80-bit extended precision it stays finite -> exact unit vector {0.707, 0.707}.
    // Either way normalizeSafe must return a finite unit vector (never NaN/Inf).
    const Vec2 nOverflow = normalizeSafe( { 1e30f, 1e30f }, { 0.0f, 1.0f } );
    EXPECT_TRUE( isFiniteVec( nOverflow ) );
    EXPECT_NEAR( length( nOverflow ), 1.0f, 1e-4f );
    const Vec2 nTiny = normalizeSafe( { 1e-22f, 0.0f }, { 0.0f, 1.0f } );
    EXPECT_FLOAT_EQ( nTiny.x, 0.0f );
    EXPECT_FLOAT_EQ( nTiny.y, 1.0f );
}

TEST_F( BallPhysicsTest, BoundedPaddleAcceleration )
{
    PaddleState ps { paddle, 0.0f };
    for ( int i = 0; i < 200; ++i )
        updatePaddle( ps, +1, 0.016f, cfg );
    EXPECT_LE( ps.speed, cfg.paddleMaxSpeed + 1e-3f );
    EXPECT_LE( ps.box.right(), cfg.width + 1e-3f );
}

TEST_F( BallPhysicsTest, EnduranceSimulation100kTicks )
{
    std::vector<BrickItem> bricks;
    for ( int r = 0; r < 5; ++r )
        for ( int c = 0; c < 8; ++c )
            bricks.push_back( { Aabb { 80.0f + c * 80.0f, 380.0f + r * 28.0f, 76.0f, 22.0f }, true } );

    BallState ball { { 400.0f, 100.0f }, 8.0f, { 220.0f, 280.0f } };
    Aabb botPaddle = paddle;
    bool okAll = true;
    int totalBroken = 0;

    for ( int tick = 0; tick < 100000; ++tick )
    {
        const float offset = std::sin( tick * 0.013f ) * 42.0f;
        botPaddle.x = std::clamp( ball.center.x - 0.5f * botPaddle.w + offset, 0.0f, cfg.width - botPaddle.w );

        const auto ev = stepBall( ball, botPaddle, bricks, 0.016f, cfg );
        totalBroken += static_cast<int>( ev.hitBricks.size() );

        if ( !isFiniteVec( ball.center ) || !isFiniteVec( ball.vel ) )
        {
            okAll = false;
            break;
        }
        if ( ball.center.x < ball.radius - 1e-2f || ball.center.x > cfg.width - ball.radius + 1e-2f ||
             ball.center.y > cfg.height - ball.radius + 1e-2f )
        {
            okAll = false;
            break;
        }
        const float spd = length( ball.vel );
        if ( spd < cfg.minSpeed - 1e-2f || spd > cfg.maxSpeed + 1e-2f )
        {
            okAll = false;
            break;
        }
        if ( ev.ballLost )
        {
            ball.center = { 400.0f, 100.0f };
            ball.vel = { 210.0f, 290.0f };
        }
        if ( totalBroken > 0 && ( tick % 5000 == 0 ) )
        {
            for ( auto& b : bricks )
                b.alive = true;
        }
    }

    EXPECT_TRUE( okAll );
    EXPECT_GT( totalBroken, 50 );
}

TEST_F( BallPhysicsTest, PaddleMouseTrackingCenter )
{
    PaddleState ps { paddle, 300.0f };
    setPaddleTargetX( ps, 250.0f, cfg );
    EXPECT_NEAR( ps.box.x, 200.0f, 1e-3f );
    EXPECT_FLOAT_EQ( ps.speed, 0.0f );
}

TEST_F( BallPhysicsTest, PaddleMouseClampedToWalls )
{
    PaddleState ps { paddle, 0.0f };
    setPaddleTargetX( ps, -50.0f, cfg );
    EXPECT_FLOAT_EQ( ps.box.x, 0.0f );

    setPaddleTargetX( ps, 9999.0f, cfg );
    EXPECT_NEAR( ps.box.right(), cfg.width, 1e-3f );
}

TEST_F( BallPhysicsTest, PaddleMouseNonFiniteIgnored )
{
    PaddleState ps { paddle, 120.0f };
    const float prevX = ps.box.x;
    setPaddleTargetX( ps, std::numeric_limits<float>::quiet_NaN(), cfg );
    EXPECT_FLOAT_EQ( ps.box.x, prevX );
    EXPECT_FLOAT_EQ( ps.speed, 120.0f );
}

TEST_F( BallPhysicsTest, RobotAimsOffCenterFromVerticalDrop )
{
    // Ball falling strictly vertically (vx == 0), brick off to the right -> robot must offset paddle to aim right.
    std::vector<BrickItem> bricks { { Aabb { 580.0f, 380.0f, 60.0f, 20.0f }, true } };
    BallState ball { { 400.0f, 180.0f }, 8.0f, { 0.0f, -300.0f } };
    PaddleState ps { paddle, 0.0f };

    bool hitPaddle = false;
    for ( int tick = 0; tick < 80 && !hitPaddle; ++tick )
    {
        updateRobotPaddle( ps, ball, bricks, 0.016f, cfg, 0 );
        const auto ev = stepBall( ball, ps.box, bricks, 0.016f, cfg );
        if ( ev.hitPaddle )
            hitPaddle = true;
    }

    EXPECT_TRUE( hitPaddle );
    EXPECT_GT( ball.vel.x, 60.0f ); // Steered decisively toward the right brick, NOT vx == 0!
    EXPECT_GT( ball.vel.y, 0.0f );
}

TEST_F( BallPhysicsTest, RobotBreaksStalemateWithBankOrAngle )
{
    // Ball dropping straight down with no brick directly above -> dryBounces triggers angled shot.
    std::vector<BrickItem> bricks { { Aabb { 60.0f, 450.0f, 50.0f, 20.0f }, true } };
    BallState ball { { 400.0f, 200.0f }, 8.0f, { 0.0f, -260.0f } };
    const float targetCenter = computeRobotTargetX( ball, paddle, bricks, cfg, 3 );
    EXPECT_GT( std::fabs( targetCenter - 400.0f ), 8.0f );
}

TEST_F( BallPhysicsTest, RobotClearsMultiRowLevelWithoutStall )
{
    std::vector<BrickItem> bricks;
    for ( int r = 0; r < 3; ++r )
        for ( int c = 0; c < 6; ++c )
            bricks.push_back( { Aabb { 110.0f + c * 95.0f, 380.0f + r * 30.0f, 80.0f, 20.0f }, true } );

    BallState ball { { 400.0f, 120.0f }, 8.0f, { 0.0f, -240.0f } }; // Worst-case strictly vertical start!
    PaddleState ps { paddle, 0.0f };
    unsigned int dryBounces = 0;
    bool ballLost = false;

    for ( int tick = 0; tick < 25000; ++tick )
    {
        updateRobotPaddle( ps, ball, bricks, 0.016f, cfg, dryBounces );
        const auto ev = stepBall( ball, ps.box, bricks, 0.016f, cfg );
        if ( ev.hitPaddle )
            ++dryBounces;
        if ( !ev.hitBricks.empty() )
            dryBounces = 0;
        if ( ev.ballLost )
        {
            ballLost = true;
            break;
        }
        const bool allCleared = std::none_of( bricks.begin(), bricks.end(), []( const BrickItem& b ) { return b.alive; } );
        if ( allCleared )
            break;
    }

    const int remaining = static_cast<int>( std::count_if( bricks.begin(), bricks.end(), []( const BrickItem& b ) { return b.alive; } ) );
    EXPECT_FALSE( ballLost );
    EXPECT_EQ( remaining, 0 );
}

TEST_F( BallPhysicsTest, BrickHitSpeedIncrement125PxPerSec )
{
    EXPECT_FLOAT_EQ( cfg.speedStepOnBrick, 1.25f );

    std::vector<BrickItem> bricks {
        { Aabb { 200.0f, 300.0f, 50.0f, 20.0f }, true },
        { Aabb { 250.0f, 300.0f, 50.0f, 20.0f }, true },
    };
    BallState ball { { 250.0f, 285.0f }, 8.0f, { 0.0f, 187.5f } };
    const float spdBefore = length( ball.vel );
    const auto ev = stepBall( ball, paddle, bricks, 0.1f, cfg );
    EXPECT_EQ( ev.hitBricks.size(), 2u );
    EXPECT_NEAR( length( ball.vel ), spdBefore + 2.0f * 1.25f, 1e-3f );
}

TEST_F( BallPhysicsTest, NonFiniteDtAndNarrowArenaAreSafe )
{
    ArenaConfig cfg;
    BallState ball;
    ball.center = { 100.0f, 100.0f };
    ball.vel = { 200.0f, -200.0f };
    ball.radius = 10.0f;
    Aabb paddle { 80.0f, 20.0f, 80.0f, 16.0f };
    std::vector<BrickItem> bricks;

    const auto evNan = stepBall( ball, paddle, bricks, std::numeric_limits<float>::quiet_NaN(), cfg );
    EXPECT_FALSE( evNan.hitWall );
    EXPECT_FALSE( evNan.hitPaddle );
    EXPECT_FLOAT_EQ( ball.center.x, 100.0f );

    // Narrow arena (width < 2 * radius) must not trigger std::clamp UB
    cfg.width = 10.0f;
    ball.center = { 5.0f, 50.0f };
    ball.vel = { 300.0f, -100.0f };
    stepBall( ball, paddle, bricks, 0.016f, cfg );
    EXPECT_TRUE( std::isfinite( ball.center.x ) );
    EXPECT_TRUE( std::isfinite( ball.center.y ) );
}

TEST_F( BallPhysicsTest, FastHorizontalPaddleSweepCatchesBallWithoutTunneling )
{
    ArenaConfig cfg;
    BallState ball;
    ball.center = { 400.0f, 40.0f };
    ball.vel = { 0.0f, -250.0f };
    ball.radius = 10.0f;
    // Paddle swept rapidly from x=150 to x=550 (crossing x=400 during the frame)
    Aabb paddleEnd { 550.0f, 20.0f, 80.0f, 16.0f };
    std::vector<BrickItem> bricks;

    const auto ev = stepBall( ball, paddleEnd, bricks, 0.016f, cfg, 400.0f );
    EXPECT_TRUE( ev.hitPaddle );
    EXPECT_FALSE( ev.ballLost );
    EXPECT_GT( ball.vel.y, 0.0f );
    EXPECT_FALSE( detail::resolveOverlapCircleAabb( ball.center, ball.radius, paddleEnd ).hit );

    // Side sweep straight onto the ball must push it cleanly outside paddleEnd (no embedding)
    BallState sideBall;
    sideBall.center = { 400.0f, 28.0f };
    sideBall.vel = { 0.0f, -200.0f };
    sideBall.radius = 10.0f;
    Aabb sidePaddleEnd { 370.0f, 20.0f, 80.0f, 16.0f };
    const auto evSide = stepBall( sideBall, sidePaddleEnd, bricks, 0.016f, cfg, 220.0f );
    EXPECT_TRUE( evSide.hitPaddle );
    EXPECT_FALSE( detail::resolveOverlapCircleAabb( sideBall.center, sideBall.radius, sidePaddleEnd ).hit );
}
