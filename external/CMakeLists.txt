cmake_minimum_required(VERSION 3.14.7 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH
 "${PROJECT_SOURCE_DIR}/external/.cmake/"
)

include(ExternalLibraries)

# External Libs

# glm
add_subdirectory(glm)

# glew
add_subdirectory_with_folder("glew-cmake" glew-cmake)

# ffmpeg
if( WIN32 )
    set(ENV{FFMPEG_HOME} "${PROJECT_SOURCE_DIR}/external/ffmpeg/win32/")
endif()

find_package(FFmpeg
    COMPONENTS AVCODEC AVFORMAT AVUTIL AVDEVICE AVFILTER SWSCALE SWRESAMPLE
    REQUIRED)

# openal-soft
option(ALSOFT_UTILS    "Build utility programs"             OFF)
OPTION(ALSOFT_EXAMPLES "Build and install example programs" OFF)

add_subdirectory_with_folder("openal-soft" openal-soft)

# wxWidgets
set(wxUSE_OPENGL, 1)
set(wxUSE_AUI 0)
set(wxUSE_PROPGRID 0)
set(wxUSE_QA 0)
set(wxUSE_RIBBON 0)
set(wxUSE_RICHTEXT 0)
set(wxUSE_WEBVIEW 0)
set(wxUSE_STC 0)
set(wxUSE_MEDIACTRL 0)
#set(wxUSE_XRC 0)

add_subdirectory_with_folder("wxWidgets" wxWidgets)

# google tests
add_subdirectory_with_folder("googletest" googletest)
