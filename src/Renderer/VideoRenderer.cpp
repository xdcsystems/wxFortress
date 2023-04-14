#include <memory>
#include <vector>
#include <string>

#include <GL/glew.h>

#include "Common/Tools.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "VideoRenderer.h"


VideoRenderer::VideoRenderer()
{
    m_shader = ResourceManager::LoadShader( "/../data/shaders/Video.vs", "/../data/shaders/Video.fraq", "", "video" );

    m_shader->setInteger( "textureY", 0, true );
    m_shader->setInteger( "textureU", 1 );
    m_shader->setInteger( "textureV", 2 );

    m_shaderNV12 = ResourceManager::LoadShader( "/../data/shaders/Video.vs", "/../data/shaders/VideoNV12.fraq", "", "videoNV12" );

    m_shaderNV12->setInteger( "textureY", 0, true );
    m_shaderNV12->setInteger( "textureUV", 1 );

    init();
}

VideoRenderer::~VideoRenderer()
{
    GL_CHECK( glDisableVertexAttribArray( m_attrVertex ) );
    GL_CHECK( glDisableVertexAttribArray( m_attrUVs ) );

    GL_CHECK( glDeleteBuffers( 1, &m_VBO ) );
    GL_CHECK( glDeleteBuffers( 1, &m_EBO ) );
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

    GL_CHECK( m_attrVertex = glGetAttribLocation( m_shader->ID, "aPos" ) );
    GL_CHECK( m_attrUVs = glGetAttribLocation( m_shader->ID, "aTexCoord" ) );

    GL_CHECK( glGenBuffers( 1, &m_VBO ) );
    GL_CHECK( glGenBuffers( 1, &m_EBO ) );

    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO ) );
    GL_CHECK( glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_DYNAMIC_DRAW ) );
    GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_EBO ) );
    GL_CHECK( glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW ) );

    GL_CHECK( glGenTextures( 3, m_texs ) );
    for( const auto &tex : m_texs )
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
    return true;
}

void VideoRenderer::setViewport( int x, int y, int width, int height )
{
    m_viewWidth = width;
    m_viewHeight = height;
    GL_CHECK( glViewport( x, y, m_viewWidth, m_viewHeight ) );
}

bool VideoRenderer::bindBuffers( int width, int height )
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

    float tr = ( float )m_texWidth / m_texHeight;
    float vr = ( float )m_viewWidth / m_viewHeight;
    float r = tr / vr;
    if ( r != m_ratio )
    {
        m_ratio = r;
        std::vector<float> vertices;
        // clang-format off
        if ( tr > vr )
        {
            float p = vr / tr;
            vertices = {
                 1.f,  p,  1.f, 0.f,
                 1.f, -p,  1.f, 1.f,
                -1.f, -p,  0.f, 1.f,
                -1.f,  p,  0.f, 0.f,
            };
        }
        else if ( tr < vr )
        {
            vertices = {
                 r,  1.f,  1.f, 0.f,
                 r, -1.f,  1.f, 1.f,
                -r, -1.f,  0.f, 1.f,
                -r,  1.f,  0.f, 0.f,
            };
        }
        else
        {
            vertices = {
                 1.f,  1.f,  1.f, 0.f,
                 1.f, -1.f,  1.f, 1.f,
                -1.f, -1.f,  0.f, 1.f,
                -1.f,  1.f,  0.f, 0.f,
            };
        }
        // clang-format on
        GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO ) );
        GL_CHECK( glBufferSubData( GL_ARRAY_BUFFER, 0, vertices.size() * sizeof( float ), vertices.data() ) );
    }
    else    
        GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO ) );

    GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_EBO ) );

    GL_CHECK( glVertexAttribPointer( m_attrVertex, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), ( void* )nullptr ) );
    GL_CHECK( glEnableVertexAttribArray( m_attrVertex ) );

    GL_CHECK( glVertexAttribPointer( m_attrUVs, 2, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), ( void* )( 2 * sizeof( float ) ) ) );
    GL_CHECK( glEnableVertexAttribArray( m_attrUVs ) );

    return changed;
}

void VideoRenderer::draw( int width, int height, uint8_t** data )
{
    const bool changed = bindBuffers( width, height );
    const GLsizei widths[ 3 ]  { width, width >> 1, width >> 1 };
    const GLsizei heights[ 3 ] { height, height >> 1, height >> 1 };

    for ( int i = 0; i < 3; ++i )
    {
        GL_CHECK( glActiveTexture( GL_TEXTURE0 + i ) );
        GL_CHECK( glBindTexture( GL_TEXTURE_2D, m_texs[ i ] ) );
        if ( changed )
        {
            GL_CHECK( glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE4, widths[ i ], heights[ i ], 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data[ i ] ) );
        }
        else
        {
            GL_CHECK( glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, widths[ i ], heights[ i ], GL_LUMINANCE, GL_UNSIGNED_BYTE, data[ i ] ) );
        }
    }

    m_shader->use();

    GL_CHECK( glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr ) );
}

void VideoRenderer::drawNV12( int width, int height, uint8_t** data )
{
    const bool changed = bindBuffers( width, height );

    GL_CHECK( glActiveTexture( GL_TEXTURE0 ) );
    GL_CHECK( glBindTexture( GL_TEXTURE_2D, m_texs[ 0 ] ) );
    if ( changed )
    {
        GL_CHECK( glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE4, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data[ 0 ] ) );
    }
    else
    {
        GL_CHECK( glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, data[ 0 ] ) );
    }

    GL_CHECK( glActiveTexture( GL_TEXTURE1 ) );
    GL_CHECK( glBindTexture( GL_TEXTURE_2D, m_texs[ 1 ] ) );
    if ( changed )
    {
        GL_CHECK( glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE4_ALPHA4, width >> 1, height >> 1, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data[ 1 ] ) );
    }
    else
    {
        GL_CHECK( glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, width >> 1, height >> 1, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data[ 1 ] ) );
    }

    m_shaderNV12->use();

    GL_CHECK( glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr ) );
}