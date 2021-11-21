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

#include <lcdapi/api/LCDBar.h>
#include <cstdlib>
#include <sstream>

namespace lcdapi {

using namespace std;

LCDBar::LCDBar(const string &widgetType, const string &id, LCDElement *parent) : LCDWidget(id, parent, widgetType), _length(0), _max(100)
{
}

LCDBar::LCDBar(const string &widgetType, int length, int x, int y, const string &id, LCDElement *parent) : LCDWidget(id, parent, widgetType), _length(length), _max(100)
{
  set(length, x, y);
}

void LCDBar::notifyChanged()
{
  ostringstream params;

  params << _x
         << " "
         << _y
         << " "
         << _length;

  setWidgetParameters(params.str());
}

void LCDBar::set(int length, int x, int y)
{
  if (_length != length || _x != x || _y != y) {
    _length = length;
    _x = x;
    _y =y;
    notifyChanged();
  }
}

void LCDBar::setPercentage(int value)
{
  const int new_length = (value * _max) / 100;
  set(new_length, _x, _y);
}

int LCDBar::getPercentage() const
{
  return ((_length * 100) / _max);

}

void LCDBar::setPercentageMax(int max)
{
  _max = max;
}

void LCDBar::valueCallback(const string& value)
{
  /// \todo Use strtol(), and handle parse errors
  const int parsed_value = atoi(value.c_str());
  setPercentage(parsed_value);
}

} // end of lcdapi namespace
