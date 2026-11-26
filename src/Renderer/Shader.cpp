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

int Shader::getUniformLocation( const char* name )
{
    if ( ID == 0 || !name )
        return -1;

    const auto it = m_uniformCache.find( name );
    if ( it != m_uniformCache.end() )
        return it->second;

    int loc = -1;
    GL_CHECK( loc = glGetUniformLocation( ID, name ) );
    m_uniformCache.emplace( name, loc );
    return loc;
}

void Shader::compile( const char* vertexSource, const char* fragmentSource, const char* geometrySource )
{
    m_uniformCache.clear();
    if ( ID != 0 )
    {
        GL_CHECK( glDeleteProgram( ID ) );
        ID = 0;
    }

    if ( !vertexSource || !fragmentSource || vertexSource[ 0 ] == '\0' || fragmentSource[ 0 ] == '\0' )
    {
        wxLogDebug( wxT( "| ERROR::SHADER: Empty vertex or fragment shader source." ) );
        return;
    }

    unsigned int sVertex( 0 );
    unsigned int sFragment( 0 );
    unsigned int gShader( 0 );

    // vertex Shader
    GL_CHECK( sVertex = glCreateShader( GL_VERTEX_SHADER ) );
    GL_CHECK( glShaderSource( sVertex, 1, &vertexSource, nullptr ) );
    GL_CHECK( glCompileShader( sVertex ) );
    const bool vertOk = checkCompileErrors( sVertex, "VERTEX" );

    // fragment Shader
    GL_CHECK( sFragment = glCreateShader( GL_FRAGMENT_SHADER ) );
    GL_CHECK( glShaderSource( sFragment, 1, &fragmentSource, nullptr ) );
    GL_CHECK( glCompileShader( sFragment ) );
    const bool fragOk = checkCompileErrors( sFragment, "FRAGMENT" );

    bool geomOk = true;
    if ( geometrySource != nullptr )
    {
        GL_CHECK( gShader = glCreateShader( GL_GEOMETRY_SHADER ) );
        GL_CHECK( glShaderSource( gShader, 1, &geometrySource, nullptr ) );
        GL_CHECK( glCompileShader( gShader ) );
        geomOk = checkCompileErrors( gShader, "GEOMETRY" );
    }

    if ( vertOk && fragOk && geomOk )
    {
        GL_CHECK( ID = glCreateProgram() );
        GL_CHECK( glAttachShader( ID, sVertex ) );
        GL_CHECK( glAttachShader( ID, sFragment ) );
        if ( geometrySource != nullptr )
        {
            GL_CHECK( glAttachShader( ID, gShader ) );
        }

        GL_CHECK( glLinkProgram( ID ) );
        if ( !checkCompileErrors( ID, "PROGRAM" ) )
        {
            GL_CHECK( glDeleteProgram( ID ) );
            ID = 0;
        }
    }

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
        use();

    const int loc = getUniformLocation( name );
    if ( loc >= 0 )
        GL_CHECK( glUniform1f( loc, value ) );
}

void Shader::setInteger( const char* name, int value, bool useShader )
{
    if ( useShader )
        use();

    const int loc = getUniformLocation( name );
    if ( loc >= 0 )
        GL_CHECK( glUniform1i( loc, value ) );
}

void Shader::setVector2f( const char* name, float x, float y, bool useShader )
{
    if ( useShader )
        use();

    const int loc = getUniformLocation( name );
    if ( loc >= 0 )
        GL_CHECK( glUniform2f( loc, x, y ) );
}

void Shader::setVector2f( const char* name, const glm::vec2& value, bool useShader )
{
    if ( useShader )
        use();

    const int loc = getUniformLocation( name );
    if ( loc >= 0 )
        GL_CHECK( glUniform2f( loc, value.x, value.y ) );
}

void Shader::setVector3f( const char* name, float x, float y, float z, bool useShader )
{
    if ( useShader )
        use();

    const int loc = getUniformLocation( name );
    if ( loc >= 0 )
        GL_CHECK( glUniform3f( loc, x, y, z ) );
}

void Shader::setVector3f( const char* name, const glm::vec3& value, bool useShader )
{
    if ( useShader )
        use();

    const int loc = getUniformLocation( name );
    if ( loc >= 0 )
        GL_CHECK( glUniform3f( loc, value.x, value.y, value.z ) );
}

void Shader::setVector4f( const char* name, float x, float y, float z, float w, bool useShader )
{
    if ( useShader )
        use();

    const int loc = getUniformLocation( name );
    if ( loc >= 0 )
        GL_CHECK( glUniform4f( loc, x, y, z, w ) );
}

void Shader::setVector4f( const char* name, const glm::vec4& value, bool useShader )
{
    if ( useShader )
        use();

    const int loc = getUniformLocation( name );
    if ( loc >= 0 )
        GL_CHECK( glUniform4f( loc, value.x, value.y, value.z, value.w ) );
}

void Shader::setMatrix4( const char* name, const glm::mat4& matrix, bool useShader )
{
    if ( useShader )
        use();

    const int loc = getUniformLocation( name );
    if ( loc >= 0 )
        GL_CHECK( glUniformMatrix4fv( loc, 1, GL_FALSE, glm::value_ptr( matrix ) ) );
}

bool Shader::checkCompileErrors( unsigned int object, const std::string &type )
{
    int success = 0;
    char infoLog[ 1024 ] {};
    if ( type != "PROGRAM" )
    {
        GL_CHECK( glGetShaderiv( object, GL_COMPILE_STATUS, &success ) );
        if ( !success )
        {
            GL_CHECK( glGetShaderInfoLog( object, sizeof( infoLog ), nullptr, infoLog ) );
            wxLogDebug( wxT( "| ERROR::SHADER: Compile-time error: Type: " ) + type );
            wxLogDebug( "%s", infoLog );
            wxLogDebug( " ----------------------------------------------------- " );
        }
    }
    else
    {
        GL_CHECK( glGetProgramiv( object, GL_LINK_STATUS, &success ) );
        if ( !success )
        {
            GL_CHECK( glGetProgramInfoLog( object, sizeof( infoLog ), nullptr, infoLog ) );
            wxLogDebug( wxT( "| | ERROR::Shader: Link-time error: Type: " ) + type );
            wxLogDebug( "%s", infoLog );
            wxLogDebug( " ----------------------------------------------------- " );
        }
    }
    return success != 0;
}
