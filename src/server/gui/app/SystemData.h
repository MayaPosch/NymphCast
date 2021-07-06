#pragma once
#ifndef ES_APP_SYSTEM_DATA_H
#define ES_APP_SYSTEM_DATA_H

#include "PlatformId.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <pugixml/src/pugixml.hpp>

class FileData;
class FileFilterIndex;
class ThemeData;
class Window;

struct SystemEnvironmentData
{
	std::string mStartPath;
	std::vector<std::string> mSearchExtensions;
	std::string mLaunchCommand;
	std::vector<PlatformIds::PlatformId> mPlatformIds;
};

class SystemData
{
public:
	SystemData(const std::string& name, const std::string& fullName, SystemEnvironmentData* envData, const std::string& themeFolder, bool CollectionSystem = false);
	~SystemData();

	inline FileData* getRootFolder() const { return mRootFolder; };
	inline const std::string& getName() const { return mName; }
	inline const std::string& getFullName() const { return mFullName; }
	inline const std::string& getStartPath() const { return mEnvData->mStartPath; }
	inline const std::vector<std::string>& getExtensions() const { return mEnvData->mSearchExtensions; }
	inline const std::string& getThemeFolder() const { return mThemeFolder; }
	inline SystemEnvironmentData* getSystemEnvData() const { return mEnvData; }
	inline const std::vector<PlatformIds::PlatformId>& getPlatformIds() const { return mEnvData->mPlatformIds; }
	inline bool hasPlatformId(PlatformIds::PlatformId id) { if (!mEnvData) return false; return std::find(mEnvData->mPlatformIds.cbegin(), mEnvData->mPlatformIds.cend(), id) != mEnvData->mPlatformIds.cend(); }

	inline const std::shared_ptr<ThemeData>& getTheme() const { return mTheme; }

	std::string getGamelistPath(bool forWrite) const;
	bool hasGamelist() const;
	std::string getThemePath() const;

	unsigned int getGameCount() const;
	unsigned int getDisplayedGameCount() const;

	static void deleteSystems();
	static bool loadConfig(Window* window); //Load the system config file at getConfigPath(). Returns true if no errors were encountered. An example will be written if the file doesn't exist.
	static void writeExampleConfig(const std::string& path);
	static std::string getConfigPath(bool forWrite); // if forWrite, will only return ~/.emulationstation/es_systems.cfg, never /etc/emulationstation/es_systems.cfg

	static std::vector<SystemData*> sSystemVector;

	inline std::vector<SystemData*>::const_iterator getIterator() const { return std::find(sSystemVector.cbegin(), sSystemVector.cend(), this); };
	inline std::vector<SystemData*>::const_reverse_iterator getRevIterator() const { return std::find(sSystemVector.crbegin(), sSystemVector.crend(), this); };
	inline bool isCollection() { return mIsCollectionSystem; };
	inline bool isGameSystem() { return mIsGameSystem; };

	bool isVisible();

	SystemData* getNext() const;
	SystemData* getPrev() const;
	static SystemData* getRandomSystem();
	FileData* getRandomGame();

	// Load or re-load theme.
	void loadTheme();

	FileFilterIndex* getIndex() { return mFilterIndex; };
	void onMetaDataSavePoint();

private:
	static SystemData* loadSystem(pugi::xml_node system);

	bool mIsCollectionSystem;
	bool mIsGameSystem;
	std::string mName;
	std::string mFullName;
	SystemEnvironmentData* mEnvData;
	std::string mThemeFolder;
	std::shared_ptr<ThemeData> mTheme;

	void populateFolder(FileData* folder);
	void indexAllGameFilters(const FileData* folder);
	void setIsGameSystemStatus();
	void writeMetaData();

	FileFilterIndex* mFilterIndex;

	FileData* mRootFolder;
};

#endif // ES_APP_SYSTEM_DATA_H
