#include <algorithm>
#include <cmath>
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
