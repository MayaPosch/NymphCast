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

#include <lcdapi/api/LCDClient.h>
#include <lcdapi/api/LCDLock.h>
#include <lcdapi/api/LCDException.h>
#include <sstream>

namespace lcdapi {

using namespace std;

void *mainRepliesLoop(void *);
void *handleKeyEvent(void *);
void *handleMenuEvent(void *);

struct KeyEventInfo
{
  KeyEvent kev;
  LCDCallback *callback;
  KeyEventInfo() : kev(), callback(NULL) {}
  ~KeyEventInfo() {}
  KeyEventInfo(const KeyEventInfo &original);
  KeyEventInfo &operator=(const KeyEventInfo &rhs);
};

class MenuEventInfo
{
private:
  MenuEventInfo(const MenuEventInfo& original);
  const MenuEventInfo& operator=(const MenuEventInfo& rhs);
  // Memberwise copying is prohibited.
public:
  MenuEventInfo() : menu_event(), value(), handler(NULL) {};
  ~MenuEventInfo() {};
  string menu_event;
  string value;
  LCDMenuEventHandler *handler;
};

LCDClient::LCDClient(const string &server, int port) : LCDElement("", ""),
                                                       _sendMutex(),
                                                       _gotAnswer(),
                                                       _mainThread(),
                                                       _serverConnection(),
                                                       _answer(),
                                                       _currentScreen(),
                                                       _connectionString(),
                                                       _serverVersion(),
                                                       _protocolVersion(),
                                                       _width(0),
                                                       _height(0),
                                                       _charWidth(0),
                                                       _charHeight(0),
                                                       _callbacks(),
                                                       _handlers(),
                                                       _sync(true)
{
  ::pthread_mutex_init(&_sendMutex, 0);
  ::pthread_cond_init(&_gotAnswer, 0);

  _serverConnection.connect(server, port);
  _serverConnection << "hello";
  _serverConnection >> _connectionString;

  string token;
  istringstream response(_connectionString);

  while (response >> token)
  {
    if (0 == token.compare("LCDproc"))
    {
      response >> _serverVersion;
    }
    else if (0 == token.compare("protocol"))
    {
      response >> _protocolVersion;
    }
    else if (0 == token.compare("wid"))
    {
      response >> _width;
    }
    else if (0 == token.compare("hgt"))
    {
      response >> _height;
    }
    else if (0 == token.compare("cellwid"))
    {
      response >> _charWidth;
    }
    else if (0 == token.compare("cellhgt"))
    {
      response >> _charHeight;
    }
  }

  ::pthread_create(&_mainThread, 0, &mainRepliesLoop, this);
}

LCDClient::~LCDClient()
{
  // It is polite to let the server know that we are disconnecting
  {
    const LCDLock l(&_sendMutex);
    _serverConnection << "bye";
  }

  if (::pthread_cancel(_mainThread) == 0)
  {
    ::pthread_join(_mainThread, 0);
  }
  ::pthread_mutex_destroy(&_sendMutex);
  ::pthread_cond_destroy(&_gotAnswer);
}

void LCDClient::sendCommand(const std::string &cmd, const std::string &parameters)
{
  if (!cmd.empty())
  {
    const LCDLock l(&_sendMutex);

    const string command = cmd + " " + parameters;

    _serverConnection << command;

    if (_sync) {
      while (_answer.empty())
      {
        ::pthread_cond_wait(&_gotAnswer, &_sendMutex);
      }

      if (0 == _answer.find("huh?"))
      {
        throw LCDException(_answer.substr(5));
      }
      _answer.clear();
    }
  }
}

void LCDClient::disableSync()
{
  _sync = false;
}

void LCDClient::notifyCreated()
{
}

void LCDClient::notifyDestroyed()
{
}

void LCDClient::setClientOption(const string& optName, const string& value)
{
  sendCommand("client_set", string("-") + optName + " " + value);
}

void LCDClient::setName(const string& name)
{
  setClientOption("name", name);
}

void LCDClient::setBackLight(const std::string& backlight)
{
  sendCommand("backlight", backlight);
}

void LCDClient::assignKey(const KeyEvent &key, LCDCallback *callback)
{
  _callbacks[key] = callback;
  sendCommand("client_add_key", string("-shared ") + key.get_key());
}

void LCDClient::deleteKey(const KeyEvent &key)
{
  _callbacks.erase(key);
  sendCommand("client_del_key", key.get_key());
}

void LCDClient::registerMenuEventHandler(const std::string& menu_id, const std::string& menu_event, LCDMenuEventHandler* handler)
{
  if (NULL == handler) {
    unregisterMenuEventHandler(menu_id, menu_event);
  } else {
    _handlers[menu_id][menu_event] = handler;
  }
}

void LCDClient::unregisterMenuEventHandler(const std::string& menu_id, const std::string& menu_event)
{
  if (_handlers.end() != _handlers.find(menu_id)) {
    _handlers[menu_id].erase(menu_event);
    if (_handlers[menu_id].empty()) {
      _handlers.erase(menu_id);
    }
  }
}

void LCDClient::menuGoto(const std::string& id)
{
  sendCommand("menu_goto", id);
}

void LCDClient::menuSetMain(const std::string& id)
{
  sendCommand("menu_set_main", id);
}

void LCDClient::mainLoop()
{
  ::pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
  string reply;
  while(true)
  {
    _serverConnection >> reply;

    if ( (0 == reply.find("huh?")) || (0 == reply.compare("success")) )
    {
      const LCDLock l(&_sendMutex);
      _answer = reply;
      ::pthread_cond_signal(&_gotAnswer);
    }
    else
    {
      if (0 == reply.find("key"))
      {
        const KeyEvent key(reply.substr(4, string::npos));
        if (_callbacks.end() != _callbacks.find(key))
        {
          KeyEventInfo *kevI = new KeyEventInfo;
          kevI->kev = key;
          kevI->callback = _callbacks[key];

          ::pthread_t tid;
          ::pthread_create(&tid, 0, &handleKeyEvent, kevI);
        }

      }
      else if (0 == reply.find("listen"))
      {
        _currentScreen = reply.substr(7);
      }
      else if (0 == reply.find("ignore"))
      {
        if (_currentScreen == reply.substr(7))
        {
          _currentScreen.clear();
        }
      }
      else if (0 == reply.find("menuevent"))
      {
        istringstream response(reply.substr(10));
        string menu_event;
        string menu_id;
        string value; // N.B. Not for all event types
        response >> menu_event >> menu_id >> value;

        // Invoke handler, if we have one.
        const MenuEventHandlerMap::const_iterator iter =
          _handlers.find(menu_id);
        if (_handlers.end() != iter) {
          const map<string, LCDMenuEventHandler*>& menu_handlers =
            iter->second;
          if (menu_handlers.end() != menu_handlers.find(menu_event)) {
            LCDMenuEventHandler* handler = _handlers[menu_id][menu_event];
            if (NULL != handler) {

              MenuEventInfo *mevI = new MenuEventInfo;
              mevI->menu_event = menu_event;
              mevI->value = value;
              mevI->handler = handler;

              ::pthread_t tid;
              ::pthread_create(&tid, 0, &handleMenuEvent, mevI);
            }
          }
        }
      }
    }
  }
}

void *mainRepliesLoop(void *param)
{
  LCDClient *client = static_cast<LCDClient*>(param);
  client->mainLoop();

  return NULL;
}

void *handleKeyEvent(void *param)
{
  if (NULL != param) {
    KeyEventInfo *kevI = static_cast<KeyEventInfo*>(param);

    if (NULL != kevI->callback) {
      (*(kevI->callback))(kevI->kev);
    }

    delete kevI;
    kevI = NULL;
  }

  return NULL;
}

void *handleMenuEvent(void *param)
{
  if (NULL != param) {
    MenuEventInfo *mevI = static_cast<MenuEventInfo*>(param);

    if (NULL != mevI->handler) {
      (*(mevI->handler))(mevI->menu_event, mevI->value);
    }

    delete mevI;
    mevI = NULL;
  }

  return NULL;
}

} // end of lcdapi namespace
