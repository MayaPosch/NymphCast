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

#include <lcdapi/api/LCDConnection.h>
#include <lcdapi/include/LCDConstants.h>
#include <lcdapi/api/LCDException.h>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <iostream>

namespace lcdapi {

using namespace std;

LCDConnection::LCDConnection()
  : _isConnected(false), _sock(socket (AF_INET, SOCK_STREAM, 0)), _addr()
{
  initialize();
}

LCDConnection::LCDConnection(const string &host, const int port)
  : _isConnected(false), _sock(socket (AF_INET, SOCK_STREAM, 0)), _addr()
{
  initialize();
  connect(host, port);
}

LCDConnection::~LCDConnection() {
  disconnect();
}

void LCDConnection::initialize() {
  memset(&_addr, 0, sizeof (_addr));

  if (!isValid()) {
    throw LCDException(LCD_SOCKET_CREATION_ERROR);
  }

  const int on = 1;
  if (setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&on), sizeof (on)) == -1) {
    throw LCDException(LCD_SOCKET_CREATION_ERROR);
  }
}

bool LCDConnection::isValid() const {
  return _sock != -1;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
void LCDConnection::connect(const string &host, const int port) {
  if (!isValid()) {
    throw LCDException(LCD_SOCKET_CREATION_ERROR);
  }

  _addr.sin_family = AF_INET;
  _addr.sin_port = htons(port);

  int status = inet_pton(AF_INET, host.c_str(), &_addr.sin_addr);

  if (errno == EAFNOSUPPORT) {
    throw LCDException(LCD_SOCKET_NOT_SUPPORTED);
  }

  status = ::connect(_sock, reinterpret_cast<struct sockaddr*>(&_addr), sizeof(_addr) );

  if (status != 0) {
    throw LCDException(LCD_SOCKET_CONNECTION_ERROR);
  }

  _isConnected = true;
}
#pragma GCC diagnostic error "-Wold-style-cast"

void LCDConnection::disconnect() {
  if (isValid()) {
    ::close(_sock);
  }
}

void LCDConnection::send(const string &toSend) {
  const string s = toSend + "\r\n";

#ifdef DEBUG
  cerr << "Sending : " << s << endl;
#endif // DEBUG

  if (!_isConnected) {
    throw LCDException(LCD_SOCKET_NOT_CONNECTED);
  }

  const int total = s.size();
  int offset = 0;

  while (offset != total) {
    const int sent = ::send(_sock, s.c_str() + offset, total - offset, 0);
    if ( ((sent == -1) && (errno != EAGAIN)) || (sent == 0) ) {
      throw LCDException(LCD_SOCKET_SEND_ERROR);
    } else {
      offset += sent;
    }
  }
}

string LCDConnection::recv() {
  if (!_isConnected) {
    throw LCDException(LCD_SOCKET_NOT_CONNECTED);
  }

  char buf[LCD_MAXRECV + 1];
  memset(buf, 0, LCD_MAXRECV + 1);

  for (char* current = buf; current < (buf + LCD_MAXRECV); ++current) {
    const int status = ::recv(_sock, current, 1, 0);
    if (-1 == status) {
      throw LCDException(LCD_SOCKET_RECV_ERROR);
    }
    if (('\0' == *current) || ('\r' == *current) || ('\n' == *current)) {
      *current = '\0'; // N.B. Delete line termination character
      break;
    }
  }

  const string result(buf);

#ifdef DEBUG
  cerr << "Receiving : " << result << endl;
#endif // DEBUG

  return result;
}

const LCDConnection& LCDConnection::operator <<(const string &s) {
  send(s);
  return *this;
}

const LCDConnection& LCDConnection::operator >>(string &s) {
  s = recv();
  return *this;
}

} // end of lcdapi namespace
