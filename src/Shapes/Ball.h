#pragma once

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

    class Ball final : public Base
    {
        using rendererPtr = std::shared_ptr<SpriteRenderer>;

        public:
            using enum ContactPosition;

            Ball();
            ContactPosition intersect( const xRect& rect ) const;

            float radius() { return m_radius; }

        private:
            float m_radius = 0;
            float m_radiusSquared = 0;
    };
}
