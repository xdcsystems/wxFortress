#include <memory>
#include <vector>
#include <string>

#include <GL/glew.h>

#include "Common/Tools.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/Shader.h"
#include "VideoRenderer.h"

// clang-format off
float vertices[] = {
    1.0,   1.0, 0.0,     1.0, 0.0,
    1.0,  -1.0, 0.0,     1.0, 1.0,
   -1.0,  -1.0, 0.0,     0.0, 1.0,
   -1.0,   1.0, 0.0,     0.0, 0.0,
};

unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3
};
// clang-format on

VideoRenderer::VideoRenderer()
{
    m_shader = ResourceManager::LoadShader( "/../data/shaders/Video.vs", "/../data/shaders/Video.fraq", "", "video" );

    GL_CHECK( glGenVertexArrays( 1, &m_VAO ) );
    GL_CHECK( glGenBuffers( 1, &m_VBO ) );
    GL_CHECK( glGenBuffers( 1, &m_EBO ) );
    GL_CHECK( glBindVertexArray( m_VAO ) );
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO ) );
    GL_CHECK( glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_DYNAMIC_DRAW ) );
    GL_CHECK( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_EBO ) );
    GL_CHECK( glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW ) );
    GL_CHECK( glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), 0 ) );
    GL_CHECK( glEnableVertexAttribArray( 0 ) );
    GL_CHECK( glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), (void *)( 3 * sizeof( float ) ) ) );
    GL_CHECK( glEnableVertexAttribArray( 1 ) );

    GL_CHECK( glGenTextures( 3, m_texs ) );
    for ( int i = 0; i < 3; i++ )
    {
        GL_CHECK( glBindTexture( GL_TEXTURE_2D, m_texs[i] ) );
        GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
        GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );
        GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
        GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
    }

    m_shader->setInteger( "textureY", 0, true );
    m_shader->setInteger( "textureU", 1 );
    m_shader->setInteger( "textureV", 2 );
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

void VideoRenderer::draw( int width, int height, uint8_t **data, int *linesize )
{
    bool changed = false;
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

    float tr = (float)m_texWidth / m_texHeight;
    float vr = (float)m_viewWidth / m_viewHeight;
    float r = tr / vr;
    if ( r != m_ratio )
    {
        m_ratio = r;
        std::vector<float> vertices;
        // clang-format off
        if (tr > vr) {
            float p = vr / tr;
            vertices = {
                1.0,  p, 0.0,     1.0, 0.0,
                1.0, -p, 0.0,     1.0, 1.0,
               -1.0, -p, 0.0,     0.0, 1.0,
               -1.0,  p, 0.0,     0.0, 0.0,
            };
        } else if (tr < vr) {
            vertices = {
                r,  1.0, 0.0,     1.0, 0.0,
                r, -1.0, 0.0,     1.0, 1.0,
               -r, -1.0, 0.0,     0.0, 1.0,
               -r,  1.0, 0.0,     0.0, 0.0,
            };
        } else {
            vertices = {
                1.0,  1.0, 0.0,     1.0, 0.0,
                1.0, -1.0, 0.0,     1.0, 1.0,
               -1.0, -1.0, 0.0,     0.0, 1.0,
               -1.0,  1.0, 0.0,     0.0, 0.0,
            };
        }
        // clang-format on
        GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO ) );
        GL_CHECK( glBufferSubData( GL_ARRAY_BUFFER, 0, vertices.size() * sizeof( float ), vertices.data() ) );
    }

    GL_CHECK( glActiveTexture( GL_TEXTURE0 ) );
    GL_CHECK( glBindTexture( GL_TEXTURE_2D, m_texs[0] ) );
    GL_CHECK( glPixelStorei( GL_UNPACK_ROW_LENGTH, linesize[0] ) );
    if ( changed )
    {
        GL_CHECK( glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data[0] ) );
    }
    else
    {
        GL_CHECK( glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, data[0] ) );
    }
    GL_CHECK( glActiveTexture( GL_TEXTURE1 ) );
    GL_CHECK( glBindTexture( GL_TEXTURE_2D, m_texs[1] ) );
    GL_CHECK( glPixelStorei( GL_UNPACK_ROW_LENGTH, linesize[1] ) );
    if ( changed )
    {
        GL_CHECK( glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, data[1] ) );
    }
    else
    {
        GL_CHECK( glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, data[1] ) );
    }
    GL_CHECK( glActiveTexture( GL_TEXTURE2 ) );
    GL_CHECK( glBindTexture( GL_TEXTURE_2D, m_texs[2] ) );
    GL_CHECK( glPixelStorei( GL_UNPACK_ROW_LENGTH, linesize[2] ) );
    if ( changed )
    {
        GL_CHECK( glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, data[2] ) );
    }
    else
    {
        GL_CHECK( glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_RED, GL_UNSIGNED_BYTE, data[2] ) );
    }

    GL_CHECK( glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 ) );

    m_shader->use();
    GL_CHECK( glBindVertexArray( m_VAO ) );
    GL_CHECK( glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 ) );
}

