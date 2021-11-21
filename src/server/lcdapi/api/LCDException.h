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

#ifndef _LCDAPI_API_LCDEXCEPTION_H_
#define _LCDAPI_API_LCDEXCEPTION_H_

#include <string>

namespace lcdapi {

/** \class LCDException LCDException.h "api/LCDException.h"
 *  \brief The exceptions thrown by this API.
 *  \ingroup main
 * This class is for the exceptions thrown by the API if a problem occurs.
 */
class LCDException {
 private:
  std::string _desc;

 public:

  explicit LCDException(const std::string& desc);

  ~LCDException();

  LCDException(const LCDException& original);
  const LCDException& operator=(const LCDException& rhs);

  /**
   * \brief Get a textual description of the problem that occured.
   *
   * When an exception is caught, this method can be called to have a
   * description of the problem.
   * \return A string containing the error description.
   */
  std::string what() const;
};

} // end of lcdapi namespace

#endif
