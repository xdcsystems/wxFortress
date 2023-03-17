#pragma once

#include <memory>
#include <glm/glm.hpp>

// Forward declarations
class Texture2D;
class SpriteRenderer;

class xRect;

namespace Shapes
{
    class Base
    {
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using texture2DPtr = std::shared_ptr<Texture2D>;

        public:
            virtual ~Base() = default;
            virtual void load( texture2DPtr sprite );
            virtual void moveTo( float x, float y );
            virtual void moveTo( const glm::vec2& position );

            virtual glm::vec2 position() { return m_position; }
            virtual glm::vec2 velocity()  { return m_velocity; }
            virtual xRect  bounds() const { return { m_position, m_size }; }

            virtual void draw( rendererPtr renderer ) const;

        protected:
            glm::vec2 m_position = { 0, 0 },
                          m_size = { 0, 0 },
                          m_velocity = { 0, 0 };

            texture2DPtr m_sprite;
    };

    using basePtr = std::shared_ptr<Base>;
}
