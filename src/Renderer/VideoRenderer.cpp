#include <memory>
#include <string>

#include <GL/glew.h>

#include "Common/Tools.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "VideoRenderer.h"


VideoRenderer::VideoRenderer()
{
    m_shader = ResourceManager::LoadShader(
        "resources/shaders/Video.vs",
        "resources/shaders/Video.fraq",
        "",
        "video" );

    m_shader->setInteger( "textureY", 0, true );
    m_shader->setInteger( "textureU", 1 );
    m_shader->setInteger( "textureV", 2 );
    // GLSL #version 110 forbids uniform initializers; initialize uScale explicitly on both programs.
    m_shader->setVector2f( "uScale", 1.0f, 1.0f );

    m_shaderNV12 = ResourceManager::LoadShader(
        "resources/shaders/Video.vs",
        "resources/shaders/VideoNV12.fraq",
        "",
        "videoNV12" );

    m_shaderNV12->setInteger( "textureY", 0, true );
    m_shaderNV12->setInteger( "textureUV", 1 );
    m_shaderNV12->setVector2f( "uScale", 1.0f, 1.0f );

    init();
}

VideoRenderer::~VideoRenderer()
{
    m_VBO.reset();
    m_EBO.reset();
    GL_CHECK( glDeleteTextures( 3, m_texs ) );
}

void VideoRenderer::init()
{
    // clang-format off
    const float vertices[] {
         1.f,  1.f,  1.f, 0.f,
         1.f, -1.f,  1.f, 1.f,
        -1.f, -1.f,  0.f, 1.f,
        -1.f,  1.f,  0.f, 0.f,
    };

    const unsigned int indices[] {
        0, 1, 3, 1, 2, 3
    };
    // clang-format on

    if ( m_shader && m_shader->ID != 0 )
    {
        GL_CHECK( m_attrVertex = glGetAttribLocation( m_shader->ID, "aPos" ) );
        GL_CHECK( m_attrUVs = glGetAttribLocation( m_shader->ID, "aTexCoord" ) );
        GL_CHECK( m_locScale = glGetUniformLocation( m_shader->ID, "uScale" ) );
    }

    if ( m_shaderNV12 && m_shaderNV12->ID != 0 )
    {
        GL_CHECK( m_attrVertexNV12 = glGetAttribLocation( m_shaderNV12->ID, "aPos" ) );
        GL_CHECK( m_attrUVsNV12 = glGetAttribLocation( m_shaderNV12->ID, "aTexCoord" ) );
        GL_CHECK( m_locScaleNV12 = glGetUniformLocation( m_shaderNV12->ID, "uScale" ) );
    }

    m_VBO.create();
    m_EBO.create();

    if ( m_VBO && m_EBO )
    {
        // Static unit quad [-1..1]; aspect ratio scaling is applied on GPU via uniform vec2 uScale.
        GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO.id() ) );
        GL_CHECK( glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW ) );
        GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_EBO.id() ) );
        GL_CHECK( glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW ) );
        GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
        GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ) );
    }

    GL_CHECK( glGenTextures( 3, m_texs ) );
    for ( const auto& tex : m_texs )
    {
        GL_CHECK( glBindTexture( GL_TEXTURE_2D, tex ) );

        GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
        GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );

        GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
        GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );

        GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0 ) );
        GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0 ) );
    }
}

bool VideoRenderer::ok()
{
    return m_VBO && m_EBO && ( ( m_shader && m_shader->ID != 0 ) || ( m_shaderNV12 && m_shaderNV12->ID != 0 ) );
}

void VideoRenderer::setViewport( int x, int y, int width, int height )
{
    m_viewWidth = width;
    m_viewHeight = height;
    GL_CHECK( glViewport( x, y, m_viewWidth, m_viewHeight ) );
}

bool VideoRenderer::updateFrameGeometry( int width, int height, FrameFormat format )
{
    bool changed { false };
    if ( m_texWidth != width )
    {
        m_texWidth = width;
        changed = true;
    }
    if ( m_texHeight != height )
    {
        m_texHeight = height;
        changed = true;
    }
    if ( m_lastFormat != format )
    {
        m_lastFormat = format;
        changed = true;
    }

    const float tr = ( m_texHeight > 0 ) ? static_cast<float>( m_texWidth ) / static_cast<float>( m_texHeight ) : 1.0f;
    const float vr = ( m_viewHeight > 0 && m_viewWidth > 0 ) ? static_cast<float>( m_viewWidth ) / static_cast<float>( m_viewHeight ) : tr;

    if ( tr > vr && tr > 0.0f )
        m_scale = { 1.0f, vr / tr };
    else if ( tr < vr && vr > 0.0f )
        m_scale = { tr / vr, 1.0f };
    else
        m_scale = { 1.0f, 1.0f };

    return changed;
}

