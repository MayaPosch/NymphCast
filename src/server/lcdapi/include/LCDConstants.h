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

#ifndef _LCDAPI_INCLUDE_LCDCONSTANTS_H_
#define _LCDAPI_INCLUDE_LCDCONSTANTS_H_

#include <string>

namespace lcdapi {

const int LCD_MAXRECV = 200;

/**
 * \defgroup constants Constants
 * All the constants that can be used with the LCD API.
 */

/**
 *  \defgroup screen_constants Screen constants
 *  \ingroup constants
 *  Constants used with LCDScreen class
 */

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setHeartBeat.
 *
 *  Used with LCDScreen::setHeartBeat to enable the heart beat.
 */
const std::string LCD_HEARTBEAT_ON = "on";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setHeartBeat.
 *
 *  Used with LCDScreen::setHeartBeat to disable the heart beat.
 */
const std::string LCD_HEARTBEAT_OFF = "off";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setHeartBeat.
 *
 *  Used with LCDScreen::setHeartBeat to let other elements decide of the
 *  heart state.
 */
const std::string LCD_HEARTBEAT_OPEN = "open";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setBackLight and LCDClient::setBackLight.
 *
 *  Used with LCDScreen::setBackLight and LCDClient::setBackLight to enable
 *  the LCD backlight.
 */
const std::string LCD_BACKLIGHT_ON = "on";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setBackLight and LCDClient::setBackLight.
 *
 *  Used with LCDScreen::setBackLight and LCDClient::setBackLight to disable
 *  the LCD backlight.
 */
const std::string LCD_BACKLIGHT_OFF = "off";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setBackLight and LCDClient::setBackLight.
 *
 *  Used with LCDScreen::setBackLight and LCDClient::setBackLight to let the
 *  other elements control the LCD backlight.
 */
const std::string LCD_BACKLIGHT_OPEN = "open";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setBackLight and LCDClient::setBackLight.
 *
 *  Used with LCDScreen::setBackLight and LCDClient::setBackLight to toggle
 *  the LCD backlight.
 */
const std::string LCD_BACKLIGHT_TOGGLE = "toggle";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setBackLight and LCDClient::setBackLight.
 *
 *  Used with LCDScreen::setBackLight and LCDClient::setBackLight to make the
 *  LCD backlight blink slowly.
 */
const std::string LCD_BACKLIGHT_BLINK = "blink";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setBackLight and LCDClient::setBackLight.
 *
 *  Used with LCDScreen::setBackLight and LCDClient::setBackLight to make the
 *  LCD backlight blink quickly.
 */
const std::string LCD_BACKLIGHT_FLASH = "flash";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setPriority.
 *
 *  Used with LCDScreen::setPriority to set to "hidden" priority of this
 *  screen. It will then never be shown.
 */
const std::string LCD_PRIORITY_HIDDEN = "hidden";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setPriority.
 *
 *  Used with LCDScreen::setPriority to set to "background" priority of this
 *  screen. It will only be shown if no normal screen exist.
 */
const std::string LCD_PRIORITY_BACKGROUND = "background";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setPriority.
 *
 *  Used with LCDScreen::setPriority to set to "info" priority of this
 *  screen. It is the default priority.
 */
const std::string LCD_PRIORITY_INFO = "info";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setPriority.
 *
 *  Used with LCDScreen::setPriority to set to "foreground" priority of this
 *  screen. The screen is considered as the active one.
 */
const std::string LCD_PRIORITY_FOREGROUND = "foreground";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setPriority.
 *
 *  Used with LCDScreen::setPriority to set to "alert" priority of this
 *  screen. Used to display important messages.
 */
const std::string LCD_PRIORITY_ALERT = "alert";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setPriority.
 *
 *  Used with LCDScreen::setPriority to set to "input" priority of this
 *  screen. For screens waiting for user input.
 */
const std::string LCD_PRIORITY_INPUT = "input";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setCursor.
 *
 *  Used with LCDScreen::setCursor to make the cursor visible.
 */
const std::string LCD_CURSOR_ON = "on";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setCursor.
 *
 *  Used with LCDScreen::setCursor to make the cursor invisible.
 */
const std::string LCD_CURSOR_OFF = "off";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setCursor.
 *
 *  Used with LCDScreen::setCursor to make the cursor look as an underline
 *  character (if available on hardware).
 */
const std::string LCD_CURSOR_UNDER = "under";

/**
 *  \ingroup screen_constants
 *  \brief For LCDScreen::setCursor.
 *
 *  Used with LCDScreen::setCursor to make the cursor look as a block
 *  character (if available on hardware).
 */
const std::string LCD_CURSOR_BLOCK = "block";

/**
 *  \defgroup menu_constants Menu constants
 *  \ingroup constants
 *  Constants used with menus
 */

/**
 *  \ingroup menu_constants
 *  \brief Select menu event.
 *
 *  Select menu event, on an action menu item.
 */
const std::string LCD_MENU_EVENT_SELECT = "select";

/**
 *  \ingroup menu_constants
 *  \brief Update menu event.
 *
 *  Update menu event, on a checkbox, ring, numeric or alpha menu item.
 */
const std::string LCD_MENU_EVENT_UPDATE = "update";

/**
 *  \ingroup menu_constants
 *  \brief Plus menu event.
 *
 *  Plus menu event, on a slider menu item.
 */
const std::string LCD_MENU_EVENT_PLUS = "plus";

/**
 *  \ingroup menu_constants
 *  \brief Minus menu event.
 *
 *  Minus menu event, on a slider menu item.
 */
const std::string LCD_MENU_EVENT_MINUS = "minus";

/**
 *  \ingroup menu_constants
 *  \brief Enter menu event.
 *
 *  Enter menu event, on a menu item.
 */
const std::string LCD_MENU_EVENT_ENTER = "enter";

/**
 *  \ingroup menu_constants
 *  \brief Leave menu event.
 *
 *  Leave menu event, on a menu item.
 */
const std::string LCD_MENU_EVENT_LEAVE = "leave";

/* Icons */
const std::string LCD_ICON_BLOCK_FILLED = "BLOCK_FILLED";
const std::string LCD_ICON_HEART_OPEN = "HEART_OPEN";
const std::string LCD_ICON_HEART_FILLED = "HEART_FILLED";
const std::string LCD_ICON_ARROW_UP = "ARROW_UP";
const std::string LCD_ICON_ARROW_DOWN = "ARROW_DOWN";
const std::string LCD_ICON_ARROW_LEFT = "ARROW_LEFT";
const std::string LCD_ICON_ARROW_RIGHT = "ARROW_RIGHT";
const std::string LCD_ICON_CHECKBOX_OFF = "CHECKBOX_OFF";
const std::string LCD_ICON_CHECKBOX_ON = "CHECKBOX_ON";
const std::string LCD_ICON_CHECKBOX_GRAY = "CHECKBOX_GRAY";
const std::string LCD_ICON_SELECTOR_AT_LEFT = "SELECTOR_AT_LEFT";
const std::string LCD_ICON_SELECTOR_AT_RIGHT = "SELECTOR_AT_RIGHT";
const std::string LCD_ICON_ELLIPSIS = "ELLIPSIS";
const std::string LCD_ICON_PAUSE = "PAUSE";
const std::string LCD_ICON_PLAY = "PLAY";
const std::string LCD_ICON_PLAYR = "PLAYR";
const std::string LCD_ICON_FF = "FF";
const std::string LCD_ICON_FR = "FR";
const std::string LCD_ICON_NEXT = "NEXT";
const std::string LCD_ICON_PREV = "PREV";
const std::string LCD_ICON_REC = "REC";

/* Error messages */
const std::string LCD_SOCKET_CREATION_ERROR = "Error during socket creation";
const std::string LCD_SOCKET_NOT_SUPPORTED = "Network type not supported";
const std::string LCD_SOCKET_CONNECTION_ERROR =
  "Error during socket connection";
const std::string LCD_SOCKET_SEND_ERROR = "Error sending data";
const std::string LCD_SOCKET_RECV_ERROR = "Error receiving data";
const std::string LCD_SOCKET_NOT_CONNECTED = "Socket not connected";

const int LCD_BIG_NUMBER_SEPARATOR = 10;
} // end of lcdapi namespace

#endif
