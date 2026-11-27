#include <algorithm>
#include <limits>
#include <vector>
#include <gtest/gtest.h>

#include "BallPhysics.hpp"
#include "BrickMechanics.hpp"

TEST( BrickMechanicsTest, RoleAndInitialHpByStage )
{
    using namespace BrickMechanics;

    EXPECT_EQ( roleForType( kTypeRed ), BrickRole::ReactiveCell );
    EXPECT_EQ( roleForType( kTypeBlue ), BrickRole::CompositeArmor );
    EXPECT_EQ( roleForType( kTypePurple ), BrickRole::EmAccelerator );
    EXPECT_EQ( roleForType( kTypeYellow ), BrickRole::ProtocolContainer );
    EXPECT_EQ( roleForType( kTypeAqua ), BrickRole::Standard );
    EXPECT_EQ( roleForType( kTypeGreen ), BrickRole::Standard );
    EXPECT_EQ( roleForType( kTypeOrange ), BrickRole::Standard );

    EXPECT_EQ( initialHpForType( kTypeNone, 0 ), 0u );
    EXPECT_EQ( initialHpForType( kTypeHalf, 0 ), 0u );
    EXPECT_EQ( initialHpForType( kTypeAqua, 0 ), 1u );
    EXPECT_EQ( initialHpForType( kTypeRed, 0 ), 1u );
    EXPECT_EQ( initialHpForType( kTypeBlue, 0 ), 2u );
    EXPECT_EQ( initialHpForType( kTypeBlue, 1 ), 3u );
    EXPECT_EQ( initialHpForType( kTypeBlue, 4 ), 3u );
}

TEST( BrickMechanicsTest, DamageTintReflectsFlashAndArmorState )
{
    using namespace BrickMechanics;

    const glm::vec3 intact = damageTint( 2u, 2u, 0.0f );
    EXPECT_FLOAT_EQ( intact.r, 1.0f );
    EXPECT_FLOAT_EQ( intact.g, 1.0f );
    EXPECT_FLOAT_EQ( intact.b, 1.0f );

    const glm::vec3 flashed = damageTint( 1u, 2u, 0.05f );
    EXPECT_GT( flashed.r, 1.0f );

    const glm::vec3 cracked = damageTint( 1u, 2u, 0.0f );
    EXPECT_LT( cracked.r, 0.6f );
    EXPECT_LT( cracked.g, 0.7f );

    const glm::vec3 stage2Mid = damageTint( 2u, 3u, 0.0f );
    EXPECT_GT( stage2Mid.r, cracked.r );
    EXPECT_LT( stage2Mid.r, intact.r );

    // NaN flash must safely fall back to normal damage tint
    const glm::vec3 nanFlash = damageTint( 1u, 2u, std::numeric_limits<float>::quiet_NaN() );
    EXPECT_FLOAT_EQ( nanFlash.r, cracked.r );
}

TEST( BrickMechanicsTest, EmBoostScalesClampsAndHandlesEdgeInputs )
{
    using namespace BrickMechanics;

    // Normal +10% boost below kEmMaxSpeed (520 px/s)
    EXPECT_NEAR( applyEmBoost( 300.0f, 187.5f, 1200.0f ), 330.0f, 1e-3f );

    // Capped at playable kEmMaxSpeed even when arena maxSpeed is 1200 px/s
    EXPECT_FLOAT_EQ( applyEmBoost( 500.0f, 187.5f, 1200.0f ), kEmMaxSpeed );

    // Never slows down a ball already faster than kEmMaxSpeed
    EXPECT_FLOAT_EQ( applyEmBoost( 600.0f, 187.5f, 1200.0f ), 600.0f );

    // Below minSpeed clamps up from safeMin
    EXPECT_NEAR( applyEmBoost( 50.0f, 187.5f, 1200.0f ), 187.5f * kEmSpeedMultiplier, 1e-3f );

    // maxSpeed < minSpeed safely clamps to minSpeed
    EXPECT_FLOAT_EQ( applyEmBoost( 300.0f, 200.0f, 100.0f ), 200.0f );

    // Non-finite inputs return safe finite speed
    EXPECT_FLOAT_EQ( applyEmBoost( std::numeric_limits<float>::quiet_NaN(), 187.5f, 1200.0f ), 187.5f );
    EXPECT_FLOAT_EQ( applyEmBoost( std::numeric_limits<float>::infinity(), 187.5f, 1200.0f ), 187.5f );
    EXPECT_NEAR( applyEmBoost( 300.0f, std::numeric_limits<float>::quiet_NaN(), 1200.0f ), 330.0f, 1e-3f );
    EXPECT_FLOAT_EQ( applyEmBoost( std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(), 1200.0f ), 0.0f );
}

