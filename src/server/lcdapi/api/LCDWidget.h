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

#ifndef _LCDAPI_API_LCDWIDGET_H_
#define _LCDAPI_API_LCDWIDGET_H_

/**
 * \defgroup widgets Widgets
 *  The widgets are the graphical components that can be displayed
 *   on the LCD. Each of them correspond to a different element,
 *   so they have different properties that can be set. They share some
 *   methods, defined in the LCDWidget class.
 */

#include <lcdapi/api/LCDElement.h>
#include <list>
#include <string>

namespace lcdapi {

/** \class LCDWidget LCDWidget.h "api/LCDWidget.h"
 *  \brief Main class for all widgets of the API.
 *  \ingroup widgets
 *  All the widgets in this API have LCDWidget as their base class.
 *   It contains common methods. But it should not be used directly.
 */
class LCDWidget : public LCDElement {
 protected:
  int _x, _y;

  LCDWidget(const std::string &id, LCDElement *parent, const std::string &widgetType);

  std::string _widgetType;

  void setWidgetParameters(const std::string &properties);

 public:
  enum Direction {
    Horizontal = 'h',
    Vertical = 'v',
    Marquee = 'm'
  };

  /**
   * \brief A list of parameters for set method.
   *
   * This is a list containing strings. Each string corresponds to a parameter.
   */
  typedef std::list<std::string> ParameterList;

  virtual void notifyChanged() = 0;

  /**
   * \brief Move the widget to a new location.
   *
   * Change the coordinate of a widget.
   * Even if all the widgets have this method, it won't have an effect on all
   * of them.
   * @param x Integer containing 1-based value for column number.
   * @param y Integer containing 1-based value for row number.
   * \see LCDScreen::setCursorPosition
   */
  void move(int x, int y = 1);

  /**
   * \brief Generic method to set widget parameter.
   *
   * This method can be used to set all parameters of a widget.
   * You should know the ones that are used by the protocol to put all of
   * them in correct order.
   * @param pList A list of all the parameters values.
   */
  void set(const ParameterList &pList);

  virtual void valueCallback(const std::string& value) = 0;
};

} // end of lcdapi namespace

#endif
