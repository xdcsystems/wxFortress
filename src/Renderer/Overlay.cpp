// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <map>

#include "Common/Tools.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "SpriteRenderer.h"
#include "CountDown.h"
#include "Overlay.h"


Overlay::Overlay( const rendererPtr& renderer )
  : m_renderer( renderer )
 {
    m_countdown = std::make_shared<CountDown>();
    
    m_pauseTex = ResourceManager::LoadTexture(
        "resources/images/Pause.png",
        "pause" );
}

void Overlay::resize( const wxSize& size )
{
    m_size = size;
    m_countdown->resize( size );
}

void Overlay::showPause() const
{
    m_pauseTex->bind();
    m_renderer->drawSprite(
        { 0.0f, m_size.y - m_pauseTex->Height },
        { m_pauseTex->Width, m_pauseTex->Height } );
}

void Overlay::showCountDown( unsigned char count ) const
{
    m_countdown->show( m_renderer, count );
}
