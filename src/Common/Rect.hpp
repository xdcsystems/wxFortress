#pragma once

#include <glm/glm.hpp>

class Rect
{
    public:
        Rect()
        {
            m_x = m_y = m_width = m_height = 0;
        }
        Rect( float x, float y, float w, float h )
        {
            m_x = x; m_y = y; m_width = w;  m_height = h;
        }
        Rect( const glm::vec2& pos, const glm::vec2& size )
        {
            m_x = pos.x; m_y = pos.y; m_width = size.x;  m_height = size.y;
        }
        /*
            Rect(const glm::vec2& topLeft, const glm::vec2& bottomRight);
            Rect(const glm::vec2& pos, const wxSize& size);
            Rect(const Rect& rect);
        */
        // single attribute accessors

        glm::vec2 position() const { return { m_x, m_y }; }
        glm::vec2 size() const { return { m_width, m_height }; }

        // for the edge and corner accessors there are two setters counterparts, the Set.. functions keep the other corners at their
        // position whenever sensible, the Move.. functions keep the size of the rect and move the other corners appropriately

        inline float left() const { return m_x; }
        inline void left( float n ) { m_width += m_x - n; m_x = n; }
        inline void moveLeftTo( float n ) { m_x = n; }
        inline float top() const { return m_y; }
        inline void top( float n ) { m_height += m_y - n; m_y = n; }
        inline void moveTopTo( float n ) { m_y = n; }
        inline float bottom() const { return m_y + m_height; }
        inline void bottom( float n ) { m_height += n - ( m_y + m_height ); }
        inline void moveBottomTo( float n ) { m_y = n - m_height; }
        inline float right() const { return m_x + m_width; }
        inline void right( float n ) { m_width += n - ( m_x + m_width ); }
        inline void moveRightTo( float n ) { m_x = n - m_width; }

        inline glm::vec2 leftTop() const { return { m_x, m_y }; }
        inline void leftTop( const glm::vec2& pt )
        {
            m_width += m_x - pt.x; m_height += m_y - pt.y; m_x = pt.x; m_y = pt.y;
        }
        inline void moveLeftTopTo( const glm::vec2& pt )
        {
            m_x = pt.x; m_y = pt.y;
        }
        inline glm::vec2 leftBottom() const
        {
            return { m_x, m_y + m_height };
        }
        inline void leftBottom( const glm::vec2& pt )
        {
            m_width += m_x - pt.x; m_height += pt.y - ( m_y + m_height ); m_x = pt.x;
        }
        inline void moveLeftBottomTo( const glm::vec2& pt )
        {
            m_x = pt.x; m_y = pt.y - m_height;
        }
        inline glm::vec2 rightTop() const
        {
            return { m_x + m_width, m_y };
        }
        inline void rightTop( const glm::vec2& pt )
        {
            m_width += pt.x - ( m_x + m_width ); m_height += m_y - pt.y; m_y = pt.y;
        }
        inline void moveRightTopTo( const glm::vec2& pt )
        {
            m_x = pt.x - m_width; m_y = pt.y;
        }
        inline glm::vec2 rightBottom() const
        {
            return { m_x + m_width, m_y + m_height };
        }
        inline void rightBottom( const glm::vec2& pt )
        {
            m_width += pt.x - ( m_x + m_width ); m_height += pt.y - ( m_y + m_height );
        }
        inline void moveRightBottomTo( const glm::vec2& pt )
        {
            m_x = pt.x - m_width; m_y = pt.y - m_height;
        }
        inline glm::vec2 centre() const
        {
            return { m_x + m_width / 2, m_y + m_height / 2 };
        }
        inline void centre( const glm::vec2& pt )
        {
            moveCentreTo( pt );
        }    // since this is impossible without moving...
        inline void moveCentreTo( const glm::vec2& pt )
        {
            m_x += pt.x - ( m_x + m_width / 2 ); m_y += pt.y - ( m_y + m_height / 2 );
        }
        inline bool contains( const Rect& rect ) const
        {
            return ( ( ( m_x <= rect.m_x ) && ( rect.m_x + rect.m_width <= m_x + m_width ) ) &&
                ( ( m_y <= rect.m_y ) && ( rect.m_y + rect.m_height <= m_y + m_height ) ) );
        }
        inline bool isEmpty() const
        {
            return m_width <= 0 || m_height <= 0;
        }
        inline bool haveEqualSize( const Rect& rect ) const
        {
            return rect.m_width == m_width && rect.m_height == m_height;
        }

        inline void inset( float x, float y )
        {
            m_x += x; m_y += y; m_width -= 2 * x; m_height -= 2 * y;
        }
        inline void inset( float left, float top, float right, float bottom )
        {
            m_x += left; m_y += top; m_width -= left + right; m_height -= top + bottom;
        }
        inline void offset( const glm::vec2& pt )
        {
            m_x += pt.x; m_y += pt.y;
        }

        void constrainTo( const Rect& rect )
        {
            if ( left() < rect.left() )
            {
                left( rect.left() );
            }

            if ( right() > rect.right() )
            {
                right( rect.right() );
            }

            if ( bottom() > rect.bottom() )
            {
                bottom( rect.bottom() );
            }

            if ( top() < rect.top() )
            {
                top( rect.top() );
            }
        }

        glm::vec2 interpolate( int widthfactor, int heightfactor ) const
        {
            return glm::vec2( m_x + m_width * widthfactor, m_y + m_height * heightfactor );
        }

        static void Intersect( const Rect& src1, const Rect& src2, Rect* dest )
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
        inline void intersect( const Rect& otherRect )
        {
            Intersect( *this, otherRect, this );
        }
        inline Rect createIntersection( const Rect& otherRect ) const
        {
            Rect result; 
            Intersect( *this, otherRect, &result ); 
            return result;
        }
        bool intersects( const Rect& rect ) const
        {
            float left = std::max( m_x, rect.m_x );
            float right = std::min( m_x + m_width, rect.m_x + rect.m_width );
            float top = std::max( m_y, rect.m_y );
            float bottom = std::min( m_y + m_height, rect.m_y + rect.m_height );

            return ( left < right&& top < bottom );
        };

        /*static void Union( const Rect& src1, const Rect& src2, Rect* dest );
        void Union( const Rect& otherRect )
        {
            Union( *this, otherRect, this );
        }
        void Union( const glm::vec2& pt );
        inline Rect createUnion( const Rect& otherRect ) const
        {
            Rect result; 
            Union( *this, otherRect, &result ); 
            return result;
        }*/

        inline void scale( float f )
        {
            m_x *= f; m_y *= f; m_width *= f; m_height *= f;
        }
        inline void scale( int num, int denum )
        {
            m_x *= ( ( float )num ) / ( ( float )denum ); m_y *= ( ( float )num ) / ( ( float )denum );
            m_width *= ( ( float )num ) / ( ( float )denum ); m_height *= ( ( float )num ) / ( ( float )denum );
        }

        inline bool operator == ( const Rect& rect ) const
        {
            return m_x == rect.m_x && m_y == rect.m_y && haveEqualSize( rect );
        }
        inline bool operator != ( const Rect& rect ) const
        {
            return !( *this == rect );
        }

        float  m_x;
        float  m_y;
        float  m_width;
        float m_height;
};