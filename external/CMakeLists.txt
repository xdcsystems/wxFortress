cmake_minimum_required(VERSION 3.14.7 FATAL_ERROR)

function(get_all_targets _result _dir)
    get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
    foreach(_subdir IN LISTS _subdirs)
        get_all_targets(${_result} "${_subdir}")
    endforeach()
    get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
    set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
endfunction()

function(add_subdirectory_with_folder _folder_name _folder)
    add_subdirectory(${_folder} ${ARGN})
    get_all_targets(_targets "${_folder}")
    foreach(_target IN LISTS _targets)
        set_target_properties(
            ${_target}
            PROPERTIES FOLDER "${_folder_name}"
        )
    endforeach()
endfunction()

set( wxUSE_OPENGL, 1 )

# External Libs

# glm
add_subdirectory(glm)

add_subdirectory_with_folder("glew-cmake" glew-cmake)

option(ALSOFT_UTILS "Build utility programs"  OFF)
OPTION(ALSOFT_EXAMPLES  "Build and install example programs"  OFF)

add_subdirectory_with_folder("openal-soft" openal-soft)

set(wxUSE_AUI 0)
set(wxUSE_PROPGRID 0)
set(wxUSE_QA 0)
set(wxUSE_RIBBON 0)
set(wxUSE_RICHTEXT 0)
set(wxUSE_WEBVIEW 0)
set(wxUSE_STC 0)

add_subdirectory_with_folder("wxWidgets" wxWidgets)




