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

#ifndef _LCDAPI_SENSORS_LCDSYSTEMSENSOR_H_
#define _LCDAPI_SENSORS_LCDSYSTEMSENSOR_H_

#include <lcdapi/sensors/LCDSensor.h>
#include <string>

namespace lcdapi {

/** \class LCDSystemSensor LCDSystemSensor.h "api/LCDSystemSensor.h"
 *  \brief A sensor that executes a shell command.
 *  \ingroup sensors
 *  This sensor takes a shell command (that can call external programs) and
 *  returns first line of its output.
 */
class LCDSystemSensor : public LCDSensor {
 private:
    std::string _command;

 public:
  virtual void waitForChange();
  virtual std::string getCurrentValue();

  /**
   * \brief Default constructor.
   *
   * Used to build such a sensor.
   * @param command A string containing the shell command to execute.
   */
  explicit LCDSystemSensor(const std::string &command);
  virtual ~LCDSystemSensor();
};

} // end of lcdapi namespace

#endif
