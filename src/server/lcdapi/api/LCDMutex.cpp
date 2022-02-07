/* Copyright 2012-2017 Simon Dawson <spdawson@gmail.com>

	 This file is part of lcdapi.

	 lcdapi is free software: you can redistribute it and/or modify
	 it under the terms of the GNU Lesser General Public License as
	 published by the Free Software Foundation, either version 3 of
	 the License, or (at your option) any later version.

	 lcdapi is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
	 GNU Lesser General Public License for more details.

	 You should have received a copy of the GNU Lesser General Public
	 License along with lcdapi.	If not,
	 see <http://www.gnu.org/licenses/>. 
	 
	2022/02/07, Maya Posch.
*/

#include <lcdapi/api/LCDMutex.h>

namespace lcdapi {

using namespace std;

LCDMutex::LCDMutex() {
	//
}

LCDMutex::~LCDMutex() {
	//
}

void LCDMutex::lock() {
	if (_owner != std::this_thread::get_id()) {
		_mutex.lock();
		_owner = std::this_thread::get_id();
	}
}

void LCDMutex::unlock() {
	_mutex.unlock();
}

} // end of lcdapi namespace
