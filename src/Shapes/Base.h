#pragma once

#include <memory>
#include <glm/glm.hpp>

// Forward declarations
class Texture2D;
class SpriteRenderer;

class wxRect2DDouble;

namespace Shapes
{
    class Base
    {
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using texture2DPtr = std::shared_ptr<Texture2D>;

        public:
            virtual ~Base() {};
            virtual void load( texture2DPtr sprite );
            virtual void moveTo( double x, double y );
            virtual void moveTo( const wxPoint2DDouble& position );

            virtual glm::vec2 position() { return m_position; }
            virtual glm::vec2 velocity()  { return m_velocity; }

            virtual wxRect2DDouble  bounds() const { 
                return { m_position.x, m_position.y, m_size.x, m_size.y };
            }

            virtual void draw( rendererPtr renderer ) const;

        protected:
            glm::vec2 m_position = glm::vec2( 0 ),
                          m_size = glm::vec2( 0 ),
                          m_velocity = glm::vec2( 0 );

            texture2DPtr m_sprite;
    };

    using basePtr = std::shared_ptr<Base>;
}

