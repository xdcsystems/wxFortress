#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "BallPhysics.hpp"

using namespace Shapes::Physics;

static int g_passed = 0;
static int g_failed = 0;

#define CHECK( cond, msg )                                                    \
    do                                                                        \
    {                                                                         \
        if ( cond )                                                           \
            ++g_passed;                                                       \
        else                                                                  \
        {                                                                     \
            ++g_failed;                                                       \
            std::fprintf( stderr, "FAIL [%s:%d]: %s\n", __FILE__, __LINE__, msg ); \
        }                                                                     \
    } while ( 0 )

static bool isFiniteVec( const Vec2& v )
{
    return std::isfinite( v.x ) && std::isfinite( v.y );
}

int main()
{
    ArenaConfig cfg;
    cfg.width = 800.0f;
    cfg.height = 600.0f;
    const Aabb paddle { 350.0f, 15.0f, 100.0f, 16.0f };

    // 1. Diagonal hit on elongated brick (4:1) — old code returned ContactPosition::Null or wrong side!
    {
        std::vector<BrickItem> bricks { { Aabb { 200.0f, 300.0f, 100.0f, 20.0f }, true } };
        BallState ball { { 215.0f, 285.0f }, 8.0f, { 0.0f, 300.0f } };
        const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
        CHECK( ev.hitBricks.size() == 1 && !bricks[ 0 ].alive, "elongated brick hit from bottom" );
        CHECK( ball.vel.y < 0.0f && std::fabs( ball.vel.x ) < 1e-3f, "bottom face reflects strictly downward (not sideways)" );
    }

    // 2. Left vs Right face orientation (old Ball::intersect inverted Left <-> Right!)
    {
        std::vector<BrickItem> bricks { { Aabb { 200.0f, 300.0f, 100.0f, 20.0f }, true } };
        BallState ball { { 185.0f, 310.0f }, 8.0f, { 400.0f, 100.0f } };
        const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
        CHECK( ev.hitBricks.size() == 1, "left face brick hit" );
        CHECK( ball.vel.x < 0.0f && ball.vel.y > 0.0f, "left face reflects vx < 0, preserves vy > 0" );
    }

    // 3. Paddle left-edge bounce reverses direction even when approaching RightDown!
    {
        std::vector<BrickItem> bricks;
        BallState ball { { 360.0f, 45.0f }, 8.0f, { 200.0f, -300.0f } };
        const float spdBefore = length( ball.vel );
        const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
        CHECK( ev.hitPaddle, "paddle left edge hit" );
        CHECK( ball.vel.x < 0.0f && ball.vel.y > 0.0f, "RightDown on left edge of paddle reflects to TopLeft" );
        CHECK( std::fabs( length( ball.vel ) - spdBefore ) < 1e-2f, "paddle bounce preserves speed magnitude" );
    }

    // 4. Side graze of paddle while moving upward — old code threw std::runtime_error("Error: Wrong ball direction")!
    {
        std::vector<BrickItem> bricks;
        BallState ball { { 338.0f, 20.0f }, 8.0f, { 250.0f, 100.0f } };
        const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
        CHECK( ev.hitPaddle, "side graze of paddle detected without throwing" );
        CHECK( ball.vel.x < 0.0f, "side graze reflects horizontally" );
        CHECK( isFiniteVec( ball.center ) && isFiniteVec( ball.vel ), "no NaN on side graze" );
    }

    // 5. Simultaneous top-right corner hit (both walls in one frame, high speed)
    {
        std::vector<BrickItem> bricks;
        BallState ball { { 785.0f, 585.0f }, 8.0f, { 600.0f, 600.0f } };
        const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
        CHECK( ev.hitWall, "corner hit registered" );
        CHECK( ball.vel.x < 0.0f && ball.vel.y < 0.0f, "both vx and vy reversed in top-right corner" );
        CHECK( ball.center.x <= cfg.width - ball.radius + 1e-3f && ball.center.y <= cfg.height - ball.radius + 1e-3f,
               "ball remains strictly inside arena after corner hit" );
    }

    // 6. Seam between two bricks: hitting exact seam breaks both and reflects cleanly without tunneling
    {
        std::vector<BrickItem> bricks {
            { Aabb { 200.0f, 300.0f, 50.0f, 20.0f }, true },
            { Aabb { 250.0f, 300.0f, 50.0f, 20.0f }, true },
        };
        BallState ball { { 250.0f, 285.0f }, 8.0f, { 0.0f, 400.0f } };
        const auto ev = stepBall( ball, paddle, bricks, 0.05f, cfg );
        CHECK( ev.hitBricks.size() == 2 && !bricks[ 0 ].alive && !bricks[ 1 ].alive, "seam hit breaks both adjacent bricks" );
        CHECK( ball.vel.y < 0.0f, "seam hit reflects downward" );
    }

    // 7. Anti-tunneling at extreme speed (1200 px/s -> 20 px/frame > thin 12 px brick!)
    {
        std::vector<BrickItem> bricks { { Aabb { 300.0f, 300.0f, 80.0f, 12.0f }, true } };
        BallState ball { { 340.0f, 285.0f }, 6.0f, { 0.0f, 1200.0f } };
        const auto ev = stepBall( ball, paddle, bricks, 0.03f, cfg ); // 36 px in 1 step!
        CHECK( ev.hitBricks.size() == 1 && !bricks[ 0 ].alive, "no tunneling through 12px brick at 36px/frame" );
        CHECK( ball.vel.y < 0.0f && ball.center.y < 300.0f, "ball reflected back below thin brick" );
    }

    // 8. Zero velocity & zero vector normalize safety (no NaN)
    {
        const Vec2 n = normalizeSafe( { 0.0f, 0.0f } );
        CHECK( isFiniteVec( n ) && std::fabs( length( n ) - 1.0f ) < 1e-4f, "normalizeSafe(0,0) returns unit fallback" );
        std::vector<BrickItem> bricks;
        BallState ball { { 400.0f, 300.0f }, 8.0f, { 0.0f, 0.0f } };
        (void)stepBall( ball, paddle, bricks, 0.016f, cfg );
        CHECK( isFiniteVec( ball.center ) && isFiniteVec( ball.vel ), "zero velocity produces no NaN" );
    }

    // 9. Bounded paddle acceleration with real dt
    {
        PaddleState ps { paddle, 0.0f };
        for ( int i = 0; i < 200; ++i )
            updatePaddle( ps, +1, 0.016f, cfg );
        CHECK( ps.speed <= cfg.paddleMaxSpeed + 1e-3f, "paddle speed clamped to maxSpeed" );
        CHECK( ps.box.right() <= cfg.width + 1e-3f, "paddle stays inside right wall" );
    }

    // 10. 100,000-tick endurance simulation (robot paddle + 40 bricks + respawns): zero NaN, zero escape, bounded speed
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
            // Robot tracks ball with slight offset to sweep all bounce angles
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
        CHECK( okAll, "100,000-tick endurance simulation: zero NaN, zero escape, bounded speed" );
        CHECK( totalBroken > 50, "100,000-tick endurance simulation: bricks actively broken" );
    }

    std::printf( "BallPhysics tests: %d passed, %d failed\n", g_passed, g_failed );
    return g_failed == 0 ? 0 : 1;
}