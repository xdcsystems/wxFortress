#include <cmath>
#include <limits>
#include <vector>
#include <gtest/gtest.h>

#include "BallPhysics.hpp"
#include "Resonance.hpp"

TEST( SkillShotTest, SmoothPaddleVelXClampsAndHandlesNonFinite )
{
    using namespace Shapes::Physics;

    // Stationary input decays smoothly toward zero
    const float decayed = smoothPaddleVelX( 400.0f, 0.0f, 1.0f / 120.0f, 750.0f );
    EXPECT_GT( decayed, 0.0f );
    EXPECT_LT( decayed, 400.0f );

    // Huge single-tick mouse jump is clamped to +-paddleMaxSpeed
    const float clampedPos = smoothPaddleVelX( 0.0f, 500.0f, 1.0f / 120.0f, 750.0f );
    EXPECT_LE( clampedPos, 750.0f );
    EXPECT_GT( clampedPos, 100.0f );

    const float clampedNeg = smoothPaddleVelX( 0.0f, -500.0f, 1.0f / 120.0f, 750.0f );
    EXPECT_GE( clampedNeg, -750.0f );
    EXPECT_LT( clampedNeg, -100.0f );

    // Non-finite or zero dt returns safe finite value
    const float healed = smoothPaddleVelX( std::numeric_limits<float>::quiet_NaN(), 10.0f, 0.01f, 750.0f );
    EXPECT_TRUE( std::isfinite( healed ) );
    EXPECT_GT( healed, 0.0f );
    EXPECT_FLOAT_EQ( smoothPaddleVelX( std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(), 0.01f, 750.0f ), 0.0f );
    EXPECT_FLOAT_EQ( smoothPaddleVelX( 200.0f, 10.0f, 0.0f, 750.0f ), 200.0f );
}

TEST( SkillShotTest, StationaryOrCenterCatchDoesNotTriggerSkillShot )
{
    using namespace Shapes::Physics;

    ArenaConfig cfg {};
    const Aabb paddle { 300.0f, 15.0f, 100.0f, 14.0f }; // center X = 350, top Y = 29
    std::vector<BrickItem> bricks;

    // Case 1: Edge hit (x = 390, u = +0.8), but stationary paddle -> NO skill shot
    BallState ball1 {};
    ball1.center = { 390.0f, 40.0f };
    ball1.vel = { 0.0f, -300.0f };
    ball1.radius = 8.0f;
    const StepEvents ev1 = stepBall( ball1, paddle, bricks, 0.05f, cfg, 0.0f, 0.0f );
    EXPECT_TRUE( ev1.hitPaddle );
    EXPECT_FALSE( ev1.skillShot );
    EXPECT_NEAR( length( ball1.vel ), 300.0f, 1e-3f );

    // Case 2: Fast paddle cut (+400 px/s), but dead-center catch (u = 0) -> NO skill shot
    BallState ball2 {};
    ball2.center = { 350.0f, 40.0f };
    ball2.vel = { 0.0f, -300.0f };
    ball2.radius = 8.0f;
    const StepEvents ev2 = stepBall( ball2, paddle, bricks, 0.05f, cfg, 0.0f, 400.0f );
    EXPECT_TRUE( ev2.hitPaddle );
    EXPECT_FALSE( ev2.skillShot );
    EXPECT_NEAR( length( ball2.vel ), 300.0f, 1e-3f );
    // Controlled spin still slightly nudges horizontal velocity in cut direction
    EXPECT_GT( ball2.vel.x, 10.0f );
}

TEST( SkillShotTest, EdgeCutTriggersSkillShotImpulseWithinCapAndAngleBounds )
{
    using namespace Shapes::Physics;

    ArenaConfig cfg {};
    const Aabb paddle { 300.0f, 15.0f, 100.0f, 14.0f }; // center X = 350, top Y = 29
    std::vector<BrickItem> bricks;

    // Right-edge cut (x = 385, u = +0.7 >= 0.45) with +350 px/s effective paddle velocity
    BallState ball {};
    ball.center = { 385.0f, 40.0f };
    ball.vel = { 0.0f, -300.0f };
    ball.radius = 8.0f;

    const StepEvents ev = stepBall( ball, paddle, bricks, 0.05f, cfg, 0.0f, 350.0f );
    EXPECT_TRUE( ev.hitPaddle );
    EXPECT_TRUE( ev.skillShot );
    EXPECT_GT( ev.skillShotDir, 0.0f );
    EXPECT_GT( ev.paddleImpactSpeed, 300.0f );

    // Speed boosted by +12% (300 -> 336 px/s) and bounce angle strictly within maxPaddleBounceDeg (60 deg)
    EXPECT_NEAR( length( ball.vel ), 300.0f * cfg.skillShotSpeedMultiplier, 1e-2f );
    EXPECT_GT( ball.vel.y, 0.0f );
    const float cosMax = std::cos( glm::radians( cfg.maxPaddleBounceDeg ) );
    EXPECT_GE( ball.vel.y / length( ball.vel ), cosMax - 1e-4f );
}

TEST( SkillShotTest, SkillShotCapsSpeedAndNeverSlowsFasterBall )
{
    using namespace Shapes::Physics;

    ArenaConfig cfg {};
    const Aabb paddle { 300.0f, 15.0f, 100.0f, 14.0f };
    std::vector<BrickItem> bricks;

    // Ball near cap (500 px/s) caps at skillShotMaxSpeed (520 px/s)
    BallState ball1 {};
    ball1.center = { 315.0f, 40.0f }; // left edge u = -0.7
    ball1.vel = { 0.0f, -500.0f };
    ball1.radius = 8.0f;
    const StepEvents ev1 = stepBall( ball1, paddle, bricks, 0.05f, cfg, 0.0f, -400.0f );
    EXPECT_TRUE( ev1.skillShot );
    EXPECT_LT( ev1.skillShotDir, 0.0f );
    EXPECT_NEAR( length( ball1.vel ), cfg.skillShotMaxSpeed, 1e-2f );

    // Ball already faster than cap (600 px/s) keeps its speed (never slowed down)
    BallState ball2 {};
    ball2.center = { 315.0f, 40.0f };
    ball2.vel = { 0.0f, -600.0f };
    ball2.radius = 8.0f;
    const StepEvents ev2 = stepBall( ball2, paddle, bricks, 0.05f, cfg, 0.0f, -400.0f );
    EXPECT_TRUE( ev2.skillShot );
    EXPECT_NEAR( length( ball2.vel ), 600.0f, 1e-2f );
}

TEST( SkillShotTest, ImpactPitchScalesWithSpeedAndLiftsOnSkillShot )
{
    const float slow = Resonance::impactPitchForSpeed( 187.5f, false );
    const float mid  = Resonance::impactPitchForSpeed( 400.0f, false );
    const float fast = Resonance::impactPitchForSpeed( 650.0f, false );
    const float skill = Resonance::impactPitchForSpeed( 400.0f, true );

    EXPECT_GT( mid, slow );
    EXPECT_GT( fast, mid );
    EXPECT_GT( skill, mid + 0.10f );
    EXPECT_LE( Resonance::impactPitchForSpeed( 5000.0f, true ), 1.45f );
    EXPECT_GE( Resonance::impactPitchForSpeed( std::numeric_limits<float>::quiet_NaN(), false ), 0.85f );
}
