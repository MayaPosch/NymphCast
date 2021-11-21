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

#ifndef _LCDAPI_API_LCDBAR_H_
#define _LCDAPI_API_LCDBAR_H_

#include <lcdapi/api/LCDWidget.h>
#include <string>

namespace lcdapi {

/** \class LCDBar LCDBar.h "api/LCDBar.h"
 *  \brief A widget to display a bar.
 *  \ingroup widgets
 * This widget is used as a base class for horizontal and vertical bars.
 */
class LCDBar : public LCDWidget
{
 protected:
  int _length;
  int _max;
 public:
  LCDBar(const std::string &widgetType, const std::string &id, LCDElement *parent);
  LCDBar(const std::string &widgetType, int length, int x, int y, const std::string &id, LCDElement *parent);

  virtual void notifyChanged();
  /**
   * \brief Set the values for the widget.
   *
   * Set or change the useful values for this widget.
   * @param length The length of the bar in pixels.
   * @param x Integer containing 1-based value for column number.
   * @param y Integer containing 1-based value for row number.
   */
  void set(int length, int x=1, int y=1);

  /**
   * \brief Set a percentage value.
   *
   * Set or change the percentage value (0-100).
   *  - The 0 value will be represented by a bar with a 0 length.
   *  - The 100 value will be represented by a bar which length is the one specified by setPercentageMax.
   * @param value The percentage value.
   */
  void setPercentage(int value);

  /**
   * \brief Get the percentage value.
   *
   * Get the percentage value (0-100).
   * @return The percentage value.
   */
  int getPercentage() const;

  /**
   * \brief Set the maximum percentage value.
   *
   * Set or change the maximum percentage value.
   * It is the length for a bar with a 100% percentage.
   * @param max The maximum value in pixels.
   */
  void setPercentageMax(int max);

  virtual void valueCallback(const std::string& value);
};

} // end of lcdapi namespace

#endif
