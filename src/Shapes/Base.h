#pragma once

#include <glm/glm.hpp>

// Forward declarations
class Texture2D;
class SpriteRenderer;
class Rect;

namespace Shapes
{
    enum class ContactPosition : unsigned char
    {
        ContactNull,
        ContactTop,
        ContactRight,
        ContactLeft,
        ContactBottom
    };

    class Base
    {
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using texture2DPtr = std::shared_ptr<Texture2D>;

        protected:
            Base() = default; // abstract base class

        public:
            virtual ~Base() = default;

            virtual inline glm::vec2 position() const { return m_position; }
            virtual inline glm::vec2 size() const { return m_size; }
            virtual inline glm::vec2 velocity() const { return m_velocity; }
            virtual glm::vec2 center() const;
            virtual inline Rect  bounds() const { return { m_position, m_size }; }
            virtual unsigned int VBO() const { return m_VBO; }

            virtual void load( const texture2DPtr &sprite );
            virtual void moveTo( float x, float y );
            virtual void moveTo( const glm::vec2& position );
            virtual void increaseVelocity( float value ) { m_velocity += value; }
            virtual void draw( const rendererPtr &renderer ) const;

            // Protected data
            glm::vec2 m_position = { 0, 0 },
                          m_size = { 0, 0 },
                          m_velocity = { 0, 0 };

            texture2DPtr m_sprite;
            unsigned int m_VBO = 0;
    };

    using basePtr = std::shared_ptr<Base>;
}