TEST( BrickMechanicsTest, ReactiveBlastRespects3x3NeighborhoodAndDepthCap )
{
    using namespace BrickMechanics;

    // 5 bricks in a row: [0: RED(detonator)] [1: RED] [2: RED] [3: RED] [4: AQUA]
    constexpr float w = 57.0f;
    constexpr float h = 27.0f;
    std::vector<BlastBrick> row( 5 );
    for ( std::size_t i = 0; i < 5; ++i )
    {
        row[ i ].center = { w * ( static_cast<float>( i ) + 0.5f ), h * 0.5f };
        row[ i ].size = { w, h };
        row[ i ].typeVal = ( i < 4 ) ? kTypeRed : kTypeAqua;
        row[ i ].hp = ( i == 0 ) ? 0u : 1u;
        row[ i ].alive = ( i != 0 );
    }

    const BlastResult res = resolveBlastChain( row, { 0u }, kMaxChainDepth );
    EXPECT_TRUE( res.anySecondaryChain );
    EXPECT_EQ( res.destroyedIndices.size(), 2u );
    EXPECT_FALSE( row[ 1 ].alive );
    EXPECT_FALSE( row[ 2 ].alive );
    EXPECT_TRUE( row[ 3 ].alive );
    EXPECT_TRUE( row[ 4 ].alive );
}

TEST( BrickMechanicsTest, ReactiveBlastCracksAndOverlappingBlastsShatterBlueArmor )
{
    using namespace BrickMechanics;

    constexpr float w = 57.0f;
    constexpr float h = 27.0f;
    // Grid:
    //   [0: RED detonator @ (0,0)] [1: RED @ (1,0)]
    //   [2: BLUE 2-HP @ (0,1)]     [3: BLUE 3-HP @ (1,1) — hit by BOTH brick 0 and brick 1!]
    //   [4: AQUA @ (0,2) — vertical gap 2*h like Level 1, must NOT be hit by brick 0]
    std::vector<BlastBrick> grid {
        { { 0.5f * w, 0.5f * h }, { w, h }, kTypeRed,  0u, false },
        { { 1.5f * w, 0.5f * h }, { w, h }, kTypeRed,  1u, true  },
        { { 0.5f * w, 1.5f * h }, { w, h }, kTypeBlue, 2u, true  },
        { { 1.5f * w, 1.5f * h }, { w, h }, kTypeBlue, 3u, true  },
        { { 0.5f * w, 2.5f * h }, { w, h }, kTypeAqua, 1u, true  },
    };

    // Duplicate, out-of-bounds, and non-RED detonator indices must be ignored safely
    const BlastResult res = resolveBlastChain( grid, { 0u, 0u, 4u, 99u }, kMaxChainDepth );
    EXPECT_FALSE( grid[ 1 ].alive );
    EXPECT_FALSE( grid[ 2 ].alive );
    EXPECT_TRUE( grid[ 3 ].alive );
    EXPECT_EQ( grid[ 3 ].hp, 1u ); // 3-HP Stage 2 BLUE took 2 overlapping hits -> 1 HP left
    EXPECT_TRUE( grid[ 4 ].alive ); // 2*h vertical gap untouched

    ASSERT_EQ( res.survivedDamaged.size(), 1u );
    EXPECT_EQ( res.survivedDamaged[ 0 ].first, 3u );
    EXPECT_EQ( res.survivedDamaged[ 0 ].second, 2u );
}

