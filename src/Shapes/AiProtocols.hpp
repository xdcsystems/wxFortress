#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/trigonometric.hpp>

#include "BallPhysics.hpp"
#include "Common/MathUtil.hpp"


namespace AiProtocols
{
    enum class ProtocolKind : std::uint8_t
    {
        None = 0,
        Split,          // Cluster split (Multiball up to 3 plasma orbs)
        OverheatPierce, // 5s pierce through bricks -> 3s manipulator overheat penalty
        MagLock,        // Magnetic paddle capture (release with <Space>)
        LimiterOff,     // 8s speed surge (+18%, cap 580 px/s) + 2x score multiplier
    };

    inline constexpr std::size_t kMaxBalls = 3u;
    inline constexpr std::size_t kMaxActiveCapsules = 3u;
    inline constexpr float kCapsuleWidth = 28.0f;
    inline constexpr float kCapsuleHeight = 10.0f;
    inline constexpr float kCapsuleFallSpeed = 145.0f;
    inline constexpr float kCapsuleSpawnCooldownSec = 3.5f;

    inline constexpr float kSplitSpreadDeg = 22.0f;
    inline constexpr float kPierceDurationSec = 5.0f;
    inline constexpr float kOverheatDurationSec = 3.0f;
    inline constexpr float kOverheatPaddleSpeedScale = 0.65f;
    inline constexpr float kMagLockDurationSec = 10.0f;
    inline constexpr unsigned int kMagLockMaxCharges = 3u;
    inline constexpr float kRobotMagHoldSec = 0.25f;
    inline constexpr float kLimiterOffDurationSec = 8.0f;
    inline constexpr float kLimiterSpeedMultiplier = 1.18f;
    inline constexpr float kLimiterMaxSpeed = 580.0f;

    struct ProtocolCapsule
    {
        ProtocolKind kind { ProtocolKind::None };
        Shapes::Physics::Aabb box {};
        float fallSpeed { kCapsuleFallSpeed };
        bool active { true };
    };

    struct StepOutcome
    {
        bool magLockExpired { false };
        bool limiterExpired { false };
    };

    struct ActiveState
    {
        float pierceRemainingSec { 0.0f };
        float overheatRemainingSec { 0.0f };
        float magLockRemainingSec { 0.0f };
        unsigned int magLockCharges { 0u };
        float limiterRemainingSec { 0.0f };
        float spawnCooldownSec { 0.0f };

        void reset() noexcept
        {
            pierceRemainingSec = 0.0f;
            overheatRemainingSec = 0.0f;
            magLockRemainingSec = 0.0f;
            magLockCharges = 0u;
            limiterRemainingSec = 0.0f;
            spawnCooldownSec = 0.0f;
        }

        bool isPierceActive() const noexcept
        {
            return std::isfinite( pierceRemainingSec ) && pierceRemainingSec > 0.0f;
        }

        bool isOverheatActive() const noexcept
        {
            return std::isfinite( overheatRemainingSec ) && overheatRemainingSec > 0.0f;
        }

        // True while MagLock can still capture additional balls.
        bool isMagLockActive() const noexcept
        {
            return std::isfinite( magLockRemainingSec ) && magLockRemainingSec > 0.0f && magLockCharges > 0u;
        }

        // True while MagLock timer is still running (even after last charge is consumed so auto-release works).
        bool isMagLockTimerActive() const noexcept
        {
            return std::isfinite( magLockRemainingSec ) && magLockRemainingSec > 0.0f;
        }

        bool isLimiterActive() const noexcept
        {
            return std::isfinite( limiterRemainingSec ) && limiterRemainingSec > 0.0f;
        }

        unsigned int scoreMultiplier() const noexcept
        {
            return isLimiterActive() ? 2u : 1u;
        }

        float paddleSpeedScale() const noexcept
        {
            return isOverheatActive() ? kOverheatPaddleSpeedScale : 1.0f;
        }

        void activate( ProtocolKind kind ) noexcept
        {
            switch ( kind )
            {
                case ProtocolKind::OverheatPierce:
                    pierceRemainingSec = kPierceDurationSec;
                    overheatRemainingSec = 0.0f;
                    break;

                case ProtocolKind::MagLock:
                    magLockRemainingSec = kMagLockDurationSec;
                    magLockCharges = kMagLockMaxCharges;
                    break;

                case ProtocolKind::LimiterOff:
                    limiterRemainingSec = kLimiterOffDurationSec;
                    break;

                case ProtocolKind::Split:
                case ProtocolKind::None:
                    break;
            }
        }

