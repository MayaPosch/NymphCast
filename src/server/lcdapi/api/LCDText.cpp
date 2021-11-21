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

#include <lcdapi/api/LCDText.h>
#include <sstream>

namespace lcdapi {

using namespace std;

LCDText::LCDText(LCDElement *parent, const string &id)
  : LCDWidget(id, parent, "string"), _text()
{
}

LCDText::LCDText(const string &text, int x, int y, LCDElement *parent, const string &id)
  : LCDWidget(id, parent, "string"), _text(text)
{
  set(text, x, y);
}

void LCDText::notifyChanged() {
  ostringstream params;

  params << _x
         << " "
         << _y
         << " \""
         << _text
         << '"';

  setWidgetParameters(params.str());
}

void LCDText::set(const string &text, int x, int y) {
  if (_x != x || _y != y || _text != text) {
    _x = x;
    _y = y;
    _text = text;
    notifyChanged();
  }
}

void LCDText::setText(const string &text) {
  set(text, _x, _y);
}

const string& LCDText::getText() const {
  return _text;
}

void LCDText::valueCallback(const string& value) {
  setText(value);
}

} // end of lcdapi namespace
