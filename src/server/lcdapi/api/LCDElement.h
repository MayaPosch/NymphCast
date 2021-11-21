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

#ifndef _LCDAPI_API_LCDELEMENT_H_
#define _LCDAPI_API_LCDELEMENT_H_

/**
 * \defgroup main Main components
 *  These are the main components of the LCDApi.
 *   Some of them are used internally and should
 *   not be useful for API users.
 *
 *  The most important one is LCDClient as each program
 *   using LCDApi should have one instance of this class
 *   to connect to LCDproc server (LCDd).
 *
 *  The screen class is also important because all the element
 *   diplayed on the LCD are in fact in an LCDScreen. You can have
 *   more than one. LCDproc will then rotate them (according to value
 *   set with LCDScreen::setDuration().
 */

#include <lcdapi/api/LCDMutex.h>

#include <string>
#include <list>
#include <map>
#include <set>

namespace lcdapi {

/** \class LCDElement LCDElement.h "api/LCDElement.h"
 *  \brief Main class for all elements of the API.
 *  \ingroup main
 *  All the classes in this API have LCDElement as their base class.
 *   It contains important methods. But it should not be used directly.
 */

class LCDElement
{
 private:
  const LCDElement& operator=(const LCDElement& rhs);
  LCDElement(const LCDElement& original);
  // Memberwise copying is prohibited.

 protected:
  static std::set<std::string> _elementsList;
  bool _iAmDead;
  static LCDMutex _elementMutex;
  static unsigned int _elementCounter;
  std::string _id;
  std::string _elementDel;
  std::string _elementAddCmd, _elementAddParam;
  LCDElement *_parent;
  class Command
  {
   public:
    std::string _cmd;
    std::string _params;
    Command(const std::string& cmd, const std::string& params) : _cmd(cmd), _params(params) {}
    ~Command() {}
  Command(const Command& original) : _cmd(original._cmd), _params(original._params) {}
    const Command& operator=(const Command& rhs) {
      if (&rhs != this) {
        _cmd = rhs._cmd;
        _params = rhs._params;
      }
      return *this;
    }
  };
  typedef std::list<Command> CommandList;
  typedef std::map<std::string, LCDElement *> ElementMap;
  ElementMap _childrenList;
  CommandList _commandBuffer;
  void setParent(LCDElement *parent);
  void addToList(LCDElement *elt);
  void removeFromList(LCDElement *elt);
  void notifyCreated();
  void notifyDestroyed();
  void notifyAdded();

 public:
  LCDElement(const std::string &id, const std::string &addCommand, const std::string &addParam = "", LCDElement *parent = 0);

  /**
   * \brief Destructor of the element.
   *
   * This destructor sends deletion commands to LCDproc server
   *  so the element is removed.
  */

  virtual ~LCDElement();

  /**
   * \brief Add a child to the component.
   *
   * This method is used to add a child to a component.
   * @param child The element to be added in the children list.
  */

  void add(LCDElement *child);
  virtual void sendCommand(const std::string &cmd, const std::string &parameters = "");

  /**
   * \brief Returns Id of the element.
   *
   * With this method one can have read access to the element identifier.
   * @return A string.
  */
  const std::string &getId() const;

  /**
   * \brief Test if a widget with given Id exists.
   *
   * This static method is used to test the existence of a widget.
   * @param id The identifier of the widget to test.
  */
  static bool exists(const std::string& id);

};

} // end of lcdapi namespace

#endif
