#[=============================================================================[

    Набор функций, определяющих зависимости от внешних библиотек, представленных 
в виде исходных кодов из git-репозиториев или APT-пакетов дистрибутивов Linux.
Зависимости генирируются в виде набора файлов сборки для утилит make, CMake.
Также создается extern_dep.txt файл с описание зависимостей. 
    Эти функции НЕ ЗАНИМАЮТСЯ импортом внешних библиотек, они СОЗДАЮТ инстументы 
для этого.


-------------------------------

create_project_dependencies(
    CONFIG_FILE 
    DESTINATION_DIR)

На основании конфигурационного файла создает следующие файлы описания зависимостей:
    - Makefile и extern_dep.mk для получения библиотек из внешних источников, их сборки и
      установки. Они могут использоваться для локальной сборки и установки внешних библиотек
      в контексте проекта и устанавливаться во внутренний каталог проекта. В последнем 
      случае зависимости подключаются путем добавления АБСОЛЮТНОГО пути внутреннего каталога 
      к переменной CMAKE_PREFIX_PATH в командной строку утилиты CMake при сборке основного 
      проекта, поскольку находятся в нестандартных путях поиска CMake. CMAKE_PREFIX_PATH 
      переменная определяет пути поиска CMake конфигурационных файлов пакетов для функции 
      find_package. Стандартный способ вызова сборки: 
        cmake -DCMAKE_PREFIX_PATH=<АБСОЛЮТНЫЙ путь к сборке внешних библиотек> <путь к проекту>
      Например:
        cmake -DCMAKE_PREFIX_PATH=`pwd`/../.extern_lib/opt ..
      Как использовать Makefile см. файл extern_dep_makefile.mk

    - packages.cmake для включения в CMakeLists.txt проекта разработки. Включение этого 
      файла делается автоматически из вызова этой функции. Файл состоит из функций
      find_package или подобных им для поиска и подключения библиотек к проекту; 

    - ${CMAKE_SOURCE_DIR}/extern_dep.txt текстовый файл описания зависимостей и их источников получения.

Параметры:
    CONFIG_FILE
        конфигурационный файл в формате CMake с описанием зависимостей, которые
        задаются в виде вызова функций set_external_library_dependency,
        set_repository_library_dependency (описание см. ниже). На основании
        этих функций таже созданы скрипты для использования библиотек GSL и GTest.
        При необходимости их можно подключить так:
            include(gsl_dep_config)
            include(gtest_dep_config)
    DESTINATION_DIR           
        Каталог, куда будут помещены созданные файлы описания зависимостей


-------------------------------

set_external_library_dependency(
    NAME 
    [VERSION]
    DESCRIPTION 
    GIT_REPOSITORY 
    [GIT_CLONE_ARGS]
    [GIT_BRANCH]
    MAKE_FUNCTION {git_build_install | git_build}
    [CMAKE_ARGS]
    [MAKE_VARS]
    [CMAKE_FIND_PACKAGE_CALLS])

    Определяет зависимость от внешей библиотеки из git-репозитория. Библиотека должна
    подерживать систему сборки CMake, в противном случае нужно воспользоваться вызовом 
    set_custom_library_dependency (см. ниже)

Параметры:
    NAME 
        Название библиотеки

    VERSION
        Версия библиотеки

    DESCRIPTION
        Описание библиотеки

    GIT_REPOSITORY 
        git-репозиторий исходных кодов библиотеки

    GIT_CLONE_ARGS
        дополнительные аргументы вызова git clone

    GIT_BRANCH
        Ветка или тэг в git-репозитории 

    MAKE_FUNCTION 
        Имя функция утилиты make. Должна принимать значения git_build_install, git_build. 
        Используется в Makefile для получения исходных кодов внешних библиотек, их компиляции 
        и установки. Эти функции формируют содержимое файла extern_dep.mk, подключаемого внутри к
        Makefile. Как использовать см. файл extern_dep_makefile.mk

    CMAKE_ARGS
        Дополнительные значения переменных для утилиты CMake. 
        Пример: 
            CMAKE_ARGS -DSANITIZE_ADDRESS=ON CMAKE_ARGS -DSANITIZE_UNDEFINED=ON

    MAKE_VARS
        Дополнительные значения переменных для утилиты make. 
        Пример: 
            MAKE_VARS VERBOSE=1

    CMAKE_FIND_PACKAGE_CALLS
        Список команд системы сборки CMake. Если не указан, используется
        find_package. Эти функции формируют содержимое файла packages.cmake.

