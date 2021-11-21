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

#ifndef _LCDAPI_SENSORS_LCDSENSOR_H_
#define _LCDAPI_SENSORS_LCDSENSOR_H_

/**
 * \defgroup sensors Sensors
 *  Sensors are components that can automatically
 *  update a widget with values from external world
 *  (including OS, hardware,...)
 *
 *  Some are predefined there. To create a new one,
 *   the LCDWidget class must be used as a base class.
 *  The class must provide
 *  \ref LCDSensor::waitForChange "waitForChange" and
 *  \ref LCDSensor::getCurrentValue "getCurrentValue" methods.
 *
 *  To assign a sensor to a widget, there are two methods:
 *   \ref LCDSensor::addOnChangeWidget "addOnChangeWidget"
 *   and \ref LCDSensor::addOnTimeOutWidget "addOnTimeOutWidget".
 */

#include <map>
#include <string>
#include <pthread.h>

namespace lcdapi {

class LCDWidget;

class LCDWidgetTimeOut {
 public:
  std::string _widgetId;
  int _timeOut;
  LCDWidget *_widget;
  ::pthread_t _thread;

 LCDWidgetTimeOut()
    : _widgetId(), _timeOut(0), _widget(NULL),
    _thread(static_cast<pthread_t>(-1))
  {
  }

  bool isValid() {
    return (_thread != static_cast<pthread_t>(-1));
  }

  const LCDWidgetTimeOut& operator=(const LCDWidgetTimeOut& rhs) {
    if (&rhs != this) {
      _widgetId = rhs._widgetId;
      _timeOut = rhs._timeOut;
      _widget = rhs._widget;
      _thread = rhs._thread;
    }

    return *this;
  }

 LCDWidgetTimeOut(const LCDWidgetTimeOut& original)
   : _widgetId(original._widgetId), _timeOut(original._timeOut),
    _widget(original._widget), _thread(original._thread)
  {
  }
};

/** \class LCDSensor LCDSensor.h "api/LCDSensor.h"
 *  \brief Main class for all sensors of the API.
 *  \ingroup sensors
 *  All the sensors in this API have LCDSensor as their base class.
 *   It should not be used directly but to create new sensors in the API or
 *   in the user application.
 *
 *  A sensor should implement \ref waitForChange and \ref getCurrentValue.
 *  It can use some methods defined here for convenience.
 */
class LCDSensor {
 private:
  bool _exist;
  bool _onChangeThreadStarted;

  typedef void*(*ThreadFunction)(void*);

  typedef std::map<std::string, LCDWidget *> WidgetList;
  typedef std::map<std::string, LCDWidgetTimeOut> WidgetTimeOutList;

  ::pthread_t _onChangeThread;
  WidgetList _onChangeList;
  WidgetTimeOutList _onTimeOutList;

 public:
  static const int MAX_CMD_RESULT_LINE_SIZE;
  LCDSensor();
  virtual ~LCDSensor();
  bool exists();

  const LCDWidgetTimeOut &getThreadWidgetInfo(const ::pthread_t &thread);

  /**
   * \brief Returns when a change occured on the measured value.
   *
   * This virtual function should be implemented by a real sensor.
   * It should wait for the value to change and then just return.
   */
  virtual void waitForChange() = 0;

  /**
   * \brief Get the current value measured by the sensor.
   *
   * This virtual function should be implemented by a real sensor.
   * It should return the current value as a string.
   * @return Current value of the sensor.
   */
  virtual std::string getCurrentValue() = 0;

  /**
   * \brief Convert an integer to a string.
   *
   * Utility function to convert an integer to a string so it can be returned
   * by \ref getCurrentValue.
   * It should return the current value as a string.
   * @param value The integer value to convert.
   * @return String version of the value.
   */
  std::string intToString(int value);

  /**
   * \brief Execute an external command.
   *
   * This method can be used to invoke another program and get its output.
   * Only the first line is returned.
   * @param cmd The command to execute as if a shell is used.
   * @return A string containing the first line of the command output.
   */
  std::string executeCommand(const std::string &cmd);

  void fireChanged();

  /**
   * \brief Add a new widget that must be set when the sensor value changes.
   *
   * This method is used to associate a wiget to a sensor.
   * When the sensor value change, the widget value will be changed
   * accordingly.
   * @param widget The widget to associate with this sensor.
   */
  void addOnChangeWidget(LCDWidget *widget);

  /**
   * \brief Add a new widget that must be changed regularly.
   *
   * This method is used to associate a wiget to a sensor.
   * Each time the specified timeout ellapsed, widget value is changed.
   * @param widget The widget to associate with this sensor.
   * @param timeOut The time to wait between each update (second tenths).
   */
  void addOnTimeOutWidget(LCDWidget *widget, int timeOut);

  /**
   * \brief Remove a widget that was added with \ref addOnChangeWidget.
   *
   * This method is used to delete the association between a wiget and a
   * sensor.
   * @param widget The widget to remove from sensor configuration.
   */
  void removeOnChangeWidget(LCDWidget *widget);

  /**
   * \brief Remove a widget that was added with \ref addOnChangeWidget.
   *
   * This method is used to delete the association between a wiget and a
   * sensor.
   * @param id The identifier of the widget to remove from sensor
   * configuration.
   */
  void removeOnChangeWidget(const std::string& id);

  /**
   * \brief Remove a widget that was added with \ref addOnTimeOutWidget.
   *
   * This method is used to delete the association between a wiget and a
   * sensor.
   * @param widget The widget to remove from sensor configuration.
   */
  void removeOnTimeOutWidget(LCDWidget *widget);

  /**
   * \brief Remove a widget that was added with \ref addOnTimeOutWidget.
   *
   * This method is used to delete the association between a wiget and a
   * sensor.
   * @param id The identifier of the widget to remove from sensor
   * configuration.
   */
  void removeOnTimeOutWidget(const std::string& id);
};

} // end of lcdapi namespace

#endif
