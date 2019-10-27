#-------------------------------------------------
#
# Project created by QtCreator 2019-08-31T00:06:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NymphCastPlayer
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    ../../../NymphRPC/src/nymph_listener.cpp \
    ../../../NymphRPC/src/nymph_logger.cpp \
    ../../../NymphRPC/src/nymph_message.cpp \
    ../../../NymphRPC/src/nymph_method.cpp \
    ../../../NymphRPC/src/nymph_server.cpp \
    ../../../NymphRPC/src/nymph_session.cpp \
    ../../../NymphRPC/src/nymph_socket_listener.cpp \
    ../../../NymphRPC/src/nymph_types.cpp \
    ../../../NymphRPC/src/nymph_utilities.cpp \
    ../../../NymphRPC/src/remote_client.cpp \
    ../../../NymphRPC/src/remote_server.cpp

HEADERS += \
        mainwindow.h \
    ../../../NymphRPC/src/nymph.h \
    ../../../NymphRPC/src/nymph_listener.h \
    ../../../NymphRPC/src/nymph_logger.h \
    ../../../NymphRPC/src/nymph_message.h \
    ../../../NymphRPC/src/nymph_method.h \
    ../../../NymphRPC/src/nymph_server.h \
    ../../../NymphRPC/src/nymph_session.h \
    ../../../NymphRPC/src/nymph_socket_listener.h \
    ../../../NymphRPC/src/nymph_types.h \
    ../../../NymphRPC/src/nymph_utilities.h \
    ../../../NymphRPC/src/remote_client.h \
    ../../../NymphRPC/src/remote_server.h

FORMS += \
        mainwindow.ui

LIBS += -lnymphcast -lnymphrpc -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON

RESOURCES     = resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
