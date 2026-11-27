#pragma once

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace MathUtil
{
    template <typename T>
    inline bool isPositiveFinite( T value ) noexcept
    {
        static_assert( std::is_floating_point_v<T>, "isPositiveFinite requires a floating-point type" );
        return std::isfinite( value ) && value > static_cast<T>( 0 );
    }

    template <typename T>
    inline T clampFinite( T value, T lo, T hi, T fallback ) noexcept
    {
        static_assert( std::is_floating_point_v<T>, "clampFinite requires a floating-point type" );
        if ( !std::isfinite( value ) || !std::isfinite( lo ) || !std::isfinite( hi ) )
            return fallback;
        if ( lo > hi )
            return lo;
        return std::clamp( value, lo, hi );
    }

    template <typename T>
    inline T moveToward( T current, T target, T maxDelta ) noexcept
    {
        static_assert( std::is_floating_point_v<T>, "moveToward requires a floating-point type" );
        if ( !std::isfinite( current ) || !std::isfinite( target ) || !std::isfinite( maxDelta ) || maxDelta <= static_cast<T>( 0 ) )
            return current;
        const T diff = target - current;
        if ( std::abs( diff ) <= maxDelta )
            return target;
        return current + ( diff > static_cast<T>( 0 ) ? maxDelta : -maxDelta );
    }
} // namespace MathUtil
