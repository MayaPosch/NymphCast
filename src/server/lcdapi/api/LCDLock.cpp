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

#include <lcdapi/api/LCDLock.h>

#include <lcdapi/api/LCDMutex.h>

namespace lcdapi {

LCDLock::LCDLock(LCDMutex *mutex)
  : _lcdMutex(mutex), _posixMutex(NULL), _useLCD(true)
{
  ::pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
  _lcdMutex->lock();
}

LCDLock::LCDLock(::pthread_mutex_t *mutex)
  : _lcdMutex(NULL), _posixMutex(mutex), _useLCD(false)
{
  ::pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
  ::pthread_mutex_lock(_posixMutex);
}

LCDLock::~LCDLock() {
  if (_useLCD) {
    _lcdMutex->unlock();
  } else {
    ::pthread_mutex_unlock(_posixMutex);
  }
  ::pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
}

} // end of lcdapi namespace
