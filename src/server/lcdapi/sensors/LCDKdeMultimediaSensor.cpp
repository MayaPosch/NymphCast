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

#include <lcdapi/sensors/LCDKdeMultimediaSensor.h>

#include <unistd.h>

namespace lcdapi {

using namespace std;

LCDKdeMultimediaSensor::LCDKdeMultimediaSensor(const std::string& defaultValue)
  : LCDSensor(),
    _previousValue("NO"),
    _defaultValue(defaultValue)
{
}

LCDKdeMultimediaSensor::~LCDKdeMultimediaSensor() {
}

void LCDKdeMultimediaSensor::waitForChange() {
  while (_previousValue == getCurrentValue()) {
    sleep(1);
  }
  _previousValue = getCurrentValue();
}

string LCDKdeMultimediaSensor::getCurrentValue() {
  string value;

  const string noatunId = executeCommand("dcop | grep noatun");

  if (!noatunId.empty()) {
    value = executeCommand("dcop " + noatunId + " Noatun title");
  } else {
    const string kscdTitle =
      executeCommand("dcop kscd CDPlayer currentTrackTitle");
    if (!kscdTitle.empty()) {
      value = kscdTitle;
    }
  }

  if (value.empty()) {
    value = _defaultValue;
  }

  return value;
}

} // end of lcdapi namespace
