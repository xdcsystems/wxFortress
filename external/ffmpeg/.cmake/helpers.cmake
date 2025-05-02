#[=============================================================================[

Вспомогательные макросы и функции


ROOT_INCLUDE_GUARD()
--------------------

Защита от многократного включения модуля,
допускающая включение только через корневой CMakeLists.txt
(напрямую или по цепочке включений через другие модули)


not_overridable_set(<var> [<value> ...])
----------------------------------------

Вариация команды 'set()'.
Устанавливает переменной '<var>' значение '<value>',
только если переменная '<var>' не была объявлена ранее.
Полезно для случаев, когда предусматривается возможность
установки значения переменной пользователем кода или CMake-ключом,
но при этом должно существовать заданное по умолчанию значение.

#]=============================================================================]

cmake_minimum_required(VERSION 3.7)

if(CMAKE_MAJOR_VERSION GREATER_EQUAL 3 AND CMAKE_MINOR_VERSION GREATER_EQUAL 10)
    include_guard(GLOBAL)
endif()

#
# ROOT_INCLUDE_GUARD
#
macro(ROOT_INCLUDE_GUARD)
    if(CMAKE_MAJOR_VERSION GREATER_EQUAL 3 AND CMAKE_MINOR_VERSION GREATER_EQUAL 10)
        include_guard()
    endif()

    get_filename_component(module_name
        "${CMAKE_CURRENT_LIST_FILE}" NAME_WE)

    if(NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
        message(FATAL_ERROR "Module '${module_name}' must be included only in root CMakeLists.txt")
    endif()

    # In CMake < 3.17 this var not defined, and condition will be just ignored
    if(CMAKE_CURRENT_FUNCTION)
        message(FATAL_ERROR "Module '${module_name}' must not be included in function body")
    endif()

    # Check that the 'project()' command already been called
    if(NOT PROJECT_SOURCE_DIR)
        message(FATAL_ERROR "Module '${_root_include_guard_module_name}' "
            "must be included after 'project()' command")
    endif()
endmacro()

#
# not_overridable_set
#
function(not_overridable_set var)
    if(NOT DEFINED ${var})
        set(${var} "${ARGN}" PARENT_SCOPE)
    endif()
endfunction()

