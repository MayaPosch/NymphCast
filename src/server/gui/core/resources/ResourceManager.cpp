#include "ResourceManager.h"

#include "utils/FileSystemUtil.h"
#include <fstream>

auto array_deleter = [](unsigned char* p) { delete[] p; };
auto nop_deleter = [](unsigned char* /*p*/) { };

std::shared_ptr<ResourceManager> ResourceManager::sInstance = nullptr;

ResourceManager::ResourceManager()
{
}

std::shared_ptr<ResourceManager>& ResourceManager::getInstance()
{
	if(!sInstance)
		sInstance = std::shared_ptr<ResourceManager>(new ResourceManager());

	return sInstance;
}

std::string ResourceManager::getResourcePath(const std::string& path) const
{
	// check if this is a resource file
	if((path[0] == ':') && (path[1] == '/'))
	{
		std::string test;

		// check in homepath
		test = Utils::FileSystem::getHomePath() + "/.emulationstation/resources/" + &path[2];
		if(Utils::FileSystem::exists(test))
			return test;

		// check in exepath
		test = Utils::FileSystem::getExePath() + "/resources/" + &path[2];
		if(Utils::FileSystem::exists(test))
			return test;

		// check in cwd
		test = Utils::FileSystem::getCWDPath() + "/resources/" + &path[2];
		if(Utils::FileSystem::exists(test))
			return test;
	}

	// not a resource, return unmodified path
	return path;
}

const ResourceData ResourceManager::getFileData(const std::string& path) const
{
	//check if its a resource
	const std::string respath = getResourcePath(path);

	if(Utils::FileSystem::exists(respath))
	{
		ResourceData data = loadFile(respath);
		return data;
	}

	//if the file doesn't exist, return an "empty" ResourceData
	ResourceData data = {NULL, 0};
	return data;
}

ResourceData ResourceManager::loadFile(const std::string& path) const
{
	std::ifstream stream(path, std::ios::binary);

	stream.seekg(0, stream.end);
	std::ifstream::pos_type size = stream.tellg();
	stream.seekg(0, stream.beg);
	if(size>0)
	{
		//supply custom deleter to properly free array
		std::shared_ptr<unsigned char> data(new unsigned char[size], array_deleter);
		stream.read((char*)data.get(), size);
		stream.close();

		ResourceData ret = {data, (size_t)size};
		return ret;
	}

	//error reading file, return an "empty" ResourceData
	ResourceData ret = {NULL, 0};
	return ret;
}

bool ResourceManager::fileExists(const std::string& path) const
{
	//if it exists as a resource file, return true
	if(getResourcePath(path) != path)
		return true;

	return Utils::FileSystem::exists(path);
}

void ResourceManager::unloadAll()
{
	auto iter = mReloadables.cbegin();
	while(iter != mReloadables.cend())
	{
		std::shared_ptr<ReloadableInfo> info = *iter;

		if (!info->data.expired())
		{
			info->reload = info->data.lock()->unload();
			iter++;
		}
		else
			iter = mReloadables.erase(iter);
	}
}

void ResourceManager::reloadAll()
{
	auto iter = mReloadables.cbegin();
	while(iter != mReloadables.cend())
	{
		std::shared_ptr<ReloadableInfo> info = *iter;

		if (!info->data.expired())
		{
			if (info->reload)
			{
				info->data.lock()->reload();
				info->reload = false;
			}

			iter++;
		}
		else
			iter = mReloadables.erase(iter);
	}
}

void ResourceManager::addReloadable(std::weak_ptr<IReloadable> reloadable)
{
	std::shared_ptr<ReloadableInfo> info = std::make_shared<ReloadableInfo>();
	info->data = reloadable;
	info->reload = false;
	mReloadables.push_back(info);
}
