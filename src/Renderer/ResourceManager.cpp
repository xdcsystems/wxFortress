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

#include "Common/Tools.h"
#include "Texture.h"
#include "Shader.h"
#include "ResourceManager.h"

namespace {
    std::string gulp( wxInputStream& in )
    {
        std::string ret;
        if ( in.CanRead() )
        {
            const auto streamSize { in.GetSize() };
            ret.resize( streamSize );
            in.Read( &ret[0], streamSize );
        }
        return ret;
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
    return s_shaders[ name ];
}

texture2DPtr ResourceManager::LoadTexture( const std::string& name, const std::string& textureName )
{
    s_textures[ textureName ] = LoadTexture( name );
    return s_textures[ textureName ];
}

texture2DPtr ResourceManager::GetTexture( const std::string& name )
{
    return s_textures[ name ];
}

void ResourceManager::Clear()
{
    // (properly) delete all shaders
    for ( const auto &iter : s_shaders )
    {
        GL_CHECK( glDeleteProgram( iter.second->ID ) );
    }

    // (properly) delete all textures
    for ( const auto &iter : s_textures )
    {
        GL_CHECK( glDeleteTextures( 1, &iter.second->ID ) );
    }
}

shaderPtr ResourceManager::LoadShader( const std::string& vShaderName, const std::string& fShaderName, const std::string& gShaderName )
{
    // 1. retrieve the vertex/fragment source code from resource
    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;
    try
    {
        vertexCode = gulp( Tools::Instance().loadResource( vShaderName ) );
        fragmentCode = gulp( Tools::Instance().loadResource( fShaderName ) );

        // if geometry shader path is present, also load a geometry shader
        if ( !gShaderName.empty() )
        {
            geometryCode = gulp( Tools::Instance().loadResource( gShaderName ) );
        }
    }
    catch ( const std::exception& e )
    {
        wxLogMessage( "ERROR::SHADER: Failed to read shader files, \"%s\"", e.what() );
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    const char* gShaderCode = geometryCode.c_str();

    // 2. now create shader object from source code
    auto shader = std::make_shared<Shader>();
    shader->compile( vShaderCode, fShaderCode, gShaderName.empty() ? nullptr : gShaderCode );

    return shader;
}

texture2DPtr ResourceManager::LoadTexture( const std::string& name )
{
    // create texture object
    auto texture = std::make_shared<Texture2D>();
    texture->generate( { Tools::Instance().loadResource( name ), wxBITMAP_TYPE_PNG } );

    return texture;
}
