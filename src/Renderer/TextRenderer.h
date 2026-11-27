#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "Common/NonCopyable.hpp"


// Forward declarations
class Texture2D;
class Shader;
class Timer;

enum class TextRendererState : unsigned char
{
    FINISHED,
    HELP,
};

enum class TextRendererFont : unsigned char
{
    NORMAL,
    OLD,
};

class TextRenderer : private NonCopyable
{
    using texture2DPtr = std::shared_ptr<Texture2D>;
    using shaderPtr = std::shared_ptr<Shader>;

    public:
        TextRenderer( wxWindow* parent );
        ~TextRenderer();

        void resize( const glm::mat4& projection );
        void renderFrame();
        void print( const std::string& text, const glm::vec2& pos, const glm::vec2& size, glm::vec3 color = glm::vec3( 1.0f ) );
        void drawProgressArc( const glm::vec2& center, float radius, float thickness, float progress, const glm::vec4& baseColor, const glm::vec4& fillColor );
        void cleanup();

        void switchToFinishState( unsigned short stage );
        void switchToHelpState();

        void selectFontType( TextRendererFont fontType ) { m_fontType = fontType; };

    private:
        void renderFinishMessage();
        void renderHelp();

        unsigned int m_vertexBufferID { 0 };
        unsigned int m_UVBufferID { 0 };
        unsigned int m_arcBufferID { 0 };
        unsigned int m_attrVertex { 0 };
        unsigned int m_attrUVs { 0 };
        int m_attrArcVertex { -1 };

        unsigned short m_col { 0 };
        unsigned short m_row { 0 };
        unsigned short m_delay { 57 };
        
        unsigned short m_stage { 0 };

        float m_startLinePosition { .0f };

        std::vector<std::string> m_message;
        std::vector<glm::vec2> m_vertices;
        std::vector<glm::vec2> m_UVs;

        std::shared_ptr<Timer> m_timer;

        wxWindow* m_parent { nullptr };
        wxCommandEvent m_eventCharShow;

        TextRendererState m_state = TextRendererState::FINISHED;
        TextRendererFont m_fontType = TextRendererFont::NORMAL;

        shaderPtr m_shader;
        shaderPtr m_arcShader;

        inline static std::map<TextRendererFont, std::pair<float, texture2DPtr> > s_fontData {
            { TextRendererFont::NORMAL, { 8.f, nullptr } },
            { TextRendererFont::OLD, { 8.f, nullptr } }
        };
};

using textRedererPtr = std::shared_ptr<TextRenderer>;

