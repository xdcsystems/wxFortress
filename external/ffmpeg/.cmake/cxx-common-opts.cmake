#[=============================================================================[

Добавляет наиболее общеупотребительные опции сборки проектов на С++/С:
    - стандарт С++ 14
    - поддержка многопоточности
    - расширенные опции предупреждений
    - расширенные опции отладки/оптимизации для сборки в Debug-режиме
    - полезные опции оптимизации процесса компиляции

#]=============================================================================]

cmake_minimum_required(VERSION 3.7)

include(helpers)
ROOT_INCLUDE_GUARD()

enable_language(C)
enable_language(CXX)

# Search thread support
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
link_libraries(Threads::Threads)

# Set C++ compiler common options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "-stdlib=libstdc++ ${CMAKE_CXX_FLAGS}")
endif()

add_compile_options(-pipe -Wall -Wextra -fno-omit-frame-pointer)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb3")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags -Wl,--hash-style=gnu")

# Need for 'create-package' module
set(CCX_COMMON_OPTS_MODULE_INCLUDED TRUE)
