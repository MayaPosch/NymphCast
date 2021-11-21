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

#ifndef _LCDAPI_API_LCDLOCK_H_
#define _LCDAPI_API_LCDLOCK_H_

#include <pthread.h>

namespace lcdapi {

class LCDMutex;

/** \class LCDLock LCDLock.h "api/LCDLock.h"
 *  \brief A class to lock mutexes.
 *  \ingroup main
 *  This class is used to lock some mutexes and be sure that they will
 *   be unlocked, even if an exception occurs.
 */
class LCDLock {
 private:
  const LCDLock& operator=(const LCDLock& rhs);
  LCDLock(const LCDLock& original);
  // Memberwise copying is prohibited.

  LCDMutex *_lcdMutex;
  ::pthread_mutex_t *_posixMutex;
  bool _useLCD;

 public:
  /**
   * \brief Constructor locking an LCD mutex.
   *
   * This constructor locks the LCD mutex and stores it.
   */
  explicit LCDLock(LCDMutex *mutex);

  /**
   * \brief Constructor locking a Posix mutex.
   *
   * This constructor locks the Posix mutex and stores it.
   */
  explicit LCDLock(::pthread_mutex_t *mutex);

  /**
   * \brief Destructor unlocking the mutex.
   *
   * This destructor unlocks the mutex.
   */
  ~LCDLock();
};

} // end of lcdapi namespace

#endif
