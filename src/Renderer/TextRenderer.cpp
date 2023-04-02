// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <vector>
#include <cstring>

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Common/defs.h"
#include "Common/Timer.h"
#include "Common/Tools.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "Renderer/ResourceManager.h"

#include "TextRenderer.h"

DEFINE_LOCAL_EVENT_TYPE( wxEVT_CHAR_SHOW )

TextRenderer::TextRenderer( wxWindow* parent )
  : m_eventHandler( parent->GetEventHandler() )
  , m_eventCharShow( wxEVT_CHAR_SHOW )
{
    m_shader = ResourceManager::GetShader( "text" );
    m_texture = ResourceManager::LoadTexture( "/../resources/images/Font.png", "font" );

    GL_CHECK( glGenBuffers( 1, &m_vertexBufferID ) );
    GL_CHECK( glGenBuffers( 1, &m_UVBufferID ) );

    GL_CHECK( m_attrVertex = glGetAttribLocation( m_shader->ID, "vertex" ) );
    GL_CHECK( m_attrUVs = glGetAttribLocation( m_shader->ID, "vertexUV" ) );

    m_timer = std::make_shared<Timer>( false );
}

TextRenderer::~TextRenderer()
{
    cleanup();
}

void TextRenderer::cleanup()
{
    GL_CHECK( glDeleteBuffers( 1, &m_vertexBufferID ) );
    GL_CHECK( glDeleteBuffers( 1, &m_UVBufferID ) );
}

void TextRenderer::switchToFinishState( unsigned short stage )
{
    static std::map<unsigned short, std::string> s_stageName = {
        { 1, "First"  },
        { 2, "Second" },
        { 3, "Third"  },
        { 4, "Fourth" },
        { 5, "Fifth"  },
        { 6, "Sixth"  },
        { 7, "Seventh"},
        { 8, "Eighth" },
        { 9, "Ninth"  },
    };

    m_state = TextRendererState::FINISHED;
    m_stage = stage;
    m_startLinePosition = 480.f;
    m_row = 0;
    m_col = 0;
    m_delay = 57;

    m_message = {
        "1844/27 DH \"Pretorian\"",
        " ",
        "Test Program " + s_stageName.at( m_stage ) + " Stage Completed Successfully.",
        "The sample demonstrated high penetration and large",
        "aggregate power.",
        "In order to improve the reliability of the product,",
        "additional tests are recommended.",
        "If confirmed, the test program will be restarted",
        "with the current values.",
        " ",
        "Start the next stage ? ( Yes / No )",
        " ",
        ">:"
    };
}

void TextRenderer::switchToHelpState()
{
    m_state = TextRendererState::HELP;
    m_startLinePosition = 500.f;

    m_message = {
        "LABORATORY NO. 8",
        " ",
        "Test stand 2644 - k/17",
        " ",
        "After video channel activated, start item movement",
        "by press the Launch button [ <space> key ].",
        " ",
        "To manipulator control use LEFT and RIGHT",
        "control lever position [ <left> <right> keys ].",
        " ",
        "To stop item movement, press the Launch button again.",
        "The chamber with the item will be transferred to stasis,",
        "the item will frozen.",
        " ",
        "To exit press Emergency button [ <Esc> key ]"
    };
}

