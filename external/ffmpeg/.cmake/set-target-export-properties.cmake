#[=============================================================================[

set_target_export_properties(<target>
    [NO_GENERATE_EXPORT_HEADER]
    [VERSION <major>[.<minor>[.<patch>[.<tweak>]]]]
    [BUILD_INCLUDE_DIRS [SYSTEM] [BEFORE]
        [[INTERFACE|PUBLIC|PRIVATE] <dir> ...] ...]
    [INSTALLABLE_HEADERS <file> ...]
    [INSTALLABLE_HEADER_DIRS <dir> ...])
---------------------------------------------------

Функция позволяет устанавливать свойства цели сборки
для последующего экспорта и инсталляции:
    - Устанавливает свойства сокрытия интерфейса компиляции для библиотеки (*):
        VISIBILITY_INLINES_HIDDEN true
        CXX_VISIBILITY_PRESET hidden
    - Генерирует заголовочный файл экспортных макросов для библиотеки (*).
        По умолчанию генерация происходит в файл
        '${CMAKE_CURRENT_BINARY_DIR}/<target>_export.h'
        Например, для библиотеки 'notion_struct':
            class NOTION_STRUCT_EXPORT Foo {
                //...
            };
        Макрос 'NOTION_STRUCT_EXPORT' содержится  в заголовочном файле
        экспортных макросов 'notion_struct_export.h' и определяется как:
            #define NOTION_STRUCT_EXPORT __attribute__((visibility("default")))
    - Задает билд и API версию цели. (Не применяется для интерфейсных библиотек)
        К имени собранного бинарного файла будет добавлен суффикс с номером версии,
        а с оригинальным именем будет создан симлинк ведущий к бинарному файлу
    - Регистрирует заголовочные файлы для сборки цели, экспорта и инсталляции

(*) Не применяется для целей, создающих исполняемые файлы или интерфейсные библиотеки

Данную функцию нужно вызывать в том же каталоге (CMakeLists.txt), в котором
была создана цель сборки (<target>), к которой применяется функция.

Данную функцию следует использовать вместо команды 'target_include_directories()'
для корректной настройки сборки, экспорта и инсталляции цели

Параметры:
    <target> - цель сборки
    NO_GENERATE_EXPORT_HEADER
        Не генерировать заголовочный файл экспортных макросов для библиотек
    VERSION <major>[.<minor>[.<patch>[.<tweak>]]]
        Build и API версия. Используется для создания симлинков
        Если не указано, используется значение переменной 'PROJECT_VERSION'
        Не применяется для интерфейсных библиотек
    BUILD_INCLUDE_DIRS [SYSTEM] [BEFORE] [[INTERFACE|PUBLIC|PRIVATE] <dir> ...] ...
        Подключение каталогов заголовочных файлов для сборки цели и экспорта.
        Значение этого параметра вместе со всеми ключевыми словами будет передано
        CMake команде 'target_include_directories()'. Каждый указанный путь к каталогу
        будет преобразован в абсолютный и обернут в защитное выражение
        '$<BUILD_INTERFACE:...>'. Это позволяет экспортировать и инсталлировать цель.
        Если не была указана область (INTERFACE|PUBLIC|PRIVATE), применяется область по умолчанию:
            'INTERFACE' для интерфейсных библиотек
            'PUBLIC' для всех остальных целей
        Если не указан BUILD_INCLUDE_DIRS параметер и присутствует каталог include в корне проекта,
        то BUILD_INCLUDE_DIRS инциализируется значением include
    INSTALLABLE_HEADERS <file> ...
        Заголовочные файлы добавляемые к инсталляции
    INSTALLABLE_HEADER_DIRS <dir>...
        Каталоги заголовочных файлов добавляемые к инсталляции.
        Добавляется содержимое указанных каталогов, иерархия подкаталогов сохраняется.
        Добавляются только файлы с расширением '.h' и '.hpp'.
        Указанные каталоги также добавляются к параметру 'BUILD_INCLUDE_DIRS'
        с областью по умолчанию. Это позволяет в простых случаях не указывать
        параметр 'BUILD_INCLUDE_DIRS' отдельно

Значения параметров 'INSTALLABLE_HEADERS' и 'INSTALLABLE_HEADER_DIRS'
будут использованы впоследствии при вызове функции 'create_cmake_package()'
с той же целью сборки (<target>). Она добавит указанные заголовочные файлы
и/или каталоги цели в общий инсталлятор. (См. модуль 'create-package')

В параметрах 'BUILD_INCLUDE_DIRS' 'INSTALLABLE_HEADERS' и 'INSTALLABLE_HEADER_DIRS'
могут использоваться относительные пути. Они будут преобразованы
в абсолютные относительно текущего каталога ('CMAKE_CURRENT_SOURCE_DIR')

Сторонние библиотеки к цели нужно ВСЕГДА подключать как импортируемую цель
с помощью команды 'target_link_libraries()'. Если такой возможности нет,
а поисковой модуль писать не стали, можно использовать команды
'target_link_libraries()' и 'target_include_directories()' как обычно в таких случаях.
Но если зависимость является публичной,
цель с такой зависимостью не будет нормально работать при экспорте и инсталляции

