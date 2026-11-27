#pragma once

// Lightweight move-only RAII wrapper for OpenGL VBO/EBO handles.
// Intentionally avoids including <GL/glew.h> so headers stay free of GL/X11 ordering issues;
// glGenBuffers / glDeleteBuffers calls live in GlBuffer.cpp.
class GlBuffer final
{
    public:
        GlBuffer() noexcept = default;
        explicit GlBuffer( unsigned int id ) noexcept
            : m_id( id )
        {}

        ~GlBuffer() noexcept
        {
            reset();
        }

        GlBuffer( const GlBuffer& ) = delete;
        GlBuffer& operator=( const GlBuffer& ) = delete;

        GlBuffer( GlBuffer&& other ) noexcept
            : m_id( other.m_id )
        {
            other.m_id = 0;
        }

        GlBuffer& operator=( GlBuffer&& other ) noexcept
        {
            if ( this != &other )
            {
                reset( other.m_id );
                other.m_id = 0;
            }
            return *this;
        }

        void create() noexcept;
        void reset( unsigned int newId = 0 ) noexcept;

        unsigned int id() const noexcept { return m_id; }
        explicit operator bool() const noexcept { return m_id != 0; }

    private:
        unsigned int m_id { 0 };
};
