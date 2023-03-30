#pragma once

// Forward declarations
class Base;
class Explosion;
class SpriteRenderer;
class Texture2D;

namespace Shapes
{
    class Explosions
    {
        using basePtr = std::shared_ptr<Base>;
        using explosionPtr = std::shared_ptr<Explosion>;
        using texture2DPtr = std::shared_ptr<Texture2D>;
        using rendererPtr = std::shared_ptr<SpriteRenderer>;

        public:
            Explosions();
            ~Explosions();

            void add( const basePtr &shape );
            void clear();
            void draw( const rendererPtr &renderer );
            bool empty() const;

        protected:
            static inline const glm::vec2 s_defaultSize = { 128.f, 128.f };
            std::vector<explosionPtr> m_explosions;
            texture2DPtr m_explosionsSprite;
            std::vector<unsigned int> m_VBO;
    };
}
