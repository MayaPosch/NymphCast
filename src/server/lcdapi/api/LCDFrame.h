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

#ifndef _LCDAPI_API_LCDFRAME_H_
#define _LCDAPI_API_LCDFRAME_H_

#include <lcdapi/api/LCDWidget.h>
#include <string>

namespace lcdapi {

/** \class LCDFrame LCDFrame.h "api/LCDFrame.h"
 *  \brief A widget to create a frame.
 *  \ingroup widgets
 * This widget is a container for other widgets.
 * They can be added in it as they are in a screen.
 */
class LCDFrame : public LCDWidget {
 private:
    int _left, _top, _right, _bottom, _width, _height, _speed;
    Direction _direction;
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
  explicit LCDFrame(LCDElement *parent = 0, const std::string &id = "");

  void sendCommand(const std::string &cmd, const std::string &parameters);

  virtual void notifyChanged();
  virtual void valueCallback(const std::string& value);

  /**
   * \brief Set the values for the widget.
   *
   * Set or change the useful values for this widget.
   * @param left Integer containing 1-based left position of the widget.
   * @param top Integer containing 1-based top position of the widget.
   * @param right Integer containing 1-based right position of the widget.
   * @param bottom Integer containing 1-based bottom position of the widget.
   * @param width Integer containing width for this widget (in characters).
   * @param height Integer containing height for this widget (in characters).
   * @param speed Number of movements per rendering stroke (8 times/second).
   * @param direction Can be 'h' for Horizontal, 'v' for Vertical or 'm' for
   * Marquee.
   */
  void set(int left, int top, int right, int bottom,
           int width, int height,
           Direction direction, int speed);
};

} // end of lcdapi namespace

#endif
