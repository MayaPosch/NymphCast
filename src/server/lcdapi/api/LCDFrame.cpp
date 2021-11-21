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

#include <lcdapi/api/LCDFrame.h>
#include <sstream>

namespace lcdapi {

using namespace std;

LCDFrame::LCDFrame(LCDElement *parent, const string &id) : LCDWidget(id, parent, "frame"),
                                                           _left(0), _top(0), _right(0), _bottom(0), _width(0), _height(0), _speed(0), _direction(Horizontal)
{
}

void LCDFrame::set(int left, int top, int right, int bottom, int width, int height, Direction direction, int speed)
{
  if (_left != left ||
      _top != top ||
      _right != right ||
      _bottom != bottom ||
      _width != width ||
      _height != height ||
      _direction != direction ||
      _speed != speed) {
    _left = left;
    _top = top;
    _right = right;
    _bottom = bottom;
    _width = width;
    _height = height;
    _direction = direction;
    _speed = speed;

    notifyChanged();
  }
}
void LCDFrame::notifyChanged()
{
  ostringstream params;

  params << _left
         << " "
         << _top
         << " "
         << _right
         << " "
         << _bottom
         << " "
         << _width
         << " "
         << _height
         << " "
         << static_cast<char>(_direction)
         << " "
         << _speed;

  setWidgetParameters(params.str());
}

void LCDFrame::valueCallback(const std::string& value __attribute__((unused)))
{
}

void LCDFrame::sendCommand(const std::string &cmd, const std::string &parameters)
{
  if (_parent)
  {
    string realParams;
    if (cmd == "widget_add")
    {
      if (parameters == "frame")
      {
        realParams =  _id + " " + parameters;
      }
      else
      {
        realParams = parameters + " " + "-in " + _id;
      }
    }
    else
    {
      realParams = parameters;
    }
    _parent->sendCommand(cmd, realParams);
  }
  else
  {
    _commandBuffer.push_back(Command(cmd, parameters));
  }
}

} // end of lcdapi namespace
