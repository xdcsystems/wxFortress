#pragma once

#include <map>
#include <glm/glm.hpp>


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

class TextRenderer
{
    using texture2DPtr = std::shared_ptr<Texture2D>;
    using shaderPtr = std::shared_ptr<Shader>;

    public:
        TextRenderer( wxWindow* parent );
        ~TextRenderer();

        void renderFrame();
        void print( const std::string &text, int x, int y, const glm::vec2& size );
        void cleanup();

        void switchToFinishState( unsigned short stage );
        void switchToHelpState();

        void selectFontType( TextRendererFont fontType ) { m_fontType = fontType; };

    private:
        void renderFinishMessage();
        void renderHelp();

        unsigned int m_vertexBufferID { 0 };
        unsigned int m_UVBufferID { 0 };
        unsigned int m_uniformID { 0 };
        unsigned int m_attrVertex { 0 };
        unsigned int m_attrUVs { 0 };

        unsigned short m_col { 0 };
        unsigned short m_row { 0 };
        unsigned short m_delay { 57 };
        
        unsigned short m_stage { 0 };

        float m_startLinePosition { .0f };

        std::vector<std::string> m_message;
        std::vector<glm::vec2> m_vertices;
        std::vector<glm::vec2> m_UVs;

        std::shared_ptr<Timer> m_timer;

        wxEvtHandler* m_eventHandler { nullptr };
        wxCommandEvent m_eventCharShow;

        TextRendererState m_state = TextRendererState::FINISHED;
        TextRendererFont m_fontType = TextRendererFont::NORMAL;

        shaderPtr m_shader;

        inline static std::map<TextRendererFont, std::pair<float, texture2DPtr> > s_fontData {
            { TextRendererFont::NORMAL, { 8.f, nullptr } },
            { TextRendererFont::OLD, { 8.f, nullptr } }
        };
};

using textRedererPtr = std::shared_ptr<TextRenderer>;