-------------------------------

set_repository_library_dependency(
    NAME 
    [VERSION]
    DESCRIPTION 
    PACKAGE
    [CMAKE_FIND_PACKAGE_CALLS])

    Определяет зависимость от внешей библиотеки из APT-репозитория Linux дистрибутива

Параметры:
    NAME 
        Название библиотеки

    VERSION 
        Версия библиотеки

    DESCRIPTION 
        Описание библиотеки

    PACKAGE
        Название APT-пакета/ов в Linux-репозитрии. Будет установлен через вызов 
        apt-get install PACKAGE

    CMAKE_FIND_PACKAGE_CALLS
        Список команд системы сборки CMake. Если не указан, используется
        find_package. Эти функции формируют содержимое файла packages.cmake.


-------------------------------

set_custom_library_dependency(
    NAME 
    [VERSION]
    DESCRIPTION 
    GIT_REPOSITORY 
    [GIT_CLONE_ARGS]
    [GIT_BRANCH]
    [REPOSITORY_PACKAGES]
    BUILD_COMMANDS 
    INSTALL_COMMANDS
    CMAKE_FIND_PACKAGE_CALLS)

    Определяет зависимость от внешей библиотеки из git-репозитория, если система сборки
    библиотеки не использует CMake или требуется специфическая конфигурация сборки.

Параметры:
    NAME 
        Название библиотеки

    VERSION
        Версия библиотеки

    DESCRIPTION
        Описание библиотеки

    GIT_REPOSITORY 
        git-репозиторий исходных кодов библиотеки

    GIT_CLONE_ARGS
        дополнительные аргументы вызова git clone

    GIT_BRANCH
        Ветка или тэг в git-репозитории 

    REPOSITORY_PACKAGES
        Список библиотек из APT репозитория дистрибутива Linux от которых зависит данная библиотека.
        Будут установлены через вызов apt-get install до сборки библиотеки

    BUILD_COMMANDS 
        Список комманд сборки, которые будут запущены из оболочки shell

    INSTALL_COMMANDS 
        Список комманд инсталляции, которые будут запущены из оболочки shell

    CMAKE_FIND_PACKAGE_CALLS
        Список команд системы сборки CMake. Если не указан, используется
        find_package. Эти функции формируют содержимое файла packages.cmake.

    Пример:
        set_custom_external_library_dependency(
            NAME aravis
            VERSION 0.8
            DESCRIPTION "Библиотека Aravis для работы с камерами по протоколу GenICam"
            GIT_REPOSITORY https://github.com/AravisProject/aravis.git
            GIT_BRANCH 0.8.19
            REPOSITORY_PACKAGES "libxml2-dev; libglib2.0-dev; meson; ninja-build"
            BUILD_COMMANDS "meson build; cd build; ninja"
            INSTALL_COMMANDS "cd build; ninja install"
            CMAKE_FIND_PACKAGE_CALLS
                "find_package(PkgConfig REQUIRED)"
                "set(ENV{PKG_CONFIG_PATH} $ENV{PKG_CONFIG_PATH}:/usr/local/lib64/pkgconfig)"
                "pkg_search_module(Aravis REQUIRED IMPORTED_TARGET aravis-0.8)"
            )


#]=============================================================================]

cmake_minimum_required(VERSION 3.7)

if(CMAKE_MAJOR_VERSION GREATER_EQUAL 3 AND CMAKE_MINOR_VERSION GREATER_EQUAL 10)
    include_guard(GLOBAL)
endif()

#
# Global variables
#
set(_CMAKE_MODULES_DIR ${CMAKE_CURRENT_LIST_DIR})

set_property(GLOBAL PROPERTY _PRJ_DESCR "")
set_property(GLOBAL PROPERTY _LIB_DOC_CONTENT "")
set_property(GLOBAL PROPERTY _LIB_MAKEFILE_CONTENT "")
set_property(GLOBAL PROPERTY _CMAKE_FIND_PACKAGE_CALLS "")

