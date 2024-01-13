// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <memory>

#include "Renderer/SpriteRenderer.h"
#include "Common/Rect.hpp"
#include "Base.h"
#include "Explosion.h"

using namespace Shapes;

Explosion::Explosion( const basePtr &shape, const glm::vec2 &size )
{
    m_position = shape->position() - ( size - shape->size() ) / 2.f;
    m_size = size;
}
