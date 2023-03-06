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
            ContactPosition intersect( const wxRect2DDouble& rect ) const;

            double radius() { return m_radius; }

        private:
            double m_radius = 0;
            double m_radiusSquared = 0;
    };
}
