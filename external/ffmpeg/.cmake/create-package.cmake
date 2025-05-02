#[=============================================================================[

Функции для настройки правил инсталляции, экспорта и сборки пакетов


Начало формирования инсталляционного пакета
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

begin_cmake_package(
    [PACKAGE <name>]
    [VERSION <major>[.<minor>[.<patch>[.<tweak>]]]])
-----------------------------------------------------

Объявление начала формирования инсталляционнного CMake-пакета. Далее при использовании
функций add_targets_to_cmake_package происходит наполение пакета CMake-целями (см. ниже).
Последующий вызов end_cmake_package() заканчивает формирование пакета (см. ниже).

Параметры:
    PACKAGE <name>
        Название CMake-пакета, который в последствии при подключении пакета во внешний проект 
        будет указаваться в 'find_package(<name> ...)'.
        Если <name> не задан, используется значение переменной 'PROJECT_NAME'

    VERSION <major>[.<minor>[.<patch>[.<tweak>]]]
        Версия CMake-пакета в 'find_package(... <version>)'
        Если не указано, используется значение переменной 'PROJECT_VERSION'


Окончание формирования инсталляционного пакета
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

end_cmake_package(
    [PACKAGE <name>]
    [PACKAGE_DEPENDENCIES <dependency> ...]
    [EXPORT_TO_REGISTERY])
------------------------------------

Заканчивает формирование cmake-пакета:
    - сформирует базовые конфигурационные файлы пакета:
        <package-name>Config.cmake
        <package-name>Targets.cmake
        <package-name>VersionConfig.cmake,
      а также инсталяционные конфигурационные файлы для инсталлируемых CMake-целей.
      Эти файлы будут экпортированы при установке пакета, а также могут быть 
      использованы как up-stream пакет для внешних проектов;
      
    - при включенном параметре EXPORT_TO_REGISTERY или глобальной опции сборки EXPORT_TO_REGISTERY
      выполнит регистрацию пакета в локальном репозитрии пакетов CMake;     

    - добавит в сборку проекта make-цель 'install', которая при
        вызове из командной строки команды "make install" установит пакет 
        в каталоги на основе правил инсталляции GNU;

    - добавит в проект make-цель 'uninstall', которая при
        вызове из командной строки команды "make uninstall" деинсталлирует пакет 
        из каталогов установки;

Параметры:
    PACKAGE <name>
        Название CMake-пакета, с которым начали работать через вызов begin_cmake_package

    DEPENDENCIES <dependency> ...
        CMake-пакеты от которых зависит данный пакет. Публичные зависимости.
        Здесь нужно указать пакеты, без нахождения которых невозможно использование
        целей входящих в создаваемый пакет. При подключении к downstream-проекту
        они будут переданы в вызов команды 'find_package()' с нужными параметрами.
        Можно указать версию, компоненты, опциональные компоненты и т.д.
        Параметры 'QUIET' и 'REQUIRED' не указывать,
        они определяются на стороне downstream-проекта. См. пример ниже

    EXPORT_TO_REGISTERY
        Выполнить экспорт пакета в локальный репозиторий cmake. По умолчанию выключено.
        Это обычно каталог ~/.cmake/packages/<имя пакета>


Добавление CMake-целей в инсталляционный пакет
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

add_targets_to_cmake_package(
    PACKAGE <name>
    [COMPONENT <component>]
    EXPORT <export_name>
    TARGETS <target> ...
    [NAMESPACE <namespace>]
    [RELATIVE_INCLUDE_DIR <dir>])
-------------------------------------------------

Параметры:
    PACKAGE <name>

    TARGETS <target>...
        CMake-цели, входящие в пакет. Начиная с версии 3.13 CMake есть возможность
        указывать цели созданные в разных каталогах

    COMPONENT <component>
        Передается как параметр 'COMPONENT' в 'install()' (см. в документации CMake)

    EXPORT <export_name>
        Задает группу для экспорта целей. Передается как параметр 'EXPORT' в 'install()'
        (см. в документации CMake)

    NAMESPACE <namespace>
        Передается как параметр 'NAMESPACE' в 'install()' (см. в документации CMake)
        Если не указано, примет значение '<package-name>::'
        Для целей водящих в пакет создаются алиасы с именем '<namespace><target>',
        в коде проекта рекомендуется использовать их для избежания ошибок

    RELATIVE_INCLUDE_DIR <dir>
        Инсталляционный относительный путь для заголовочных файлов

