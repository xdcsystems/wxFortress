#pragma once

#include <glm/glm.hpp>

class Point
{
    public:
        inline Point();
        inline Point( float x, float y );
        Point( const glm::vec2 &pt )
        {
            m_x = (float)pt.x;
            m_y = (float)pt.y;
        }

        // default copy ctor and copy-assign operator are OK

        // two different conversions to integers, floor and rounding
        inline void GetFloor( int *x, int *y ) const;
        inline void GetRounded( int *x, int *y ) const;

        inline float GetVectorLength() const;
        void SetVectorLength( float length );

        // set the vector length to 1.0, preserving the angle
        void Normalize();

        inline float GetDistance( const Point &pt ) const;
        inline float GetDistanceSquare( const Point &pt ) const;
        inline float GetDotProduct( const Point &vec ) const;
        inline float GetCrossProduct( const Point &vec ) const;

        // the reflection of this point
        Point operator- () const;

        inline Point &operator+= ( const Point &pt );
        inline Point &operator-= ( const Point &pt );
        inline Point &operator*= ( const Point &pt );
        inline Point &operator*= ( float n );
        inline Point &operator*= ( int n );
        inline Point &operator/= ( const Point &pt );
        inline Point &operator/= ( float n );
        inline Point &operator/= ( int n );

        inline bool operator== ( const Point &pt ) const;
        inline bool operator!= ( const Point &pt ) const;

        float m_x;
        float m_y;
};

inline Point operator+ ( const Point &pt1, const Point &pt2 );
inline Point operator- ( const Point &pt1, const Point &pt2 );
inline Point operator* ( const Point &pt1, const Point &pt2 );
inline Point operator* ( float n, const Point &pt );
inline Point operator* ( int n, const Point &pt );
inline Point operator* ( const Point &pt, float n );
inline Point operator* ( const Point &pt, int n );
inline Point operator/ ( const Point &pt1, const Point &pt2 );
inline Point operator/ ( const Point &pt, float n );
inline Point operator/ ( const Point &pt, int n );

inline Point::Point()
{
    m_x = 0.0;
    m_y = 0.0;
}

inline Point::Point( float x, float y )
{
    m_x = x;
    m_y = y;
}

inline void Point::GetFloor( int *x, int *y ) const
{
    *x = (int)floor( m_x );
    *y = (int)floor( m_y );
}

inline void Point::GetRounded( int *x, int *y ) const
{
    *x = (int)floor( m_x + 0.5 );
    *y = (int)floor( m_y + 0.5 );
}

inline float Point::GetVectorLength() const
{
    return sqrt( ( m_x ) * ( m_x ) + ( m_y ) * ( m_y ) );
}

inline void Point::SetVectorLength( float length )
{
    float before = GetVectorLength();
    m_x = ( m_x * length / before );
    m_y = ( m_y * length / before );
}

inline void Point::Normalize()
{
    SetVectorLength( 1 );
}

inline float Point::GetDistance( const Point &pt ) const
{
    return sqrt( GetDistanceSquare( pt ) );
}

inline float Point::GetDistanceSquare( const Point &pt ) const
{
    return ( ( pt.m_x - m_x ) * ( pt.m_x - m_x ) + ( pt.m_y - m_y ) * ( pt.m_y - m_y ) );
}

inline float Point::GetDotProduct( const Point &vec ) const
{
    return ( m_x * vec.m_x + m_y * vec.m_y );
}

inline float Point::GetCrossProduct( const Point &vec ) const
{
    return ( m_x * vec.m_y - vec.m_x * m_y );
}

inline Point Point::operator- () const
{
    return Point( -m_x, -m_y );
}

inline Point &Point::operator+= ( const Point &pt )
{
    m_x = m_x + pt.m_x;
    m_y = m_y + pt.m_y;
    return *this;
}

inline Point &Point::operator-= ( const Point &pt )
{
    m_x = m_x - pt.m_x;
    m_y = m_y - pt.m_y;
    return *this;
}

inline Point &Point::operator*= ( const Point &pt )
{
    m_x = m_x * pt.m_x;
    m_y = m_y * pt.m_y;
    return *this;
}

inline Point &Point::operator/= ( const Point &pt )
{
    m_x = m_x / pt.m_x;
    m_y = m_y / pt.m_y;
    return *this;
}

inline bool Point::operator== ( const Point &pt ) const
{
    return m_x == pt.m_x && m_y == pt.m_y;
}

inline bool Point::operator!= ( const Point &pt ) const
{
    return !( *this == pt );
}

inline Point operator+ ( const Point &pt1, const Point &pt2 )
{
    return Point( pt1.m_x + pt2.m_x, pt1.m_y + pt2.m_y );
}

inline Point operator- ( const Point &pt1, const Point &pt2 )
{
    return Point( pt1.m_x - pt2.m_x, pt1.m_y - pt2.m_y );
}

inline Point operator* ( const Point &pt1, const Point &pt2 )
{
    return Point( pt1.m_x * pt2.m_x, pt1.m_y * pt2.m_y );
}

inline Point operator* ( float n, const Point &pt )
{
    return Point( pt.m_x * n, pt.m_y * n );
}

inline Point operator* ( int n, const Point &pt )
{
    return Point( pt.m_x * n, pt.m_y * n );
}

inline Point operator* ( const Point &pt, float n )
{
    return Point( pt.m_x * n, pt.m_y * n );
}

inline Point operator* ( const Point &pt, int n )
{
    return Point( pt.m_x * n, pt.m_y * n );
}

inline Point operator/ ( const Point &pt1, const Point &pt2 )
{
    return Point( pt1.m_x / pt2.m_x, pt1.m_y / pt2.m_y );
}

inline Point operator/ ( const Point &pt, float n )
{
    return Point( pt.m_x / n, pt.m_y / n );
}

inline Point operator/ ( const Point &pt, int n )
{
    return Point( pt.m_x / n, pt.m_y / n );
}
