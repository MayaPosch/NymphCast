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

#ifndef _LCDAPI_API_LCDICON_H_
#define _LCDAPI_API_LCDICON_H_

#include <lcdapi/api/LCDWidget.h>
#include <string>

namespace lcdapi {

/** \class LCDIcon LCDIcon.h "api/LCDIcon.h"
 *  \brief A widget to display a predefined icon.
 *  \ingroup widgets
 * This widget is used to display a predefined icon somewhere on screen.
 */
class LCDIcon : public LCDWidget
{
 private:
  std::string _type;
 public:

  /**
   * \brief Default Constructor.
   *
   * This constructor can be used without parameter in most cases.
   * But the widget will have to be added to a parent (screen or frame).
   * @param parent A pointer to parent of this screen. It should be a LCDClient object.
   * @param id A string with the identifier for the screen. If not provided, a unique one will be generated automatically.
   */
  explicit LCDIcon(LCDElement *parent = 0, const std::string &id = "");
  /**
   * \brief Constructor with widget values specified.
   *
   * This construct can be used to specify the values for the widget while building it.
   * @param type A string containing the name of the icon.
   * @param x Integer containing 1-based value for column number.
   * @param y Integer containing 1-based value for row number.
   * @param parent A pointer to parent of this screen. It should be a LCDClient object.
   * @param id A string with the identifier for the screen. If not provided, a unique one will be generated automatically.
   */
  explicit LCDIcon(const std::string &type, int x=1, int y=1, LCDElement *parent = 0, const std::string &id = "");

  /**
   * \brief Set the values for the widget.
   *
   * Set or change the useful values for this widget.
   * @param type A string containing the name of the icon.
   * @param x Integer containing 1-based value for column number.
   * @param y Integer containing 1-based value for row number.
   */
  void set(const std::string &type, int x=1, int y=1);

  /**
   * \brief Set the displayed icon.
   *
   * Set or change the icon displayed on the LCD by this widget.
   * @param type A string containing the icon name.
   */
  void setIcon(const std::string &type);

  /**
   * \brief Get the displayed icon.
   *
   * Get the icon displayed on the LCD by this widget.
   * @return A string containing the icon name.
   */
  const std::string& getIcon() const;

  virtual void valueCallback(const std::string& value);

  virtual void notifyChanged();
};

} // end of lcdapi namespace

#endif
