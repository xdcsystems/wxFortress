#[=============================================================================[

Функции для добавления тестов в систему тестирования CTest


add_test_directories(
    [DIRS <dir> ...])
-------------------------------

Добавляет каталоги тестов для cборки

Параметры:
    DIRS <dir> ...
        Список директорий, реализующих тесты.
        Если не указан, будет использован каталог test из корня проекта



add_auto_test(
    [NAME <test_name>]
    TARGET <target>
    DESCRIPTION <description>
    [TYPE <type>]
    [COMMAND <command> [<arg> ...]]
    [CONFIGURATIONS <config> ...]
    [WORKING_DIRECTORY <dir>])
-----------------------------------

Добавляет автоматический тест в систему тестирования CTest.
Автоматический тест не предполагает ручного вмешательства в работу теста,
является самопроверяющимся. Выполняемый файл теста будет расположен в каталоге
'${CMAKE_BINARY_DIR}/test/auto'

Параметры:
    NAME, COMMAND, CONFIGURATIONS, WORKING_DIRECTORY
        Имеют тот же смысл как в функции 'add_test()'
        (см. https://cmake.org/cmake/help/v3.5/command/add_test.html)
    TARGET <target>
        Имя цели, реализующей тест
    DESCRIPTION <description>
        Описание, что делает тест
    TYPE <type>
        Тип автоматического теста.
        Параметр '<type>' должен принимать значения
            'UnitTest' - модульный тест (xUnit-тест)
            'UserTest' - пользовательский тест
        Если не указан, принимает значение UnitTest.


add_manual_test(TARGET <target> DESCRIPTION <description>)
----------------------------------------------------------

Добавляет ручной тест в систему тестирования CTest.
Предполагается ручное вмешательство в работу теста
со стороны пользователя или исполняющей системы.
Выполняемый файл теста будет расположен в каталоге
'${CMAKE_BINARY_DIR}/test/manual'

Параметры:
    TARGET <target>
        Имя цели, реализующей тест
    DESCRIPTION <description>
        Описание, что делает тест


store_test_list()
-----------------

Сохраняет описание тестов в файлах.
Для автоматических тестов в файле '${CMAKE_BINARY_DIR}/test/auto/_all_test_description.txt'
Для ручных тестов в файле '${CMAKE_BINARY_DIR}/test/manual/_all_test_description.txt'
Вызов данной функции необходимо располагать ТОЛЬКО в корневом файле сборки проекта CMakeLists.txt.

#]=============================================================================]

cmake_minimum_required(VERSION 3.7)

include(helpers)
ROOT_INCLUDE_GUARD()

include(CTest)
enable_testing()

set(_AUTO_TEST_TRAIT_LIST "" CACHE INTERNAL "")
set(_MANUAL_TEST_TRAIT_LIST "" CACHE INTERNAL "")

set(TEST_BINARY_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test")
set(AUTO_TEST_BINARY_DIR "${TEST_BINARY_DIR}/auto")
set(MANUAL_TEST_BINARY_DIR "${TEST_BINARY_DIR}/manual")

#
# add_test_option
#
function(add_test_directories)
    if (NOT BUILD_TESTING)
        return()
    endif()

    set(one_value_args)
    set(multi_value_args DIRS)
    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}"
        ${ARGN})
    if (NOT ARG_DIRS)
        set(ARG_DIRS "test")
    endif()

    foreach(test_dir ${ARG_DIRS})
        add_subdirectory(${test_dir})
    endforeach()
endfunction()

