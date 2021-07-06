project("nanosvg")

set(NSVG_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/nanosvg.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nanosvgrast.h
)

set(NSVG_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/nanosvg_impl.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(nanosvg STATIC ${NSVG_SOURCES} ${NSVG_HEADERS})
