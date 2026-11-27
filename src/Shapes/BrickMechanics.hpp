#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>


namespace BrickMechanics
{
    // Matches Shapes::BrickType enum values without pulling in wxWidgets / Base.h
    constexpr unsigned char kTypeNone   = 0;
    constexpr unsigned char kTypeAqua   = 1;
    constexpr unsigned char kTypeRed    = 2;
    constexpr unsigned char kTypeYellow = 3;
    constexpr unsigned char kTypeGreen  = 4;
    constexpr unsigned char kTypeOrange = 5;
    constexpr unsigned char kTypePurple = 6;
    constexpr unsigned char kTypeBlue   = 7;
    constexpr unsigned char kTypeHalf   = 8;

    // Reactive RED cell blast constraints (prevents single-hit full-row wipe)
    constexpr unsigned int kMaxChainDepth = 1u;
    constexpr float kBlastRangeScale = 1.15f;

    // EM Accelerator (PURPLE) direct-hit impulse (+10%, capped at playable EM ceiling)
    constexpr float kEmSpeedMultiplier = 1.10f;
    constexpr float kEmMaxSpeed = 520.0f;
    // Per-level speed ceiling on level transition (tension sawtooth: prevents runaway speed into Level 5)
    constexpr float kLevelStartMaxSpeed = 360.0f;

    // Armor hit flash duration (seconds)
    constexpr float kArmorFlashDurationSec = 0.09f;

    enum class BrickRole : unsigned char
    {
        Standard,
        ReactiveCell,      // RED: 3x3 chain detonation (depth-capped)
        CompositeArmor,    // BLUE: 2-HP (3-HP on Stage 2+) with Ping SFX and visual damage tint
        EmAccelerator,     // PURPLE: direct-hit +10% EM velocity boost (clamped)
        ProtocolContainer, // YELLOW: 2x resonance score + bonus streak step
    };

    inline BrickRole roleForType( unsigned char typeVal ) noexcept
    {
        switch ( typeVal )
        {
            case kTypeRed:    return BrickRole::ReactiveCell;
            case kTypeBlue:   return BrickRole::CompositeArmor;
            case kTypePurple: return BrickRole::EmAccelerator;
            case kTypeYellow: return BrickRole::ProtocolContainer;
            default:          return BrickRole::Standard;
        }
    }

    inline unsigned char initialHpForType( unsigned char typeVal, unsigned short stage = 0 ) noexcept
    {
        if ( typeVal == kTypeNone || typeVal == kTypeHalf )
            return 0u;
        if ( typeVal == kTypeBlue )
            return ( stage >= 1u ) ? 3u : 2u;
        return 1u;
    }

    // Returns OpenGL 2.1 SpriteRenderer tint for a brick (no extra textures/shaders).
    // On armor impact flash, slight RGB overdrive (>1.0 clamped after texel multiply on RGBA8)
    // yields a crisp white-cyan flash; damaged armor settles into a visibly darker steel tone.
    inline glm::vec3 damageTint( unsigned char hp, unsigned char maxHp, float flashSec ) noexcept
    {
        if ( std::isfinite( flashSec ) && flashSec > 0.0f )
            return { 1.65f, 1.65f, 1.65f };

        if ( maxHp <= 1u || hp >= maxHp )
            return { 1.0f, 1.0f, 1.0f };

        if ( hp <= 1u )
            return { 0.48f, 0.56f, 0.70f };

        // Intermediate damage (e.g. 2 of 3 HP on Stage 2+)
        return { 0.72f, 0.78f, 0.88f };
    }

    inline float applyEmBoost( float currentSpeed, float minSpeed, float maxSpeed, float emCap = kEmMaxSpeed ) noexcept
    {
        const float safeMin = std::isfinite( minSpeed ) ? ( std::max )( 0.0f, minSpeed ) : 0.0f;
        if ( !std::isfinite( currentSpeed ) || !std::isfinite( maxSpeed ) || !std::isfinite( emCap ) )
            return safeMin;
        const float arenaHi = ( std::max )( safeMin, maxSpeed );
        const float clampedCur = std::clamp( currentSpeed, safeMin, arenaHi );
        const float effectiveCap = ( std::min )( arenaHi, ( std::max )( clampedCur, ( std::max )( safeMin, emCap ) ) );
        return std::clamp( clampedCur * kEmSpeedMultiplier, safeMin, effectiveCap );
    }

