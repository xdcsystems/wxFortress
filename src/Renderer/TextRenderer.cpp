// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
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
    m_shader = ResourceManager::LoadShader(
        "resources/shaders/Text.vs",
        "resources/shaders/Text.frag",
        "",
        "text" );

    m_shader->setInteger( "charImage", 0, true );

    s_fontData.at( TextRendererFont::NORMAL ).second = 
        ResourceManager::LoadTexture( "resources/images/Font18.png", "font18" );
    
    s_fontData.at( TextRendererFont::OLD ).second = 
        ResourceManager::LoadTexture( "resources/images/Font.png", "font" );

    GL_CHECK( glGenBuffers( 1, &m_vertexBufferID ) );
    GL_CHECK( glGenBuffers( 1, &m_UVBufferID ) );
    GL_CHECK( glGenBuffers( 1, &m_arcBufferID ) );

    GL_CHECK( m_attrVertex = glGetAttribLocation( m_shader->ID, "vertex" ) );
    GL_CHECK( m_attrUVs = glGetAttribLocation( m_shader->ID, "vertexUV" ) );

    static constexpr const char* kArcVertexSrc =
        "#version 110\n"
        "attribute vec2 vertex;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * vec4( vertex, 0.0, 1.0 );\n"
        "}\n";
    static constexpr const char* kArcFragmentSrc =
        "#version 110\n"
        "uniform vec4 arcColor;\n"
        "void main() {\n"
        "    gl_FragColor = arcColor;\n"
        "}\n";
    m_arcShader = std::make_shared<Shader>();
    m_arcShader->compile( kArcVertexSrc, kArcFragmentSrc );
    if ( m_arcShader->ID != 0 )
    {
        GLint linked = GL_FALSE;
        GL_CHECK( glGetProgramiv( m_arcShader->ID, GL_LINK_STATUS, &linked ) );
        if ( linked == GL_TRUE )
        {
            GL_CHECK( m_attrArcVertex = glGetAttribLocation( m_arcShader->ID, "vertex" ) );
        }
        else
        {
            GL_CHECK( glDeleteProgram( m_arcShader->ID ) );
            m_arcShader->ID = 0;
        }
    }

    m_timer = std::make_shared<Timer>( false );
}

TextRenderer::~TextRenderer()
{
    cleanup();
}

void TextRenderer::cleanup()
{
    if ( m_vertexBufferID != 0 )
    {
        GL_CHECK( glDeleteBuffers( 1, &m_vertexBufferID ) );
        m_vertexBufferID = 0;
    }
    if ( m_UVBufferID != 0 )
    {
        GL_CHECK( glDeleteBuffers( 1, &m_UVBufferID ) );
        m_UVBufferID = 0;
    }
    if ( m_arcBufferID != 0 )
    {
        GL_CHECK( glDeleteBuffers( 1, &m_arcBufferID ) );
        m_arcBufferID = 0;
    }
    if ( m_arcShader && m_arcShader->ID != 0 )
    {
        GL_CHECK( glDeleteProgram( m_arcShader->ID ) );
        m_arcShader->ID = 0;
    }
    for ( auto& entry : s_fontData )
        entry.second.second.reset();
}

void TextRenderer::resize( const glm::mat4& projection )
{
    m_shader->setMatrix4( "projection", projection, true );
    if ( m_arcShader && m_arcShader->ID != 0 && m_attrArcVertex >= 0 )
    {
        m_arcShader->setMatrix4( "projection", projection, true );
    }
}