До версии CMake 13.3 функцию нужно вызывать только из того CMakeLists.txt, в котором 
определены цели.

Функция спроектирована для использования в связке с функцией
'set_target_export_properties()', которая задает индивидуальные параметры отдельным
целям, в частности, заголовочные файлы и/или каталоги заголовочных файлов,
которые будут добавлены к инсталляции. Все заголовочные файлы и директории
будут инсталлироваться в один общий каталог, который будет добавлен к каждой
экспортируемой цели. Внутри этого каталога можно задать дополнительный уровень
вложенности с помощью параметра 'RELATIVE_INCLUDE_DIR'.

Пример:
    project(my_awesome VERSION 1.2.3)
    # ...
    find_package(app_control REQUIRED)
    find_package(app_utils 1.4.2 EXACT REQUIRED)
    find_package(Qt5 5.5 REQUIRED COMPONENTS Core Gui)

    # Если есть опциональные зависимости, решение об их добавлении
    # в качестве зависимостей пакета нужно принимать индивидуально

    # Зависимость используемая только в приватных секциях
    # и поэтому не указывается в зависимостях CMake-пакета
    find_package(sockets REQUIRED)
    # ...

    # Creating targets: 'app', 'stuff', 'stuff_extra'
    add_libary(app...)
    ...

    set(pkg_name ${PROJECT_NAME})
    begin_cmake_package(${pkg_name})

    # ...
    add_targets_to_cmake_package(
        PACKAGE ${pkg_name}
        TARGETS
            app
            stuff
            stuff_extra
        DEPENDENCIES
            "app_control"
            "app_utils 1.4.2 EXACT"
            "Qt5 5.5 COMPONENTS Core Gui"
    )

    end_cmake_package(${pkg_name})
    # end of CMakeLists.txt

    Т.к. выше есть вызов 'project()', ничего дополнительно не указывая, получим:
        Имя пакета: "my_awesome"
        Версия: "1.2.3"
        Namespace: "my_awesome::"
        Импортируемые и alias цели:
            'my_awesome::app'
            'my_awesome::stuff'
            'my_awesome::stuff_extra'

(*) О компонентном подходе:
https://cmake.org/cmake/help/latest/guide/importing-exporting/index.html#adding-components



Определить требуемые зависимые пакеты
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

REQUIRE_PACKAGE(<out_var> <find_package_args_str>)
--------------------------------------------------

Вспомогательный макрос. Поможет не дублировать лишний раз версию, компоненты
и прочие аргументы при поиске внешних CMake-пакетов для работы внутри проекта
и для добавления зависимостей при создании своего CMake-пакета.
Вызовет команду 'find_package(<find_package_args_str> REQUIRED)'
и установит переменной '<out_var>' значение '<find_package_args_str>'.

Пример:
    REQUIRE_PACKAGE(QT_PKG "Qt5 5.15 COMPONENTS Core")
    REQUIRE_PACKAGE(OPENCV_PKG "OpenCV 3.2")

    # ...
    # create targets
    # ...
    add_targets_to_cmake_package(
        PACKAGE some_package_name
        TARGETS some_stuff
        DEPENDENCIES
            ${QT_PKG}
            ${OPENCV_PKG}
        )


setup_make_package()
--------------------

Функция настраивает возможность сборки Deb-пакета на основе инсталляционных правил
созданных функцией create_cmake_package() и/или командами install().
Добавляет цель сборки 'package'. Вызов 'make package' в билд каталоге проекта
соберет готовый к использованию Deb-пакет.

