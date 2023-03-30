// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <map>

#include "Common/Tools.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "SpriteRenderer.h"
#include "CountDown.h"
#include "Overlay.h"


Overlay::Overlay( const wxSize& size )
 : m_font( 36, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL )
 , m_size( size )
{
    m_countdown = std::make_shared<CountDown>( size );
    
    m_pauseTex = ResourceManager::LoadTexture(
        "/../resources/images/Pause.png",
        "pause" );
}

void Overlay::showPause( const rendererPtr &renderer )
{
    m_pauseTex->bind();
    renderer->drawSprite(
        { 0.0f, m_size.y - m_pauseTex->Height },
        { m_pauseTex->Width, m_pauseTex->Height } );
}

void Overlay::showCountDown( const rendererPtr &renderer, unsigned char count )
{
    m_countdown->show( renderer, count );
}
