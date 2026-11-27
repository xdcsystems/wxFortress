#pragma once

// EBO-friendly mixin banning copy and move for resource-owning classes.
class NonCopyable
{
    protected:
        NonCopyable() = default;
        ~NonCopyable() = default;

        NonCopyable( const NonCopyable& ) = delete;
        NonCopyable& operator=( const NonCopyable& ) = delete;
        NonCopyable( NonCopyable&& ) = delete;
        NonCopyable& operator=( NonCopyable&& ) = delete;
};