void VideoRenderer::uploadPlane( int unit, unsigned int tex, int width, int height, int rowLengthTexels, unsigned int format, const uint8_t* data, bool alloc )
{
    if ( !data || width <= 0 || height <= 0 )
        return;

    GL_CHECK( glActiveTexture( static_cast<GLenum>( GL_TEXTURE0 + unit ) ) );
    GL_CHECK( glBindTexture( GL_TEXTURE_2D, tex ) );
    GL_CHECK( glPixelStorei( GL_UNPACK_ROW_LENGTH, rowLengthTexels > 0 ? rowLengthTexels : 0 ) );
    if ( alloc )
    {
        GL_CHECK( glTexImage2D( GL_TEXTURE_2D, 0, static_cast<GLint>( format ), width, height, 0, format, GL_UNSIGNED_BYTE, data ) );
    }
    else
    {
        GL_CHECK( glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data ) );
    }
}

void VideoRenderer::drawQuad( const shaderPtr& shader, int attrVertex, int attrUVs, int locScale )
{
    if ( !shader || shader->ID == 0 || attrVertex < 0 || attrUVs < 0 || !m_VBO || !m_EBO )
        return;

    shader->use();
    if ( locScale >= 0 )
        GL_CHECK( glUniform2f( locScale, m_scale.x, m_scale.y ) );

    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO.id() ) );
    GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_EBO.id() ) );

    GL_CHECK( glVertexAttribPointer( static_cast<GLuint>( attrVertex ), 2, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), ( void* )nullptr ) );
    GL_CHECK( glEnableVertexAttribArray( static_cast<GLuint>( attrVertex ) ) );

    GL_CHECK( glVertexAttribPointer( static_cast<GLuint>( attrUVs ), 2, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), ( void* )( 2 * sizeof( float ) ) ) );
    GL_CHECK( glEnableVertexAttribArray( static_cast<GLuint>( attrUVs ) ) );

    GL_CHECK( glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr ) );

    GL_CHECK( glDisableVertexAttribArray( static_cast<GLuint>( attrVertex ) ) );
    GL_CHECK( glDisableVertexAttribArray( static_cast<GLuint>( attrUVs ) ) );
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
    GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 ) );
}

void VideoRenderer::draw( int width, int height, uint8_t** data, const int* linesize )
{
    if ( width <= 0 || height <= 0 || !data || !linesize || !data[ 0 ] || !data[ 1 ] || !data[ 2 ] )
        return;

    const bool changed = updateFrameGeometry( width, height, FrameFormat::YUV420P );
    const int chromaW = ( width + 1 ) >> 1;
    const int chromaH = ( height + 1 ) >> 1;
    const int widths[ 3 ]  { width, chromaW, chromaW };
    const int heights[ 3 ] { height, chromaH, chromaH };

    GL_CHECK( glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ) );
    for ( int i = 0; i < 3; ++i )
        uploadPlane( i, m_texs[ i ], widths[ i ], heights[ i ], linesize[ i ], GL_LUMINANCE, data[ i ], changed );
    GL_CHECK( glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 ) );
    GL_CHECK( glPixelStorei( GL_UNPACK_ALIGNMENT, 4 ) );
    GL_CHECK( glActiveTexture( GL_TEXTURE0 ) );

    drawQuad( m_shader, m_attrVertex, m_attrUVs, m_locScale );
}

void VideoRenderer::drawNV12( int width, int height, uint8_t** data, const int* linesize )
{
    if ( width <= 0 || height <= 0 || !data || !linesize || !data[ 0 ] || !data[ 1 ] )
        return;

    const bool changed = updateFrameGeometry( width, height, FrameFormat::NV12 );
    const int chromaW = ( width + 1 ) >> 1;
    const int chromaH = ( height + 1 ) >> 1;
    const int uvRowTexels = ( linesize[ 1 ] >= 2 ) ? ( linesize[ 1 ] >> 1 ) : 0;

    GL_CHECK( glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ) );
    uploadPlane( 0, m_texs[ 0 ], width, height, linesize[ 0 ], GL_LUMINANCE, data[ 0 ], changed );
    // UV plane has 2 bytes per texel (GL_LUMINANCE_ALPHA), so row length in texels is linesize[1] / 2.
    uploadPlane( 1, m_texs[ 1 ], chromaW, chromaH, uvRowTexels, GL_LUMINANCE_ALPHA, data[ 1 ], changed );
    GL_CHECK( glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 ) );
    GL_CHECK( glPixelStorei( GL_UNPACK_ALIGNMENT, 4 ) );
    GL_CHECK( glActiveTexture( GL_TEXTURE0 ) );

    drawQuad( m_shaderNV12, m_attrVertexNV12, m_attrUVsNV12, m_locScaleNV12 );
}