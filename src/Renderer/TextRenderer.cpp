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

#include "Common/Timer.h"
#include "Common/Tools.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "Renderer/ResourceManager.h"

#include "TextRenderer.h"


TextRenderer::~TextRenderer()
{
	cleanup();
}

void TextRenderer::init()
{
    // Initialize texture
    m_texture = ResourceManager::LoadTexture(
		"/../resources/images/Text.png",
		"font" );

    // Initialize VBO
    glGenBuffers( 1, &m_vertexBufferID );
    glGenBuffers( 1, &m_UVBufferID );

	m_shader = ResourceManager::GetShader( "text" );

	GL_CHECK( m_attrVertex = glGetAttribLocation( m_shader->ID, "vertex" ) );
	GL_CHECK( m_attrUVs = glGetAttribLocation( m_shader->ID, "vertexUV" ) );

	m_timer = std::make_shared<Timer>( false );
}

void TextRenderer::print( const std::string& text, int x, int y, const glm::vec2 &size )
{
    unsigned int length = text.length();

    // Fill buffers
    std::vector<glm::vec2> vertices;
    std::vector<glm::vec2> UVs;

	for ( unsigned int i = 0; i < length; i++ )
	{
		glm::vec2 vertex_up_left = glm::vec2( x + i * size.x, y + size.y );
		glm::vec2 vertex_up_right = glm::vec2( x + i * size.x + size.x, y + size.y );
		glm::vec2 vertex_down_right = glm::vec2( x + i * size.x + size.x, y );
		glm::vec2 vertex_down_left = glm::vec2( x + i * size.x, y );

		vertices.emplace_back( vertex_up_left );
		vertices.emplace_back( vertex_down_left );
		vertices.emplace_back( vertex_up_right );

		vertices.emplace_back( vertex_down_right );
		vertices.emplace_back( vertex_up_right );
		vertices.emplace_back( vertex_down_left );

		char character = text[ i ];
		float uv_x = ( character % 16 ) / 16.0f;
		float uv_y = ( character / 16 ) / 8.0f;

		glm::vec2 uv_up_left = glm::vec2( uv_x, uv_y );
		glm::vec2 uv_up_right = glm::vec2( uv_x + 1.0f / 16.0f, uv_y );
		glm::vec2 uv_down_right = glm::vec2( uv_x + 1.0f / 16.0f, ( uv_y + 1.0f / 8.0f ) );
		glm::vec2 uv_down_left = glm::vec2( uv_x, ( uv_y + 1.0f / 8.0f ) );
		
		UVs.emplace_back( uv_up_left );
		UVs.emplace_back( uv_down_left );
		UVs.emplace_back( uv_up_right );

		UVs.emplace_back( uv_down_right );
		UVs.emplace_back( uv_up_right );
		UVs.emplace_back( uv_down_left );
	}

	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_vertexBufferID ) );
	GL_CHECK( glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof( glm::vec2 ), &vertices[ 0 ], GL_STATIC_DRAW ) );
	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_UVBufferID ) );
	GL_CHECK( glBufferData( GL_ARRAY_BUFFER, UVs.size() * sizeof( glm::vec2 ), &UVs[ 0 ], GL_STATIC_DRAW ) );

	// Bind shader
	m_shader->use();

	// Bind texture
	m_texture->bind();

	// 1rst attribute buffer : vertices
	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_vertexBufferID ) );
	GL_CHECK( glVertexAttribPointer( m_attrVertex, 2, GL_FLOAT, GL_FALSE, 0, ( void* )0 ) );
	GL_CHECK( glEnableVertexAttribArray( m_attrVertex ) );

	// 2nd attribute buffer : UVs
	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_UVBufferID ) );
	GL_CHECK( glVertexAttribPointer( m_attrUVs, 2, GL_FLOAT, GL_FALSE, 0, ( void* )0 ) );
	GL_CHECK( glEnableVertexAttribArray( m_attrUVs ) );

	// Draw call
	GL_CHECK( glDrawArrays( GL_TRIANGLES, 0, vertices.size() ) );

	GL_CHECK( glDisableVertexAttribArray( m_attrVertex ) );
	GL_CHECK( glDisableVertexAttribArray( m_attrUVs ) );
}

void TextRenderer::cleanup()
{
	// Delete buffers
	GL_CHECK( glDeleteBuffers( 1, &m_vertexBufferID ) );
	GL_CHECK( glDeleteBuffers( 1, &m_UVBufferID ) );
}

void TextRenderer::renderFrame()
{
	const std::vector<std::string> message = {
		"Test Program First Stage Completed Successfully.",
		"The sample demonstrated high penetration and large aggregate power.",
		"In order to improve the reliability of the product,",
		"additional tests are recommended.",
		"If confirmed, the test program will be restarted",
		"with the current values.",
		" ",
		"Start the next stage ? ( Yes / No )",
		" ",
		">:"
	};

	constexpr unsigned char lineHeight = 22;
	constexpr glm::vec2 charSize = { 11.f, 15.25f };

	unsigned int i = 0;
	for (; i < message.size(); ++i )
		print( message.at( i ), 35, 450 - i * lineHeight, charSize );

	const auto elapsedTime = m_timer->getElapsedTimeInMilliSec();
	if ( elapsedTime >= 500 )
	{
		print( " \x60", 57, 450 - ( i - 1 ) * lineHeight, charSize );
		
		if ( elapsedTime >= 1000 )
			m_timer->start();
	}
}