TEST( BrickMechanicsTest, ReactiveBlastIgnoresNaNCoordinatesSafely )
{
    using namespace BrickMechanics;

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    std::vector<BlastBrick> grid {
        { { 50.0f, 50.0f }, { 57.0f, 27.0f }, kTypeRed, 0u, false },
        { { qnan,  50.0f }, { 57.0f, 27.0f }, kTypeAqua, 1u, true },
    };

    const BlastResult res = resolveBlastChain( grid, { 0u }, kMaxChainDepth );
    EXPECT_TRUE( res.destroyedIndices.empty() );
    EXPECT_TRUE( grid[ 1 ].alive );
}

TEST( BrickMechanicsTest, MultiHpBrickSurvivesFirstHitAndShattersOnSecond )
{
    using namespace Shapes::Physics;

    ArenaConfig cfg {};
    BallState ball {};
    ball.center = { 100.0f, 200.0f };
    ball.vel = { 0.0f, 300.0f };
    ball.radius = 8.0f;

    const Aabb paddle { 50.0f, 15.0f, 100.0f, 14.0f };
    std::vector<BrickItem> bricks {
        { Aabb { 71.5f, 220.0f, 57.0f, 27.0f }, true, 2u }
    };

    // Absorbed armor hit bounces ball without adding speedStepOnBrick
    const StepEvents ev1 = stepBall( ball, paddle, bricks, 0.1f, cfg );
    ASSERT_EQ( ev1.hitBricks.size(), 1u );
    EXPECT_EQ( ev1.hitBricks[ 0 ], 0u );
    EXPECT_TRUE( bricks[ 0 ].alive );
    EXPECT_EQ( bricks[ 0 ].hp, 1u );
    EXPECT_LT( ball.vel.y, 0.0f );
    EXPECT_NEAR( length( ball.vel ), 300.0f, 1e-3f );

    // Second hit shatters the brick and increments speed by speedStepOnBrick
    ball.center = { 100.0f, 200.0f };
    ball.vel = { 0.0f, 300.0f };
    const StepEvents ev2 = stepBall( ball, paddle, bricks, 0.1f, cfg );
    ASSERT_EQ( ev2.hitBricks.size(), 1u );
    EXPECT_FALSE( bricks[ 0 ].alive );
    EXPECT_EQ( bricks[ 0 ].hp, 0u );
    EXPECT_NEAR( length( ball.vel ), 300.0f + cfg.speedStepOnBrick, 1e-3f );
}

TEST( BrickMechanicsTest, SeamHitWithMultiHpBrickAppliesSingleDamageEach )
{
    using namespace Shapes::Physics;

    ArenaConfig cfg {};
    BallState ball {};
    // Aim dead-center at the vertical seam x = 200.0f between a 2-HP brick and a 1-HP brick
    ball.center = { 200.0f, 200.0f };
    ball.vel = { 0.0f, 300.0f };
    ball.radius = 8.0f;

    const Aabb paddle { 150.0f, 15.0f, 100.0f, 14.0f };
    std::vector<BrickItem> bricks {
        { Aabb { 143.0f, 220.0f, 57.0f, 27.0f }, true, 2u },
        { Aabb { 200.0f, 220.0f, 57.0f, 27.0f }, true, 1u }
    };

    const StepEvents ev = stepBall( ball, paddle, bricks, 0.1f, cfg );
    ASSERT_EQ( ev.hitBricks.size(), 2u );
    // 2-HP brick must take exactly 1 damage (NOT 2), 1-HP brick shattered
    EXPECT_TRUE( bricks[ 0 ].alive );
    EXPECT_EQ( bricks[ 0 ].hp, 1u );
    EXPECT_FALSE( bricks[ 1 ].alive );
    EXPECT_EQ( bricks[ 1 ].hp, 0u );
    // Only the 1 destroyed brick adds speedStepOnBrick
    EXPECT_NEAR( length( ball.vel ), 300.0f + cfg.speedStepOnBrick, 1e-3f );
}
