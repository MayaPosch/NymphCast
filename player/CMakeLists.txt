cmake_minimum_required(VERSION 3.6.0)

project(nymphcast-player
	VERSION 0.1
	DESCRIPTION "The reference client for the Nymphcast protocol")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "..." FORCE)

include(FeatureSummary)
include(GNUInstallDirs)

set(QT_MIN_VERSION "5.15.0")
find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS Core Gui Widgets REQUIRED)
find_package(Poco COMPONENTS Net REQUIRED)

add_subdirectory(NymphCastPlayer)

feature_summary(WHAT ALL)