set(GENERATE_EXTERN_DEP_REFS)

#
# define_project_description 
#
macro(define_project_description text)
    set_property(GLOBAL PROPERTY _PRJ_DESCR "${text}")

    set(PROJECT_DESCRIPTION "${text}" CACHE STRING "Project description")
    set(PROJECT_DESCRIPTION "${text}" PARENT_SCOPE)
endmacro()

#
# set_external_library_dependency
#
function(set_external_library_dependency)
    set(options)
    set(one_value_args 
        NAME 
        VERSION 
        DESCRIPTION 
        GIT_REPOSITORY 
        GIT_BRANCH
        MAKE_FUNCTION 
        )
    set(multi_value_args 
        GIT_CLONE_ARGS
        CMAKE_FIND_PACKAGE_CALLS
        CMAKE_ARGS 
        MAKE_VARS
        )

    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}"
        ${ARGN})

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Function 'set_external_library_dependency' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if(NOT ARG_NAME)
        message(FATAL_ERROR "The NAME argument must be defined")
    endif()
    if(NOT ARG_DESCRIPTION)
        message(FATAL_ERROR "The DESCRIPTION argument must be defined")
    endif()
    if(NOT ARG_GIT_REPOSITORY)
        message(FATAL_ERROR "The GIT_REPOSITORY argument must be defined")
    endif()
    
    if(NOT ARG_MAKE_FUNCTION)
        message(FATAL_ERROR "The MAKE_FUNCTION argument must be defined")
    endif()
    if(NOT (${ARG_MAKE_FUNCTION} STREQUAL "git_build_install") 
       AND NOT (${ARG_MAKE_FUNCTION} STREQUAL "git_build"))
        message(FATAL_ERROR "The MAKE_FUNCTION argument must be \"git_build_install\" or \"git_build\"")
    endif()

    set(text)
    string(APPEND text 
        "- ${ARG_NAME}: ${ARG_DESCRIPTION}\n"
        "\tверсия: ${ARG_VERSION}\n" 
        "\tисточник: git clone ${ARG_GIT_REPOSITORY}"
        )
    if(ARG_GIT_BRANCH)
        string(APPEND text " -b ${ARG_GIT_BRANCH}")
    endif()

    set(git_clone_args)
    foreach(arg ${ARG_GIT_CLONE_ARGS})
        string(STRIP ${arg} arg)
        string(APPEND git_clone_args "${arg} ")
        string(APPEND text " ${arg}")
    endforeach()
    string(APPEND text "\n")

    string(APPEND text "\n")

    if(ARG_CMAKE_ARGS)
        string(APPEND text "\tсборка: cmake ")
        foreach(arg ${ARG_CMAKE_ARGS})
            string(APPEND text "${arg} ")
        endforeach()
        string(APPEND text ".\n")
    endif()
    string(APPEND text "\n")

    get_property(_LIB_DOC_CONTENT GLOBAL PROPERTY _LIB_DOC_CONTENT)
    set_property(GLOBAL PROPERTY _LIB_DOC_CONTENT "${_LIB_DOC_CONTENT}${text}")

    foreach(mvar ${ARG_MAKE_VARS})
        string(APPEND make_vars ${mvar} "\n")
    endforeach()
	if(make_vars)
    	string(APPEND _LIB_MAKEFILE_CONTENT "${make_vars}\n")
	endif()	

    foreach(carg ${ARG_CMAKE_ARGS})
        string(APPEND cmake_args ${carg} " ")
    endforeach()
    get_property(_LIB_MAKEFILE_CONTENT GLOBAL PROPERTY _LIB_MAKEFILE_CONTENT)
    string(REPLACE "#" "\\\#" git_branch "${ARG_GIT_BRANCH}")

    string(APPEND _LIB_MAKEFILE_CONTENT 
        "$(call ${ARG_MAKE_FUNCTION}, \\\n"
        "\t${ARG_NAME}, ${ARG_VERSION}, \\\n"
        "\t${ARG_GIT_REPOSITORY}, ${git_clone_args}, ${git_branch}, \\\n"
        "\t${cmake_args})\n\n"
        )
    set_property(GLOBAL PROPERTY _LIB_MAKEFILE_CONTENT "${_LIB_MAKEFILE_CONTENT}")

    get_property(_CMAKE_FIND_PACKAGE_CALLS GLOBAL PROPERTY _CMAKE_FIND_PACKAGE_CALLS)
    if(NOT ARG_CMAKE_FIND_PACKAGE_CALLS)
       string(APPEND _CMAKE_FIND_PACKAGE_CALLS 
            "find_package(${ARG_NAME} ${ARG_VERSION} REQUIRED)\n")
    else()
        foreach(cmd ${ARG_CMAKE_FIND_PACKAGE_CALLS})
           string(APPEND _CMAKE_FIND_PACKAGE_CALLS 
                "${cmd}\n")
        endforeach()
    endif()
    save_dependency_package_descriptor(PACKAGE ${ARG_NAME} VERSION ${ARG_VERSION}) 

    set_property(GLOBAL PROPERTY _CMAKE_FIND_PACKAGE_CALLS "${_CMAKE_FIND_PACKAGE_CALLS}")
