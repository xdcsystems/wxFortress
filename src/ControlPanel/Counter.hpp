#pragma once

#include <cstdint>
#include <memory>

namespace ControlPanel
{
    class Counter
    {
            using bitmapPtr = std::shared_ptr<wxBitmap>;
            using memoryDCPtr = std::shared_ptr<wxMemoryDC>;

        public:
            Counter( int xPos, int yPos, uint32_t value = 0 )
                : m_position( xPos, yPos )
                , m_value( value )
            {}

            void clear( const bitmapPtr& bitmap, wxDC& dc ) const
            {
                dc.DrawBitmap( *bitmap, m_position.x, m_position.y, true );
            }

            void render( const memoryDCPtr& dc, const memoryDCPtr& numbersDC ) const
            {
                unsigned char count = 1;
                auto currentDigit = m_capacity;
                auto currentValue = m_value;

                while ( currentValue > 0 || currentDigit > 0 )
                {
                    dc->Blit( m_position.x + m_size.x - 12 * count++ - 5,
                              m_position.y + 1,
                              12,
                              19,
                              numbersDC.get(),
                              static_cast<int>( currentValue % 10 ) * 12,
                              0 );
                    currentValue /= 10;
                    if ( currentDigit > 0 )
                        --currentDigit;
                }
            }

            void increaseValue() { ++m_value; }
            void decreaseValue()
            {
                if ( m_value > 0 )
                    --m_value;
            }
            uint32_t value() const { return m_value; }
            void reset( uint32_t newValue = 0 ) { m_value = newValue; }
            wxRect bounds() const { return { m_position, m_size }; }

        private:
            const wxSize m_size { 79, 22 };
            wxPoint m_position;

            uint32_t m_value { 0 };
            unsigned char m_capacity { 3 };
    };
} // namespace ControlPanel