void TextRenderer::print( const std::string& text, int x, int y, const glm::vec2& size )
{
    unsigned int length = text.length();

    // Fill buffers
    std::vector<glm::vec2> vertices;
    std::vector<glm::vec2> coordsUV;

    for ( unsigned int i = 0; i < length; i++ )
    {
        glm::vec2 vertexUpLeft = glm::vec2( x + i * size.x, y + size.y );
        glm::vec2 vertexUpRight = glm::vec2( x + i * size.x + size.x, y + size.y );
        glm::vec2 vertexDownRight = glm::vec2( x + i * size.x + size.x, y );
        glm::vec2 vertexDownLeft = glm::vec2( x + i * size.x, y );

        vertices.emplace_back( vertexUpLeft );
        vertices.emplace_back( vertexDownLeft );
        vertices.emplace_back( vertexUpRight );

        vertices.emplace_back( vertexDownRight );
        vertices.emplace_back( vertexUpRight );
        vertices.emplace_back( vertexDownLeft );

        char character = text[i];
        float uvX = ( character % 16 ) / 16.0f;
        float uvY = ( character / 16 ) / 8.0f;

        glm::vec2 uvUpLeft = glm::vec2( uvX, uvY );
        glm::vec2 uvUpRight = glm::vec2( uvX + 1.0f / 16.0f, uvY );
        glm::vec2 uvDownRight = glm::vec2( uvX + 1.0f / 16.0f, ( uvY + 1.0f / 8.0f ) );
        glm::vec2 uvDownLeft = glm::vec2( uvX, ( uvY + 1.0f / 8.0f ) );

        coordsUV.emplace_back( uvUpLeft );
        coordsUV.emplace_back( uvDownLeft );
        coordsUV.emplace_back( uvUpRight );

        coordsUV.emplace_back( uvDownRight );
        coordsUV.emplace_back( uvUpRight );
        coordsUV.emplace_back( uvDownLeft );
    }

    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_vertexBufferID ) );
    GL_CHECK( glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( glm::vec2 ), &vertices[0], GL_STATIC_DRAW ) );
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_UVBufferID ) );
    GL_CHECK( glBufferData( GL_ARRAY_BUFFER, coordsUV.size() * sizeof( glm::vec2 ), &coordsUV[0], GL_STATIC_DRAW ) );

    // Bind shader
    m_shader->use();

    // Bind texture
    m_texture->bind();

    // 1rst attribute buffer : vertices
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_vertexBufferID ) );
    GL_CHECK( glVertexAttribPointer( m_attrVertex, 2, GL_FLOAT, GL_FALSE, 0, (void*)nullptr ) );
    GL_CHECK( glEnableVertexAttribArray( m_attrVertex ) );

    // 2nd attribute buffer : UVs
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_UVBufferID ) );
    GL_CHECK( glVertexAttribPointer( m_attrUVs, 2, GL_FLOAT, GL_FALSE, 0, (void*)nullptr ) );
    GL_CHECK( glEnableVertexAttribArray( m_attrUVs ) );

    // Draw call
    GL_CHECK( glDrawArrays( GL_TRIANGLES, 0, vertices.size() ) );

    GL_CHECK( glDisableVertexAttribArray( m_attrVertex ) );
    GL_CHECK( glDisableVertexAttribArray( m_attrUVs ) );
}

void TextRenderer::renderFinishMessage()
{
    constexpr unsigned char lineHeight = 22;
    constexpr glm::vec2 charSize = { 11.f, 15.25f };
    const glm::vec2 beginPos = { 35.f, m_startLinePosition };

    unsigned short countRow = 0;
    for ( ; countRow < m_row; ++countRow )
        print( m_message[countRow], beginPos.x, beginPos.y - lineHeight * countRow, charSize );

    if ( m_row >= m_message.size() || m_delay == 500 )
    {
        const auto elapsedTime = m_timer->getElapsedTimeInMilliSec();
        if ( elapsedTime < 500 )
            return;

        countRow -= 1;
        print( "\x60", beginPos.x + m_message[countRow].length() * charSize.x, beginPos.y - lineHeight * countRow, charSize );

        if ( elapsedTime >= 1000 )
        {
            m_delay = 57;
            m_timer->start();
        }

        return;
    }

    print( m_message[countRow].substr( 0, m_col ) + '\x60', beginPos.x, beginPos.y - lineHeight * m_row, charSize );

    if ( m_timer->getElapsedTimeInMilliSec() >= m_delay )
    {
        m_timer->stop();

        if ( m_col++ )
            m_eventHandler->ProcessEvent( m_eventCharShow );

        if ( m_col < m_message[m_row].length() )
        {
            m_timer->start();
            return;
        }

        m_col = 0;
        if ( ++m_row < m_message.size() )
        {
            if ( m_message[m_row - 1].back() == '.' || m_message[m_row - 1].back() == '\"' )
                m_delay = 500;

            renderFinishMessage();
        }

        m_timer->start();
    }
}

void TextRenderer::renderHelp()
{
    constexpr unsigned char lineHeight = 22;
    constexpr glm::vec2 charSize = { 11.f, 15.25f };
    const glm::vec2 beginPos = { 35.f, m_startLinePosition };

    unsigned int i = 0;
    for ( ; i < m_message.size(); ++i )
        print( m_message.at( i ), beginPos.x, beginPos.y - i * lineHeight, charSize );
}

void TextRenderer::renderFrame()
{
    switch ( m_state )
    {
        case TextRendererState::FINISHED :
            renderFinishMessage();
            break;

        case TextRendererState::HELP :
            renderHelp();
            break;
    }
}