endfunction()

#
# set_repository_library_dependency
#
function(set_repository_library_dependency)
    set(options)
    set(one_value_args 
        NAME 
        VERSION 
        DESCRIPTION 
        PACKAGE
        )
    set(multi_value_args 
        CMAKE_FIND_PACKAGE_CALLS
        CMAKE_ARGS
        )

    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}" 
        ${ARGN})

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Function 'set_repository_library_dependency' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if(NOT ARG_NAME)
        message(FATAL_ERROR "The NAME argument must be defined")
    endif()
    if(NOT ARG_DESCRIPTION)
        message(FATAL_ERROR "The DESCRIPTION argument must be defined")
    endif()
    if(NOT ARG_PACKAGE)
        message(FATAL_ERROR "The PACKAGE argument must be defined")
    endif()

    set(text)
    string(APPEND text 
        "- ${ARG_NAME}: ${ARG_DESCRIPTION}\n")
    if(ARG_VERSION)
        string(APPEND text "\tверсия: ${ARG_VERSION}\n")
    endif()
    string(APPEND text 
        "\tAPT пакеты: ${ARG_PACKAGE}\n"
        "\tустановка: sudo apt-get install ${ARG_PACKAGE}\n\n\n"
        )
    get_property(_LIB_DOC_CONTENT GLOBAL PROPERTY _LIB_DOC_CONTENT)
    set_property(GLOBAL PROPERTY _LIB_DOC_CONTENT "${_LIB_DOC_CONTENT}${text}")

    get_property(_LIB_MAKEFILE_CONTENT GLOBAL PROPERTY _LIB_MAKEFILE_CONTENT)
    string(APPEND _LIB_MAKEFILE_CONTENT 
        "$(call apt_packages, ${ARG_PACKAGE})\n\n")
    set_property(GLOBAL PROPERTY _LIB_MAKEFILE_CONTENT "${_LIB_MAKEFILE_CONTENT}")

    get_property(_CMAKE_FIND_PACKAGE_CALLS GLOBAL PROPERTY _CMAKE_FIND_PACKAGE_CALLS)

    if(NOT ARG_CMAKE_FIND_PACKAGE_CALLS)
        string(APPEND _CMAKE_FIND_PACKAGE_CALLS "find_package(${ARG_NAME}")
        if (ARG_VERSION)
           string(APPEND _CMAKE_FIND_PACKAGE_CALLS "${ARG_VERSION}")
        endif()
        string(APPEND _CMAKE_FIND_PACKAGE_CALLS " REQUIRED)\n")
    else()
        foreach(cmd ${ARG_CMAKE_FIND_PACKAGE_CALLS})
        string(APPEND _CMAKE_FIND_PACKAGE_CALLS 
            "${cmd}\n")
        endforeach()
    endif()
    save_dependency_package_descriptor(PACKAGE ${ARG_NAME} VERSION ${ARG_VERSION}) 

    set_property(GLOBAL PROPERTY _CMAKE_FIND_PACKAGE_CALLS "${_CMAKE_FIND_PACKAGE_CALLS}") 
