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

#include <lcdapi/api/LCDIcon.h>
#include <sstream>

namespace lcdapi {

using namespace std;

LCDIcon::LCDIcon(LCDElement *parent, const string &id) : LCDWidget(id, parent, "icon"), _type()
{
}

LCDIcon::LCDIcon(const string &type, int x, int y, LCDElement *parent, const string &id) : LCDWidget(id, parent, "string"), _type(type)
{
  set(type, x, y);
}

void LCDIcon::set(const string &type, int x, int y)
{
  if (_type != type ||
      _x != x ||
      _y != y) {
    _type = type;
    _x = x;
    _y = y;
    notifyChanged();
  }
}

void LCDIcon::valueCallback(const std::string& value __attribute__((unused)))
{
}

void LCDIcon::notifyChanged()
{
  ostringstream params;

  params << _x
         << " "
         << _y
         << " "
         << _type;

  setWidgetParameters(params.str());
}

void LCDIcon::setIcon(const string &type)
{
  set(type, _x, _y);
}

const string& LCDIcon::getIcon() const
{
  return _type;
}

} // end of lcdapi namespace
