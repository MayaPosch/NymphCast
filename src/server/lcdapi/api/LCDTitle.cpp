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

#include <lcdapi/api/LCDTitle.h>
#include <sstream>

namespace lcdapi {

using namespace std;

LCDTitle::LCDTitle(LCDElement *parent, const string &id)
  : LCDWidget(id, parent, "title"), _text()
{
}

LCDTitle::LCDTitle(const string &text, LCDElement *parent, const string &id)
  : LCDWidget(id, parent, "title"), _text(text)
{
  set(text);
}

void LCDTitle::notifyChanged() {
  ostringstream params;

  params << '"'
         << _text
         << '"';

  setWidgetParameters(params.str());
}

void LCDTitle::set(const string& text) {
  if (_text != text) {
    _text = text;
    notifyChanged();
  }
}

const string& LCDTitle::get() const {
  return _text;
}

void LCDTitle::valueCallback(const std::string& value) {
  set(value);
}

} // end of lcdapi namespace
