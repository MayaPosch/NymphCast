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

#ifndef _LCDAPI_API_LCDSCROLLER_H_
#define _LCDAPI_API_LCDSCROLLER_H_

#include <lcdapi/api/LCDWidget.h>
#include <string>

namespace lcdapi {

/** \class LCDScroller LCDScroller.h "api/LCDScroller.h"
 *  \brief A widget to display scrolling text.
 *  \ingroup widgets
 * This widget is used to display some text that will scroll on the screen.
 */
class LCDScroller : public LCDWidget {
 private:
  std::string _text;
  int _right, _bottom, _speed;
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
  explicit LCDScroller(LCDElement *parent = 0, const std::string &id = "");

  virtual void notifyChanged();
  virtual void valueCallback(const std::string& value);

  /**
   * \brief Set the values for the widget.
   *
   * Set or change the useful values for this widget.
   * @param text A string containing the text that will be displayed.
   * @param left Integer containing 1-based left position of the widget.
   * @param top Integer containing 1-based top position of the widget.
   * @param right Integer containing 1-based right position of the widget.
   * @param bottom Integer containing 1-based bottom position of the widget.
   * @param speed Number of movements per rendering stroke (8 times/second).
   * @param direction Can be 'h' for Horizontal, 'v' for Vertical or 'm' for Marquee.
   */
  void set(const std::string &text,
           int left = 1, int top = 1, int right = 10, int bottom = 1,
           int speed = 8, Direction direction = Horizontal);

  /**
   * \brief Set the displayed text.
   *
   * Set or change the text displayed on the LCD by this widget.
   * @param text A string containing the text that will be displayed.
   */
  void setText(const std::string &text);

  /**
   * \brief Get the displayed text.
   *
   * Get the text displayed on the LCD by this widget.
   * @return A string containing the text that is displayed.
   */
  const std::string& getText() const;

  /**
   * \brief Set the width of the scroller.
   *
   * Set or change the width of the widget.
   * @param width The new width to be used (in characters).
   */
  void setWidth(int width);

  /**
   * \brief Set the height of the scroller.
   *
   * Set or change the height of the widget.
   * @param height The new height to be used (in characters).
   */
  void setHeight(int height);

  /**
   * \brief Set the speed of the scroller.
   *
   * Set or change the speed of the scrolling.
   * @param speed The new speed to be used . Is is a number of movements per
   * rendering stroke (8 times/second).
   */
  void setSpeed(int speed);
};

} // end of lcdapi namespace

#endif
