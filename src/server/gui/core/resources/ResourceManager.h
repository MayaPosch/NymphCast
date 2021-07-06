#pragma once
#ifndef ES_CORE_RESOURCES_RESOURCE_MANAGER_H
#define ES_CORE_RESOURCES_RESOURCE_MANAGER_H

#include <list>
#include <memory>

//The ResourceManager exists to...
//Allow loading resources embedded into the executable like an actual file.
//Allow embedded resources to be optionally remapped to actual files for further customization.

struct ResourceData
{
	const std::shared_ptr<unsigned char> ptr;
	const size_t length;
};

class ResourceManager;

class IReloadable
{
public:
	virtual bool unload() = 0;
	virtual void reload() = 0;
};

class ResourceManager
{
public:
	static std::shared_ptr<ResourceManager>& getInstance();

	void addReloadable(std::weak_ptr<IReloadable> reloadable);

	void unloadAll();
	void reloadAll();

	std::string getResourcePath(const std::string& path) const;
	const ResourceData getFileData(const std::string& path) const;
	bool fileExists(const std::string& path) const;

private:
	ResourceManager();

	static std::shared_ptr<ResourceManager> sInstance;

	ResourceData loadFile(const std::string& path) const;

	class ReloadableInfo
	{
	public:
		std::weak_ptr<IReloadable> data;
		bool reload;
	};

	std::list<std::shared_ptr<ReloadableInfo>> mReloadables; //  std::weak_ptr<IReloadable>
};

#endif // ES_CORE_RESOURCES_RESOURCE_MANAGER_H
