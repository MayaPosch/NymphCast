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

#ifndef _LCDAPI_API_LCDCONNECTION_H_
#define _LCDAPI_API_LCDCONNECTION_H_

#ifdef _WIN32
#include <winsock.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include <string>

namespace lcdapi {

class LCDConnection
{
 private:
  LCDConnection(const LCDConnection &source);
  const LCDConnection& operator=(const LCDConnection &copy);
  // Memberwise copying is prohibited.

  void initialize();

  bool _isConnected;
  int _sock;
  sockaddr_in _addr;

 public:
  LCDConnection();
  LCDConnection(const std::string &host, const int port);
  virtual ~LCDConnection();

  bool isValid() const;

  void connect(const std::string &host, const int port);
  void disconnect();

  void send(const std::string &toSend);
  std::string recv();

  const LCDConnection & operator << (const std::string &s);
  const LCDConnection & operator >> (std::string &s);

};

} // end of lcdapi namespace

#endif
