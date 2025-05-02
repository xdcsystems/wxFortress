cmake_minimum_required(VERSION 3.7)

include(FindPackageHandleStandardArgs)

find_file(curl_EXECUTABLE curl)

find_package_handle_standard_args(curl
    REQUIRED_VARS
        curl_EXECUTABLE
    FAIL_MESSAGE "curl utility not found"
    )

mark_as_advanced(
    curl_EXECUTABLE
    )