Данная функция вызывается автоматически при подключении этого модуля к проекту,
применяя базовые параметры необходимые для создания пакета:
    - Генератор - Debian (DEB)
    - Имя пакета - Имя проекта ('CMAKE_PROJECT_NAME')
    - Версия пакета - Версия проекта ('CMAKE_PROJECT_VERSION')
    - Поставщик - "KBL"
    - Контакт для связи - "KBL"
    - Префикс пути инсталляции - Значение переменной 'CMAKE_INSTALL_PREFIX'

Их уже достаточно, чтобы запустить сборку пакета, но крайне рекомендуется дополнительно
как минимум задать свое осмысленное имя пакета и добавить описание.
Для этого нужно добавить в конец корневого CMakeLists.txt повторный вызов этой функции,
а перед ним установить ряд переменных задающих параметры пакетирования.

Пример:
    # root CMakeLists.txt
    # ...
    set(CPACK_PACKAGE_NAME "libmy-awesome")
    set(CPACK_PACKAGE_DESCRIPTION "My awesome library")
    setup_make_package()
    # end of root CMakeLists.txt

Для названия Deb-пакета не используйте пробелы и нижние подчеркивания
Пример корректного имени: 'libkbl-app-utils'

Для указания Deb-зависимостей есть две переменные:
    1. CPACK_DEBIAN_PACKAGE_SHLIBDEPS
        Если значение 'ON', активирует автоматический поиск зависимостей
        при сборке пакета. Использовать ТОЛЬКО, ЕСЛИ ВСЕ внешние библиотеки
        используемые в проекте являются установленными в систему Deb-пакетами,
        иначе пакет будет невозможно собрать! Пример:
            not_overridable_set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    2. CPACK_DEBIAN_PACKAGE_DEPENDS
        Для указания зависимостей вручную. Пример:
            not_overridable_set(CPACK_DEBIAN_PACKAGE_DEPENDS
                "libkbl-app-utils (>= 2.3.1-6), libkbl-app-control (< 2.4)")
Возможно задействовать обе переменные сразу, например, чтобы вручную задать
дополнительные ограничения версиям зависимостей. Установку значений этих опций
РЕКОМЕНДУЕТСЯ производить с помощью функции 'not_overridable_set()' (см. модуль 'helpers'),
чтобы осталась возможность задать их "снаружи проекта" CMake-ключами при сборке
проекта пользователем.

Если автопоиск возможен, крайне РЕКОМЕНДУЕТСЯ его активировать, т.к. он находит
и добавляет все системные зависимости (например, 'libc'). В противном случае стоит
вручную указать явно подключаемые в проекте зависимости, которые являются
или могут являться Deb-пакетами. Например, взятую из репозитория 'libjsoncpp-dev'
стоит указать, как и 'libkbl-storage-dev', в проекте которой предусмотрена возможность
создание Deb-пакета с таким именем. Используемую собранную вручную OpenCV
указывать не нужно. Если нет уверенности, лучше вовсе не указывать Deb-зависимости.

Посмотреть что получилось в результате сборки пакета можно с помощью команды:
    dpkg --info <package-file>

Список и назначение основных общих переменных влияющих на пакетирование:
https://cmake.org/cmake/help/v3.10/module/CPack.html
Переменные влияющие на генератор Deb-пакетов:
https://cmake.org/cmake/help/v3.10/module/CPackDeb.html
Проверяйте свежую версию документации, в ней могут быть дополнительные пояснения

Можно настроить применение других генераторов, разделение на компоненты.
Применение функции никак не ограничивает эти возможности.


Обратите внимание! Deb-пакет и CMake-пакет и их зависимости это не одно и то же!

#]=============================================================================]

cmake_minimum_required(VERSION 3.7)

include(helpers)
ROOT_INCLUDE_GUARD()

include(CMakePackageConfigHelpers)
include(extern_dep)

option(EXPORT_TO_REGISTERY "Export project packages into the CMake local repository" OFF)

#
# Add 'uninstall' target at top level
#
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/package-uninstall.cmake.in"
    "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
    @ONLY)
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
    COMMENT "Uninstall project '${CMAKE_PROJECT_NAME}'")

