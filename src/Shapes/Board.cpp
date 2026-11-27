// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <memory>

#include "Common/defs.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/SpriteRenderer.h"
#include "Renderer/Texture.h"
#include "Base.h"
#include "Board.h"

using namespace Shapes;

Board::Board()
{
    load( ResourceManager::LoadTexture(
        "resources/images/SimpleBoard.png",
        "simple_board" ) );

    m_velocity = { BEGIN_BOARD_VELOCITY, BEGIN_BOARD_VELOCITY };
}

void Board::drawTinted( const std::shared_ptr<SpriteRenderer>& renderer, const glm::vec3& color ) const
{
    if ( !m_sprite || !renderer )
        return;

    m_sprite->bind();
    renderer->drawSprite( m_position, m_size, color );
}
