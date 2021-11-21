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

#ifndef _LCDAPI_API_LCDSCREEN_H_
#define _LCDAPI_API_LCDSCREEN_H_

#include <lcdapi/include/LCDConstants.h>
#include <lcdapi/api/LCDElement.h>

namespace lcdapi {

/** \class LCDScreen LCDScreen.h "api/LCDScreen.h"
 *  \brief This is the class that creates a new screen on the display.
 *  \ingroup main
 *
 *  With this class, you can create a new screen. It will contain some widgets.
 *  You can create as many screen as you want. The server will rotate them on
 * the display.
 */
class LCDScreen : public LCDElement {
 protected:

  template <typename T> void setScreenOption(const std::string& optName, T value);

 public:
  static std::string valueToString(const std::string& value);
  static std::string valueToString(int value);

  /**
   * \brief Default Constructor.
   *
   * This constructor can be used without parameter in most cases.
   * But the screen will have to be added to a client.
   * @param parent A pointer to parent of this screen. It should be a
   * LCDClient object.
   * @param name A string containing the name for this screen.
   * @param id A string with the identifier for the screen. If not provided,
   * a unique one will be generated automatically.
   */
  explicit LCDScreen(LCDElement *parent = 0, const std::string& name = "", const std::string& id = "");

  /**
   * \brief Change the name of the screen.
   *
   * Sets the screen's name as visible to a user.
   * @param name A string containing the new name.
   */
  void setName(const std::string& name);

  /**
   * \brief Change the width of the screen.
   *
   * Sets the width of the screen in characters. If unset, the full display
   * size is assumed.
   * @param width An integer containing the new number of characters to be
   * used as width.
   */
  void setWidth(int width);

  /**
   * \brief Change the height of the screen.
   *
   * Sets the height of the screen in characters. If unset, the full display
   * size is assumed.
   * @param height An integer containing the new number of characters to be
   * used as height.
   */
  void setHeight(int height);

  /**
   * \brief Change the priority of the screen.
   *
   * Sets priority of the screen. Only screens with highest priority at a
   * moment will be shown.
   * @param priority A string containing the priority class. These constants
   * can be used: \ref LCD_PRIORITY_HIDDEN, \ref LCD_PRIORITY_BACKGROUND,
   * \ref LCD_PRIORITY_INFO, \ref LCD_PRIORITY_FOREGROUND,
   * \ref LCD_PRIORITY_ALERT, \ref LCD_PRIORITY_INPUT.
   */
  void setPriority(const std::string& priority);

  /**
   * \brief Enable or disable heart beat.
   *
   * Enable or disable the heart beat used by LCDproc to show data
   * transmission.
   * @param heartbeat A string with the value to set. Constants
   * \ref LCD_HEARTBEAT_ON, \ref LCD_HEARTBEAT_OFF and
   * \ref LCD_HEARTBEAT_OPEN can be used.
   */
  void setHeartBeat(const std::string& heartbeat);

  /**
   * \brief Enable or disable screen backlight.
   *
   * Enable or disable the backlight for the current screen.
   * @param backlight A string with the value to set. Constants
   * \ref LCD_BACKLIGHT_ON, \ref LCD_BACKLIGHT_OFF, \ref LCD_BACKLIGHT_OPEN,
   * \ref LCD_BACKLIGHT_TOGGLE, \ref LCD_BACKLIGHT_BLINK and
   * \ref LCD_BACKLIGHT_FLASH can be used.
   */
  void setBackLight(const std::string& backlight);

  /**
   * \brief Set the time the screen is displayed during each rotation.
   *
   * Change the time the screen will be visible during each rotation.
   * @param secondEights The time in eights of a second to display the screen.
   */
  void setDuration(int secondEights);

  /**
   * \brief Set the time to live of the screen.
   *
   * Change the time to live of the screen. After the screen has been visible
   * for a total of this amount of time, it will be deleted
   * @param secondEights The time in eights of a second for the time to live.
   */
  void setTimeOut(int secondEights);

  /**
   * \brief Set the cursor state.
   *
   * Change the way the cursor is displayed.
   * @param cursor A string with the value to set. Constants
   * \ref LCD_CURSOR_ON, \ref LCD_CURSOR_OFF,\ref  LCD_CURSOR_UNDER and
   * \ref LCD_CURSOR_BLOCK can be used.
   */
  void setCursor(const std::string& cursor);

  /**
   * \brief Set the cursor position on screen.
   *
   * Move the cursor to a point specified with its coordinates. They are
   * always 1-based, wich means that top-left is (1,1).
   * @param x Integer containing 1-based value for column number.
   * @param y Integer containing 1-based value for row number.
   */
  void setCursorPosition(int x, int y);

  /**
   * \brief Set the x coordinate of the cursor.
   *
   * Change only the x coordinate of the cursor position on screen.
   * \see setCursorPosition
   */
  void setCursorX(int x);

  /**
   * \brief Set the y coordinate of the cursor.
   *
   * Change only the y coordinate of the cursor position on screen.
   * \see setCursorPosition
   */
  void setCursorY(int y);
};

} // end of lcdapi namespace

#endif