        // Consumes one capture charge while keeping magLockRemainingSec alive so the last captured
        // ball still auto-releases on timer expiry and remains visible on the Zone 2 HUD.
        bool consumeMagLockCharge() noexcept
        {
            if ( !isMagLockActive() )
                return false;
            --magLockCharges;
            return true;
        }

        StepOutcome step( float dtSec ) noexcept
        {
            StepOutcome out {};
            if ( !MathUtil::isPositiveFinite( dtSec ) )
                return out;
            const float dt = ( std::min )( dtSec, 0.05f );

            if ( spawnCooldownSec > 0.0f )
                spawnCooldownSec = ( std::max )( 0.0f, spawnCooldownSec - dt );

            if ( pierceRemainingSec > 0.0f )
            {
                pierceRemainingSec -= dt;
                if ( pierceRemainingSec <= 0.0f )
                {
                    pierceRemainingSec = 0.0f;
                    overheatRemainingSec = kOverheatDurationSec;
                }
            }
            else if ( overheatRemainingSec > 0.0f )
            {
                overheatRemainingSec = ( std::max )( 0.0f, overheatRemainingSec - dt );
            }

            if ( magLockRemainingSec > 0.0f )
            {
                magLockRemainingSec = ( std::max )( 0.0f, magLockRemainingSec - dt );
                if ( magLockRemainingSec <= 0.0f )
                {
                    magLockCharges = 0u;
                    out.magLockExpired = true;
                }
            }

            if ( limiterRemainingSec > 0.0f )
            {
                limiterRemainingSec = ( std::max )( 0.0f, limiterRemainingSec - dt );
                if ( limiterRemainingSec <= 0.0f )
                    out.limiterExpired = true;
            }

            return out;
        }

        // Concise persistent Zone 2 telemetry label for active protocol/penalty (empty when idle).
        // Priority: OVERHEAT > PIERCE > LIMITER x2 > MAG-LOCK.
        std::string hudStatusText( bool hasLockedBalls = false ) const
        {
            auto formatSec = []( float sec ) -> std::string
            {
                const int whole = static_cast<int>( std::ceil( sec ) );
                return std::to_string( ( std::max )( 1, whole ) ) + "S";
            };

            if ( isOverheatActive() )
                return "OVERHEAT " + formatSec( overheatRemainingSec );
            if ( isPierceActive() )
                return "PIERCE " + formatSec( pierceRemainingSec );
            if ( isLimiterActive() )
                return "LIMITER x2 " + formatSec( limiterRemainingSec );
            if ( isMagLockActive() || ( hasLockedBalls && isMagLockTimerActive() ) )
                return "MAG-LOCK " + formatSec( magLockRemainingSec );
            if ( hasLockedBalls )
                return "MAG-LOCK";
            return {};
        }
    };

    inline const char* activationBanner( ProtocolKind kind ) noexcept
    {
        switch ( kind )
        {
            case ProtocolKind::Split:          return "PROTOCOL: SPLIT";
            case ProtocolKind::OverheatPierce: return "PROTOCOL: PIERCE";
            case ProtocolKind::MagLock:        return "PROTOCOL: MAG-LOCK";
            case ProtocolKind::LimiterOff:     return "PROTOCOL: LIMITER x2";
            default:                           return "";
        }
    }

    inline bool isRiskyProtocol( ProtocolKind kind ) noexcept
    {
        return kind == ProtocolKind::OverheatPierce || kind == ProtocolKind::LimiterOff;
    }

    inline glm::vec3 capsuleTint( ProtocolKind kind ) noexcept
    {
        switch ( kind )
        {
            case ProtocolKind::Split:          return { 0.45f, 1.00f, 1.00f };
            case ProtocolKind::OverheatPierce: return { 1.00f, 0.50f, 0.22f };
            case ProtocolKind::MagLock:        return { 0.40f, 1.00f, 0.58f };
            case ProtocolKind::LimiterOff:     return { 1.00f, 0.90f, 0.28f };
            default:                           return { 1.00f, 1.00f, 1.00f };
        }
    }

