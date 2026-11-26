// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <GL/glew.h>

#include <iostream>
#include <sstream>
#include <fstream>

#include <wx/mstream.h>

#include "Common/Tools.h"
#include "Texture.h"
#include "Shader.h"
#include "ResourceManager.h"

namespace {
    std::string loadResourceText( const std::string& name )
    {
        const auto bytes = Tools::Instance().loadResourceBytes( name );
        return std::string( reinterpret_cast<const char*>( bytes.data() ), bytes.size() );
    }
}

bool ResourceManager::LoadResources()
{
    Tools::Instance().loadResources();
    return true;
}

shaderPtr ResourceManager::LoadShader( const std::string& vShaderName, const std::string& fShaderName, const std::string& gShaderName, const std::string& name )
{
    s_shaders[ name ] = LoadShader( vShaderName, fShaderName, gShaderName );
    return s_shaders[ name ];
}

shaderPtr ResourceManager::GetShader( const std::string& name )
{
    const auto it = s_shaders.find( name );
    return ( it != s_shaders.end() ) ? it->second : nullptr;
}

texture2DPtr ResourceManager::LoadTexture( const std::string& name, const std::string& textureName )
{
    s_textures[ textureName ] = LoadTexture( name );
    return s_textures[ textureName ];
}

texture2DPtr ResourceManager::GetTexture( const std::string& name )
{
    const auto it = s_textures.find( name );
    return ( it != s_textures.end() ) ? it->second : nullptr;
}

void ResourceManager::Clear()
{
    // (properly) delete all shaders
    for ( const auto &iter : s_shaders )
    {
        if ( iter.second && iter.second->ID != 0 )
        {
            GL_CHECK( glDeleteProgram( iter.second->ID ) );
            iter.second->ID = 0;
        }
    }
    s_shaders.clear();

    // (properly) delete all textures
    for ( const auto &iter : s_textures )
    {
        if ( iter.second && iter.second->ID != 0 )
        {
            GL_CHECK( glDeleteTextures( 1, &iter.second->ID ) );
            iter.second->ID = 0;
        }
    }
    s_textures.clear();
}

shaderPtr ResourceManager::LoadShader( const std::string& vShaderName, const std::string& fShaderName, const std::string& gShaderName )
{
    // 1. retrieve the vertex/fragment source code from resource
    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;
    try
    {
        vertexCode = loadResourceText( vShaderName );
        fragmentCode = loadResourceText( fShaderName );

        // if geometry shader path is present, also load a geometry shader
        if ( !gShaderName.empty() )
        {
            geometryCode = loadResourceText( gShaderName );
        }
    }
    catch ( const std::exception& e )
    {
        wxLogMessage( "ERROR::SHADER: Failed to read shader files, \"%s\"", e.what() );
        return std::make_shared<Shader>();
    }

    auto shader = std::make_shared<Shader>();
    if ( vertexCode.empty() || fragmentCode.empty() || ( !gShaderName.empty() && geometryCode.empty() ) )
    {
        wxLogMessage( "ERROR::SHADER: Empty shader source for \"%s\" / \"%s\"", vShaderName.c_str(), fShaderName.c_str() );
        return shader;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    const char* gShaderCode = geometryCode.c_str();

    // 2. now create shader object from source code
    shader->compile( vShaderCode, fShaderCode, gShaderName.empty() ? nullptr : gShaderCode );

    return shader;
}

texture2DPtr ResourceManager::LoadTexture( const std::string& name )
{
    // create texture object
    const auto bytes = Tools::Instance().loadResourceBytes( name );
    wxMemoryInputStream memStream( bytes.data(), bytes.size() );
    auto texture = std::make_shared<Texture2D>();
    texture->generate( { memStream, wxBITMAP_TYPE_PNG } );

    return texture;
}
