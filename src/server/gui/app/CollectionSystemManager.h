#pragma once
#ifndef ES_APP_COLLECTION_SYSTEM_MANAGER_H
#define ES_APP_COLLECTION_SYSTEM_MANAGER_H

#include <map>
#include <string>
#include <vector>

class FileData;
class SystemData;
class Window;
struct SystemEnvironmentData;

enum CollectionSystemType
{
	AUTO_ALL_GAMES,
	AUTO_LAST_PLAYED,
	AUTO_FAVORITES,
	CUSTOM_COLLECTION
};

struct CollectionSystemDecl
{
	CollectionSystemType type; // type of system
	std::string name;
	std::string longName;
	std::string defaultSort;
	std::string themeFolder;
	bool isCustom;
};

struct CollectionSystemData
{
	SystemData* system;
	CollectionSystemDecl decl;
	bool isEnabled;
	bool isPopulated;
	bool needsSave;
};

class CollectionSystemManager
{
public:
	CollectionSystemManager(Window* window);
	~CollectionSystemManager();

	static CollectionSystemManager* get();
	static void init(Window* window);
	static void deinit();
	void saveCustomCollection(SystemData* sys);

	void loadCollectionSystems(bool async=false);
	void loadEnabledListFromSettings();
	void updateSystemsList();

	void refreshCollectionSystems(FileData* file);
	void updateCollectionSystem(FileData* file, CollectionSystemData sysData);
	void deleteCollectionFiles(FileData* file);

	inline std::map<std::string, CollectionSystemData> getAutoCollectionSystems() { return mAutoCollectionSystemsData; };
	inline std::map<std::string, CollectionSystemData> getCustomCollectionSystems() { return mCustomCollectionSystemsData; };
	inline SystemData* getCustomCollectionsBundle() { return mCustomCollectionsBundle; };
	std::vector<std::string> getUnusedSystemsFromTheme();
	SystemData* addNewCustomCollection(std::string name);

	bool isThemeGenericCollectionCompatible(bool genericCustomCollections);
	bool isThemeCustomCollectionCompatible(std::vector<std::string> stringVector);
	std::string getValidNewCollectionName(std::string name, int index = 0);

	void setEditMode(std::string collectionName);
	void exitEditMode();
	inline bool isEditing() { return mIsEditingCustom; };
	inline std::string getEditingCollection() { return mEditingCollection; };
	bool toggleGameInCollection(FileData* file, int presscount);

	SystemData* getSystemToView(SystemData* sys);
	void updateCollectionFolderMetadata(SystemData* sys);

	SystemData* getAllGamesCollection();

private:
	static CollectionSystemManager* sInstance;
	SystemEnvironmentData* mCollectionEnvData;
	std::map<std::string, CollectionSystemDecl> mCollectionSystemDeclsIndex;
	std::map<std::string, CollectionSystemData> mAutoCollectionSystemsData;
	std::map<std::string, CollectionSystemData> mCustomCollectionSystemsData;
	Window* mWindow;
	bool mIsEditingCustom;
	std::string mEditingCollection;
	CollectionSystemData* mEditingCollectionSystemData;

	void initAutoCollectionSystems();
	void initCustomCollectionSystems();
	SystemData* createNewCollectionEntry(std::string name, CollectionSystemDecl sysDecl, bool index = true);
	void populateAutoCollection(CollectionSystemData* sysData);
	void populateCustomCollection(CollectionSystemData* sysData);

	void removeCollectionsFromDisplayedSystems();
	void addEnabledCollectionsToDisplayedSystems(std::map<std::string, CollectionSystemData>* colSystemData);

	std::vector<std::string> getSystemsFromConfig();
	std::vector<std::string> getSystemsFromTheme();
	std::vector<std::string> getCollectionsFromConfigFolder();
	std::vector<std::string> getCollectionThemeFolders(bool custom);
	std::vector<std::string> getUserCollectionThemeFolders();

	void trimCollectionCount(FileData* rootFolder, int limit);

	bool themeFolderExists(std::string folder);

	bool includeFileInAutoCollections(FileData* file);

	bool needDoublePress(int presscount);

	SystemData* mCustomCollectionsBundle;
};

std::string getCustomCollectionConfigPath(std::string collectionName);
std::string getCollectionsFolder();
bool systemSort(SystemData* sys1, SystemData* sys2);

#endif // ES_APP_COLLECTION_SYSTEM_MANAGER_H