endfunction()

#
# set_custom_external_library_dependency
#
function(set_custom_external_library_dependency)
    set(options)
    set(one_value_args 
        NAME 
        VERSION 
        DESCRIPTION 
        GIT_REPOSITORY 
        GIT_BRANCH
        )
    set(multi_value_args 
        GIT_CLONE_ARGS
        REPOSITORY_PACKAGES
        BUILD_COMMANDS 
        INSTALL_COMMANDS
        CMAKE_FIND_PACKAGE_CALLS
        )

    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}"
        ${ARGN})

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Function 'set_custom_external_library_dependency' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if(NOT ARG_NAME)
        message(FATAL_ERROR "The NAME argument must be defined")
    endif()
    if(NOT ARG_DESCRIPTION)
        message(FATAL_ERROR "The DESCRIPTION argument must be defined")
    endif()
    if(NOT ARG_GIT_REPOSITORY)
        message(FATAL_ERROR "The GIT_REPOSITORY argument must be defined")
    endif()
    
    if(NOT ARG_BUILD_COMMANDS)
        message(FATAL_ERROR "The BUILD_COMMANDS argument must be defined")
    endif()
    if(NOT ARG_INSTALL_COMMANDS)
        message(FATAL_ERROR "The INSTALL_COMMANDS argument must be defined")
    endif()

    set(text)
    string(APPEND text 
        "- ${ARG_NAME}: ${ARG_DESCRIPTION}\n"
        "\tверсия: ${ARG_VERSION}\n" 
        "\tисточник: git clone ${ARG_GIT_REPOSITORY}"
        )
    if(ARG_GIT_BRANCH)
        string(APPEND text " -b ${ARG_GIT_BRANCH}")
    endif()

    set(git_clone_args)
    foreach(arg ${ARG_GIT_CLONE_ARGS})
        string(STRIP ${arg} arg)
        string(APPEND git_clone_args "${arg} ")
        string(APPEND text " ${arg}")
    endforeach()
    string(APPEND text "\n")

    string(APPEND text "\tсборка:\n")
    foreach(cmd ${ARG_BUILD_COMMANDS})
        string(STRIP ${cmd} cmd)
        string(APPEND text "\t\t${cmd}\n")
    endforeach()

    string(APPEND text "\tустановка:\n")
    foreach(cmd ${ARG_INSTALL_COMMANDS})
        string(STRIP ${cmd} cmd)
        string(APPEND text "\t\t${cmd}\n")
    endforeach()

    if(ARG_REPOSITORY_PACKAGES)
        string(APPEND text "\tAPT пакеты от которых зависит:\n")
        foreach(pkg ${ARG_REPOSITORY_PACKAGES})
            string(STRIP ${pkg} pkg)
            string(APPEND text "\t\t${pkg}\n")
        endforeach()
    endif()
    string(APPEND text "\n\n")

    get_property(_LIB_DOC_CONTENT GLOBAL PROPERTY _LIB_DOC_CONTENT)
    set_property(GLOBAL PROPERTY _LIB_DOC_CONTENT "${_LIB_DOC_CONTENT}${text}")

    get_property(_LIB_MAKEFILE_CONTENT GLOBAL PROPERTY _LIB_MAKEFILE_CONTENT)

    foreach(pkg ${ARG_REPOSITORY_PACKAGES})
        string(STRIP ${pkg} pkg)
        string(APPEND _LIB_MAKEFILE_CONTENT 
            "$(call apt_packages, ${pkg})\n\n")
    endforeach()

    string(REPLACE "#" "\\\#" git_branch "${ARG_GIT_BRANCH}")

    escape_chars_for_make(build_commands "${ARG_BUILD_COMMANDS}")
    escape_chars_for_make(install_commands "${ARG_INSTALL_COMMANDS}")

    string(APPEND _LIB_MAKEFILE_CONTENT 
        "$(call git_custom_build_install, \\\n"
        "\t${ARG_NAME}, ${ARG_VERSION}, \\\n"
        "\t${ARG_GIT_REPOSITORY}, ${git_clone_args}, ${git_branch}, \\\n"
        "\t'${build_commands}', \\\n"
        "\t'${install_commands}')\n\n"
        )

    set_property(GLOBAL PROPERTY _LIB_MAKEFILE_CONTENT "${_LIB_MAKEFILE_CONTENT}")

    get_property(_CMAKE_FIND_PACKAGE_CALLS GLOBAL PROPERTY _CMAKE_FIND_PACKAGE_CALLS)
    if(NOT ARG_CMAKE_FIND_PACKAGE_CALLS)
       string(APPEND _CMAKE_FIND_PACKAGE_CALLS 
            "find_package(${ARG_NAME} ${ARG_VERSION} REQUIRED)\n")
    else()
        foreach(cmd ${ARG_CMAKE_FIND_PACKAGE_CALLS})
            string(STRIP ${cmd} cmd)
            string(APPEND _CMAKE_FIND_PACKAGE_CALLS "${cmd}\n")
        endforeach()
    endif()
    save_dependency_package_descriptor(PACKAGE ${ARG_NAME} VERSION ${ARG_VERSION}) 

    set_property(GLOBAL PROPERTY _CMAKE_FIND_PACKAGE_CALLS "${_CMAKE_FIND_PACKAGE_CALLS}")
