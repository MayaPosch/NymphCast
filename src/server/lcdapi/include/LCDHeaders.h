/* Copyright 2012-2017 Simon Dawson <spdawson@gmail.com>

   This file is part of lcdapi.

   lcdapi is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of
   the License, or (at your option) any later version.

   lcdapi is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with lcdapi.  If not,
   see <http://www.gnu.org/licenses/>. */

#ifndef _LCDAPI_INCLUDE_LCDHEADERS_H_
#define _LCDAPI_INCLUDE_LCDHEADERS_H_

/** \mainpage Documentation for the C++ LCD API.
 *
 * <ul>
 *  <li> \ref intro </li>
 *  <li> \ref classes </li>
 *  <li> \ref install </li>
 *  <li> \ref use </li>
 *  <li> \ref example </li>
 * </ul>
 *
 * \section intro Presentation
 *
 * This API can  be used to create clients for LCDproc server.
 *  It contains a set of C++ classes for each widgets.
 *
 * \section classes Classes
 *
 * The class that must be used for all programms is LCDClient as it contains
 * everything to connect to the server.
 *
 * You'll probably also need LCDScreen to create a new screen to be displayed.
 *
 * To learn more about all the classes, consult the
 * <a href="modules.html">Modules section</a>.
 *
 * \section install Installation
 *
 * You can get the source from the following link:
 *
 * https://github.com/spdawson/lcdapi/tarball/v0.7
 *
 * You can then unpack them with:
 * \code
 * tar xaf lcdapi-0.7.tar.gz
 * \endcode
 * Go into the new directory and execute the following commands:
 * \code
 * libtoolize --force
 * autoreconf -i
 * ./configure
 * make
 * sudo make install
 * \endcode
 * This will install the application to the following directories:
 *
 * <ul>
 *   <li>/usr/local/lib: The shared library (liblcdapi.so).</li>
 *   <li>/usr/local/include/lcdapi: The include (*.h) files for the
 *     library.</li>
 *   <li>/usr/local/share/doc/lcdapi: The HTML documentation. Go to
 *     index.html for the root.</li>
 *   <li>/usr/local/share/lcdapi/examples: The example application and
 *     Makefile.</li>
 * </ul>
 *
 * Man pages can also be accessed for many classes in the application by
 * typing: man <classname>. man main is the main main page for lcdapi.
 *
 * \section uninstall Uninstallation
 *
 * From the source folder, run:
 * \code
 * sudo make uninstall
 * \endcode
 *
 * \section use Usage
 *
 * To use this library, you have to include only one file, LCDHeaders.h as it
 *  includes all the other files.
 *
 * Your program must be linked with the socket and the pthread libraries.
 *
 * To compile client.cpp for example, you should use these commands on a
 * GNU/Linux system:
 * \code
 * export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
 * g++ -o client client.cpp -I/usr/local/include/lcdapi -L/usr/local/lib -llcdapi -lpthread
 * \endcode
 * There may have some differences on other systems.
 *  For example you must add -lsocket on Solaris.
 *
 * \section example Example
 *
 * You can find in the <a href="examples.html">examples page</a> some example
 * of programs using the API.
 */

#include <lcdapi/include/LCDConstants.h>

/* API generals */
#include <lcdapi/api/LCDException.h>
#include <lcdapi/api/LCDClient.h>
#include <lcdapi/api/LCDScreen.h>

/* Widgets */
#include <lcdapi/api/LCDText.h>
#include <lcdapi/api/LCDBigNumber.h>
#include <lcdapi/api/LCDTitle.h>
#include <lcdapi/api/LCDHorizontalBar.h>
#include <lcdapi/api/LCDVerticalBar.h>
#include <lcdapi/api/LCDFrame.h>
#include <lcdapi/api/LCDIcon.h>
#include <lcdapi/api/LCDScroller.h>

/* Menus */
#include <lcdapi/menus/LCDMenuEventHandler.h>

/* Sensors */
#include <lcdapi/sensors/LCDTimeSensor.h>
#include <lcdapi/sensors/LCDSystemSensor.h>
#include <lcdapi/sensors/LCDCpuSensor.h>
#include <lcdapi/sensors/LCDKdeMultimediaSensor.h>
#include <lcdapi/sensors/LCDNymphCastSensor.h>

/* Keys */
#include <lcdapi/keys/LCDCallback.h>
#include <lcdapi/keys/LCDKeyEvent.h>

/**
 * \example client.cpp
 * A small example of what can be done.
 *
 * It creates 3 screens, each one being displayed 2 seconds.
 * <ul>
 *  <li> First screen contains a title and the name of the current playing
 *    song with notun or kscd.</li>
 *  <li> Second screen contains time of the day displayed with big
 *    numbers.</li>
 *  <li> Third screen contains a title, a bar representing CPU usage and the
 *    textual value.</li>
 * </ul>
 *
 * It also uses callback for keys.
 *
 * The one for B key is defined using LCD_CALLBACK_FUNCTION_* macros. They
 * are useful if you only want a function without parameter to be executed.
 *
 * For the A key, the callback is in a TitleSwitcher class because it needs
 * more parameters. It will switch from English to French title for first
 * screen.
 */

#endif
