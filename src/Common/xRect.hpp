#pragma once

#include <glm/glm.hpp>

class xRect
{
    public:
        xRect()
        {
            m_x = m_y = m_width = m_height = 0;
        }
        xRect( float x, float y, float w, float h )
        {
            m_x = x; m_y = y; m_width = w;  m_height = h;
        }
        xRect( const glm::vec2& pos, const glm::vec2& size )
        {
            m_x = pos.x; m_y = pos.y; m_width = size.x;  m_height = size.y;
        }
        /*
            xRect(const glm::vec2& topLeft, const glm::vec2& bottomRight);
            xRect(const glm::vec2& pos, const wxSize& size);
            xRect(const xRect& rect);
        */
        // single attribute accessors

        glm::vec2 GetPosition() const { return { m_x, m_y }; }
        glm::vec2 GetSize() const { return { m_width, m_height }; }

        // for the edge and corner accessors there are two setters counterparts, the Set.. functions keep the other corners at their
        // position whenever sensible, the Move.. functions keep the size of the rect and move the other corners appropriately

        inline float GetLeft() const { return m_x; }
        inline void SetLeft( float n ) { m_width += m_x - n; m_x = n; }
        inline void MoveLeftTo( float n ) { m_x = n; }
        inline float GetTop() const { return m_y; }
        inline void SetTop( float n ) { m_height += m_y - n; m_y = n; }
        inline void MoveTopTo( float n ) { m_y = n; }
        inline float GetBottom() const { return m_y + m_height; }
        inline void SetBottom( float n ) { m_height += n - ( m_y + m_height ); }
        inline void MoveBottomTo( float n ) { m_y = n - m_height; }
        inline float GetRight() const { return m_x + m_width; }
        inline void SetRight( float n ) { m_width += n - ( m_x + m_width ); }
        inline void MoveRightTo( float n ) { m_x = n - m_width; }

        inline glm::vec2 GetLeftTop() const { return { m_x, m_y }; }
        inline void SetLeftTop( const glm::vec2& pt )
        {
            m_width += m_x - pt.x; m_height += m_y - pt.y; m_x = pt.x; m_y = pt.y;
        }
        inline void MoveLeftTopTo( const glm::vec2& pt )
        {
            m_x = pt.x; m_y = pt.y;
        }
        inline glm::vec2 GetLeftBottom() const
        {
            return { m_x, m_y + m_height };
        }
        inline void SetLeftBottom( const glm::vec2& pt )
        {
            m_width += m_x - pt.x; m_height += pt.y - ( m_y + m_height ); m_x = pt.x;
        }
        inline void MoveLeftBottomTo( const glm::vec2& pt )
        {
            m_x = pt.x; m_y = pt.y - m_height;
        }
        inline glm::vec2 GetRightTop() const
        {
            return { m_x + m_width, m_y };
        }
        inline void SetRightTop( const glm::vec2& pt )
        {
            m_width += pt.x - ( m_x + m_width ); m_height += m_y - pt.y; m_y = pt.y;
        }
        inline void MoveRightTopTo( const glm::vec2& pt )
        {
            m_x = pt.x - m_width; m_y = pt.y;
        }
        inline glm::vec2 GetRightBottom() const
        {
            return { m_x + m_width, m_y + m_height };
        }
        inline void SetRightBottom( const glm::vec2& pt )
        {
            m_width += pt.x - ( m_x + m_width ); m_height += pt.y - ( m_y + m_height );
        }
        inline void MoveRightBottomTo( const glm::vec2& pt )
        {
            m_x = pt.x - m_width; m_y = pt.y - m_height;
        }
        inline glm::vec2 GetCentre() const
        {
            return { m_x + m_width / 2, m_y + m_height / 2 };
        }
        inline void SetCentre( const glm::vec2& pt )
        {
            MoveCentreTo( pt );
        }    // since this is impossible without moving...
        inline void MoveCentreTo( const glm::vec2& pt )
        {
            m_x += pt.x - ( m_x + m_width / 2 ); m_y += pt.y - ( m_y + m_height / 2 );
        }
        inline bool Contains( const xRect& rect ) const
        {
            return ( ( ( m_x <= rect.m_x ) && ( rect.m_x + rect.m_width <= m_x + m_width ) ) &&
                ( ( m_y <= rect.m_y ) && ( rect.m_y + rect.m_height <= m_y + m_height ) ) );
        }
        inline bool IsEmpty() const
        {
            return m_width <= 0 || m_height <= 0;
        }
        inline bool HaveEqualSize( const xRect& rect ) const
        {
            return rect.m_width == m_width && rect.m_height == m_height;
        }

