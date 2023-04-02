// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "Renderer/SpriteRenderer.h"
#include "Common/Rect.hpp"
#include "Base.h"
#include "Brick.h"

using namespace Shapes;

Brick::Brick( const glm::vec2& position, BrickType type, const glm::vec2& texSize )
{
     m_size = SizeOf( type );
     m_position = position;

     m_alive = !( type == BrickType::NONE || type == BrickType::HALF );

    if ( m_alive )
    {
        const float divisionScale = 1.f / texSize.x;
        const float left = divisionScale * ( static_cast< unsigned char >( type ) - 1 ) * m_size.x; // left coord
        const float right = left + divisionScale * m_size.x; // right coord

        m_VBO = SpriteRenderer::generateBuffer( {
            // pos      // tex
            0.f, 0.f,   left,    0.f,
            0.f, 1.f,   left,   -1.f,
            1.f, 0.f,   right,  0.f,
            1.f, 1.f,   right, -1.f,
        } );
    }
}

Brick::~Brick()
{
    SpriteRenderer::clearBuffer( m_VBO );
}
