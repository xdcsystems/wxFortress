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
#include "Overlay.h"


Overlay::Overlay( const wxSize& size )
 : m_font( 36, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL )
 , m_size( size )
{
    for ( const auto& countdown : s_countdownMap )
    {
        s_countdownSprites[ countdown.first ] = ResourceManager::LoadTexture(
            "/../resources/images/Countdown/" + countdown.second,
            true,
            "countdown_" + static_cast< int >( countdown.first )
        );
    }
    
    m_pauseTex = ResourceManager::LoadTexture(
        "/../resources/images/Pause.png",
        true,
        "pause" 
    );
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
    s_countdownSprites[ count ]->bind();
    renderer->drawSprite(
        { 0.0f, 0.0f },
        { s_countdownSprites[ count ]->Width, s_countdownSprites[ count ]->Height } );
}
