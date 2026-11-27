#include <GL/glew.h>

#include "Common/Tools.h"
#include "GlBuffer.hpp"

void GlBuffer::create() noexcept
{
    reset();
    GL_CHECK( glGenBuffers( 1, &m_id ) );
}

void GlBuffer::reset( unsigned int newId ) noexcept
{
    if ( m_id == newId )
        return;

    if ( m_id != 0 )
    {
        GL_CHECK( glDeleteBuffers( 1, &m_id ) );
    }
    m_id = newId;
}
