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
   see <http://www.gnu.org/licenses/>.
*/

#ifndef _LCDAPI_KEYS_LCDKEYEVENT_H_
#define _LCDAPI_KEYS_LCDKEYEVENT_H_

#include <string>

namespace lcdapi {

/** \class KeyEvent LCDKeyEvent.h "keys/LCDKeyEvent.h"
 *  \brief The type used to define a key event.
 *  \ingroup keys
 *  To create a new key handler, one should create a derivated class
 *  from this one and implement operator ().
 */
class KeyEvent {
public:
  /** Default constructor */
  KeyEvent() : m_key() {
  }

  /** Constructor */
  KeyEvent(char key) : m_key(1, key) {
  }

  /** Constructor */
  KeyEvent(const std::string &key) : m_key(key) {
  }

  /** Destructor */
  ~KeyEvent() {
  }

  /** Copy constructor */
  KeyEvent(const KeyEvent &original) : m_key(original.get_key()) {
  }

  /** Copy assignment operator */
  KeyEvent &operator=(const KeyEvent &rhs) {
    m_key = rhs.get_key();
    return *this;
  }

  /** Get the underlying key string */
  const std::string &get_key() const {
    return m_key;
  }

  /** Cast to char, with loss of information if underlying
   * key string has length greater than one */
  operator char() const {
    return m_key[0];
  }

private:
  /** The underlying key string */
  std::string m_key;
};

} // end of lcdapi namespace

#endif
