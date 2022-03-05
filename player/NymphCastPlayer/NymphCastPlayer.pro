#-------------------------------------------------
#
# Project created by QtCreator 2019-08-31T00:06:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

android {
lessThan(QT_MAJOR_VERSION, 6): QT += androidextras
ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
#ANDROID_MIN_SDK_VERSION = 24
#ANDROID_TARGET_SDK_VERSION = 30
}

TARGET = NymphCastPlayer
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS LITEHTML_UTF8

#ANDROID_ABIS= arm64-v8a 

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++14
#CONFIG += console

INCLUDEPATH += litehtml/ litehtml/include litehtml/include/litehtml litehtml/src/gumbo/include \
				litehtml/src/gumbo/include/gumbo

SOURCES += \
        container_qpainter.cpp \
        litehtml/src/background.cpp \
        litehtml/src/box.cpp \
        litehtml/src/context.cpp \
        litehtml/src/css_length.cpp \
        litehtml/src/css_selector.cpp \
        litehtml/src/document.cpp \
        litehtml/src/el_anchor.cpp \
        litehtml/src/el_base.cpp \
        litehtml/src/el_before_after.cpp \
        litehtml/src/el_body.cpp \
        litehtml/src/el_break.cpp \
        litehtml/src/el_cdata.cpp \
        litehtml/src/el_comment.cpp \
        litehtml/src/el_div.cpp \
        litehtml/src/el_font.cpp \
        litehtml/src/el_image.cpp \
        litehtml/src/el_li.cpp \
        litehtml/src/el_link.cpp \
        litehtml/src/el_para.cpp \
        litehtml/src/el_script.cpp \
        litehtml/src/el_space.cpp \
        litehtml/src/el_style.cpp \
        litehtml/src/el_table.cpp \
        litehtml/src/el_td.cpp \
        litehtml/src/el_text.cpp \
        litehtml/src/el_title.cpp \
        litehtml/src/el_tr.cpp \
        litehtml/src/element.cpp \
        litehtml/src/gumbo/attribute.c \
        litehtml/src/gumbo/char_ref.c \
        litehtml/src/gumbo/error.c \
        litehtml/src/gumbo/parser.c \
        litehtml/src/gumbo/string_buffer.c \
        litehtml/src/gumbo/string_piece.c \
        litehtml/src/gumbo/tag.c \
        litehtml/src/gumbo/tokenizer.c \
        litehtml/src/gumbo/utf8.c \
        litehtml/src/gumbo/util.c \
        litehtml/src/gumbo/vector.c \
        litehtml/src/html.cpp \
        litehtml/src/html_tag.cpp \
        litehtml/src/iterators.cpp \
        litehtml/src/media_query.cpp \
        litehtml/src/num_cvt.cpp \
        litehtml/src/style.cpp \
        litehtml/src/stylesheet.cpp \
        litehtml/src/table.cpp \
        litehtml/src/utf8_strings.cpp \
        litehtml/src/web_color.cpp \
        main.cpp \
        mainwindow.cpp \
        qlitehtmlwidget.cpp \
		remotes.cpp \
		custom_remotes.cpp

