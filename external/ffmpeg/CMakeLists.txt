cmake_minimum_required(VERSION 3.16)

project(ffmpeg
    VERSION 0.0.1
    DESCRIPTION "Библиотека предоставляет API FFmpeg"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH
 "${PROJECT_SOURCE_DIR}/.cmake"
)

include(cxx-project-environ)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CONFIGURATION_TYPES Release Debug)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    set(MARCH_OPT "armv8.2-a")
else()
    set(MARCH_OPT "native")
endif()

set(ADDITIONAL_CXXFLAGS "-g -fsanitize=address -fno-omit-frame-pointer")
set(ADDITIONAL_CFLAGS "-fsanitize=address -fno-omit-frame-pointer")
set(ADDITIONAL_LFLAGS "-fsanitize=address")

#TODO: set(MARCH_OPT "native") - приводит к ошибкам выполнения при серверной сборке,
#      необходимо проработать вопрос с тулчейном
#set(COMMON_C_FLAGS "-pipe -Wall -Wextra -fno-omit-frame-pointer -pedantic -ftree-vectorize -march=${MARCH_OPT}")
set(COMMON_C_FLAGS "-pipe -Wall -Wextra -fno-omit-frame-pointer -pedantic -ftree-vectorize")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_C_FLAGS}")

if(CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_STAGING_PREFIX /tmp/${PROJECT_NAME})
endif()

set(DEPS_BUILD_PATH "${PROJECT_BINARY_DIR}" CACHE PATH "Full path to build directory for project.")
set(DEPS_INSTALL_PATH "${PROJECT_SOURCE_DIR}/install" CACHE PATH "Install path for the dependencies.")
set(CMAKE_INSTALL_PREFIX "${DEPS_INSTALL_PATH}" CACHE PATH "Where the library will be installed to" FORCE)

option(CMAKE_POSITION_INDEPENDENT_CODE "Position independent code" ON)

# add external libs
include(GNUInstallDirs)

if(DEFINED ENV{CROSS_COMPILE})
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
endif()

set(ENV{FFMPEG_HOME} "${PROJECT_BINARY_DIR}")

set(FFmpeg_FIND_COMPONENTS AVDEVICE AVFILTER AVFORMAT AVCODEC SWRESAMPLE SWSCALE AVUTIL)
include(FindFFmpeg)

if(NOT(FFMPEG_FOUND))
    execute_process(
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build_ffmpeg.sh
    )
    include(FindFFmpeg)
    if(NOT(FFMPEG_FOUND))
        message(FATAL_ERROR - "Error: FFMPEG not found ")
        return()
    endif()
endif()