# Global read-only private vars
set(_PKG_CONFIG_TEMPLATE_FILE "${CMAKE_CURRENT_LIST_DIR}/package-config.cmake.in")

#
# declare_cmake_package
#
function(begin_cmake_package
        package)
    set(options)
    set(oneValueArgs VERSION)
    set(multiValueArgs)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Function 'begin_package()' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if(NOT package)
        message(FATAL_ERROR "Package name is not defined")
    endif()

    if(NOT ARG_VERSION)
        if (PROJECT_VERSION)
            set(ARG_VERSION ${PROJECT_VERSION})
        else()
            message(FATAL_ERROR "Package version is not defined")
        endif()
    endif()

    set_property(GLOBAL PROPERTY 
        "${package}_FULL_NAME" "${package}-${ARG_VERSION}")
    set_property(GLOBAL PROPERTY 
        "${package}_VERSION" "${ARG_VERSION}")
endfunction()

#
# add_targets_to_cmake_package
#
function(add_targets_to_cmake_package)
    set(options)
    set(oneValueArgs 
        PACKAGE 
        COMPONENT 
        EXPORT 
        NAMESPACE 
        RELATIVE_INCLUDE_DIR
        )
    set(multiValueArgs 
        TARGETS
        )

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Function 'add_target_export_to_package()' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if(NOT ARG_PACKAGE)
        message(FATAL_ERROR "Package name is not defined")
    endif()

    if(NOT ARG_EXPORT)
        message(FATAL_ERROR "Package EXPORT is not defined")
    endif()

    if(NOT ARG_TARGETS)
        message(FATAL_ERROR "TARGETS are not defined")
    endif()

    if(NOT ARG_NAMESPACE)
        set(ARG_NAMESPACE "${ARG_PACKAGE}")
    endif()

    get_package_install_dirs( 
        ${ARG_PACKAGE} 
        pkg_cmake_install_dir 
        pkg_include_base_dir 
        pkg_bin_dir 
        pkg_lib_dir
        )

    if(NOT ARG_RELATIVE_INCLUDE_DIR)
        set(pkg_include_dir "${pkg_include_base_dir}")
    else()
        set(pkg_include_dir "${pkg_include_base_dir}/${ARG_RELATIVE_INCLUDE_DIR}")
    endif()

    set(export_name "${ARG_PACKAGE}-${ARG_EXPORT}Targets")
    set_property(GLOBAL APPEND_STRING PROPERTY "${ARG_PACKAGE}_TARGET_EXPORTS" " ${export_name}") 

    # Add targets into installer
    install(TARGETS ${ARG_TARGETS}
        EXPORT ${export_name} 
        RUNTIME
            COMPONENT ${ARG_COMPONENT}
            DESTINATION ${pkg_bin_dir} 
        LIBRARY
            COMPONENT ${ARG_COMPONENT}
            DESTINATION ${pkg_lib_dir}
        ARCHIVE
            COMPONENT ${ARG_COMPONENT}
            DESTINATION ${pkg_lib_dir}
        INCLUDES DESTINATION ${pkg_include_base_dir}
        )

    install(EXPORT ${export_name} 
        DESTINATION ${pkg_cmake_install_dir}
        NAMESPACE "${ARG_NAMESPACE}::"
        )

    #
    # Additional setup, that allow import package from build dir
    #
    # Create in build dir special *Targets.cmake export-file for BUILD-TREE,
    # which will be included by *Config.cmake when finding a package

    # Use the same export-set as in installer
    export(EXPORT ${export_name} 
        NAMESPACE "${ARG_NAMESPACE}::"
    )

    #
    #   Install header files
    #
    set(hdr_targets "")
    foreach(target ${ARG_TARGETS})
        set(${target}_hdr_files)

        get_target_property(target_hdr_dirs ${target} INTERFACE_HEADER_DIRS)
        foreach(hdr_dir ${target_hdr_dirs})
            file(GLOB hdr_files 
                LIST_DIRECTORIES false 
                "${hdr_dir}/*.h" "${hdr_dir}/*.hpp"
                )
            list(APPEND ${target}_hdr_files ${hdr_files})
        endforeach()

        get_target_property(target_hdr_files ${target} INTERFACE_HEADER)
        if(target_hdr_files)
            list(APPEND ${target}_hdr_files ${target_hdr_files})
        endif()

        if(${target}_hdr_files)
            list(APPEND hdr_targets "${target}") 
        endif()

        # Add alias target
        get_target_property(target_type ${target} TYPE)
        if(target_type STREQUAL EXECUTABLE)
            add_executable("${ARG_NAMESPACE}::${target}" ALIAS ${target})
        else()
            add_library("${ARG_NAMESPACE}::${target}" ALIAS ${target})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES hdr_targets)
    
    foreach(hdr_target ${hdr_targets})
        get_target_property(target_dir ${hdr_target} INTERFACE_TARGET_DIR)
        if (NOT target_dir)
            continue()
        endif()

        foreach(hdr_file ${${hdr_target}_hdr_files})
            file(RELATIVE_PATH hdr_rel_path ${target_dir} ${hdr_file})
            
            string(FIND ${hdr_rel_path} "${CMAKE_INSTALL_INCLUDEDIR}/" include_dir_pos)
            if(NOT (include_dir_pos EQUAL 0))
                file(RELATIVE_PATH build_rel_path ${CMAKE_BINARY_DIR} ${hdr_file})

                string(FIND ${build_rel_path} "${CMAKE_INSTALL_INCLUDEDIR}/" include_dir_pos)
                if(include_dir_pos EQUAL 0)
                    set(hdr_rel_path ${build_rel_path})
                endif()
            endif()

            if(include_dir_pos EQUAL 0)
                string(REGEX REPLACE 
                    "${CMAKE_INSTALL_INCLUDEDIR}/(.+)" 
                    "\\1" 
                    hdr_rel_path 
                    ${hdr_rel_path}
                    )
            endif()

            get_filename_component(hdr_rel_path ${hdr_rel_path} DIRECTORY)
            set(dest_path "${pkg_include_dir}/${hdr_rel_path}")

            install(FILES ${hdr_file}
                DESTINATION ${dest_path}
                COMPONENT ${ARG_COMPONENT}
                )
        endforeach()
    endforeach()
