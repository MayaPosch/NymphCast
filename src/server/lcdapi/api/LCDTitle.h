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

#ifndef _LCDAPI_API_LCDTITLE_H_
#define _LCDAPI_API_LCDTITLE_H_

#include <lcdapi/api/LCDWidget.h>
#include <string>

namespace lcdapi {

/** \class LCDTitle LCDTitle.h "api/LCDTitle.h"
 *  \brief A widget to display a title.
 *  \ingroup widgets
 * This widget is used to display a title on first line of the LCD.
 */
class LCDTitle : public LCDWidget {
 private:
  std::string _text;
 public:
  /**
   * \brief Default Constructor.
   *
   * This constructor can be used without parameter in most cases.
   * But the widget will have to be added to a parent (screen or frame).
   * @param parent A pointer to parent of this screen. It should be a
   * LCDClient object.
   * @param id A string with the identifier for the screen. If not provided,
   * a unique one will be generated automatically.
   */
  explicit LCDTitle(LCDElement *parent = 0, const std::string &id = "");
  /**
   * \brief Constructor with widget values specified.
   *
   * This construct can be used to specify the values for the widget while
   * building it.
   * @param text A string containing the text that will be displayed.
   * @param parent A pointer to parent of this screen. It should be a
   * LCDClient object.
   * @param id A string with the identifier for the screen. If not provided,
   * a unique one will be generated automatically.
   */
  explicit LCDTitle(const std::string &text, LCDElement *parent = 0, const std::string &id = "");

  virtual void notifyChanged();
  virtual void valueCallback(const std::string& value);

  /**
   * \brief Set the text for the widget.
   *
   * Set or change the text used for the title.
   * @param text A string containing the title that will be displayed.
   */

  void set(const std::string& text);
  /**
   * \brief Get the text for the widget.
   *
   * G the text used for the title.
   * @return A string containing the title that is displayed.
   */
  const std::string& get() const;
};

} // end of lcdapi namespace

#endif
