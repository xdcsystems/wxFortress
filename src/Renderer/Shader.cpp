// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <iostream>
#include <GL/glew.h>

#include "Common/Tools.h"
#include "Shader.h"


Shader& Shader::use()
{
    GL_CHECK( glUseProgram( ID ) );
    return *this;
}

void Shader::compile( const char* vertexSource, const char* fragmentSource, const char* geometrySource )
{
    unsigned int sVertex( 0 );
    unsigned int sFragment( 0 );
    unsigned int gShader( 0 );

    // vertex Shader
    GL_CHECK( sVertex = glCreateShader( GL_VERTEX_SHADER ) );
    GL_CHECK( glShaderSource( sVertex, 1, &vertexSource, nullptr ) );
    GL_CHECK( glCompileShader( sVertex ) );

    checkCompileErrors( sVertex, "VERTEX" );

    // fragment Shader
    GL_CHECK( sFragment = glCreateShader( GL_FRAGMENT_SHADER ) );
    GL_CHECK( glShaderSource( sFragment, 1, &fragmentSource, nullptr ) );
    GL_CHECK( glCompileShader( sFragment ) );

    checkCompileErrors( sFragment, "FRAGMENT" );

    // if geometry shader source code is given, also compile geometry m_shader
    if ( geometrySource != nullptr )
    {
        GL_CHECK( gShader = glCreateShader( GL_GEOMETRY_SHADER ) );
        GL_CHECK( glShaderSource( gShader, 1, &geometrySource, nullptr ) );
        GL_CHECK( glCompileShader( gShader ) );

        checkCompileErrors( gShader, "GEOMETRY" );
    }

    // shader program
    GL_CHECK( ID = glCreateProgram() );
    GL_CHECK( glAttachShader( ID, sVertex ) );
    GL_CHECK( glAttachShader( ID, sFragment ) );

    if ( geometrySource != nullptr )
    {
        GL_CHECK( glAttachShader( ID, gShader ) );
    }

    GL_CHECK( glLinkProgram( ID ) );

    checkCompileErrors( ID, "PROGRAM" );

    // delete the shaders as they're linked into our program now and no longer necessary
    GL_CHECK( glDeleteShader( sVertex ) );
    GL_CHECK( glDeleteShader( sFragment ) );

    if ( geometrySource != nullptr )
    {
        GL_CHECK( glDeleteShader( gShader ) );
    }
}

void Shader::setFloat( const char* name, float value, bool useShader )
{
    if ( useShader )
    {
        use();
    }

    GL_CHECK( glUniform1f( glGetUniformLocation( ID, name ), value ) );
}

void Shader::setInteger( const char* name, int value, bool useShader )
{
    if ( useShader )
    {
        use();
    }

    GL_CHECK( glUniform1i( glGetUniformLocation( ID, name ), value ) );
}

void Shader::setVector2f( const char* name, float x, float y, bool useShader )
{
    if ( useShader )
    {
        use();
    }

    GL_CHECK( glUniform2f( glGetUniformLocation( ID, name ), x, y ) );
}

void Shader::setVector2f( const char* name, const glm::vec2& value, bool useShader )
{
    if ( useShader )
    {
        use();
    }

    GL_CHECK( glUniform2f( glGetUniformLocation( ID, name ), value.x, value.y ) );
}

void Shader::setVector3f( const char* name, float x, float y, float z, bool useShader )
{
    if ( useShader )
    {
        use();
    }

    GL_CHECK( glUniform3f( glGetUniformLocation( ID, name ), x, y, z ) );
}

void Shader::setVector3f( const char* name, const glm::vec3& value, bool useShader )
{
    if ( useShader )
    {
        use();
    }

    GL_CHECK( glUniform3f( glGetUniformLocation( ID, name ), value.x, value.y, value.z ) );
}

void Shader::setVector4f( const char* name, float x, float y, float z, float w, bool useShader )
{
    if ( useShader )
    {
        use();
    }

    GL_CHECK( glUniform4f( glGetUniformLocation( ID, name ), x, y, z, w ) );
}

void Shader::setVector4f( const char* name, const glm::vec4& value, bool useShader )
{
    if ( useShader )
    {
        use();
    }

    GL_CHECK( glUniform4f( glGetUniformLocation( ID, name ), value.x, value.y, value.z, value.w ) );
}

void Shader::setMatrix4( const char* name, const glm::mat4& matrix, bool useShader )
{
    if ( useShader )
    {
        use();
    }

    GL_CHECK( glUniformMatrix4fv( glGetUniformLocation( ID, name ), 1, false, glm::value_ptr( matrix ) ) );
}

void Shader::checkCompileErrors( unsigned int object, const std::string &type )
{
    int success( 0 );
    char infoLog[ 1024 ];
    if ( type != "PROGRAM" )
    {
        GL_CHECK( glGetShaderiv( object, GL_COMPILE_STATUS, &success ) );
        if ( !success )
        {
            GL_CHECK( glGetShaderInfoLog( object, 1024, nullptr, infoLog ) );
            wxLogDebug( wxT( "| ERROR::SHADER: Compile-time error: Type: " ) + type );
            wxLogDebug( infoLog );
            wxLogDebug( " ----------------------------------------------------- " );
        }
    }
    else
    {
        GL_CHECK( glGetProgramiv( object, GL_LINK_STATUS, &success ) );
        if ( !success )
        {
            GL_CHECK( glGetProgramInfoLog( object, 1024, nullptr, infoLog ) );
            wxLogDebug( wxT( "| | ERROR::Shader: Link-time error: Type: " ) + type );
            wxLogDebug( infoLog );
            wxLogDebug( " ----------------------------------------------------- " );
        }
    }
}