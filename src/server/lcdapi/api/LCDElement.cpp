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

#include <lcdapi/api/LCDElement.h>
#include <lcdapi/api/LCDLock.h>
#include <unistd.h>
#include <sstream>

namespace lcdapi {

using namespace std;

set<std::string> LCDElement::_elementsList;
LCDMutex LCDElement::_elementMutex;
unsigned int LCDElement::_elementCounter = 0;

LCDElement::LCDElement(const string &id, const string &addCommand, const string &addParam, LCDElement *parent)
  : _iAmDead(false),
    _id(),
    _elementDel(),
    _elementAddCmd(addCommand),
    _elementAddParam(addParam),
    _parent(parent),
    _childrenList(),
    _commandBuffer()
{
  if (id.empty())
  {
    const LCDLock l(&LCDElement::_elementMutex);
    ostringstream idBuffer;
    idBuffer << "LCDAPI_"
             << getpid()
             << "_"
             << LCDElement::_elementCounter;

    LCDElement::_elementCounter++;
    _id = idBuffer.str();
  }
  else
  {
    _id = id;
  }
  if (_parent)
  {
    _parent->addToList(this);
  }
  notifyCreated();
}

LCDElement::~LCDElement()
{
  notifyDestroyed();
}

void LCDElement::notifyCreated()
{
  const LCDLock l(&LCDElement::_elementMutex);
  sendCommand(_elementAddCmd, _elementAddParam);
  _iAmDead = false;
  LCDElement::_elementsList.insert(_id);
}

void LCDElement::notifyDestroyed()
{
  const LCDLock l(&LCDElement::_elementMutex);
  if (!_iAmDead)
  {
    LCDElement::_elementsList.erase(_id);
    for (ElementMap::const_iterator it = _childrenList.begin();
         it != _childrenList.end(); ++it)
    {
      it->second->notifyDestroyed();
    }
    _childrenList.clear();
    sendCommand(_elementDel);
    if (_parent)
    {
      _parent->removeFromList(this);
    }
    _iAmDead = true;
  }
}

bool LCDElement::exists(const string& id)
{
  const LCDLock l(&LCDElement::_elementMutex);
  return ( (LCDElement::_elementsList.find(id)) != (LCDElement::_elementsList.end()) );
}

const string &LCDElement::getId() const
{
  return _id;
}

void LCDElement::sendCommand(const std::string &cmd, const std::string &parameters)
{
  const LCDLock l(&LCDElement::_elementMutex);
  if (!cmd.empty())
  {
    if (_parent)
    {
      const string realParams = _id + " " + parameters;
      _parent->sendCommand(cmd, realParams);
    }
    else
    {
      _commandBuffer.push_back(Command(cmd, parameters));
    }
  }
}

void LCDElement::setParent(LCDElement *parent)
{
  _parent = parent;
  notifyAdded();
}

void LCDElement::add(LCDElement *child)
{
  child->setParent(this);
  addToList(child);
}

void LCDElement::addToList(LCDElement *elt)
{
  _childrenList[elt->getId()] = elt;
}

void LCDElement::removeFromList(LCDElement *elt)
{
  _childrenList.erase(elt->getId());
}

void LCDElement::notifyAdded()
{
  for (CommandList::const_iterator it = _commandBuffer.begin();
       it != _commandBuffer.end(); ++it)
  {
    sendCommand(it->_cmd, it->_params);
  }
}

} // end of lcdapi namespace
