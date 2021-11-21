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

#include <lcdapi/api/LCDScroller.h>
#include <sstream>

namespace lcdapi {

using namespace std;

LCDScroller::LCDScroller(LCDElement *parent, const string &id)
  : LCDWidget(id, parent, "scroller"),
    _text(),
    _right(10),
    _bottom(1),
    _speed(8),
    _direction(Horizontal)
{
  _x = 1;
  _y = 1;
}

void LCDScroller::notifyChanged() {
  ostringstream params;

  params << _x
         << " "
         << _y
         << " "
         << _right
         << " "
         << _bottom
         << " "
         << static_cast<char>(_direction)
         << " "
         << _speed
         << " \""
         << _text
         << '"';

  setWidgetParameters(params.str());
}

void LCDScroller::set(const string &text, int left, int top, int right, int bottom, int speed, Direction direction) {
  if (_text != text ||
      _x != left ||
      _y != top ||
      _right != right ||
      _bottom != bottom ||
      _speed != speed ||
      _direction != direction) {
    _text = text;
    _x = left;
    _y = top;
    _right = right;
    _bottom = bottom;
    _speed = speed;
    _direction = direction;
    notifyChanged();
  }
}

void LCDScroller::setText(const string &text) {
  if (_text != text) {
    _text = text;
    notifyChanged();
  }
}

const string& LCDScroller::getText() const {
  return _text;
}

void LCDScroller::setWidth(int width) {
  const int new_right = _x + width - 1;
  if (_right != new_right) {
    _right = new_right;
    notifyChanged();
  }
}

void LCDScroller::setHeight(int height) {
  const int new_bottom = _y + height - 1;
  if (_bottom != new_bottom) {
    _bottom = new_bottom;
    notifyChanged();
  }
}

void LCDScroller::setSpeed(int speed) {
  if (_speed != speed) {
    _speed = speed;
    notifyChanged();
  }
}

void LCDScroller::valueCallback(const std::string& value) {
  setText(value);
}

} // end of lcdapi namespace
