#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


// General purpose shader object. Compiles from file, generates
// compile/link-time error messages and hosts several utility 
// functions for easy management.
class Shader
{
    public:
        Shader() = default;
        Shader( const Shader& ) = delete;
        Shader& operator=( const Shader& ) = delete;
        Shader( Shader&& ) = delete;
        Shader& operator=( Shader&& ) = delete;

        // state
        unsigned int ID { 0 };

        // sets the current shader as active
        Shader& use();

        // compiles the shader from given source code
        void compile( const char* vertexSource, const char* fragmentSource, const char* geometrySource = nullptr ); // note: geometry source code is optional 

        // utility functions
        void setFloat( const char* name, float value, bool useShader = false );
        void setInteger( const char* name, int value, bool useShader = false );
        void setVector2f( const char* name, float x, float y, bool useShader = false );
        void setVector2f( const char* name, const glm::vec2& value, bool useShader = false );
        void setVector3f( const char* name, float x, float y, float z, bool useShader = false );
        void setVector3f( const char* name, const glm::vec3& value, bool useShader = false );
        void setVector4f( const char* name, float x, float y, float z, float w, bool useShader = false );
        void setVector4f( const char* name, const glm::vec4& value, bool useShader = false );
        void setMatrix4( const char* name, const glm::mat4& matrix, bool useShader = false );

    private:
        int getUniformLocation( const char* name );
        // checks if compilation or linking failed and if so, print the error logs
        bool checkCompileErrors( unsigned int object, const std::string &type );

        std::unordered_map<std::string, int> m_uniformCache;
};

using shaderPtr = std::shared_ptr<Shader>;
