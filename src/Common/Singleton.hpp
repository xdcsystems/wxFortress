#pragma once

// Meyers' Singleton Pattern implementation
template <typename T>
class Singleton
{
    public:
        static T& Instance();

        Singleton( const Singleton& ) = delete;
        Singleton& operator=( const Singleton& ) = delete;
        Singleton( Singleton&& ) = delete;
        Singleton& operator=( Singleton&& ) = delete;

    protected:
        Singleton() = default;
        ~Singleton() = default;
        struct token
        {};
};

template <typename T>
T& Singleton<T>::Instance()
{
    static T instance { token {} };
    return instance;
}