Примеры:
    # Обычная интерфейсная библиотека
    # Указанный каталог подключается к цели
    # для сборки, экспорта и регистрируется для инсталляции
    set_target_export_properties(my_interface_lib
        INSTALLABLE_HEADER_DIRS
            "src/include"
    )

    # Другой вариант интерфейсной библиотеки
    # Здесь, для примера, указывается текущий каталог
    set_target_export_properties(my_interface_lib
        INSTALLABLE_HEADER_DIRS
            "."
    )

    # Разделяемая библиотека
    # Здесь для примера к инсталляции добавляется только "src/include"
    set_target_export_properties(my_shared_lib
        BUILD_INCLUDE_DIRS
            INTERFACE
                "src/mock_headers"
            PUBLIC
                "src/dev_headers"
            PRIVATE
                "src/private_headers"
        INSTALLABLE_HEADER_DIRS
            "src/include"
    )

    # Приложение
    set_target_export_properties(my_app
        BUILD_INCLUDE_DIRS "src/include"
        VERSION "1.2.3"
    )

#]=============================================================================]

cmake_minimum_required(VERSION 3.7)

include(helpers)
root_include_guard()

#
# set_target_export_properties
#
function(set_target_export_properties 
    target)
    set(options NO_GENERATE_EXPORT_HEADER)
    set(oneValueArgs VERSION)
    set(multiValueArgs INSTALLABLE_HEADERS INSTALLABLE_HEADER_DIRS BUILD_INCLUDE_DIRS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Function 'set_target_export_properties()' called "
            "with unrecognized parameters: '${ARG_UNPARSED_ARGUMENTS}'")
    endif()

    if(NOT TARGET ${target})
        message(FATAL_ERROR "'${target}' is not a target")
    endif()

    # Check target dir
    get_directory_property(source_dir_targets BUILDSYSTEM_TARGETS)
    list(FIND source_dir_targets ${target} find_target_out)
    if(find_target_out EQUAL -1)
        message(FATAL_ERROR "Function 'set_target_export_properties()'"
            "can only be called in the same directory as directory of specified target")
    endif()

    get_target_property(target_type ${target} TYPE)

    # Set version
    if(NOT (target_type STREQUAL INTERFACE_LIBRARY))
        if(NOT ARG_VERSION)
            if(PROJECT_VERSION)
                set(ARG_VERSION ${PROJECT_VERSION})
            else()
                message(FATAL_ERROR "Version is not defined")
            endif()
        endif()
        set_target_properties(${target} PROPERTIES
            VERSION ${ARG_VERSION})
    endif()

    # Turn on 'visibility hidden' and generate export header
    if (NOT ARG_NO_GENERATE_EXPORT_HEADER)
        set(hdr_export_file "${CMAKE_BINARY_DIR}/include/${target}_export.h") 
        get_filename_component(hdr_export_file ${hdr_export_file} ABSOLUTE)

        if(target_type STREQUAL INTERFACE_LIBRARY)
            configure_file(
                "${CMAKE_SOURCE_DIR}/dev_tool/cmake/visibility_decl.h" 
                "${hdr_export_file}" 
                COPYONLY)

            target_include_directories(${target} 
                INTERFACE
                    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>)

        else()
            include(GenerateExportHeader)

            generate_export_header(${target}
                EXPORT_FILE_NAME ${hdr_export_file})

            set_target_properties(${target} 
                PROPERTIES
                    VISIBILITY_INLINES_HIDDEN true
                    CXX_VISIBILITY_PRESET hidden)

            target_include_directories(${target} 
                PUBLIC
                    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>)
        endif()

        set_property(TARGET ${target}
            APPEND PROPERTY 
                INTERFACE_HEADER "${hdr_export_file}")
    endif()

    if (ARG_INSTALLABLE_HEADERS)
        # Transform all paths to absolute
        set(headers)
        foreach(header ${ARG_INSTALLABLE_HEADERS})
            get_filename_component(header_full_path ${header} ABSOLUTE)
            list(APPEND headers ${header_full_path})
        endforeach()
        
        set_property(TARGET ${target}
            APPEND PROPERTY INTERFACE_HEADER "${headers}")
    endif()

    if(ARG_INSTALLABLE_HEADER_DIRS)
        # Transform all paths to absolute
        set(headers_dirs)
        foreach(dir ${ARG_INSTALLABLE_HEADER_DIRS})
            get_filename_component(dir_full_path ${dir} ABSOLUTE)
            list(APPEND headers_dirs "${dir_full_path}")
        endforeach()

        set_property(TARGET ${target}
            APPEND PROPERTY INTERFACE_HEADER_DIRS "${headers_dirs}")
    endif()

    if (NOT ARG_BUILD_INCLUDE_DIRS)
        if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
            set(ARG_BUILD_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")
        endif()
    endif()

    # Prepare args for invoke target_include_directories()
    # Bypass all keywords and wrap all dirs paths
    # by $<BUILD_INTERFACE:...> generator expression

    # Args for target_include_directories()
    # start with 'PUBLIC' or 'INTERFACE' as a default scope
    # for allow use 'BUILD_INCLUDE_DIRS' without any keywords
    if(target_type STREQUAL INTERFACE_LIBRARY)
        set(target_include_dirs "INTERFACE")
    else()
        set(target_include_dirs "PUBLIC")
    endif()

    foreach(include_dir ${ARG_BUILD_INCLUDE_DIRS})
        if((include_dir STREQUAL "SYSTEM") OR
           (include_dir STREQUAL "BEFORE") OR
           (include_dir STREQUAL "INTERFACE") OR
           (include_dir STREQUAL "PUBLIC") OR
           (include_dir STREQUAL "PRIVATE"))
            # Bypass keywords
            list(APPEND target_include_dirs ${include_dir})
        else()
            get_filename_component(include_dir "${include_dir}" ABSOLUTE)
            list(APPEND target_include_dirs "$<BUILD_INTERFACE:${include_dir}>")
        endif()
    endforeach()

    target_include_directories(${target} ${target_include_dirs})

    set_property(TARGET ${target} 
        PROPERTY INTERFACE_TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR}
        )
endfunction()

