#[=============================================================================[

Подключает к проекту модули с обобщенными опциями сборки и
проверки корректности работы кода во время его исполнения.

#]=============================================================================]

cmake_minimum_required(VERSION 3.7)

include(helpers)
ROOT_INCLUDE_GUARD()

foreach(path ${CMAKE_PREFIX_PATH})
    if (NOT EXISTS ${path})
        message(WARNING "Path \"${path}\" does not exist")
    else()
        if (NOT IS_DIRECTORY ${path})
            message(FATAL_ERROR "Path \"${path}\" is not directory")
        endif()
        if (NOT IS_ABSOLUTE ${path})
            message(FATAL_ERROR "Path \"${path}\" must be absolute")
        endif()
    endif()
endforeach()

set(KBL_APP_PATH "/opt/kbl" CACHE PATH "Default path for application location")
mark_as_advanced(KBL_APP_PATH)

list(INSERT CMAKE_PREFIX_PATH 0 
    ${KBL_APP_PATH} 
    /opt)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set_property(CACHE CMAKE_INSTALL_PREFIX 
        PROPERTY VALUE ${KBL_APP_PATH})
endif()

include(gnu-dirs)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_INSTALL_FULL_DATAROOTDIR}/cmake_find_package
    ${KBL_APP_PATH}/${CMAKE_INSTALL_DATAROOTDIR}/cmake_find_package
    )

list(FIND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR} lib_dir_rpath_presented)
if (lib_dir_rpath_presented EQUAL -1)
    list(APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
endif()
list(FIND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBEXECDIR} lib_exec_dir_rpath_presented)
if (lib_exec_dir_rpath_presented EQUAL -1)
    list(APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBEXECDIR})
endif()
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")

if (NOT CMAKE_CONFIGURATION_TYPES)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE RelWithDebInfo)
        message(STATUS "RelWithDebInfo build configuration will be used")
    endif()
endif()

include(cxx-common-opts)
include(cxx-sanitizers)