endfunction()

#
# end_cmake_package
#
function(end_cmake_package 
        package)
    if(NOT package)
        message(FATAL_ERROR "Package name is not defined")
    endif()

    set(options 
        EXPORT_TO_REGISTERY
        )
    set(oneValueArgs)
    set(multiValueArgs 
        PACKAGE_DEPENDENCIES
        )

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Function 'add_target_export_to_package()' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    get_property(pkg_full_name GLOBAL PROPERTY "${package}_FULL_NAME")
    if (NOT pkg_full_name)
        message(FATAL_ERROR "Package `${package}' is not defined")
    endif()

    get_property(pkg_version GLOBAL PROPERTY "${package}_VERSION")

    get_package_install_dirs( 
        ${package} 
        pkg_cmake_install_dir 
        pkg_include_base_dir 
        pkg_bin_dir 
        pkg_lib_dir
        )

    set(pkg_cmake_file "${CMAKE_CURRENT_BINARY_DIR}/${package}Config.cmake")
    set(pkg_cmake_vers_file "${CMAKE_CURRENT_BINARY_DIR}/${package}ConfigVersion.cmake")

    # PACKAGE_DEPENDENCY_INIT_CODE is used in package-config.cmake.in
    set(PACKAGE_DEPENDENCY_INIT_CODE)

    if(ARG_PACKAGE_DEPENDENCIES OR CCX_COMMON_OPTS_MODULE_INCLUDED)
        string(CONCAT PACKAGE_DEPENDENCY_INIT_CODE
            "include(CMakeFindDependencyMacro)\n\n"
            )

        if(CCX_COMMON_OPTS_MODULE_INCLUDED)
            string(APPEND PACKAGE_DEPENDENCY_INIT_CODE
                "# Find Threads library\n"
                "set(CMAKE_THREAD_PREFER_PTHREAD ON)\n"
                "set(THREADS_PREFER_PTHREAD_FLAG ON)\n"
                "find_dependency(Threads)\n"
                "\n"
                )
        endif()

        if (ARG_PACKAGE_DEPENDENCIES)
            string(APPEND PACKAGE_DEPENDENCY_INIT_CODE "# Dependency packages\n")
            foreach(dep_pkg ${ARG_PACKAGE_DEPENDENCIES})
                get_dependency_package_descriptor(pkg_descr PACKAGE ${dep_pkg})
                if(pkg_descr)
                    set(dep_pkg ${pkg_descr})
                endif()
                string(APPEND PACKAGE_DEPENDENCY_INIT_CODE "find_dependency(${dep_pkg})\n")
            endforeach()
        endif()
    endif()

    # PACKAGE_TARGET_EXPORTS is used in package-config.cmake.in
    get_property(PACKAGE_TARGET_EXPORTS 
        GLOBAL PROPERTY "${package}_TARGET_EXPORTS"
        ) 

    # Generate package configs
    configure_package_config_file(
        ${_PKG_CONFIG_TEMPLATE_FILE} ${pkg_cmake_file}
        INSTALL_DESTINATION ${pkg_cmake_install_dir}
        )
    write_basic_package_version_file(${pkg_cmake_vers_file}
        VERSION ${ARG_VERSION}
        COMPATIBILITY SameMajorVersion
        )

    # Add package config files into installer
    install(FILES
        ${pkg_cmake_file}
        ${pkg_cmake_vers_file}
        DESTINATION ${pkg_cmake_install_dir}
        )

    # Export to CMake local registery ~/.cmake/packages/<package>
    set(export_to_registery ${EXPORT_TO_REGISTERY})
    if (NOT export_to_registery)
        set(export_to_registery ${ARG_EXPORT_TO_REGISTERY})
    endif()
    if(export_to_registery)
        export(PACKAGE ${package})
    endif()
