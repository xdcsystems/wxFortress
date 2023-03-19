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