    struct BlastBrick
    {
        glm::vec2 center { 0.0f, 0.0f };
        glm::vec2 size { 57.0f, 27.0f };
        unsigned char typeVal { kTypeNone };
        unsigned char hp { 0 };
        bool alive { false };
    };

    struct BlastResult
    {
        // Indices destroyed by the 3x3 blast (excluding initial direct detonators)
        std::vector<std::size_t> destroyedIndices;
        // Pairs of (index, damageDealt) for bricks that took non-fatal blast damage and survived
        std::vector<std::pair<std::size_t, unsigned char>> survivedDamaged;
        bool anySecondaryChain { false };
    };

    // Resolves depth-capped BFS 3x3 reactive detonation starting from already-destroyed RED bricks.
    // Mutates `bricks` hp/alive in place so callers and tests observe exact post-blast state.
    inline BlastResult resolveBlastChain(
        std::vector<BlastBrick>& bricks,
        const std::vector<std::size_t>& initialDetonators,
        unsigned int maxChainDepth = kMaxChainDepth )
    {
        BlastResult out {};
        if ( bricks.empty() || initialDetonators.empty() )
            return out;

        const std::size_t n = bricks.size();
        std::vector<unsigned char> detonated( n, 0u );
        std::vector<unsigned char> damageTaken( n, 0u );
        std::vector<unsigned char> destroyedByBlast( n, 0u );
        std::deque<std::pair<std::size_t, unsigned int>> queue;

        for ( const std::size_t idx : initialDetonators )
        {
            if ( idx < n && !detonated[ idx ] && roleForType( bricks[ idx ].typeVal ) == BrickRole::ReactiveCell )
            {
                detonated[ idx ] = 1u;
                queue.emplace_back( idx, 0u );
            }
        }

        while ( !queue.empty() )
        {
            const auto [ srcIdx, depth ] = queue.front();
            queue.pop_front();

            const BlastBrick& src = bricks[ srcIdx ];
            const float rangeX = ( std::max )( 1.0f, src.size.x ) * kBlastRangeScale;
            const float rangeY = ( std::max )( 1.0f, src.size.y ) * kBlastRangeScale;

            for ( std::size_t j = 0; j < n; ++j )
            {
                if ( j == srcIdx || !bricks[ j ].alive )
                    continue;

                const float dx = std::fabs( bricks[ j ].center.x - src.center.x );
                const float dy = std::fabs( bricks[ j ].center.y - src.center.y );
                if ( !( dx <= rangeX && dy <= rangeY ) )
                    continue;

                if ( bricks[ j ].hp > 1u )
                {
                    --bricks[ j ].hp;
                    ++damageTaken[ j ];
                }
                else
                {
                    bricks[ j ].hp = 0u;
                    bricks[ j ].alive = false;
                    destroyedByBlast[ j ] = 1u;
                    out.destroyedIndices.push_back( j );

                    if ( roleForType( bricks[ j ].typeVal ) == BrickRole::ReactiveCell &&
                         depth < maxChainDepth &&
                         !detonated[ j ] )
                    {
                        detonated[ j ] = 1u;
                        out.anySecondaryChain = true;
                        queue.emplace_back( j, depth + 1u );
                    }
                }
            }
        }

        for ( std::size_t j = 0; j < n; ++j )
        {
            if ( damageTaken[ j ] > 0u && !destroyedByBlast[ j ] && bricks[ j ].alive )
            {
                out.survivedDamaged.emplace_back( j, damageTaken[ j ] );
            }
        }

        return out;
    }
} // namespace BrickMechanics
