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

#ifndef _LCDAPI_SENSORS_LCDTIMESENSOR_H_
#define _LCDAPI_SENSORS_LCDTIMESENSOR_H_

#include <lcdapi/sensors/LCDSystemSensor.h>
#include <string>

namespace lcdapi {

const std::string LCD_TIME_HOUR_MINUTE = "+%H:%M";
const std::string LCD_TIME_HOUR_MINUTE_SECOND = "+%H:%M:%S";

/** \class LCDTimeSensor LCDTimeSensor.h "api/LCDTimeSensor.h"
 *  \brief A sensor for current time.
 *  \ingroup sensors
 *  This sensor will return the current time in specified format.
 *  The format is the one used by date(1)
 */
class LCDTimeSensor : public LCDSystemSensor {
 public:

  /**
   * \brief Default constructor.
   *
   * Used to build such a sensor.
   * @param format A string containing the format to use, as defined by
   * date(1). The default value makes this sensor return current hour and
   * minute seperated by a colon.
   */
  explicit LCDTimeSensor(const std::string &format = LCD_TIME_HOUR_MINUTE);
  virtual ~LCDTimeSensor();
};

} // end of lcdapi namespace

#endif