        inline void Inset( float x, float y )
        {
            m_x += x; m_y += y; m_width -= 2 * x; m_height -= 2 * y;
        }
        inline void Inset( float left, float top, float right, float bottom )
        {
            m_x += left; m_y += top; m_width -= left + right; m_height -= top + bottom;
        }
        inline void Offset( const glm::vec2& pt )
        {
            m_x += pt.x; m_y += pt.y;
        }

        void ConstrainTo( const xRect& rect )
        {
            if ( GetLeft() < rect.GetLeft() )
                SetLeft( rect.GetLeft() );

            if ( GetRight() > rect.GetRight() )
                SetRight( rect.GetRight() );

            if ( GetBottom() > rect.GetBottom() )
                SetBottom( rect.GetBottom() );

            if ( GetTop() < rect.GetTop() )
                SetTop( rect.GetTop() );
        }

        glm::vec2 Interpolate( int widthfactor, int heightfactor ) const
        {
            return glm::vec2( m_x + m_width * widthfactor, m_y + m_height * heightfactor );
        }

        static void Intersect( const xRect& src1, const xRect& src2, xRect* dest )
        {
            float left = std::max( src1.m_x, src2.m_x );
            float right = std::min( src1.m_x + src1.m_width, src2.m_x + src2.m_width );
            float top = std::max( src1.m_y, src2.m_y );
            float bottom = std::min( src1.m_y + src1.m_height, src2.m_y + src2.m_height );

            if ( left < right && top < bottom )
            {
                dest->m_x = left;
                dest->m_y = top;
                dest->m_width = right - left;
                dest->m_height = bottom - top;
            }
            else
            {
                dest->m_width = dest->m_height = 0;
            }
        };
        inline void Intersect( const xRect& otherRect )
        {
            Intersect( *this, otherRect, this );
        }
        inline xRect CreateIntersection( const xRect& otherRect ) const
        {
            xRect result; Intersect( *this, otherRect, &result ); return result;
        }
        bool Intersects( const xRect& rect ) const
        {
            float left = std::max( m_x, rect.m_x );
            float right = std::min( m_x + m_width, rect.m_x + rect.m_width );
            float top = std::max( m_y, rect.m_y );
            float bottom = std::min( m_y + m_height, rect.m_y + rect.m_height );

            if ( left < right && top < bottom )
            {
                return true;
            }
            return false;

        };

        static void Union( const xRect& src1, const xRect& src2, xRect* dest );
        void Union( const xRect& otherRect )
        {
            Union( *this, otherRect, this );
        }
        void Union( const glm::vec2& pt );
        inline xRect CreateUnion( const xRect& otherRect ) const
        {
            xRect result; Union( *this, otherRect, &result ); return result;
        }

        inline void Scale( float f )
        {
            m_x *= f; m_y *= f; m_width *= f; m_height *= f;
        }
        inline void Scale( int num, int denum )
        {
            m_x *= ( ( float )num ) / ( ( float )denum ); m_y *= ( ( float )num ) / ( ( float )denum );
            m_width *= ( ( float )num ) / ( ( float )denum ); m_height *= ( ( float )num ) / ( ( float )denum );
        }

        inline bool operator == ( const xRect& rect ) const
        {
            return m_x == rect.m_x && m_y == rect.m_y && HaveEqualSize( rect );
        }
        inline bool operator != ( const xRect& rect ) const
        {
            return !( *this == rect );
        }

        float  m_x;
        float  m_y;
        float  m_width;
        float m_height;
};