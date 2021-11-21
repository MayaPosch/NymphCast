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

#ifndef _LCDAPI_KEYS_LCDCALLBACK_H_
#define _LCDAPI_KEYS_LCDCALLBACK_H_

#include <lcdapi/keys/LCDKeyEvent.h>
#include <string>
#include <map>

namespace lcdapi {

/**
 * \defgroup keys Keys handling
 *
 *  To assign a callback to a key event, there is the need to create
 *   a new class inheriting from LCDCallback.
 *
 *  The operator() should then be overriden to do actions. It will
 *   be called when the key event occurs.
 *
 *  To define only a single function, there are some useful macros (see below).
 *
 *  Then the callback is associated to the event
 *   with the method LCDClient::assignKey()
 *
 */

/**
 * \ingroup keys
 * A macro that can be used when only a function is needed
 *  and not a real class.
 * Use it with the name of the pseudo function you want to create
 *  and then you can define your function code (between braces)
 */
#define LCD_CALLBACK_FUNCTION_BEGIN(fname) class LCDClass_##fname : public LCDCallback \
{ \
public: \
  void operator()(KeyEvent lcdKey)

/**
 * \ingroup keys
 * A macro that can be used when only a function is needed
 *  and not a real class.
 * It has to be used after the function code and match the name
 *  that was used for LCD_CALLBACK_FUNCTION_BEGIN
 */
#define LCD_CALLBACK_FUNCTION_END(fname) \
}; \
LCDClass_##fname fname;

/** \class LCDCallback LCDCallback.h "api/LCDCallback.h"
 *  \brief Main class to create new callback for key events.
 *  \ingroup keys
 *  To create a new key handler, one should subclass LCDCallback,
 *  and implement operator().
 */
class LCDCallback {
 public:
  /** Default constructor */
  LCDCallback() {
  }

  /** Destructor */
  virtual ~LCDCallback() {
  }

  /**
   * \brief Called when key event occurs.
   *
   * This method should be overrided by classes used to handle key event.
   *  It will be called when the key is pressed.
   * @param lcdKey The key that was pressed.
   */
  virtual void operator()(KeyEvent lcdKey) = 0;
};

typedef std::map<KeyEvent, LCDCallback *> CallbackMap;

} // end of lcdapi namespace

#endif
