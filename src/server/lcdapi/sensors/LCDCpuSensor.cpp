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

#include <lcdapi/sensors/LCDCpuSensor.h>

#include <fstream>
#include <unistd.h>

namespace lcdapi {

using namespace std;

LCDCpuSensor::LCDCpuSensor(const string& cpuName)
  : LCDSensor(),
    _cpuName(cpuName),
    _userTicks(0),
    _sysTicks(0),
    _niceTicks(0),
    _idleTicks(0),
    _load(0)
{
}

LCDCpuSensor::~LCDCpuSensor() {
}

void LCDCpuSensor::waitForChange() {
  sleep(1);
}

string LCDCpuSensor::getCurrentValue() {
  long uTicks, sTicks, nTicks, iTicks;

  getTicks(uTicks, sTicks, nTicks, iTicks);

  const long totalTicks = (uTicks - _userTicks) +
                           (sTicks - _sysTicks) +
                           (nTicks - _niceTicks) +
                           (iTicks - _idleTicks);

  const int load  = (totalTicks == 0) ? 0 : int( 100.0 * ( (uTicks+sTicks+nTicks) - (_userTicks+_sysTicks+_niceTicks))/( totalTicks+0.001) + 0.5 );

  _userTicks = uTicks;
  _sysTicks = sTicks;
  _niceTicks = nTicks;
  _idleTicks = iTicks;

  return intToString(load);
}

void LCDCpuSensor::getTicks(long &u, long &s, long &n, long &i) const {
    fstream file;
    string item;

    file.open("/proc/stat", ios::in);

    if (file.is_open()) {
      while (item != _cpuName) {
        file >> item;
      }
      file >> u >> s >> n >> i;

      file.close();
    } else {
        u = 0;
        s = 0;
        n = 0;
        i = 0;
    }
}

} // end of lcdapi namespace