void TextRenderer::switchToFinishState( unsigned short stage )
{
    static const std::map<unsigned short, std::string> s_stageName = {
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
    m_startLinePosition = 530.f;

    m_message = {
        "LABORATORY NO. 8",
        " ",
        "Test stand 2644 - k/17",
        " ",
        "After video channel activated, start item movement",
        "by press the Launch button [ <space> key ].",
        " ",
        "To manipulator control use LEFT / RIGHT lever",
        "or pointer [ <left> <right> keys / mouse ].",
        " ",
        "To stop item movement, press the Launch button again.",
        "The chamber with the item will be transferred to stasis,",
        "the item will frozen.",
        " ",
        "Chamber acoustic resonance feed runs nonstop.",
        "To mute or restore it, press [ <M> key ].",
        " ",
        "To exit press Emergency button [ <Esc> key ]"
    };
}

void TextRenderer::print( const std::string& text, int x, int y, const glm::vec2& size, glm::vec3 color )
{
    unsigned int length = text.length();

    // Fill buffers
    std::vector<glm::vec2> vertices;
    std::vector<glm::vec2> coordsUV;

    for ( unsigned int i = 0; i < length; i++ )
    {
        glm::vec2 vertexUpLeft    { x + i * size.x,          y + size.y };
        glm::vec2 vertexUpRight   { x + i * size.x + size.x, y + size.y };
        glm::vec2 vertexDownRight { x + i * size.x + size.x, y };
        glm::vec2 vertexDownLeft  { x + i * size.x,          y };

        vertices.emplace_back( vertexUpLeft );
        vertices.emplace_back( vertexDownLeft );
        vertices.emplace_back( vertexUpRight );

        vertices.emplace_back( vertexDownRight );
        vertices.emplace_back( vertexUpRight );
        vertices.emplace_back( vertexDownLeft );

        char character = text[i];
        float uvX = ( character % 16 ) / 16.f;
        float uvY = ( character / 16 ) / s_fontData.at( m_fontType ).first;

        glm::vec2 uvUpLeft    { uvX, uvY };
        glm::vec2 uvUpRight   { uvX + 1.f / 16.f, uvY };
        glm::vec2 uvDownRight { uvX + 1.f / 16.f, ( uvY + 1.f / s_fontData.at( m_fontType ).first ) };
        glm::vec2 uvDownLeft  { uvX, ( uvY + 1.f / s_fontData.at( m_fontType ).first ) };

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
    s_fontData.at( m_fontType ).second->bind();

    // Set text color
    m_shader->setVector3f( "textColor", color );

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

void TextRenderer::drawProgressArc( float cx, float cy, float innerRadius, float outerRadius, float progress, const glm::vec4& color )
{
    if ( !m_arcShader || m_arcShader->ID == 0 || m_attrArcVertex < 0 || m_arcBufferID == 0 )
        return;

    if ( !std::isfinite( cx ) || !std::isfinite( cy ) ||
         !std::isfinite( innerRadius ) || !std::isfinite( outerRadius ) ||
         !std::isfinite( progress ) ||
         progress <= 0.0f || outerRadius <= innerRadius || innerRadius < 0.0f )
        return;

    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTwoPi = 2.0f * kPi;
    constexpr float kHalfPi = 0.5f * kPi;
    constexpr int kFullCircleSegments = 96;

    const float clamped = std::clamp( progress, 0.0f, 1.0f );
    const float totalAngle = clamped * kTwoPi;
    const int segments = std::max( 1, static_cast<int>( std::ceil( clamped * kFullCircleSegments ) ) );

    std::array<glm::vec2, ( kFullCircleSegments + 1 ) * 2> verts {};
    size_t count = 0;

    for ( int i = 0; i <= segments; ++i )
    {
        const float t = static_cast<float>( i ) / static_cast<float>( segments );
        const float angle = kHalfPi - t * totalAngle;
        const float cosA = std::cos( angle );
        const float sinA = std::sin( angle );
        verts[ count++ ] = { cx + outerRadius * cosA, cy + outerRadius * sinA };
        verts[ count++ ] = { cx + innerRadius * cosA, cy + innerRadius * sinA };
    }

    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_arcBufferID ) );
    GL_CHECK( glBufferData( GL_ARRAY_BUFFER, count * sizeof( glm::vec2 ), verts.data(), GL_DYNAMIC_DRAW ) );

    m_arcShader->use();
    m_arcShader->setVector4f( "arcColor", color );

    GL_CHECK( glVertexAttribPointer( static_cast<GLuint>( m_attrArcVertex ), 2, GL_FLOAT, GL_FALSE, 0, nullptr ) );
    GL_CHECK( glEnableVertexAttribArray( static_cast<GLuint>( m_attrArcVertex ) ) );
    GL_CHECK( glDrawArrays( GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>( count ) ) );
    GL_CHECK( glDisableVertexAttribArray( static_cast<GLuint>( m_attrArcVertex ) ) );
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
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

    print( m_message[ countRow ].substr( 0, m_col ) + '\x60', beginPos.x, beginPos.y - lineHeight * m_row, charSize );

    if ( m_timer->getElapsedTimeInMilliSec() >= m_delay )
    {
        m_timer->stop();

        if ( m_col++ )
            m_eventHandler->ProcessEvent( m_eventCharShow );

        if ( m_col < m_message[ m_row ].length() )
        {
            m_timer->start();
            return;
        }

        m_col = 0;
        if ( ++m_row < m_message.size() )
        {
            if ( m_message[ m_row - 1 ].back() == '.' || m_message[ m_row - 1 ].back() == '\"' )
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

