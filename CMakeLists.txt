cmake_minimum_required(VERSION 3.14.7 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(NAME wxArkanoid)

set(CMAKE_GENERATOR_PLATFORM Win32)
project(${NAME})

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Disable ZERO_CHECK target
set(CMAKE_SUPPRESS_REGENERATION true)

set(TODO ON CACHE BOOL "Enable compiletime TODO messages")

set(RESOURCE_INSTALL_DIR "" CACHE PATH "Path to install resources to (leave empty for running uninstalled)")

if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib/static")
endif()

if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
endif()

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib/${OUTPUTCONFIG} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib/${OUTPUTCONFIG} )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin/${OUTPUTCONFIG} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

if (WIN32)
    # file(GLOB_RECURSE SOURCES "src/*.h" "src/*.cpp" )
    file(GLOB SOURCE_HEADERS "src/*.h*" )
    file(GLOB SOURCE_FILES "src/*.cpp" )
    file(GLOB SOURCE_HEADERS_SHAPES "src/Shapes/*.h*" )
    file(GLOB SOURCE_FILES_SHAPES "src/Shapes/*.cpp" )
    file(GLOB SOURCE_HEADERS_CONTROL_PANEL "src/ControlPanel/*.h*" )
    file(GLOB SOURCE_FILES_CONTROL_PANEL "src/ControlPanel/*.cpp" )

    source_group( "Header Files\\Shapes" FILES ${SOURCE_HEADERS_SHAPES} )
    source_group( "Source Files\\Shapes" FILES ${SOURCE_FILES_SHAPES} )
    source_group( "Header Files\\ControlPanel" FILES ${SOURCE_HEADERS_CONTROL_PANEL} )
    source_group( "Source Files\\ControlPanel" FILES ${SOURCE_FILES_CONTROL_PANEL} )
 
    add_executable(
       ${PROJECT_NAME} 
       WIN32 
       ${SOURCE_HEADERS} 
       ${SOURCE_FILES}
       ${SOURCE_HEADERS_SHAPES}
       ${SOURCE_FILES_SHAPES}
       ${SOURCE_HEADERS_CONTROL_PANEL}
       ${SOURCE_FILES_CONTROL_PANEL}
    )
    add_subdirectory( external )
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

target_link_libraries (${PROJECT_NAME} wx::core wx::base wx::media OpenAL::OpenAL)
