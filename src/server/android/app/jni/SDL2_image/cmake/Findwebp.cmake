include(FindPackageHandleStandardArgs)

find_library(webp_LIBRARY
    NAMES webp
)

find_path(webp_INCLUDE_PATH
    NAMES webp/decode.h
)

set(webp_COMPILE_OPTIONS "" CACHE STRING "Extra compile options of webp")

set(webp_LINK_LIBRARIES "" CACHE STRING "Extra link libraries of webp")

set(webp_LINK_FLAGS "" CACHE STRING "Extra link flags of webp")

find_package_handle_standard_args(webp
    REQUIRED_VARS webp_LIBRARY webp_INCLUDE_PATH
)

if (webp_FOUND)
    if (NOT TARGET WebP::webp)
        add_library(WebP::webp UNKNOWN IMPORTED)
        set_target_properties(WebP::webp PROPERTIES
            IMPORTED_LOCATION "${webp_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${webp_INCLUDE_PATH}"
            INTERFACE_COMPILE_OPTIONS "${webp_COMPILE_FLAGS}"
            INTERFACE_LINK_LIBRARIES "${webp_LINK_LIBRARIES}"
            INTERFACE_LINK_FLAGS "${webp_LINK_FLAGS}"
        )
    endif()
endif()
