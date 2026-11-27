// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "Common/Tools.h"
#include "Shader.h"

Shader& Shader::use()
{
    if ( ID != 0 )
        GL_CHECK( glUseProgram( ID ) );

    return *this;
}

int Shader::getUniformLocation( const char* name )
{
    if ( ID == 0 || name == nullptr )
        return -1;

    const auto it = m_uniformCache.find( name );
    if ( it != m_uniformCache.end() )
        return it->second;

    int loc = -1;
    GL_CHECK( loc = glGetUniformLocation( ID, name ) );
    m_uniformCache.emplace( name, loc );
    return loc;
}

template <typename Fn>
void Shader::applyUniform( const char* name, bool useShader, Fn&& fn )
{
    if ( useShader )
        use();

    const int loc = getUniformLocation( name );
    if ( loc >= 0 )
        fn( loc );
}

void Shader::compile( const char* vertexSource, const char* fragmentSource, const char* geometrySource )
{
    m_uniformCache.clear();
    if ( ID != 0 )
    {
        GL_CHECK( glDeleteProgram( ID ) );
        ID = 0;
    }

    if ( vertexSource == nullptr || fragmentSource == nullptr || vertexSource[ 0 ] == '\0' || fragmentSource[ 0 ] == '\0' )
    {
        wxLogDebug( wxT( "| ERROR::SHADER: Empty vertex or fragment shader source." ) );
        return;
    }

    auto compileStage = [ this ]( GLenum stage, const char* source, const char* label, unsigned int& outId ) -> bool
    {
        GL_CHECK( outId = glCreateShader( stage ) );
        GL_CHECK( glShaderSource( outId, 1, &source, nullptr ) );
        GL_CHECK( glCompileShader( outId ) );
        return checkCompileErrors( outId, label );
    };

    unsigned int sVertex = 0;
    unsigned int sFragment = 0;
    unsigned int gShader = 0;

    const bool vertOk = compileStage( GL_VERTEX_SHADER, vertexSource, "VERTEX", sVertex );
    const bool fragOk = compileStage( GL_FRAGMENT_SHADER, fragmentSource, "FRAGMENT", sFragment );
    const bool geomOk = ( geometrySource == nullptr )
        ? true
        : compileStage( GL_GEOMETRY_SHADER, geometrySource, "GEOMETRY", gShader );

    if ( vertOk && fragOk && geomOk )
    {
        GL_CHECK( ID = glCreateProgram() );
        GL_CHECK( glAttachShader( ID, sVertex ) );
        GL_CHECK( glAttachShader( ID, sFragment ) );
        if ( geometrySource != nullptr )
            GL_CHECK( glAttachShader( ID, gShader ) );

        GL_CHECK( glLinkProgram( ID ) );
        if ( !checkCompileErrors( ID, "PROGRAM" ) )
        {
            GL_CHECK( glDeleteProgram( ID ) );
            ID = 0;
        }
    }

    GL_CHECK( glDeleteShader( sVertex ) );
    GL_CHECK( glDeleteShader( sFragment ) );
    if ( geometrySource != nullptr )
        GL_CHECK( glDeleteShader( gShader ) );
}

void Shader::setFloat( const char* name, float value, bool useShader )
{
    applyUniform( name, useShader, [ value ]( int loc ) {
        GL_CHECK( glUniform1f( loc, value ) );
    } );
}

void Shader::setInteger( const char* name, int value, bool useShader )
{
    applyUniform( name, useShader, [ value ]( int loc ) {
        GL_CHECK( glUniform1i( loc, value ) );
    } );
}

void Shader::setVector2f( const char* name, float x, float y, bool useShader )
{
    applyUniform( name, useShader, [ x, y ]( int loc ) {
        GL_CHECK( glUniform2f( loc, x, y ) );
    } );
}

void Shader::setVector2f( const char* name, const glm::vec2& value, bool useShader )
{
    setVector2f( name, value.x, value.y, useShader );
}

void Shader::setVector3f( const char* name, float x, float y, float z, bool useShader )
{
    applyUniform( name, useShader, [ x, y, z ]( int loc ) {
        GL_CHECK( glUniform3f( loc, x, y, z ) );
    } );
}

void Shader::setVector3f( const char* name, const glm::vec3& value, bool useShader )
{
    setVector3f( name, value.x, value.y, value.z, useShader );
}

void Shader::setVector4f( const char* name, float x, float y, float z, float w, bool useShader )
{
    applyUniform( name, useShader, [ x, y, z, w ]( int loc ) {
        GL_CHECK( glUniform4f( loc, x, y, z, w ) );
    } );
}

void Shader::setVector4f( const char* name, const glm::vec4& value, bool useShader )
{
    setVector4f( name, value.x, value.y, value.z, value.w, useShader );
}

void Shader::setMatrix4( const char* name, const glm::mat4& matrix, bool useShader )
{
    applyUniform( name, useShader, [ &matrix ]( int loc ) {
        GL_CHECK( glUniformMatrix4fv( loc, 1, GL_FALSE, glm::value_ptr( matrix ) ) );
    } );
}

bool Shader::checkCompileErrors( unsigned int object, const std::string& type )
{
    int success = 0;
    char infoLog[ 1024 ] {};
    if ( type != "PROGRAM" )
    {
        GL_CHECK( glGetShaderiv( object, GL_COMPILE_STATUS, &success ) );
        if ( !success )
        {
            GL_CHECK( glGetShaderInfoLog( object, static_cast<GLsizei>( sizeof( infoLog ) ), nullptr, infoLog ) );
            wxLogDebug( wxT( "| ERROR::SHADER: Compile-time error: Type: %s\n%s\n -- --------------------------------------------------- -- " ), type.c_str(), infoLog );
            return false;
        }
    }
    else
    {
        GL_CHECK( glGetProgramiv( object, GL_LINK_STATUS, &success ) );
        if ( !success )
        {
            GL_CHECK( glGetProgramInfoLog( object, static_cast<GLsizei>( sizeof( infoLog ) ), nullptr, infoLog ) );
            wxLogDebug( wxT( "| ERROR::Shader: Link-time error: Type: %s\n%s\n -- --------------------------------------------------- -- " ), type.c_str(), infoLog );
            return false;
        }
    }

    return true;
}