endfunction()

function(get_package_install_dirs 
        package 
        pkg_cmake_install_dir 
        pkg_include_dir 
        pkg_bin_dir 
        pkg_lib_dir)
    get_property(pkg_full_name GLOBAL PROPERTY "${package}_FULL_NAME")
    if (NOT pkg_full_name)
        message(FATAL_ERROR "Package `${package}' is not defined")
    endif()

    if(pkg_cmake_install_dir)
        set(${pkg_cmake_install_dir} "${CMAKE_INSTALL_DATAROOTDIR}/cmake/${pkg_full_name}" PARENT_SCOPE)
    endif()
    if(pkg_include_dir)
        set(${pkg_include_dir} "${CMAKE_INSTALL_INCLUDEDIR}/${pkg_full_name}" PARENT_SCOPE)
    endif()
    if(pkg_bin_dir)
        set(${pkg_bin_dir} "${CMAKE_INSTALL_BINDIR}" PARENT_SCOPE)
    endif()
    if(pkg_lib_dir)
        set(${pkg_lib_dir} "${CMAKE_INSTALL_LIBDIR}" PARENT_SCOPE)
    endif()
endfunction()

#
# REQUIRE_PACKAGE
#
macro(REQUIRE_PACKAGE out_var 
        find_package_args)
    set(${out_var} "${find_package_args}")

    string(REPLACE " " ";" _args_list ${find_package_args})
    find_package(${_args_list} REQUIRED)
endmacro()

#
# setup_make_package
#
function(setup_make_package)
    if (NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
        message(FATAL_ERROR "Function 'setup_make_package()' "
            "may be called only via root CMakeLists.txt")
    endif()

    include(InstallRequiredSystemLibraries)

    not_overridable_set(CPACK_GENERATOR DEB)
    not_overridable_set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})

    not_overridable_set(CPACK_PACKAGE_VENDOR "KBL")
    not_overridable_set(CPACK_PACKAGE_CONTACT "KBL")

    not_overridable_set(CPACK_SOURCE_GENERATOR ZIP)

    include(CPack)
endfunction()

# For apply default CPack settings
setup_make_package()

