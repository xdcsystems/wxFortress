#pragma once

#include <glm/glm.hpp>

class xPoint
{
    public:
        inline xPoint();
        inline xPoint( float x, float y );
        xPoint( const glm::vec2& pt )
        {
            m_x = ( float )pt.x; m_y = ( float )pt.y;
        }

        // default copy ctor and copy-assign operator are OK

        // two different conversions to integers, floor and rounding
        inline void GetFloor( int* x, int* y ) const;
        inline void GetRounded( int* x, int* y ) const;

        inline float GetVectorLength() const;
        float GetVectorAngle() const;
        void SetVectorLength( float length );
        void SetVectorAngle( float degrees );
        // set the vector length to 1.0, preserving the angle
        void Normalize();

        inline float GetDistance( const xPoint& pt ) const;
        inline float GetDistanceSquare( const xPoint& pt ) const;
        inline float GetDotProduct( const xPoint& vec ) const;
        inline float GetCrossProduct( const xPoint& vec ) const;

        // the reflection of this point
        xPoint operator-() const;

        inline xPoint& operator+=( const xPoint& pt );
        inline xPoint& operator-=( const xPoint& pt );
        inline xPoint& operator*=( const xPoint& pt );
        inline xPoint& operator*=( float n );
        inline xPoint& operator*=( int n );
        inline xPoint& operator/=( const xPoint& pt );
        inline xPoint& operator/=( float n );
        inline xPoint& operator/=( int n );

        inline bool operator==( const xPoint& pt ) const;
        inline bool operator!=( const xPoint& pt ) const;

        float m_x;
        float m_y;
};

inline xPoint operator+( const xPoint& pt1, const xPoint& pt2 );
inline xPoint operator-( const xPoint& pt1, const xPoint& pt2 );
inline xPoint operator*( const xPoint& pt1, const xPoint& pt2 );
inline xPoint operator*( float n, const xPoint& pt );
inline xPoint operator*( int n, const xPoint& pt );
inline xPoint operator*( const xPoint& pt, float n );
inline xPoint operator*( const xPoint& pt, int n );
inline xPoint operator/( const xPoint& pt1, const xPoint& pt2 );
inline xPoint operator/( const xPoint& pt, float n );
inline xPoint operator/( const xPoint& pt, int n );

inline xPoint::xPoint()
{
    m_x = 0.0;
    m_y = 0.0;
}

inline xPoint::xPoint( float x, float y )
{
    m_x = x;
    m_y = y;
}

inline void xPoint::GetFloor( int* x, int* y ) const
{
    *x = ( int )floor( m_x );
    *y = ( int )floor( m_y );
}

inline void xPoint::GetRounded( int* x, int* y ) const
{
    *x = ( int )floor( m_x + 0.5 );
    *y = ( int )floor( m_y + 0.5 );
}

inline float xPoint::GetVectorLength() const
{
    return sqrt( ( m_x ) * ( m_x )+( m_y ) * ( m_y ) );
}

inline void xPoint::SetVectorLength( float length )
{
    float before = GetVectorLength();
    m_x = ( m_x * length / before );
    m_y = ( m_y * length / before );
}

inline void xPoint::Normalize()
{
    SetVectorLength( 1 );
}

inline float xPoint::GetDistance( const xPoint& pt ) const
{
    return sqrt( GetDistanceSquare( pt ) );
}

inline float xPoint::GetDistanceSquare( const xPoint& pt ) const
{
    return ( ( pt.m_x - m_x ) * ( pt.m_x - m_x ) + ( pt.m_y - m_y ) * ( pt.m_y - m_y ) );
}

inline float xPoint::GetDotProduct( const xPoint& vec ) const
{
    return ( m_x * vec.m_x + m_y * vec.m_y );
}

inline float xPoint::GetCrossProduct( const xPoint& vec ) const
{
    return ( m_x * vec.m_y - vec.m_x * m_y );
}

inline xPoint xPoint::operator-() const
{
    return xPoint( -m_x, -m_y );
}

inline xPoint& xPoint::operator+=( const xPoint& pt )
{
    m_x = m_x + pt.m_x;
    m_y = m_y + pt.m_y;
    return *this;
}

inline xPoint& xPoint::operator-=( const xPoint& pt )
{
    m_x = m_x - pt.m_x;
    m_y = m_y - pt.m_y;
    return *this;
}

inline xPoint& xPoint::operator*=( const xPoint& pt )
{
    m_x = m_x * pt.m_x;
    m_y = m_y * pt.m_y;
    return *this;
}

inline xPoint& xPoint::operator/=( const xPoint& pt )
{
    m_x = m_x / pt.m_x;
    m_y = m_y / pt.m_y;
    return *this;
}

inline bool xPoint::operator==( const xPoint& pt ) const
{
    return m_x == pt.m_x && m_y == pt.m_y;
}

inline bool xPoint::operator!=( const xPoint& pt ) const
{
    return !( *this == pt );
}

inline xPoint operator+( const xPoint& pt1, const xPoint& pt2 )
{
    return xPoint( pt1.m_x + pt2.m_x, pt1.m_y + pt2.m_y );
}

inline xPoint operator-( const xPoint& pt1, const xPoint& pt2 )
{
    return xPoint( pt1.m_x - pt2.m_x, pt1.m_y - pt2.m_y );
}


inline xPoint operator*( const xPoint& pt1, const xPoint& pt2 )
{
    return xPoint( pt1.m_x * pt2.m_x, pt1.m_y * pt2.m_y );
}

inline xPoint operator*( float n, const xPoint& pt )
{
    return xPoint( pt.m_x * n, pt.m_y * n );
}

inline xPoint operator*( int n, const xPoint& pt )
{
    return xPoint( pt.m_x * n, pt.m_y * n );
}

inline xPoint operator*( const xPoint& pt, float n )
{
    return xPoint( pt.m_x * n, pt.m_y * n );
}

inline xPoint operator*( const xPoint& pt, int n )
{
    return xPoint( pt.m_x * n, pt.m_y * n );
}

inline xPoint operator/( const xPoint& pt1, const xPoint& pt2 )
{
    return xPoint( pt1.m_x / pt2.m_x, pt1.m_y / pt2.m_y );
}

inline xPoint operator/( const xPoint& pt, float n )
{
    return xPoint( pt.m_x / n, pt.m_y / n );
}

inline xPoint operator/( const xPoint& pt, int n )
{
    return xPoint( pt.m_x / n, pt.m_y / n );
}

