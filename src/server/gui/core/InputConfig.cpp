#include "InputConfig.h"

#include "Log.h"
#include <pugixml/src/pugixml.hpp>

//some util functions
std::string inputTypeToString(InputType type)
{
	switch(type)
	{
	case TYPE_AXIS:
		return "axis";
	case TYPE_BUTTON:
		return "button";
	case TYPE_HAT:
		return "hat";
	case TYPE_KEY:
		return "key";
	case TYPE_CEC_BUTTON:
		return "cec-button";
	default:
		return "error";
	}
}

InputType stringToInputType(const std::string& type)
{
	if(type == "axis")
		return TYPE_AXIS;
	if(type == "button")
		return TYPE_BUTTON;
	if(type == "hat")
		return TYPE_HAT;
	if(type == "key")
		return TYPE_KEY;
	if(type == "cec-button")
		return TYPE_CEC_BUTTON;
	return TYPE_COUNT;
}


std::string toLower(std::string str)
{
	for(unsigned int i = 0; i < str.length(); i++)
	{
		str[i] = (char)tolower(str[i]);
	}

	return str;
}
//end util functions

InputConfig::InputConfig(int deviceId, const std::string& deviceName, const std::string& deviceGUID) : mDeviceId(deviceId), mDeviceName(deviceName), mDeviceGUID(deviceGUID)
{
}

void InputConfig::clear()
{
	mNameMap.clear();
}

bool InputConfig::isConfigured()
{
	return mNameMap.size() > 0;
}

void InputConfig::mapInput(const std::string& name, Input input)
{
	mNameMap[toLower(name)] = input;
}

void InputConfig::unmapInput(const std::string& name)
{
	auto it = mNameMap.find(toLower(name));
	if(it != mNameMap.cend())
		mNameMap.erase(it);
}

bool InputConfig::getInputByName(const std::string& name, Input* result)
{
	auto it = mNameMap.find(toLower(name));
	if(it != mNameMap.cend())
	{
		*result = it->second;
		return true;
	}

	return false;
}

bool InputConfig::isMappedTo(const std::string& name, Input input)
{
	Input comp;
	if(!getInputByName(name, &comp))
		return false;

	if(comp.configured && comp.type == input.type && comp.id == input.id)
	{
		if(comp.type == TYPE_HAT)
		{
			return (input.value == 0 || input.value & comp.value);
		}

		if(comp.type == TYPE_AXIS)
		{
			return input.value == 0 || comp.value == input.value;
		}else{
			return true;
		}
	}
	return false;
}

bool InputConfig::isMappedLike(const std::string& name, Input input)
{
	if(name == "left")
	{
		return isMappedTo("left", input) || isMappedTo("leftanalogleft", input) || isMappedTo("rightanalogleft", input);
	}else if(name == "right"){
		return isMappedTo("right", input) || isMappedTo("leftanalogright", input) || isMappedTo("rightanalogright", input);
	}else if(name == "up"){
		return isMappedTo("up", input) || isMappedTo("leftanalogup", input) || isMappedTo("rightanalogup", input);
	}else if(name == "down"){
		return isMappedTo("down", input) || isMappedTo("leftanalogdown", input) || isMappedTo("rightanalogdown", input);
	}else if(name == "leftshoulder"){
		return isMappedTo("leftshoulder", input) || isMappedTo("pageup", input);
	}else if(name == "rightshoulder"){
		return isMappedTo("rightshoulder", input) || isMappedTo("pagedown", input);
	}
	return isMappedTo(name, input);
}

std::vector<std::string> InputConfig::getMappedTo(Input input)
{
	std::vector<std::string> maps;

	typedef std::map<std::string, Input>::const_iterator it_type;
	for(it_type iterator = mNameMap.cbegin(); iterator != mNameMap.cend(); iterator++)
	{
		Input chk = iterator->second;

		if(!chk.configured)
			continue;

		if(chk.device == input.device && chk.type == input.type && chk.id == input.id)
		{
			if(chk.type == TYPE_HAT)
			{
				if(input.value == 0 || input.value & chk.value)
				{
					maps.push_back(iterator->first);
				}
				continue;
			}

			if(input.type == TYPE_AXIS)
			{
				if(input.value == 0 || chk.value == input.value)
					maps.push_back(iterator->first);
			}else{
				maps.push_back(iterator->first);
			}
		}
	}

	return maps;
}

void InputConfig::loadFromXML(pugi::xml_node& node)
{
	clear();

	for(pugi::xml_node input = node.child("input"); input; input = input.next_sibling("input"))
	{
		std::string name = input.attribute("name").as_string();
		std::string type = input.attribute("type").as_string();
		InputType typeEnum = stringToInputType(type);

		if(typeEnum == TYPE_COUNT)
		{
			LOG(LogError) << "InputConfig load error - input of type \"" << type << "\" is invalid! Skipping input \"" << name << "\".\n";
			continue;
		}

		int id = input.attribute("id").as_int();
		int value = input.attribute("value").as_int();

		if(value == 0)
			LOG(LogWarning) << "WARNING: InputConfig value is 0 for " << type << " " << id << "!\n";

		mNameMap[toLower(name)] = Input(mDeviceId, typeEnum, id, value, true);
	}
}

void InputConfig::writeToXML(pugi::xml_node& parent)
{
	pugi::xml_node cfg = parent.append_child("inputConfig");

	if(mDeviceId == DEVICE_KEYBOARD)
	{
		cfg.append_attribute("type") = "keyboard";
		cfg.append_attribute("deviceName") = "Keyboard";
	}
	else if(mDeviceId == DEVICE_CEC)
	{
		cfg.append_attribute("type") = "cec";
		cfg.append_attribute("deviceName") = "CEC";
	}
	else
	{
		cfg.append_attribute("type") = "joystick";
		cfg.append_attribute("deviceName") = mDeviceName.c_str();
	}

	cfg.append_attribute("deviceGUID") = mDeviceGUID.c_str();

	typedef std::map<std::string, Input>::const_iterator it_type;
	for(it_type iterator = mNameMap.cbegin(); iterator != mNameMap.cend(); iterator++)
	{
		if(!iterator->second.configured)
			continue;

		pugi::xml_node input = cfg.append_child("input");
		input.append_attribute("name") = iterator->first.c_str();
		input.append_attribute("type") = inputTypeToString(iterator->second.type).c_str();
		input.append_attribute("id").set_value(iterator->second.id);
		input.append_attribute("value").set_value(iterator->second.value);
	}
}