    // Adaptive protocol selector (deterministic for a given roll in [0, 99]).
    // Avoids redundant Split when already at kMaxBalls and boosts MagLock stabilization after >= 2 breaches.
    inline ProtocolKind selectNextProtocol(
        std::size_t activeBallCount,
        unsigned int stageIncidents,
        unsigned int roll0To99 ) noexcept
    {
        const unsigned int r = roll0To99 % 100u;
        const bool canSplit = ( activeBallCount < kMaxBalls );

        if ( stageIncidents >= 2u )
        {
            if ( r < 35u )
                return ProtocolKind::MagLock;
            if ( r < 65u )
                return canSplit ? ProtocolKind::Split : ProtocolKind::LimiterOff;
            if ( r < 85u )
                return ProtocolKind::OverheatPierce;
            return ProtocolKind::LimiterOff;
        }

        if ( r < 30u )
            return canSplit ? ProtocolKind::Split : ProtocolKind::LimiterOff;
        if ( r < 55u )
            return ProtocolKind::OverheatPierce;
        if ( r < 78u )
            return ProtocolKind::MagLock;
        return ProtocolKind::LimiterOff;
    }

    inline bool aabbOverlap( const Shapes::Physics::Aabb& a, const Shapes::Physics::Aabb& b ) noexcept
    {
        return a.left() < b.right() &&
               a.right() > b.left() &&
               a.bottom() < b.top() &&
               a.top() > b.bottom();
    }

    // Advances falling capsules, collects paddle catches, and prunes off-screen capsules.
    inline std::vector<ProtocolKind> stepCapsules(
        std::vector<ProtocolCapsule>& capsules,
        const Shapes::Physics::Aabb& paddleBox,
        float dtSec )
    {
        std::vector<ProtocolKind> caught;
        if ( !MathUtil::isPositiveFinite( dtSec ) )
            return caught;
        const float dt = ( std::min )( dtSec, 0.05f );

        for ( auto& cap : capsules )
        {
            if ( !cap.active )
                continue;
            const float spd = MathUtil::isPositiveFinite( cap.fallSpeed ) ? cap.fallSpeed : kCapsuleFallSpeed;
            cap.box.y -= spd * dt;

            if ( aabbOverlap( cap.box, paddleBox ) )
            {
                cap.active = false;
                if ( cap.kind != ProtocolKind::None )
                    caught.push_back( cap.kind );
            }
            else if ( cap.box.top() < 0.0f )
            {
                cap.active = false;
            }
        }

        capsules.erase(
            std::remove_if(
                capsules.begin(),
                capsules.end(),
                []( const ProtocolCapsule& c ) { return !c.active; } ),
            capsules.end() );

        return caught;
    }

    inline float applyLimiterBoost(
        float currentSpeed,
        float minSpeed,
        float maxSpeed,
        float limiterCap = kLimiterMaxSpeed ) noexcept
    {
        const float safeMin = std::isfinite( minSpeed ) ? ( std::max )( 0.0f, minSpeed ) : 0.0f;
        if ( !std::isfinite( currentSpeed ) || !std::isfinite( maxSpeed ) || !std::isfinite( limiterCap ) )
            return safeMin;
        const float arenaHi = ( std::max )( safeMin, maxSpeed );
        const float clampedCur = std::clamp( currentSpeed, safeMin, arenaHi );
        const float effectiveCap = ( std::min )( arenaHi, ( std::max )( clampedCur, ( std::max )( safeMin, limiterCap ) ) );
        return std::clamp( clampedCur * kLimiterSpeedMultiplier, safeMin, effectiveCap );
    }

    // Restores ball speed after 8s LimiterOff ends (preserves any brick/EM acceleration gained during the burst).
    inline float restoreLimiterSpeed(
        float currentSpeed,
        float savedBaseSpeed,
        float minSpeed,
        float maxSpeed ) noexcept
    {
        const float safeMin = std::isfinite( minSpeed ) ? ( std::max )( 0.0f, minSpeed ) : 0.0f;
        const float arenaHi = std::isfinite( maxSpeed ) ? ( std::max )( safeMin, maxSpeed ) : safeMin;
        if ( !std::isfinite( currentSpeed ) || !std::isfinite( savedBaseSpeed ) || savedBaseSpeed <= 0.0f )
            return std::isfinite( currentSpeed ) ? std::clamp( currentSpeed, safeMin, arenaHi ) : safeMin;

        const float unboosted = currentSpeed / kLimiterSpeedMultiplier;
        const float target = ( std::max )( savedBaseSpeed, unboosted );
        return std::clamp( ( std::min )( currentSpeed, target ), safeMin, arenaHi );
    }

