#pragma once

namespace Shapes
{
    class Base
    {
        public:
            virtual ~Base() {};
            virtual void load( const std::wstring& filename );
            virtual void moveTo( int x, int y );
            virtual void moveTo( const wxPoint& position );

            virtual wxRect bounds() const { return { m_position, m_size }; };
            virtual void clear( wxDC& dc ) const;
            virtual void draw( wxDC& dc ) const;

        protected:
            std::shared_ptr<wxBitmap> m_bitmap;
            wxPoint m_position;
            wxSize m_size;
    };
}