#
# add_auto_test
#
function(add_auto_test)
    if (NOT BUILD_TESTING)
        return()
    endif()

    if(NOT AUTO_TEST_BINARY_DIR)
        return()
    endif()

    set(options)
    set(oneValueArgs NAME TARGET TYPE DESCRIPTION WORKING_DIRECTORY)
    set(multiValueArgs COMMAND CONFIGURATIONS)
    cmake_parse_arguments(ARG
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
        ${ARGN})
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Function 'add_auto_test()' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if (NOT ARG_TARGET)
        message(FATAL_ERROR "The target name is not defined")
    endif()
    if (NOT ARG_DESCRIPTION)
        message(FATAL_ERROR "The test description mast be defined")
    endif()
    if (NOT ARG_NAME)
        set(ARG_NAME ${ARG_TARGET})
    endif()
    if (NOT ARG_COMMAND)
        set(ARG_COMMAND ${ARG_TARGET})
    endif()

    string(TOUPPER "${ARG_TYPE}" ARG_TYPE)

    if ((ARG_TYPE STREQUAL "UNITTEST") OR (NOT ARG_TYPE))
        set(test_type "Unit test")
    elseif(ARG_TYPE STREQUAL "USERTEST")
        set(test_type "User test")
    else()
        message(FATAL_ERROR "An invalid test type ${ARG_TYPE}")
    endif()

    set_target_properties(${ARG_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${AUTO_TEST_BINARY_DIR})

    get_target_property(src_path ${ARG_TARGET} SOURCE_DIR)

    set(test_traits ${ARG_TARGET} "${ARG_DESCRIPTION}" ${test_type} ${src_path})
    list(APPEND _AUTO_TEST_TRAIT_LIST ${test_traits})
    set(_AUTO_TEST_TRAIT_LIST ${_AUTO_TEST_TRAIT_LIST} CACHE INTERNAL "")

    add_test(NAME ${ARG_NAME}
        COMMAND ${ARG_COMMAND}
        CONFIGURATIONS ${ARG_CONFIGURATIONS}
        WORKING_DIRECTORY ${ARG_WORKING_DIRECTORY}
    )

    set_tests_properties(${ARG_NAME} PROPERTIES
        LABELS ${ARG_DESCRIPTION})
endfunction()

#
# add_manual_test
#
function(add_manual_test)
    if (NOT BUILD_TESTING)
        return()
    endif()

    if(NOT MANUAL_TEST_BINARY_DIR)
        return()
    endif()

    set(options)
    set(oneValueArgs TARGET DESCRIPTION)
    set(multiValueArgs)
    cmake_parse_arguments(ARG
        "${options}" "${oneValueArgs}" "${multiValueArgs}"
        ${ARGN})
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Function 'add_manual_test()' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if (NOT ARG_TARGET)
        message(FATAL_ERROR "The target name is not defined")
    endif()
    if (NOT ARG_DESCRIPTION)
        message(FATAL_ERROR "The test description mast be defined")
    endif()

    set_target_properties(${ARG_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${MANUAL_TEST_BINARY_DIR})

    get_target_property(src_path ${ARG_TARGET} SOURCE_DIR)

    set(test_traits ${ARG_TARGET} "${ARG_DESCRIPTION}" ${src_path})
    list(APPEND _MANUAL_TEST_TRAIT_LIST ${test_traits})
    set(_MANUAL_TEST_TRAIT_LIST ${_MANUAL_TEST_TRAIT_LIST} CACHE INTERNAL "")
endfunction()

#
# store_auto_test_list
#
function(store_auto_test_list)
    if((NOT AUTO_TEST_BINARY_DIR) OR (NOT _AUTO_TEST_TRAIT_LIST))
        return()
    endif()
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
        message(FATAL_ERROR "Function 'store_auto_test_list()' "
            "may be called only via root CMakeLists.txt")
    endif()

    set(test_trait_list_file "${AUTO_TEST_BINARY_DIR}/_all_test_description.txt")

    file(REMOVE ${test_trait_list_file})
    file(APPEND ${test_trait_list_file}
        "          Auto tests:\n         -------------\n\n")

    list(LENGTH _AUTO_TEST_TRAIT_LIST list_size)
    math(EXPR list_size "${list_size} - 1")

    foreach (i RANGE 0 ${list_size} 4)
        math(EXPR item1 ${i})
        math(EXPR item2 "${i} + 1")
        math(EXPR item3 "${i} + 2")
        math(EXPR item4 "${i} + 3")

        list(GET _AUTO_TEST_TRAIT_LIST ${item1} ${item2} ${item3} ${item4} test_traits)
        list(GET test_traits 0 test_name)
        list(GET test_traits 1 test_descr)
        list(GET test_traits 2 test_type)
        list(GET test_traits 3 test_path)
        string(CONCAT test_section
            "Description: ${test_descr}\n"
            "Name: ${test_name}\n"
            "Type: ${test_type}, auto\n"
            "Source path: ${test_path}\n")

        file(APPEND ${test_trait_list_file} "${test_section}\n")
    endforeach()
endfunction()

#
# store_manual_test_list
#
function(store_manual_test_list)
    if((NOT MANUAL_TEST_BINARY_DIR) OR (NOT _MANUAL_TEST_TRAIT_LIST))
        return()
    endif()
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
        message(FATAL_ERROR "Function 'store_manual_test_list()' "
            "may be called only via root CMakeLists.txt")
    endif()

    set(test_trait_list_file "${MANUAL_TEST_BINARY_DIR}/_all_test_description.txt")

    file(REMOVE ${test_trait_list_file})
    file(APPEND ${test_trait_list_file}
        "          Manual tests:\n         ---------------\n\n")

    list(LENGTH _MANUAL_TEST_TRAIT_LIST list_size)
    math(EXPR list_size "${list_size} - 1")

    foreach (i RANGE 0 ${list_size} 3)
        math(EXPR item1 ${i})
        math(EXPR item2 "${i} + 1")
        math(EXPR item3 "${i} + 2")

        list(GET _MANUAL_TEST_TRAIT_LIST ${item1} ${item2} ${item3} test_traits)
        list(GET test_traits 0 test_name)
        list(GET test_traits 1 test_descr)
        list(GET test_traits 2 test_path)
        string(CONCAT test_section
            "Description: ${test_descr}\n"
            "Name: ${test_name}\n"
            "Source path: ${test_path}\n")

        file(APPEND ${test_trait_list_file} "${test_section}\n")
    endforeach()
endfunction()

#
# store_test_list
#
function(store_test_list)
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
        message(FATAL_ERROR "Function 'store_test_list()' "
            "may be called only via root CMakeLists.txt")
    endif()

    store_auto_test_list()
    store_manual_test_list()
endfunction()