    inline Shapes::Physics::Vec2 rotateVec2( const Shapes::Physics::Vec2& v, float angleRad ) noexcept
    {
        const float c = std::cos( angleRad );
        const float s = std::sin( angleRad );
        return { v.x * c - v.y * s, v.x * s + v.y * c };
    }

    // Spawns angled split balls from the lead ball up to maxBalls (preserving speed and minAbsSinAngle).
    // When ensureUpward is true (e.g. splitting from a Mag-Locked ball), guarantees clone.vel.y > 0.
    inline std::vector<Shapes::Physics::BallState> createSplitBalls(
        const Shapes::Physics::BallState& sourceBall,
        std::size_t currentBallCount,
        const Shapes::Physics::ArenaConfig& cfg,
        std::size_t maxBalls = kMaxBalls,
        bool ensureUpward = false )
    {
        std::vector<Shapes::Physics::BallState> spawned;
        if ( currentBallCount >= maxBalls || !std::isfinite( sourceBall.radius ) || sourceBall.radius <= 0.0f )
            return spawned;

        const float safeMin = std::isfinite( cfg.minSpeed ) ? ( std::max )( 0.0f, cfg.minSpeed ) : 60.0f;
        const float arenaHi = std::isfinite( cfg.maxSpeed ) ? ( std::max )( safeMin, cfg.maxSpeed ) : ( std::max )( safeMin, 1200.0f );
        const float rawLen = Shapes::Physics::length( sourceBall.vel );
        const float spd = std::isfinite( rawLen ) ? std::clamp( rawLen, safeMin, arenaHi ) : safeMin;
        const Shapes::Physics::Vec2 baseDir = Shapes::Physics::normalizeSafe( sourceBall.vel, { 0.4f, 0.9f } );

        const std::size_t toSpawn = maxBalls - currentBallCount;
        const float spreadRad = glm::radians( kSplitSpreadDeg );
        const float angles[ 2 ] = { -spreadRad, +spreadRad };

        for ( std::size_t i = 0; i < toSpawn && i < 2u; ++i )
        {
            Shapes::Physics::BallState clone = sourceBall;
            const Shapes::Physics::Vec2 rotDir = rotateVec2( baseDir, angles[ i ] );
            clone.vel = Shapes::Physics::detail::enforceMinVerticalSlope( rotDir * spd, cfg.minAbsSinAngle );
            if ( ensureUpward && clone.vel.y < 0.0f )
                clone.vel.y = -clone.vel.y;
            spawned.push_back( clone );
        }
        return spawned;
    }

    // Selects the most urgent ball for --robot tracking during Multiball:
    // smallest positive time-to-impact among descending balls, otherwise the lowest ball.
    inline std::size_t selectPrimaryBallIndex(
        const std::vector<Shapes::Physics::BallState>& balls,
        float paddleTopY ) noexcept
    {
        if ( balls.empty() )
            return 0u;

        bool foundDescending = false;
        std::size_t bestIdx = 0u;
        float bestTti = ( std::numeric_limits<float>::max )();
        float lowestY = ( std::numeric_limits<float>::max )();
        std::size_t lowestIdx = 0u;

        for ( std::size_t i = 0; i < balls.size(); ++i )
        {
            const auto& b = balls[ i ];
            if ( b.center.y < lowestY )
            {
                lowestY = b.center.y;
                lowestIdx = i;
            }

            if ( b.vel.y < -1e-4f )
            {
                const float dy = ( std::max )( 0.0f, b.center.y - ( paddleTopY + b.radius ) );
                const float tti = dy / ( -b.vel.y );
                if ( !foundDescending || tti < bestTti )
                {
                    foundDescending = true;
                    bestTti = tti;
                    bestIdx = i;
                }
            }
        }

        return foundDescending ? bestIdx : lowestIdx;
    }
} // namespace AiProtocols
