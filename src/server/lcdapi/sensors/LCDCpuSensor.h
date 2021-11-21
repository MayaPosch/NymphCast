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

#ifndef _LCDAPI_SENSORS_LCDCPUSENSOR_H_
#define _LCDAPI_SENSORS_LCDCPUSENSOR_H_

#include <lcdapi/sensors/LCDSensor.h>
#include <string>

namespace lcdapi {

/** \class LCDCpuSensor LCDCpuSensor.h "api/LCDCpuSensor.h"
 *  \brief A sensor for CPU usage.
 *  \ingroup sensors
 *  This sensor will return the current CPU usage.
 */
class LCDCpuSensor : public LCDSensor {
 private:
  std::string _cpuName;
  long _userTicks;
  long _sysTicks;
  long _niceTicks;
  long _idleTicks;
  int _load;

  void getTicks(long &u, long &s, long &n, long &i) const;

 public:
  virtual void waitForChange();
  virtual std::string getCurrentValue();

  /**
   * \brief Default constructor.
   *
   * Used to build such a sensor.
   * @param cpuName The CPU this sensor should monitor. Use "cpu" for total
   * CPU usage. "cpu0" for first CPU on an SMP machine, "cpu1" for second
   * one, ...
   */
  explicit LCDCpuSensor(const std::string& cpuName = "cpu");
  virtual ~LCDCpuSensor();
};

} // end of lcdapi namespace

#endif
