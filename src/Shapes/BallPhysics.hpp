#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace Shapes::Physics
{
    struct Vec2
    {
        float x { 0.0f };
        float y { 0.0f };

        Vec2 operator+( const Vec2& o ) const { return { x + o.x, y + o.y }; }
        Vec2 operator-( const Vec2& o ) const { return { x - o.x, y - o.y }; }
        Vec2 operator*( float s ) const { return { x * s, y * s }; }
        Vec2 operator/( float s ) const { return { x / s, y / s }; }
    };

    inline float dot( const Vec2& a, const Vec2& b )
    {
        return a.x * b.x + a.y * b.y;
    }

    inline float lengthSq( const Vec2& v )
    {
        return dot( v, v );
    }

    inline float length( const Vec2& v )
    {
        return std::sqrt( lengthSq( v ) );
    }

    inline Vec2 normalizeSafe( const Vec2& v, const Vec2& fallback = { 0.0f, 1.0f } )
    {
        const float len = length( v );
        if ( len <= 1e-6f )
            return fallback;
        return v / len;
    }

    // Axis-aligned box in wxFortress coordinates (Y grows upwards, (x, y) is bottom-left).
    struct Aabb
    {
        float x { 0.0f };
        float y { 0.0f };
        float w { 0.0f };
        float h { 0.0f };

        float left() const { return x; }
        float right() const { return x + w; }
        float bottom() const { return y; }
        float top() const { return y + h; }
        Vec2 center() const { return { x + 0.5f * w, y + 0.5f * h }; }
    };

    struct BallState
    {
        Vec2 center { 0.0f, 0.0f };
        float radius { 8.0f };
        Vec2 vel { 0.0f, 0.0f }; // pixels per second
    };

    struct PaddleState
    {
        Aabb box {};
        float speed { 0.0f }; // signed current horizontal speed (px/s)
    };

    struct BrickItem
    {
        Aabb box {};
        bool alive { true };
    };

    struct ArenaConfig
    {
        float width { 800.0f };
        float height { 600.0f };
        float minSpeed { 60.0f };
        float maxSpeed { 1200.0f };
        float speedStepOnBrick { 8.0f };  // added to |vel| per brick hit (px/s)
        float maxPaddleBounceDeg { 60.0f }; // angle from +Y axis -> [30..150 deg] from +X
        float minAbsSinAngle { 0.1736f }; // ~10 deg minimum vertical slope to prevent horizontal lock
        float paddleAccel { 2200.0f };    // px/s^2
        float paddleMaxSpeed { 750.0f };  // px/s
    };

    enum class HitKind : std::uint8_t
    {
        None,
        Wall,
        Paddle,
        Brick
    };

    struct StepEvents
    {
        bool hitWall { false };
        bool hitPaddle { false };
        bool ballLost { false };
        std::vector<std::size_t> hitBricks;
    };

    struct SweepHit
    {
        bool hit { false };
        float t { 1.0f };
        Vec2 normal { 0.0f, 0.0f };
        float penetration { 0.0f };
    };

    namespace detail
    {
        constexpr float kEps = 1e-5f;
        constexpr float kPushOut = 1e-3f;
        constexpr float kPi = 3.14159265358979323846f;

        // Static overlap resolution: circle at c with radius r vs AABB b.
        inline SweepHit resolveOverlapCircleAabb( const Vec2& c, float r, const Aabb& b )
        {
            const float cx = std::clamp( c.x, b.left(), b.right() );
            const float cy = std::clamp( c.y, b.bottom(), b.top() );
            const Vec2 diff { c.x - cx, c.y - cy };
            const float distSq = lengthSq( diff );

            if ( distSq >= r * r )
                return {};

            SweepHit res;
            res.hit = true;
            res.t = 0.0f;

            if ( distSq > kEps * kEps )
            {
                const float dist = std::sqrt( distSq );
                res.normal = diff / dist;
                res.penetration = r - dist;
                return res;
            }

            // Circle center is inside the AABB: push out along minimum penetration axis.
            const float dl = c.x - b.left();
            const float dr = b.right() - c.x;
            const float db = c.y - b.bottom();
            const float dt = b.top() - c.y;
            const float minD = std::min( { dl, dr, db, dt } );

            if ( minD == dt )
                res.normal = { 0.0f, 1.0f };
            else if ( minD == db )
                res.normal = { 0.0f, -1.0f };
            else if ( minD == dr )
                res.normal = { 1.0f, 0.0f };
            else
                res.normal = { -1.0f, 0.0f };

            res.penetration = minD + r;
            return res;
        }

        // Ray P(t) = o + t*d vs circle (corner, r), smallest t in [0, 1] with dot(d, normal) < 0.
        inline bool intersectRayCircle( const Vec2& o, const Vec2& d, const Vec2& corner, float r, float& outT, Vec2& outN )
        {
            const Vec2 m = o - corner;
            const float a = dot( d, d );
            if ( a <= kEps * kEps )
                return false;

            const float b = dot( m, d );
            const float c = dot( m, m ) - r * r;
            if ( c <= 0.0f )
                return false; // already overlapping (handled before)

            if ( b >= 0.0f )
                return false; // moving away from corner

            const float disc = b * b - a * c;
            if ( disc < 0.0f )
                return false;

            const float t = ( -b - std::sqrt( disc ) ) / a;
            if ( t < 0.0f || t > 1.0f )
                return false;

            const Vec2 hitPos = o + d * t;
            outT = t;
            outN = normalizeSafe( hitPos - corner );
            return true;
        }

        // Exact swept circle (c0 -> c0 + d, radius r) vs static AABB b (Minkowski sum).
        inline SweepHit sweepCircleAabb( const Vec2& c0, const Vec2& d, float r, const Aabb& b )
        {
            // 1. Check initial overlap first.
            const SweepHit ov = resolveOverlapCircleAabb( c0, r, b );
            if ( ov.hit )
                return ov;

            // 2. Broad-phase slab test against outer expanded AABB [x-r, x+w+r] x [y-r, y+h+r].
            const float exMinX = b.left() - r;
            const float exMaxX = b.right() + r;
            const float exMinY = b.bottom() - r;
            const float exMaxY = b.top() + r;

            float tMin = 0.0f;
            float tMax = 1.0f;

            auto clipAxis = [ & ]( float o, float dir, float mn, float mx ) -> bool {
                if ( std::fabs( dir ) <= kEps )
                    return ( o >= mn && o <= mx );
                float t1 = ( mn - o ) / dir;
                float t2 = ( mx - o ) / dir;
                if ( t1 > t2 )
                    std::swap( t1, t2 );
                tMin = std::max( tMin, t1 );
                tMax = std::min( tMax, t2 );
                return tMin <= tMax;
            };

            if ( !clipAxis( c0.x, d.x, exMinX, exMaxX ) || !clipAxis( c0.y, d.y, exMinY, exMaxY ) )
                return {};

            SweepHit best;
            best.t = 2.0f;

            // 3. Test 4 flat faces of the Minkowski sum.
            auto testVerticalFace = [ & ]( float faceX, float nx ) {
                if ( d.x * nx >= -kEps )
                    return;
                const float t = ( faceX - c0.x ) / d.x;
                if ( t < 0.0f || t > 1.0f || t >= best.t )
                    return;
                const float y = c0.y + d.y * t;
                if ( y >= b.bottom() && y <= b.top() )
                {
                    best.hit = true;
                    best.t = t;
                    best.normal = { nx, 0.0f };
                }
            };

            auto testHorizontalFace = [ & ]( float faceY, float ny ) {
                if ( d.y * ny >= -kEps )
                    return;
                const float t = ( faceY - c0.y ) / d.y;
                if ( t < 0.0f || t > 1.0f || t >= best.t )
                    return;
                const float x = c0.x + d.x * t;
                if ( x >= b.left() && x <= b.right() )
                {
                    best.hit = true;
                    best.t = t;
                    best.normal = { 0.0f, ny };
                }
            };

            testVerticalFace( b.left() - r, -1.0f );
            testVerticalFace( b.right() + r, 1.0f );
            testHorizontalFace( b.bottom() - r, -1.0f );
            testHorizontalFace( b.top() + r, 1.0f );

            // 4. Test 4 rounded corners (circles of radius r centered at each AABB vertex).
            const Vec2 corners[ 4 ] = {
                { b.left(), b.bottom() },
                { b.right(), b.bottom() },
                { b.left(), b.top() },
                { b.right(), b.top() },
            };

            for ( const Vec2& corner : corners )
            {
                float tc = 0.0f;
                Vec2 nc {};
                if ( intersectRayCircle( c0, d, corner, r, tc, nc ) && tc < best.t )
                {
                    best.hit = true;
                    best.t = tc;
                    best.normal = nc;
                }
            }

            return best;
        }

        // Preserve speed magnitude while preventing nearly-horizontal infinite bounce loops.
        inline Vec2 enforceMinVerticalSlope( const Vec2& v, float minAbsSin )
        {
            const float spd = length( v );
            if ( spd <= kEps )
                return v;
            const float minVy = spd * minAbsSin;
            if ( std::fabs( v.y ) >= minVy )
                return v;

            const float signY = ( v.y < 0.0f ) ? -1.0f : 1.0f;
            const float signX = ( v.x < 0.0f ) ? -1.0f : 1.0f;
            const float newVy = signY * minVy;
            const float newVx = signX * std::sqrt( std::max( 0.0f, spd * spd - newVy * newVy ) );
            return { newVx, newVy };
        }

        inline Vec2 scaleSpeed( const Vec2& v, float deltaSpeed, float minSpd, float maxSpd )
        {
            const float spd = length( v );
            if ( spd <= kEps )
                return v;
            const float nextSpd = std::clamp( spd + deltaSpeed, minSpd, maxSpd );
            return v * ( nextSpd / spd );
        }
    } // namespace detail

    // Updates paddle horizontal position with bounded velocity and real dt (seconds).
    // moveDir: -1 = left, 0 = none, +1 = right.
    inline void updatePaddle( PaddleState& paddle, int moveDir, float dtSec, const ArenaConfig& cfg )
    {
        if ( dtSec <= 0.0f )
            return;

        if ( moveDir == 0 )
        {
            paddle.speed = 0.0f;
            return;
        }

        const float sign = ( moveDir < 0 ) ? -1.0f : 1.0f;
        if ( paddle.speed * sign < 0.0f )
            paddle.speed = 0.0f; // instant reversal on direction flip

        paddle.speed = std::clamp( paddle.speed + sign * cfg.paddleAccel * dtSec, -cfg.paddleMaxSpeed, cfg.paddleMaxSpeed );
        paddle.box.x = std::clamp( paddle.box.x + paddle.speed * dtSec, 0.0f, std::max( 0.0f, cfg.width - paddle.box.w ) );
    }

    // Advances ball physics by dtSec using analytical swept-circle CCD (up to kMaxSubsteps).
    // Never throws, never produces NaN, never tunnels even at high speed.
    inline StepEvents stepBall(
        BallState& ball,
        const Aabb& paddle,
        std::vector<BrickItem>& bricks,
        float dtSec,
        const ArenaConfig& cfg )
    {
        StepEvents events;
        if ( dtSec <= 0.0f || ball.radius <= 0.0f )
            return events;

        // Clamp huge frame hiccups (e.g. window drag) so one frame doesn't jump across the whole screen.
        const float clampedDt = std::min( dtSec, 0.05f );
        float remTime = clampedDt;

        constexpr int kMaxSubsteps = 8;
        constexpr float kSimultaneousTol = 1e-4f;

        for ( int iter = 0; iter < kMaxSubsteps && remTime > 1e-6f; ++iter )
        {
            const Vec2 disp = ball.vel * remTime;
            if ( lengthSq( disp ) <= detail::kEps * detail::kEps )
                break;

            HitKind bestKind = HitKind::None;
            float bestT = 2.0f;
            Vec2 bestNormal { 0.0f, 0.0f };
            float bestPen = 0.0f;
            std::size_t bestBrickIdx = std::numeric_limits<std::size_t>::max();

            auto consider = [ & ]( HitKind kind, const SweepHit& h, std::size_t idx ) {
                if ( !h.hit )
                    return;
                if ( h.t < bestT - kSimultaneousTol )
                {
                    bestKind = kind;
                    bestT = h.t;
                    bestNormal = h.normal;
                    bestPen = h.penetration;
                    bestBrickIdx = idx;
                }
            };

            // 1. Arena walls (left, right, top).
            auto checkWall = [ & ]( float coord, float dir, float limit, Vec2 normal ) {
                if ( ( coord - limit ) * ( normal.x + normal.y ) < 0.0f )
                {
                    // Already past wall -> resolve overlap immediately.
                    SweepHit h;
                    h.hit = true;
                    h.t = 0.0f;
                    h.normal = normal;
                    h.penetration = std::fabs( limit - coord );
                    consider( HitKind::Wall, h, 0 );
                    return;
                }
                if ( dir * ( normal.x + normal.y ) >= -detail::kEps )
                    return;
                const float t = ( limit - coord ) / dir;
                if ( t >= 0.0f && t <= 1.0f )
                {
                    SweepHit h;
                    h.hit = true;
                    h.t = t;
                    h.normal = normal;
                    consider( HitKind::Wall, h, 0 );
                }
            };

            checkWall( ball.center.x, disp.x, ball.radius, { 1.0f, 0.0f } );
            checkWall( ball.center.x, disp.x, cfg.width - ball.radius, { -1.0f, 0.0f } );
            checkWall( ball.center.y, disp.y, cfg.height - ball.radius, { 0.0f, -1.0f } );

            // 2. Paddle.
            const SweepHit paddleHit = detail::sweepCircleAabb( ball.center, disp, ball.radius, paddle );
            consider( HitKind::Paddle, paddleHit, 0 );

            // 3. Alive bricks.
            for ( std::size_t i = 0; i < bricks.size(); ++i )
            {
                if ( !bricks[ i ].alive )
                    continue;
                const SweepHit bh = detail::sweepCircleAabb( ball.center, disp, ball.radius, bricks[ i ].box );
                consider( HitKind::Brick, bh, i );
            }

            if ( bestKind == HitKind::None )
            {
                ball.center = ball.center + disp;
                remTime = 0.0f;
                break;
            }

            // Advance to impact point and push cleanly outside the surface.
            const Vec2 startCenter = ball.center;
            ball.center = startCenter + disp * bestT + bestNormal * ( bestPen + detail::kPushOut );
            remTime *= ( 1.0f - bestT );

            if ( bestKind == HitKind::Paddle )
            {
                events.hitPaddle = true;
                const float spd = std::clamp( length( ball.vel ), cfg.minSpeed, cfg.maxSpeed );
                if ( bestNormal.y > 0.25f )
                {
                    // Top bounce: angle determined smoothly across full paddle width [-maxDeg, +maxDeg] from +Y.
                    const float halfW = std::max( 0.5f * paddle.w, 1.0f );
                    const float u = std::clamp( ( ball.center.x - paddle.center().x ) / halfW, -1.0f, 1.0f );
                    const float maxRad = cfg.maxPaddleBounceDeg * ( detail::kPi / 180.0f );
                    const float theta = u * maxRad;
                    ball.vel = { spd * std::sin( theta ), spd * std::cos( theta ) };
                }
                else
                {
                    // Side/corner graze of paddle: pure elastic reflection (never throws!).
                    const float vn = dot( ball.vel, bestNormal );
                    if ( vn < 0.0f )
                        ball.vel = ball.vel - bestNormal * ( 2.0f * vn );
                }
            }
            else if ( bestKind == HitKind::Wall )
            {
                events.hitWall = true;
                const float vn = dot( ball.vel, bestNormal );
                if ( vn < 0.0f )
                    ball.vel = ball.vel - bestNormal * ( 2.0f * vn );
                ball.vel = detail::enforceMinVerticalSlope( ball.vel, cfg.minAbsSinAngle );
            }
            else if ( bestKind == HitKind::Brick )
            {
                // Collect simultaneous brick hits (e.g. ball hitting the seam between two bricks).
                const std::size_t hitsBefore = events.hitBricks.size();
                Vec2 combinedNormal = bestNormal;
                if ( bestBrickIdx < bricks.size() && bricks[ bestBrickIdx ].alive )
                {
                    bricks[ bestBrickIdx ].alive = false;
                    events.hitBricks.push_back( bestBrickIdx );
                }

                for ( std::size_t i = 0; i < bricks.size(); ++i )
                {
                    if ( !bricks[ i ].alive )
                        continue;
                    const SweepHit bh = detail::sweepCircleAabb( startCenter, disp, ball.radius, bricks[ i ].box );
                    if ( bh.hit && std::fabs( bh.t - bestT ) <= kSimultaneousTol )
                    {
                        bricks[ i ].alive = false;
                        events.hitBricks.push_back( i );
                        combinedNormal = combinedNormal + bh.normal;
                    }
                }

                const Vec2 reflN = normalizeSafe( combinedNormal, bestNormal );
                const float vn = dot( ball.vel, reflN );
                if ( vn < 0.0f )
                    ball.vel = ball.vel - reflN * ( 2.0f * vn );

                const std::size_t hitsThisSubstep = events.hitBricks.size() - hitsBefore;
                ball.vel = detail::scaleSpeed(
                    ball.vel,
                    cfg.speedStepOnBrick * static_cast<float>( hitsThisSubstep ),
                    cfg.minSpeed,
                    cfg.maxSpeed );
                ball.vel = detail::enforceMinVerticalSlope( ball.vel, cfg.minAbsSinAngle );
            }

            // Clamp inside left/right/top walls after push-out so corner hits stay strictly inside arena.
            ball.center.x = std::clamp( ball.center.x, ball.radius, cfg.width - ball.radius );
            ball.center.y = std::min( ball.center.y, cfg.height - ball.radius );
        }

        if ( ball.center.y + ball.radius < 0.0f )
            events.ballLost = true;

        return events;
    }
    // Positions paddle so its horizontal center tracks targetCenterX (e.g. mouse X),
    // clamped strictly within [0, cfg.width - paddle.box.w]. Resets keyboard velocity
    // so keyboard inertia does not fight pointer input. Non-finite inputs are ignored.
    inline void setPaddleTargetX( PaddleState& paddle, float targetCenterX, const ArenaConfig& cfg )
    {
        if ( !std::isfinite( targetCenterX ) )
            return;

        const float maxX = std::max( 0.0f, cfg.width - paddle.box.w );
        paddle.box.x = std::clamp( targetCenterX - 0.5f * paddle.box.w, 0.0f, maxX );
        paddle.speed = 0.0f;
    }


    // Predicts exact horizontal landing coordinate X when the ball descends to targetY,
    // analytically folding left/right wall bounces in O(1).
    inline float predictBallLandingX( const BallState& ball, float targetY, const ArenaConfig& cfg )
    {
        const float minX = ball.radius;
        const float maxX = std::max( minX, cfg.width - ball.radius );
        const float span = maxX - minX;
        if ( span <= detail::kEps || ball.vel.y >= -detail::kEps || ball.center.y <= targetY )
            return std::clamp( ball.center.x, minX, maxX );

        const float t = ( ball.center.y - targetY ) / ( -ball.vel.y );
        const float rawX = ball.center.x + ball.vel.x * t;
        float rel = std::fmod( rawX - minX, 2.0f * span );
        if ( rel < 0.0f )
            rel += 2.0f * span;

        return ( rel <= span ) ? ( minX + rel ) : ( maxX - ( rel - span ) );
    }

    // Computes desired paddle center X for the autonomous robot.
    // Uses inverse paddle bounce geometry (theta = u * maxPaddleBounceDeg) to aim
    // at alive bricks directly or via left/right wall bank shots, preventing vertical loops.
    inline float computeRobotTargetX(
        const BallState& ball,
        const Aabb& paddle,
        const std::vector<BrickItem>& bricks,
        const ArenaConfig& cfg,
        unsigned int dryBounces = 0 )
    {
        const float halfW = std::max( 0.5f * paddle.w, 1.0f );
        const float minCenter = halfW;
        const float maxCenter = std::max( minCenter, cfg.width - halfW );

        // Compute alive brick centroid for upward pre-positioning & tie-breaking.
        float brickSumX = 0.0f;
        int aliveCount = 0;
        for ( const auto& b : bricks )
        {
            if ( b.alive )
            {
                brickSumX += b.box.center().x;
                ++aliveCount;
            }
        }
        const float bricksMeanX = ( aliveCount > 0 ) ? ( brickSumX / static_cast<float>( aliveCount ) ) : ( 0.5f * cfg.width );

        // While ball moves upward, pre-position smoothly between ball X and remaining brick cluster.
        if ( ball.vel.y >= -detail::kEps )
        {
            const float preX = 0.55f * ball.center.x + 0.45f * bricksMeanX;
            return std::clamp( preX, minCenter, maxCenter );
        }

        const float paddleTopY = paddle.top() + ball.radius;
        const float landX = predictBallLandingX( ball, paddleTopY, cfg );
        const float maxRad = std::max( cfg.maxPaddleBounceDeg * ( detail::kPi / 180.0f ), 0.1f );
        constexpr float kMaxSafeU = 0.80f;

        bool foundTarget = false;
        float bestScore = std::numeric_limits<float>::max();
        float bestU = 0.0f;

        auto evalCandidate = [ & ]( float tx, float ty, bool isBank ) {
            const float dy = ty - paddleTopY;
            if ( dy <= 1.0f )
                return;
            const float dx = tx - landX;
            const float theta = std::atan2( dx, dy );
            const float u = theta / maxRad;
            if ( std::fabs( u ) > kMaxSafeU )
                return;

            const float dist = std::hypot( dx, dy );
            float score = dist;

            // Normally prefer direct shots; if stuck in dry bounces, prefer bank shots & wider angles.
            if ( dryBounces >= 2 )
            {
                score += isBank ? -120.0f : 60.0f;
                if ( std::fabs( u ) < 0.30f )
                    score += 200.0f;
            }
            else
            {
                if ( isBank )
                    score += 140.0f;
                if ( std::fabs( u ) < 0.08f )
                    score += 45.0f;
            }

            if ( score < bestScore )
            {
                bestScore = score;
                bestU = u;
                foundTarget = true;
            }
        };

        const float leftWallX = ball.radius;
        const float rightWallX = cfg.width - ball.radius;

        for ( const auto& b : bricks )
        {
            if ( !b.alive )
                continue;
            const Vec2 bc = b.box.center();
            evalCandidate( bc.x, bc.y, false );
            evalCandidate( 2.0f * leftWallX - bc.x, bc.y, true );
            evalCandidate( 2.0f * rightWallX - bc.x, bc.y, true );
        }

        if ( !foundTarget )
        {
            const float side = ( bricksMeanX >= landX ) ? 1.0f : -1.0f;
            bestU = side * 0.35f;
        }
        else if ( dryBounces >= 1 && std::fabs( bestU ) < 0.12f )
        {
            // Break any incipient vertical oscillation.
            const float nudgeSign = ( ( dryBounces % 2 ) == 0 ) ? 1.0f : -1.0f;
            bestU = nudgeSign * 0.38f;
        }

        // Clamp desired paddle center so the paddle stays inside walls AND still catches landX.
        float desiredCenter = std::clamp( landX - bestU * halfW, minCenter, maxCenter );
        const float maxOffset = kMaxSafeU * halfW;
        desiredCenter = std::clamp( desiredCenter, landX - maxOffset, landX + maxOffset );
        return std::clamp( desiredCenter, minCenter, maxCenter );
    }

    // Moves the robot paddle smoothly toward the computed aiming target, with emergency catch safety.
    inline void updateRobotPaddle(
        PaddleState& paddle,
        const BallState& ball,
        const std::vector<BrickItem>& bricks,
        float dtSec,
        const ArenaConfig& cfg,
        unsigned int dryBounces = 0 )
    {
        if ( dtSec <= 0.0f )
            return;

        const float halfW = 0.5f * paddle.box.w;
        const float maxLeft = std::max( 0.0f, cfg.width - paddle.box.w );
        const float targetCenter = computeRobotTargetX( ball, paddle.box, bricks, cfg, dryBounces );
        const float targetLeft = std::clamp( targetCenter - halfW, 0.0f, maxLeft );

        const float maxStep = ( cfg.paddleMaxSpeed * 1.6f ) * dtSec;
        const float diff = targetLeft - paddle.box.x;
        if ( std::fabs( diff ) <= maxStep )
            paddle.box.x = targetLeft;
        else
            paddle.box.x += ( diff > 0.0f ? maxStep : -maxStep );

        // Emergency catch guarantee right before impact if a late brick ricochet changed landX.
        const float paddleTopY = paddle.box.top() + ball.radius;
        if ( ball.vel.y < -detail::kEps && ball.center.y > paddleTopY )
        {
            const float tti = ( ball.center.y - paddleTopY ) / ( -ball.vel.y );
            if ( tti <= 0.08f )
                paddle.box.x = targetLeft;
        }

        paddle.box.x = std::clamp( paddle.box.x, 0.0f, maxLeft );
        paddle.speed = 0.0f;
    }
} // namespace Shapes::Physics
