cmake_minimum_required(VERSION 3.14.7 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH
 "${PROJECT_SOURCE_DIR}/external/.cmake/"
)

include(ExternalLibraries)

## External Libs

# glm
add_subdirectory(glm)

# glew
if( WIN32 )
  add_subdirectory_with_folder("glew-cmake" glew-cmake)
elseif( UNIX )  
#  If wxWidgets is compiled with egl, glew also needs to be compiled with egl support.
  add_subdirectory(${PROJECT_SOURCE_DIR}/external/glew-cmake/build/cmake)
endif()

# ffmpeg
if( WIN32 )
  set(ENV{FFMPEG_HOME} "${PROJECT_SOURCE_DIR}/external/ffmpeg/win32/")
elseif( UNIX )
  set(ENV{FFMPEG_HOME} "${PROJECT_SOURCE_DIR}/external/ffmpeg/linux/.build/")
  set(ENV{PKG_CONFIG_PATH} "${PROJECT_SOURCE_DIR}/external/ffmpeg/linux/.build/lib/pkgconfig")
endif()

find_package(FFmpeg
    COMPONENTS AVCODEC AVFORMAT AVUTIL SWRESAMPLE
    REQUIRED)

# openal-soft
option(ALSOFT_UTILS    "Build utility programs"             OFF)
option(ALSOFT_EXAMPLES "Build and install example programs" OFF)

add_subdirectory_with_folder("openal-soft" openal-soft)

# wxWidgets
set(wxUSE_OPENGL 1)
set(wxUSE_AUI 0)
set(wxUSE_PROPGRID 0)
set(wxUSE_QA 0)
set(wxUSE_RIBBON 0)
set(wxUSE_RICHTEXT 0)
set(wxUSE_WEBVIEW 0)
set(wxUSE_STC 0)
set(wxUSE_MEDIACTRL 0)
set(wxUSE_XRC 0)
set(wxUSE_XML 0)
set(wxUSE_WXHTML_HELP 0)
set(wxUSE_HTML 0)
set(wxUSE_DEBUGREPORT 0)
set(wxUSE_SOCKETS 0)
set(wxUSE_URL 0)
set(wxUSE_PROTOCOL_FTP 0)
set(wxUSE_PROTOCOL_HTTP 0)

add_subdirectory_with_folder("wxWidgets" wxWidgets)

# google tests
if( WIN32 )
    set( gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll" FORCE)
endif()

#enable_testing()
#add_subdirectory_with_folder("googletest" googletest)
