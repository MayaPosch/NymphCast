#include "MameNames.h"

#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "Log.h"
#include <pugixml/src/pugixml.hpp>
#include <string.h>

MameNames* MameNames::sInstance = nullptr;

void MameNames::init()
{
	if(!sInstance)
		sInstance = new MameNames();

} // init

void MameNames::deinit()
{
	if(sInstance)
	{
		delete sInstance;
		sInstance = nullptr;
	}

} // deinit

MameNames* MameNames::getInstance()
{
	if(!sInstance)
		sInstance = new MameNames();

	return sInstance;

} // getInstance

MameNames::MameNames()
{
	std::string xmlpath = ResourceManager::getInstance()->getResourcePath(":/mamenames.xml");

	if(!Utils::FileSystem::exists(xmlpath))
		return;

	LOG(LogInfo) << "Parsing XML file \"" << xmlpath << "\"...";

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(xmlpath.c_str());

	if(!result)
	{
		LOG(LogError) << "Error parsing XML file \"" << xmlpath << "\"!\n	" << result.description();
		return;
	}

	for(pugi::xml_node gameNode = doc.child("game"); gameNode; gameNode = gameNode.next_sibling("game"))
	{
		NamePair namePair = { gameNode.child("mamename").text().get(), gameNode.child("realname").text().get() };
		mNamePairs.push_back(namePair);
	}

	// Read bios
	xmlpath = ResourceManager::getInstance()->getResourcePath(":/mamebioses.xml");

	if(!Utils::FileSystem::exists(xmlpath))
		return;

	LOG(LogInfo) << "Parsing XML file \"" << xmlpath << "\"...";

	result = doc.load_file(xmlpath.c_str());

	if(!result)
	{
		LOG(LogError) << "Error parsing XML file \"" << xmlpath << "\"!\n	" << result.description();
		return;
	}

	for(pugi::xml_node biosNode = doc.child("bios"); biosNode; biosNode = biosNode.next_sibling("bios"))
	{
		std::string bios = biosNode.text().get();
		mMameBioses.push_back(bios);
	}

	// Read devices
	xmlpath = ResourceManager::getInstance()->getResourcePath(":/mamedevices.xml");

	if(!Utils::FileSystem::exists(xmlpath))
		return;

	LOG(LogInfo) << "Parsing XML file \"" << xmlpath << "\"...";

	result = doc.load_file(xmlpath.c_str());

	if(!result)
	{
		LOG(LogError) << "Error parsing XML file \"" << xmlpath << "\"!\n	" << result.description();
		return;
	}

	for(pugi::xml_node deviceNode = doc.child("device"); deviceNode; deviceNode = deviceNode.next_sibling("device"))
	{
		std::string device = deviceNode.text().get();
		mMameDevices.push_back(device);
	}

} // MameNames

MameNames::~MameNames()
{

} // ~MameNames

std::string MameNames::getRealName(const std::string& _mameName)
{
	size_t start = 0;
	size_t end   = mNamePairs.size();

	while(start < end)
	{
		const size_t index   = (start + end) / 2;
		const int    compare = strcmp(mNamePairs[index].mameName.c_str(), _mameName.c_str());

		if(compare < 0)       start = index + 1;
		else if( compare > 0) end   = index;
		else                  return mNamePairs[index].realName;
	}

	return _mameName;

} // getRealName

const bool MameNames::isBios(const std::string& _biosName)
{
	return MameNames::find(mMameBioses, _biosName);

} // isBios

const bool MameNames::isDevice(const std::string& _deviceName)
{
	return MameNames::find(mMameDevices, _deviceName);

} // isDevice

const bool MameNames::find(std::vector<std::string> devices, const std::string& name)
{
	size_t start = 0;
	size_t end   = devices.size();

	while(start < end)
	{
		const size_t index   = (start + end) / 2;
		const int    compare = strcmp(devices[index].c_str(), name.c_str());

		if(compare < 0)       start = index + 1;
		else if( compare > 0) end   = index;
		else                  return true;
	}

	return false;

}
