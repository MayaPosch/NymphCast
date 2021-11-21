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

#ifndef _LCDAPI_API_LCDVERTICALBAR_H_
#define _LCDAPI_API_LCDVERTICALBAR_H_

#include <lcdapi/api/LCDBar.h>
#include <string>

namespace lcdapi {

/** \class LCDVerticalBar LCDVerticalBar.h "api/LCDVerticalBar.h"
 *  \brief A widget to display a vertical bar.
 *  \ingroup widgets
 * This widget is used to display a vertical bar somewhere on screen.
 */
class LCDVerticalBar : public LCDBar {
 public:
  /**
   * \brief Default Constructor.
   *
   * This constructor can be used without parameter in most cases.
   * But the widget will have to be added to a parent (screen or frame).
   * @param parent A pointer to parent of this screen. It should be a
   * LCDClient object.
   * @param id A string with the identifier for the screen. If not provided,
   * a unique one will be generated automatically.
   */
  explicit LCDVerticalBar(LCDElement *parent = 0, const std::string &id = "");

  /**
   * \brief Constructor with widget values specified.
   *
   * This construct can be used to specify the values for the widget while
   * building it.
   * @param length The length of the bar in pixels.
   * @param x Integer containing 1-based value for column number.
   * @param y Integer containing 1-based value for row number.
   * @param parent A pointer to parent of this screen. It should be a
   * LCDClient object.
   * @param id A string with the identifier for the screen. If not provided,
   * a unique one will be generated automatically.
   */
  explicit LCDVerticalBar(int length, int x=1, int y=1,
                          LCDElement *parent = 0, const std::string &id = "");
};

} // end of lcdapi namespace

#endif
