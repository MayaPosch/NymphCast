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

#include <lcdapi/api/LCDWidget.h>

#include <sstream>

namespace lcdapi {

using namespace std;

LCDWidget::LCDWidget(const string &id, LCDElement *parent, const string &widgetType) : LCDElement(id, "widget_add", widgetType, parent), _x(1), _y(1), _widgetType(widgetType)
{
  _elementDel = "widget_del";
}

void LCDWidget::move(int x, int y)
{
  if (_x != x || _y != y) {
    _x = x;
    _y = y;
    notifyChanged();
  }
}

void LCDWidget::setWidgetParameters(const std::string &properties)
{
  sendCommand("widget_set", properties);
}

void LCDWidget::set(const ParameterList &pList)
{
  ostringstream params;
  for (ParameterList::const_iterator it = pList.begin();
       it != pList.end(); ++it)
  {
    params << *it << " ";
  }

  setWidgetParameters(params.str());
}

} // end of lcdapi namespace
