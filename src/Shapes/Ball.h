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
            void clear( wxDC& dc ) const;
            ContactPosition intersect( const wxRect& rect ) const;

        private:
            std::shared_ptr<wxBitmap> m_bitmapClear;
            int m_radius = 0;
            int m_radiusSquared = 0;
    };
}
