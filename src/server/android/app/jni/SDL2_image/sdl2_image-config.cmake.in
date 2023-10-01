# sdl2_image cmake project-config input for ./configure scripts

include(FeatureSummary)
set_package_properties(SDL2_image PROPERTIES
    URL "https://www.libsdl.org/projects/SDL_image/"
    DESCRIPTION "SDL_image is an image file loading library"
)

set(SDL2_image_FOUND TRUE)

set(SDL2IMAGE_AVIF  @LOAD_AVIF@)
set(SDL2IMAGE_BMP   @LOAD_BMP@)
set(SDL2IMAGE_GIF   @LOAD_GIF@)
set(SDL2IMAGE_JPG   @LOAD_JPG@)
set(SDL2IMAGE_JXL   @LOAD_JXL@)
set(SDL2IMAGE_LBM   @LOAD_LBM@)
set(SDL2IMAGE_PCX   @LOAD_PCX@)
set(SDL2IMAGE_PNG   @LOAD_PNG@)
set(SDL2IMAGE_PNM   @LOAD_PNM@)
set(SDL2IMAGE_QOI   @LOAD_QOI@)
set(SDL2IMAGE_SVG   @LOAD_SVG@)
set(SDL2IMAGE_TGA   @LOAD_TGA@)
set(SDL2IMAGE_TIF   @LOAD_TIF@)
set(SDL2IMAGE_XCF   @LOAD_XCF@)
set(SDL2IMAGE_XPM   @LOAD_XPM@)
set(SDL2IMAGE_XV    @LOAD_XV@)
set(SDL2IMAGE_WEBP  @LOAD_WEBP@)

set(SDL2IMAGE_JPG_SAVE @SDL2IMAGE_JPG_SAVE@)
set(SDL2IMAGE_PNG_SAVE @SDL2IMAGE_PNG_SAVE@)

set(SDL2IMAGE_VENDORED  FALSE)

set(SDL2IMAGE_BACKEND_IMAGEIO   @USE_IMAGEIO@)
set(SDL2IMAGE_BACKEND_STB       @USE_STBIMAGE@)
set(SDL2IMAGE_BACKEND_WIC       @USE_WIC@)

get_filename_component(prefix "${CMAKE_CURRENT_LIST_DIR}/@cmake_prefix_relpath@" ABSOLUTE)
set(exec_prefix "@exec_prefix@")
set(bindir "@bindir@")
set(includedir "@includedir@")
set(libdir "@libdir@")
set(_sdl2image_extra_static_libraries "@IMG_LIBS@ @PC_LIBS@")
string(STRIP "${_sdl2image_extra_static_libraries}" _sdl2image_extra_static_libraries)

set(_sdl2image_bindir   "${bindir}")
set(_sdl2image_libdir   "${libdir}")
set(_sdl2image_incdir   "${includedir}/SDL2")

# Convert _sdl2image_extra_static_libraries to list and keep only libraries
string(REGEX MATCHALL "(-[lm]([-a-zA-Z0-9._]+))|(-Wl,[^ ]*framework[^ ]*)" _sdl2image_extra_static_libraries "${_sdl2image_extra_static_libraries}")
string(REGEX REPLACE "^-l" "" _sdl2image_extra_static_libraries "${_sdl2image_extra_static_libraries}")
string(REGEX REPLACE ";-l" ";" _sdl2image_extra_static_libraries "${_sdl2image_extra_static_libraries}")

unset(prefix)
unset(exec_prefix)
unset(bindir)
unset(includedir)
unset(libdir)

include(CMakeFindDependencyMacro)

if(NOT TARGET SDL2_image::SDL2_image)
    add_library(SDL2_image::SDL2_image SHARED IMPORTED)
    set_target_properties(SDL2_image::SDL2_image
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_sdl2image_incdir}"
            COMPATIBLE_INTERFACE_BOOL "SDL2_SHARED"
            INTERFACE_SDL2_SHARED "ON"
    )
    if(WIN32)
        set_target_properties(SDL2_image::SDL2_image
            PROPERTIES
                IMPORTED_LOCATION "${_sdl2image_bindir}/SDL2_image.dll"
                IMPORTED_IMPLIB "${_sdl2image_libdir}/${CMAKE_STATIC_LIBRARY_PREFIX}SDL2_image.dll${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )
    else()
        set_target_properties(SDL2_image::SDL2_image
            PROPERTIES
                IMPORTED_LOCATION "${_sdl2image_libdir}/${CMAKE_SHARED_LIBRARY_PREFIX}SDL2_image${CMAKE_SHARED_LIBRARY_SUFFIX}"
        )
    endif()
endif()

if(NOT TARGET SDL2_image::SDL2_image-static)
    add_library(SDL2_image::SDL2_image-static STATIC IMPORTED)

    set_target_properties(SDL2_image::SDL2_image-static
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_sdl2image_incdir}"
            IMPORTED_LOCATION "${_sdl2image_libdir}/${CMAKE_STATIC_LIBRARY_PREFIX}SDL2_image${CMAKE_STATIC_LIBRARY_SUFFIX}"
            INTERFACE_LINK_LIBRARIES "${_sdl2image_extra_static_libraries}"
    )
endif()

unset(_sdl2image_extra_static_libraries)
unset(_sdl2image_bindir)
unset(_sdl2image_libdir)
unset(_sdl2image_incdir)