endfunction()

#
# create_project_dependencies
#
macro(create_project_dependencies 
    config_file_param 
    config_file_value 
    destination_dir_param 
    destination_dir_value
    )
    generate_project_dependencies(
        ${config_file_param} ${config_file_value} 
        ${destination_dir_param} ${destination_dir_value}
        )

    if(NOT CMAKE_SCRIPT_MODE_FILE)
        set(extern_lib_repository ${CMAKE_SOURCE_DIR}/.extern_lib/opt)
        if(IS_DIRECTORY ${extern_lib_repository})
            list(INSERT CMAKE_PREFIX_PATH 0 ${extern_lib_repository})
            list(INSERT CMAKE_MODULE_PATH 0 "${extern_lib_repository}/${CMAKE_INSTALL_DATAROOTDIR}/cmake_find_package")
        endif()

        set(cmake_pkg_list_file "${destination_dir_value}/packages.cmake")
        include(${cmake_pkg_list_file})
    endif()
endmacro()

#
# __create_project_dependencies
#
function(generate_project_dependencies)
    set(options)
    set(one_value_args CONFIG_FILE DESTINATION_DIR)
    set(multi_value_args)

    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}"
        ${ARGN})

    if(ARG_CONFIG_FILE)
        if(NOT IS_ABSOLUTE "${ARG_CONFIG_FILE}")
            set(ARG_CONFIG_FILE ${CMAKE_SOURCE_DIR}/${ARG_CONFIG_FILE})
        endif()
    else()
        message(FATAL_ERROR "The argument CONFIG_FILE is empty")
    endif()

    if(ARG_DESTINATION_DIR)
        if(NOT IS_ABSOLUTE "${ARG_DESTINATION_DIR}")
            set(ARG_DESTINATION_DIR ${CMAKE_SOURCE_DIR}/${ARG_DESTINATION_DIR})
        endif()
    else()
        message(FATAL_ERROR "The argument DESTINATION_DIR is empty")
    endif()

    # Пока всегда регенерируется, поскольку сложно учесть все изменения в файлах скриптов.
    set(GENERATE_EXTERN_DEP_REFS 1)

    set(cmake_pkg_list_file ${ARG_DESTINATION_DIR}/packages.cmake)
    if (${ARG_CONFIG_FILE} IS_NEWER_THAN ${cmake_pkg_list_file})
        set(GENERATE_EXTERN_DEP_REFS 1)
    endif()

    include(${ARG_CONFIG_FILE})

    file(MAKE_DIRECTORY ${ARG_DESTINATION_DIR})

    set(makefile ${ARG_DESTINATION_DIR}/Makefile)
    if(NOT EXISTS ${makefile} OR GENERATE_EXTERN_DEP_REFS)
        configure_file(
            ${_CMAKE_MODULES_DIR}/extern_dep_makefile.mk 
            ${makefile}
            COPYONLY)
    endif()

    set(sub_makefile ${ARG_DESTINATION_DIR}/extern_dep.mk)
    set(document_file ${ARG_DESTINATION_DIR}/extern_dep.txt)

    if((NOT EXISTS ${cmake_pkg_list_file} 
         OR NOT EXISTS ${sub_makefile} 
         OR NOT EXISTS ${document_file}) 
        OR GENERATE_EXTERN_DEP_REFS)
        create_project_dependency_cmake_file(${cmake_pkg_list_file})
        create_project_dependency_makefile(${sub_makefile})
        create_project_dependency_doc(${document_file})
    endif()

    unset(GENERATE_EXTERN_DEP_REFS CACHE)