HEADERS += \
        container_qpainter.h \
        container_qpainter_p.h \
        litehtml/include/litehtml.h \
        litehtml/include/litehtml/attributes.h \
        litehtml/include/litehtml/background.h \
        litehtml/include/litehtml/borders.h \
        litehtml/include/litehtml/box.h \
        litehtml/include/litehtml/context.h \
        litehtml/include/litehtml/css_length.h \
        litehtml/include/litehtml/css_margins.h \
        litehtml/include/litehtml/css_offsets.h \
        litehtml/include/litehtml/css_position.h \
        litehtml/include/litehtml/css_selector.h \
        litehtml/include/litehtml/document.h \
        litehtml/include/litehtml/el_anchor.h \
        litehtml/include/litehtml/el_base.h \
        litehtml/include/litehtml/el_before_after.h \
        litehtml/include/litehtml/el_body.h \
        litehtml/include/litehtml/el_break.h \
        litehtml/include/litehtml/el_cdata.h \
        litehtml/include/litehtml/el_comment.h \
        litehtml/include/litehtml/el_div.h \
        litehtml/include/litehtml/el_font.h \
        litehtml/include/litehtml/el_image.h \
        litehtml/include/litehtml/el_li.h \
        litehtml/include/litehtml/el_link.h \
        litehtml/include/litehtml/el_para.h \
        litehtml/include/litehtml/el_script.h \
        litehtml/include/litehtml/el_space.h \
        litehtml/include/litehtml/el_style.h \
        litehtml/include/litehtml/el_table.h \
        litehtml/include/litehtml/el_td.h \
        litehtml/include/litehtml/el_text.h \
        litehtml/include/litehtml/el_title.h \
        litehtml/include/litehtml/el_tr.h \
        litehtml/include/litehtml/element.h \
        litehtml/include/litehtml/html.h \
        litehtml/include/litehtml/html_tag.h \
        litehtml/include/litehtml/iterators.h \
        litehtml/include/litehtml/media_query.h \
        litehtml/include/litehtml/num_cvt.h \
        litehtml/include/litehtml/os_types.h \
        litehtml/include/litehtml/style.h \
        litehtml/include/litehtml/stylesheet.h \
        litehtml/include/litehtml/table.h \
        litehtml/include/litehtml/types.h \
        litehtml/include/litehtml/utf8_strings.h \
        litehtml/include/litehtml/web_color.h \
        litehtml/src/gumbo/char_ref.rl \
        litehtml/src/gumbo/include/gumbo.h \
        litehtml/src/gumbo/include/gumbo/attribute.h \
        litehtml/src/gumbo/include/gumbo/char_ref.h \
        litehtml/src/gumbo/include/gumbo/error.h \
        litehtml/src/gumbo/include/gumbo/insertion_mode.h \
        litehtml/src/gumbo/include/gumbo/parser.h \
        litehtml/src/gumbo/include/gumbo/string_buffer.h \
        litehtml/src/gumbo/include/gumbo/string_piece.h \
        litehtml/src/gumbo/include/gumbo/tag_enum.h \
        litehtml/src/gumbo/include/gumbo/tag_gperf.h \
        litehtml/src/gumbo/include/gumbo/tag_sizes.h \
        litehtml/src/gumbo/include/gumbo/tag_strings.h \
        litehtml/src/gumbo/include/gumbo/token_type.h \
        litehtml/src/gumbo/include/gumbo/tokenizer.h \
        litehtml/src/gumbo/include/gumbo/tokenizer_states.h \
        litehtml/src/gumbo/include/gumbo/utf8.h \
        litehtml/src/gumbo/include/gumbo/util.h \
        litehtml/src/gumbo/include/gumbo/vector.h \
        litehtml/src/gumbo/visualc/include/strings.h \
        mainwindow.h \
        qlitehtml_global.h \
        qlitehtmlwidget.h \
		remotes.h \
		custom_remotes.h

FORMS += \
        mainwindow.ui \
		remotes.ui \
		custom_remotes.ui

LIBS += -lnymphcast -lnymphrpc -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON

win32:LIBS += -lws2_32

RESOURCES     = resources.qrc

# Include version file.
include($$PWD/version.pri)

DEFINES += __NCVERSION="\"$$NCVERSION\""

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android {
	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}
	target.path = $$PREFIX/bin
}

!isEmpty(target.path): INSTALLS += target

DISTFILES += \
	android/AndroidManifest.xml \
	android/build.gradle \
	android/gradle/wrapper/gradle-wrapper.jar \
	android/gradle/wrapper/gradle-wrapper.properties \
	android/gradlew \
	android/gradlew.bat \
	android/res/values/libs.xml \
	android/res/xml/network_security_config.xml \
	litehtml/include/master.css \
	litehtml/src/gumbo/CMakeLists.txt \
	litehtml/src/gumbo/tag.in

android {
QMAKE_CFLAGS +=  -fno-strict-aliasing 
QMAKE_CXXFLAGS +=  -fno-strict-aliasing
}
