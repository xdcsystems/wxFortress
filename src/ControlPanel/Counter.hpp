#pragma once

namespace ControlPanel
{
    template <typename T>
    class Counter
    {
            using bitmapPtr = std::shared_ptr<wxBitmap>;
            using memoryDCPtr = std::shared_ptr<wxMemoryDC>;

        public:
            Counter( int xPos, int yPos, const T &value = 0 )
                : m_position( xPos, yPos )
                , m_value( value ) 
            {}

            void clear( bitmapPtr bitmap, wxDC &dc ) const
            {
                dc.DrawBitmap( *bitmap, m_position.x, m_position.y, true );
            }

            void render( memoryDCPtr dc, memoryDCPtr numbersDC ) const
            {
                unsigned char count = 1;
                auto currentDigit = m_capacity;
                auto currentValue = m_value;

                while ( currentValue > 0 || currentDigit > 0 )
                {
                    dc->Blit(
                        m_position.x + m_size.x - 12 * count++ - 5,
                        m_position.y + 1,
                        12,
                        19,
                        numbersDC.get(),
                        currentValue % 10 * 12,
                        0
                    );
                    currentValue /= 10;
                    --currentDigit;
                }
            }

            void increaseValue()
            {
                ++m_value;
            }
            void decreaseValue()
            {
                --m_value;
            }
            T value() const
            {
                return m_value;
            }
            void reset( T newValue = 0 )
            {
                m_value = newValue;
            }

        private:
            const wxSize m_size = { 79, 22 };
            wxPoint m_position;

            T m_value;
            unsigned char m_capacity = 3;
    };
}  // namespace ControlPanel
