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

#ifndef _LCDAPI_API_LCDMUTEX_H_
#define _LCDAPI_API_LCDMUTEX_H_

#include <pthread.h>

namespace lcdapi {

/** \class LCDMutex LCDMutex.h "api/LCDMutex.h"
 *  \brief A class implementing mutexes.
 *  \ingroup main
 *  This class is used to create some recursive mutexes.
 *   If a thread has already locked it, it won't be blocked by
 *   another lock.
 */
class LCDMutex {
 private:
  ::pthread_mutex_t _mutex;
  ::pthread_t _owner;
 public:
  /**
   * \brief Default constructor.
   *
   * This constructor initializes the mutex.
   */
  LCDMutex();

  /**
   * \brief Destructor.
   *
   * This destructor destroys he mutex.
   */
  ~LCDMutex();

  /**
   * \brief Lock the mutex.
   *
   * This method is used to lock the mutex. It will have an effect
   *  only if it is not already locked by the same thread.
   */
  void lock();

  /**
   * \brief Unlock the mutex.
   *
   * This method will release the mutex so another thread can acquire it.
   */
  void unlock();
};

} // end of lcdapi namespace

#endif
