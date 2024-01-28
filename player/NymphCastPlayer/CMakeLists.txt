set(SRC
	container_qpainter.cpp
	litehtml/src/background.cpp
	litehtml/src/box.cpp
	litehtml/src/context.cpp
	litehtml/src/css_length.cpp
	litehtml/src/css_selector.cpp
	litehtml/src/document.cpp
	litehtml/src/el_anchor.cpp
	litehtml/src/el_base.cpp
	litehtml/src/el_before_after.cpp
	litehtml/src/el_body.cpp
	litehtml/src/el_break.cpp
	litehtml/src/el_cdata.cpp
	litehtml/src/el_comment.cpp
	litehtml/src/el_div.cpp
	litehtml/src/el_font.cpp
	litehtml/src/el_image.cpp
	litehtml/src/el_li.cpp
	litehtml/src/el_link.cpp
	litehtml/src/el_para.cpp
	litehtml/src/el_script.cpp
	litehtml/src/el_space.cpp
	litehtml/src/el_style.cpp
	litehtml/src/el_table.cpp
	litehtml/src/el_td.cpp
	litehtml/src/el_text.cpp
	litehtml/src/el_title.cpp
	litehtml/src/el_tr.cpp
	litehtml/src/element.cpp
	litehtml/src/gumbo/attribute.c
	litehtml/src/gumbo/char_ref.c
	litehtml/src/gumbo/error.c
	litehtml/src/gumbo/parser.c
	litehtml/src/gumbo/string_buffer.c
	litehtml/src/gumbo/string_piece.c
	litehtml/src/gumbo/tag.c
	litehtml/src/gumbo/tokenizer.c
	litehtml/src/gumbo/utf8.c
	litehtml/src/gumbo/util.c
	litehtml/src/gumbo/vector.c
	litehtml/src/html.cpp
	litehtml/src/html_tag.cpp
	litehtml/src/iterators.cpp
	litehtml/src/media_query.cpp
	litehtml/src/num_cvt.cpp
	litehtml/src/style.cpp
	litehtml/src/stylesheet.cpp
	litehtml/src/table.cpp
	litehtml/src/utf8_strings.cpp
	litehtml/src/web_color.cpp
	main.cpp
	mainwindow.cpp
	qlitehtmlwidget.cpp
	remotes.cpp
	custom_remotes.cpp)

set(HEADERS
	container_qpainter.h
	container_qpainter_p.h
	mainwindow.h
	qlitehtml_global.h
	qlitehtmlwidget.h
	remotes.h
	custom_remotes.h)

include_directories("litehtml/include")
include_directories("litehtml/include/litehtml")
include_directories("litehtml/src/gumbo/include")
include_directories("litehtml/src/gumbo/include/gumbo")

qt_add_resources(RESOURCES resources.qrc)

if (ANDROID)
	add_library(nymphcast-player ${SRC} ${HEADERS} ${RESOURCES})
else()
	add_executable(nymphcast-player ${SRC} ${HEADERS} ${RESOURCES})
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")

add_definitions(-D__NCVERSION=${PROJECT_VERSION})

target_link_libraries(nymphcast-player
	Qt5::Gui
	Qt5::Widgets
	Poco::Net
	nymphcast
	nymphrpc)

if (ANDROID)
	target_link_libraries(nymphcast-player
		Qt5::AndroidExtras)

	set(ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENTG_SOURCE_DIR}/android)
elif(WIN32)
	target_link_libraries(nymphcast-player
		ws2_32)
endif()

install(TARGETS nymphcast-player RUNTIME
	DESTINATION ${CMAKE_INSTALL_BINDIR})
