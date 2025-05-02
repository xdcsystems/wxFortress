cmake_minimum_required(VERSION 3.7)

include(helpers)
ROOT_INCLUDE_GUARD()

set(doc_dir "${CMAKE_BINARY_DIR}/doc")

function(add_doc_build_target)
    configure_file(
        ${CMAKE_SOURCE_DIR}/dev_tool/cmake/Doxyfile.in
        ${CMAKE_BINARY_DIR}/doc/Doxyfile
        )

    add_custom_target(doc 
        doxygen 
        COMMAND echo "Документация сгенерирована в ${doc_dir}" 
        COMMAND ln -f -s ${doc_dir}/html/index.html ${doc_dir}/doc.html 
        WORKING_DIRECTORY
            ${CMAKE_BINARY_DIR}/doc
        )
endfunction()

