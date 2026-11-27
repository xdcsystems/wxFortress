#pragma once

#include <type_traits>
#include <utility>

template <typename Fn>
class ScopeExit
{
    public:
        template <
            typename F,
            typename = std::enable_if_t<
                std::is_constructible_v<Fn, F> && !std::is_same_v<std::decay_t<F>, ScopeExit>>>
        explicit ScopeExit( F&& fn ) noexcept( std::is_nothrow_constructible_v<Fn, F> )
            : m_fn( std::forward<F>( fn ) )
        {}

        ~ScopeExit()
        {
            if ( m_active )
                m_fn();
        }

        ScopeExit( const ScopeExit& ) = delete;
        ScopeExit& operator=( const ScopeExit& ) = delete;

        ScopeExit( ScopeExit&& other ) noexcept( std::is_nothrow_move_constructible_v<Fn> )
            : m_fn( std::move( other.m_fn ) )
            , m_active( other.m_active )
        {
            other.m_active = false;
        }

        ScopeExit& operator=( ScopeExit&& ) = delete;

        void dismiss() noexcept { m_active = false; }

    private:
        Fn m_fn;
        bool m_active { true };
};

template <typename Fn>
[[nodiscard]] inline ScopeExit<std::decay_t<Fn>> makeScopeExit( Fn&& fn ) noexcept( std::is_nothrow_constructible_v<std::decay_t<Fn>, Fn> )
{
    return ScopeExit<std::decay_t<Fn>>( std::forward<Fn>( fn ) );
}