endfunction()

#
# create_project_dependency_doc
#
function(create_project_dependency_doc 
        doc_path)
    if(NOT doc_path)
        return()
    endif()

    get_property(_PRJ_DESCR GLOBAL PROPERTY _PRJ_DESCR)
    if (_PRJ_DESCR)
        set(text "\n\t${_PRJ_DESCR}\n\n")
    elseif(CMAKE_PROJECT_DESCRIPTION)
        set(text ${CMAKE_PROJECT_DESCRIPTIONdt})
    endif() 

    get_property(_LIB_DOC_CONTENT GLOBAL PROPERTY _LIB_DOC_CONTENT)
    if(_LIB_DOC_CONTENT)
        string(APPEND text "Внешние библиотеки:\n\n${_LIB_DOC_CONTENT}")
    endif()

    file(WRITE ${doc_path} "${text}")

    message(STATUS "Generated external dependency reference document: ${doc_path}")
endfunction()

#
# create_project_dependency_makefile
#
function(create_project_dependency_makefile 
        file_path)
    if(NOT file_path)
        return()
    endif()

    get_property(_LIB_MAKEFILE_CONTENT GLOBAL PROPERTY _LIB_MAKEFILE_CONTENT)

    file(WRITE ${file_path} "${_LIB_MAKEFILE_CONTENT}")

    message(STATUS "Generated Makefile for build of external dependencies: ${file_path}")
endfunction()

#
# create_project_dependency_cmake_file
#
function(create_project_dependency_cmake_file 
        file_path)
    if(NOT file_path)
        return()
    endif()

    get_property(_CMAKE_FIND_PACKAGE_CALLS GLOBAL PROPERTY _CMAKE_FIND_PACKAGE_CALLS)

    file(WRITE ${file_path} "${_CMAKE_FIND_PACKAGE_CALLS}")

    message(STATUS "Generated CMake find package file: ${file_path}")
endfunction()

function(get_dependency_package_descriptor 
        var)
    set(options)
    set(one_value_args 
        PACKAGE
        )
    set(multi_value_args)

    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}" 
        ${ARGN})

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "The function 'get_dependency_package_descriptor' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if(NOT ARG_PACKAGE)
        message(FATAL_ERROR "The PACKAGE argument must be defined")
    endif()

    get_property(prop 
        GLOBAL PROPERTY "${ARG_PACKAGE}_PACKAGE_DESCRIPTOR")
    if(NOT prop)
        message(FATAL_ERROR "The package dependency ${ARG_PACKAGE} is not registered")
    endif()
    set(${var} ${prop} PARENT_SCOPE)
endfunction()

function(save_dependency_package_descriptor)
    set(options)
    set(one_value_args 
        PACKAGE
        VERSION 
        )
    set(multi_value_args)

    cmake_parse_arguments(ARG
        "${options}" "${one_value_args}" "${multi_value_args}" 
        ${ARGN})

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "The function 'save_dependency_package_descriptor' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if(NOT ARG_PACKAGE)
        message(FATAL_ERROR "The PACKAGE argument must be defined")
    endif()

    set_property(GLOBAL PROPERTY 
        "${ARG_PACKAGE}_PACKAGE_DESCRIPTOR" "${ARG_PACKAGE} ${ARG_VERSION}")
endfunction()

function(escape_chars_for_make var text)
    string(REPLACE "\$" "$$$$" text "${text}")
    string(REPLACE "'" '"'"' text "${text}")
    string(REPLACE "," "\\," text "${text}")
    set(${var} ${text} PARENT_SCOPE) 
endfunction()
