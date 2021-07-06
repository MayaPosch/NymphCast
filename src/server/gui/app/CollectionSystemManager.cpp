#include "CollectionSystemManager.h"

#include "guis/GuiInfoPopup.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "views/gamelist/IGameListView.h"
#include "views/gamelist/ISimpleGameListView.h"
#include "views/ViewController.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include "ThemeData.h"
#include <pugixml/src/pugixml.hpp>
#include <fstream>

std::string myCollectionsName = "collections";

#define LAST_PLAYED_MAX	50

/* Handling the getting, initialization, deinitialization, saving and deletion of
 * a CollectionSystemManager Instance */
CollectionSystemManager* CollectionSystemManager::sInstance = NULL;

CollectionSystemManager::CollectionSystemManager(Window* window) : mWindow(window)
{
	CollectionSystemDecl systemDecls[] = {
		//type                  name            long name            //default sort              // theme folder            // isCustom
		{ AUTO_ALL_GAMES,       "all",          "all games",         "filename, ascending",      "auto-allgames",           false },
		{ AUTO_LAST_PLAYED,     "recent",       "last played",       "last played, descending",  "auto-lastplayed",         false },
		{ AUTO_FAVORITES,       "favorites",    "favorites",         "filename, ascending",      "auto-favorites",          false },
		{ CUSTOM_COLLECTION,    myCollectionsName,  "collections",    "filename, ascending",      "custom-collections",      true }
	};

	// create a map
	std::vector<CollectionSystemDecl> tempSystemDecl = std::vector<CollectionSystemDecl>(systemDecls, systemDecls + sizeof(systemDecls) / sizeof(systemDecls[0]));

	for (std::vector<CollectionSystemDecl>::const_iterator it = tempSystemDecl.cbegin(); it != tempSystemDecl.cend(); ++it )
	{
		mCollectionSystemDeclsIndex[(*it).name] = (*it);
	}

	// creating standard environment data
	mCollectionEnvData = new SystemEnvironmentData;
	mCollectionEnvData->mStartPath = "";
	std::vector<std::string> exts;
	mCollectionEnvData->mSearchExtensions = exts;
	mCollectionEnvData->mLaunchCommand = "";
	std::vector<PlatformIds::PlatformId> allPlatformIds;
	allPlatformIds.push_back(PlatformIds::PLATFORM_IGNORE);
	mCollectionEnvData->mPlatformIds = allPlatformIds;

	std::string path = getCollectionsFolder();
	if(!Utils::FileSystem::exists(path))
		Utils::FileSystem::createDirectory(path);

	mIsEditingCustom = false;
	mEditingCollection = "Favorites";
	mEditingCollectionSystemData = NULL;
	mCustomCollectionsBundle = NULL;
}

CollectionSystemManager::~CollectionSystemManager()
{
	assert(sInstance == this);
	removeCollectionsFromDisplayedSystems();

	// iterate the map
	for(std::map<std::string, CollectionSystemData>::const_iterator it = mCustomCollectionSystemsData.cbegin() ; it != mCustomCollectionSystemsData.cend() ; it++ )
	{
		if (it->second.isPopulated)
		{
			saveCustomCollection(it->second.system);
		}
		delete it->second.system;
	}
	sInstance = NULL;
}

CollectionSystemManager* CollectionSystemManager::get()
{
	assert(sInstance);
	return sInstance;
}

void CollectionSystemManager::init(Window* window)
{
	assert(!sInstance);
	sInstance = new CollectionSystemManager(window);
}

void CollectionSystemManager::deinit()
{
	if (sInstance)
	{
		delete sInstance;
	}
}

void CollectionSystemManager::saveCustomCollection(SystemData* sys)
{
	std::string name = sys->getName();
	std::unordered_map<std::string, FileData*> games = sys->getRootFolder()->getChildrenByFilename();
	bool found = mCustomCollectionSystemsData.find(name) != mCustomCollectionSystemsData.cend();
	if (found) {
		CollectionSystemData sysData = mCustomCollectionSystemsData.at(name);
		if (sysData.needsSave)
		{
			std::ofstream configFile;
			configFile.open(getCustomCollectionConfigPath(name));
			for(std::unordered_map<std::string, FileData*>::const_iterator iter = games.cbegin(); iter != games.cend(); ++iter)
			{
				std::string path =  iter->first;
				configFile << path << std::endl;
			}
			configFile.close();
		}
	}
	else
	{
		LOG(LogError) << "Couldn't find collection to save! " << name;
	}
}

/* Methods to load all Collections into memory, and handle enabling the active ones */
// loads all Collection Systems
void CollectionSystemManager::loadCollectionSystems(bool async)
{
	initAutoCollectionSystems();
	CollectionSystemDecl decl = mCollectionSystemDeclsIndex[myCollectionsName];
	mCustomCollectionsBundle = createNewCollectionEntry(decl.name, decl, false);
	// we will also load custom systems here
	initCustomCollectionSystems();
	if(Settings::getInstance()->getString("CollectionSystemsAuto") != "" || Settings::getInstance()->getString("CollectionSystemsCustom") != "")
	{
		// Now see which ones are enabled
		loadEnabledListFromSettings();

		// add to the main System Vector, and create Views as needed
		if (!async)
			updateSystemsList();
	}
}

// loads settings
void CollectionSystemManager::loadEnabledListFromSettings()
{
	// we parse the auto collection settings list
	std::vector<std::string> autoSelected = Utils::String::delimitedStringToVector(Settings::getInstance()->getString("CollectionSystemsAuto"), ",", true);

	// iterate the map
	for(std::map<std::string, CollectionSystemData>::iterator it = mAutoCollectionSystemsData.begin() ; it != mAutoCollectionSystemsData.end() ; it++ )
	{
		it->second.isEnabled = (std::find(autoSelected.cbegin(), autoSelected.cend(), it->first) != autoSelected.cend());
	}

	// we parse the custom collection settings list
	std::vector<std::string> customSelected = Utils::String::delimitedStringToVector(Settings::getInstance()->getString("CollectionSystemsCustom"), ",", true);

	// iterate the map
	for(std::map<std::string, CollectionSystemData>::iterator it = mCustomCollectionSystemsData.begin() ; it != mCustomCollectionSystemsData.end() ; it++ )
	{
		it->second.isEnabled = (std::find(customSelected.cbegin(), customSelected.cend(), it->first) != customSelected.cend());
	}
}

// updates enabled system list in System View
void CollectionSystemManager::updateSystemsList()
{
	// remove all Collection Systems
	removeCollectionsFromDisplayedSystems();
	// add custom enabled ones
	addEnabledCollectionsToDisplayedSystems(&mCustomCollectionSystemsData);

	if(Settings::getInstance()->getBool("SortAllSystems"))
	{
		// sort custom individual systems with other systems
		std::sort(SystemData::sSystemVector.begin(), SystemData::sSystemVector.end(), systemSort);

		// move RetroPie system to end, before auto collections
		for(auto sysIt = SystemData::sSystemVector.cbegin(); sysIt != SystemData::sSystemVector.cend(); )
		{
			if ((*sysIt)->getName() == "retropie")
			{
				SystemData* retroPieSystem = (*sysIt);
				sysIt = SystemData::sSystemVector.erase(sysIt);
				SystemData::sSystemVector.push_back(retroPieSystem);
				break;
			}
			else
			{
				sysIt++;
			}
		}
	}

	if(mCustomCollectionsBundle->getRootFolder()->getChildren().size() > 0)
	{
		mCustomCollectionsBundle->getRootFolder()->sort(getSortTypeFromString(mCollectionSystemDeclsIndex[myCollectionsName].defaultSort));
		SystemData::sSystemVector.push_back(mCustomCollectionsBundle);
	}

	// add auto enabled ones
	addEnabledCollectionsToDisplayedSystems(&mAutoCollectionSystemsData);

	// create views for collections, before reload
	for(auto sysIt = SystemData::sSystemVector.cbegin(); sysIt != SystemData::sSystemVector.cend(); sysIt++)
	{
		if ((*sysIt)->isCollection())
		{
			ViewController::get()->getGameListView((*sysIt));
		}
	}

	// if we were editing a custom collection, and it's no longer enabled, exit edit mode
	if(mIsEditingCustom && !mEditingCollectionSystemData->isEnabled)
	{
		exitEditMode();
	}
}

/* Methods to manage collection files related to a source FileData */
// updates all collection files related to the source file
void CollectionSystemManager::refreshCollectionSystems(FileData* file)
{
	if (!file->getSystem()->isGameSystem() || file->getType() != GAME)
		return;

	std::map<std::string, CollectionSystemData> allCollections;
	allCollections.insert(mAutoCollectionSystemsData.cbegin(), mAutoCollectionSystemsData.cend());
	allCollections.insert(mCustomCollectionSystemsData.cbegin(), mCustomCollectionSystemsData.cend());

	for(auto sysDataIt = allCollections.cbegin(); sysDataIt != allCollections.cend(); sysDataIt++)
	{
		updateCollectionSystem(file, sysDataIt->second);
	}
}

void CollectionSystemManager::updateCollectionSystem(FileData* file, CollectionSystemData sysData)
{
	if (sysData.isPopulated)
	{
		// collection files use the full path as key, to avoid clashes
		std::string key = file->getFullPath();

		SystemData* curSys = sysData.system;
		const std::unordered_map<std::string, FileData*>& children = curSys->getRootFolder()->getChildrenByFilename();
		bool found = children.find(key) != children.cend();
		FileData* rootFolder = curSys->getRootFolder();
		FileFilterIndex* fileIndex = curSys->getIndex();
		std::string name = curSys->getName();

		if (found) {
			// if we found it, we need to update it
			FileData* collectionEntry = children.at(key);
			// remove from index, so we can re-index metadata after refreshing
			fileIndex->removeFromIndex(collectionEntry);
			collectionEntry->refreshMetadata();
			// found and we are removing
			if (name == "favorites" && file->metadata.get("favorite") == "false") {
				// need to check if still marked as favorite, if not remove
				ViewController::get()->getGameListView(curSys).get()->remove(collectionEntry, false);
			}
			else
			{
				// re-index with new metadata
				fileIndex->addToIndex(collectionEntry);
				ViewController::get()->onFileChanged(collectionEntry, FILE_METADATA_CHANGED);
			}
		}
		else
		{
			// we didn't find it here - we need to check if we should add it
			if (name == "recent" && file->metadata.get("playcount") > "0" && includeFileInAutoCollections(file) ||
				name == "favorites" && file->metadata.get("favorite") == "true") {
				CollectionFileData* newGame = new CollectionFileData(file, curSys);
				rootFolder->addChild(newGame);
				fileIndex->addToIndex(newGame);
				ViewController::get()->onFileChanged(file, FILE_METADATA_CHANGED);
				ViewController::get()->getGameListView(curSys)->onFileChanged(newGame, FILE_METADATA_CHANGED);
			}
		}
		rootFolder->sort(getSortTypeFromString(mCollectionSystemDeclsIndex[name].defaultSort));
		if (name == "recent")
		{
			trimCollectionCount(rootFolder, LAST_PLAYED_MAX);
			ViewController::get()->onFileChanged(rootFolder, FILE_METADATA_CHANGED);
		}
		else
			ViewController::get()->onFileChanged(rootFolder, FILE_SORTED);
	}
}

void CollectionSystemManager::trimCollectionCount(FileData* rootFolder, int limit)
{
	SystemData* curSys = rootFolder->getSystem();
	while ((int)rootFolder->getChildrenListToDisplay().size() > limit)
	{
		CollectionFileData* gameToRemove = (CollectionFileData*)rootFolder->getChildrenListToDisplay().back();
		ViewController::get()->getGameListView(curSys).get()->remove(gameToRemove, false);
	}
}

// deletes all collection files from collection systems related to the source file
void CollectionSystemManager::deleteCollectionFiles(FileData* file)
{
	// collection files use the full path as key, to avoid clashes
	std::string key = file->getFullPath();
	// find games in collection systems
	std::map<std::string, CollectionSystemData> allCollections;
	allCollections.insert(mAutoCollectionSystemsData.cbegin(), mAutoCollectionSystemsData.cend());
	allCollections.insert(mCustomCollectionSystemsData.cbegin(), mCustomCollectionSystemsData.cend());

	for(auto sysDataIt = allCollections.begin(); sysDataIt != allCollections.end(); sysDataIt++)
	{
		if (sysDataIt->second.isPopulated)
		{
			const std::unordered_map<std::string, FileData*>& children = (sysDataIt->second.system)->getRootFolder()->getChildrenByFilename();

			bool found = children.find(key) != children.cend();
			if (found) {
				sysDataIt->second.needsSave = true;
				FileData* collectionEntry = children.at(key);
				SystemData* systemViewToUpdate = getSystemToView(sysDataIt->second.system);
				ViewController::get()->getGameListView(systemViewToUpdate).get()->remove(collectionEntry, false);
			}
		}
	}
}

// returns whether the current theme is compatible with Automatic or Custom Collections
bool CollectionSystemManager::isThemeGenericCollectionCompatible(bool genericCustomCollections)
{
	std::vector<std::string> cfgSys = getCollectionThemeFolders(genericCustomCollections);
	for(auto sysIt = cfgSys.cbegin(); sysIt != cfgSys.cend(); sysIt++)
	{
		if(!themeFolderExists(*sysIt))
			return false;
	}
	return true;
}

bool CollectionSystemManager::isThemeCustomCollectionCompatible(std::vector<std::string> stringVector)
{
	if (isThemeGenericCollectionCompatible(true))
		return true;

	// get theme path
	auto themeSets = ThemeData::getThemeSets();
	auto set = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
	if(set != themeSets.cend())
	{
		std::string defaultThemeFilePath = set->second.path + "/theme.xml";
		if (Utils::FileSystem::exists(defaultThemeFilePath))
		{
			return true;
		}
	}

	for(auto sysIt = stringVector.cbegin(); sysIt != stringVector.cend(); sysIt++)
	{
		if(!themeFolderExists(*sysIt))
			return false;
	}
	return true;
}

std::string CollectionSystemManager::getValidNewCollectionName(std::string inName, int index)
{
	std::string name = inName;

	if(index == 0)
	{
		size_t remove = std::string::npos;

		// get valid name
		while((remove = name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-[]() ")) != std::string::npos)
		{
			name.erase(remove, 1);
		}
	}
	else
	{
		name += " (" + std::to_string(index) + ")";
	}

	if(name == "")
	{
		name = "New Collection";
	}

	if(name != inName)
	{
		LOG(LogInfo) << "Had to change name, from: " << inName << " to: " << name;
	}

	// get used systems in es_systems.cfg
	std::vector<std::string> systemsInUse = getSystemsFromConfig();
	// get folders assigned to custom collections
	std::vector<std::string> autoSys = getCollectionThemeFolders(false);
	// get folder assigned to custom collections
	std::vector<std::string> customSys = getCollectionThemeFolders(true);
	// get folders assigned to user collections
	std::vector<std::string> userSys = getUserCollectionThemeFolders();
	// add them all to the list of systems in use
	systemsInUse.insert(systemsInUse.cend(), autoSys.cbegin(), autoSys.cend());
	systemsInUse.insert(systemsInUse.cend(), customSys.cbegin(), customSys.cend());
	systemsInUse.insert(systemsInUse.cend(), userSys.cbegin(), userSys.cend());
	for(auto sysIt = systemsInUse.cbegin(); sysIt != systemsInUse.cend(); sysIt++)
	{
		if (*sysIt == name)
		{
			if(index > 0) {
				name = name.substr(0, name.size()-4);
			}
			return getValidNewCollectionName(name, index+1);
		}
	}
	// if it matches one of the custom collections reserved names
	if (mCollectionSystemDeclsIndex.find(name) != mCollectionSystemDeclsIndex.cend())
		return getValidNewCollectionName(name, index+1);
	return name;
}

void CollectionSystemManager::setEditMode(std::string collectionName)
{
	if (mCustomCollectionSystemsData.find(collectionName) == mCustomCollectionSystemsData.cend())
	{
		LOG(LogError) << "Tried to edit a non-existing collection: " << collectionName;
		return;
	}
	mIsEditingCustom = true;
	mEditingCollection = collectionName;

	CollectionSystemData* sysData = &(mCustomCollectionSystemsData.at(mEditingCollection));
	if (!sysData->isPopulated)
	{
		populateCustomCollection(sysData);
	}
	// if it's bundled, this needs to be the bundle system
	mEditingCollectionSystemData = sysData;

	GuiInfoPopup* s = new GuiInfoPopup(mWindow, "Editing the '" + Utils::String::toUpper(collectionName) + "' Collection. Add/remove games with Y.", 10000);
	mWindow->setInfoPopup(s);
}

void CollectionSystemManager::exitEditMode()
{
	GuiInfoPopup* s = new GuiInfoPopup(mWindow, "Finished editing the '" + mEditingCollection + "' Collection.", 4000);
	mWindow->setInfoPopup(s);
	mIsEditingCustom = false;
	mEditingCollection = "Favorites";

	mEditingCollectionSystemData->system->onMetaDataSavePoint();
}

// adds or removes a game from a specific collection
bool CollectionSystemManager::toggleGameInCollection(FileData* file, int presscount)
{
	if (file->getType() == GAME)
	{
		GuiInfoPopup* s;
		bool adding = true;
		std::string name = file->getName();
		std::string sysName = mEditingCollection;
		if (mIsEditingCustom)
		{
			SystemData* sysData = mEditingCollectionSystemData->system;
			mEditingCollectionSystemData->needsSave = true;
			if (!mEditingCollectionSystemData->isPopulated)
			{
				populateCustomCollection(mEditingCollectionSystemData);
			}
			std::string key = file->getFullPath();
			FileData* rootFolder = sysData->getRootFolder();
			const std::unordered_map<std::string, FileData*>& children = rootFolder->getChildrenByFilename();
			bool found = children.find(key) != children.cend();
			FileFilterIndex* fileIndex = sysData->getIndex();
			std::string name = sysData->getName();

			SystemData* systemViewToUpdate = getSystemToView(sysData);

			if (found) {
				if (needDoublePress(presscount)) {
					return true;
				}
				adding = false;
				// if we found it, we need to remove it
				FileData* collectionEntry = children.at(key);
				// remove from index
				fileIndex->removeFromIndex(collectionEntry);
				// remove from bundle index as well, if needed
				if(systemViewToUpdate != sysData)
				{
					systemViewToUpdate->getIndex()->removeFromIndex(collectionEntry);
				}
				ViewController::get()->getGameListView(systemViewToUpdate).get()->remove(collectionEntry, false);
			}
			else
			{
				// we didn't find it here, we should add it
				CollectionFileData* newGame = new CollectionFileData(file, sysData);
				rootFolder->addChild(newGame);
				fileIndex->addToIndex(newGame);
				ViewController::get()->getGameListView(systemViewToUpdate)->onFileChanged(newGame, FILE_METADATA_CHANGED);
				rootFolder->sort(getSortTypeFromString(mEditingCollectionSystemData->decl.defaultSort));
				ViewController::get()->onFileChanged(systemViewToUpdate->getRootFolder(), FILE_SORTED);
				// add to bundle index as well, if needed
				if(systemViewToUpdate != sysData)
				{
					systemViewToUpdate->getIndex()->addToIndex(newGame);
				}
			}
			updateCollectionFolderMetadata(sysData);
		}
		else
		{
			file->getSourceFileData()->getSystem()->getIndex()->removeFromIndex(file);
			MetaDataList* md = &file->getSourceFileData()->metadata;
			std::string value = md->get("favorite");
			if (value == "false")
			{
				md->set("favorite", "true");
			}
			else
			{
				if (needDoublePress(presscount)) {
					return true;
				}
				adding = false;
				md->set("favorite", "false");
			}
			file->getSourceFileData()->getSystem()->getIndex()->addToIndex(file);

			file->getSourceFileData()->getSystem()->onMetaDataSavePoint();

			refreshCollectionSystems(file->getSourceFileData());
		}
		if (adding)
		{
			s = new GuiInfoPopup(mWindow, "Added '" + Utils::String::removeParenthesis(name) + "' to '" + Utils::String::toUpper(sysName) + "'", 4000);
		}
		else
		{
			s = new GuiInfoPopup(mWindow, "Removed '" + Utils::String::removeParenthesis(name) + "' from '" + Utils::String::toUpper(sysName) + "'", 4000);
		}
		mWindow->setInfoPopup(s);
		return true;
	}
	return false;
}


SystemData* CollectionSystemManager::getSystemToView(SystemData* sys)
{
	SystemData* systemToView = sys;
	FileData* rootFolder = sys->getRootFolder();

	FileData* bundleRootFolder = mCustomCollectionsBundle->getRootFolder();
	const std::unordered_map<std::string, FileData*>& bundleChildren = bundleRootFolder->getChildrenByFilename();

	// is the rootFolder bundled in the "My Collections" system?
	bool sysFoundInBundle = bundleChildren.find(rootFolder->getKey()) != bundleChildren.cend();

	if (sysFoundInBundle && sys->isCollection())
	{
		systemToView = mCustomCollectionsBundle;
	}
	return systemToView;
}

/* Handles loading a collection system, creating an empty one, and populating on demand */
// loads Automatic Collection systems (All, Favorites, Last Played)
void CollectionSystemManager::initAutoCollectionSystems()
{
	for(std::map<std::string, CollectionSystemDecl>::const_iterator it = mCollectionSystemDeclsIndex.cbegin() ; it != mCollectionSystemDeclsIndex.cend() ; it++ )
	{
		CollectionSystemDecl sysDecl = it->second;
		if (!sysDecl.isCustom)
		{
			createNewCollectionEntry(sysDecl.name, sysDecl);
		}
	}
}

// this may come in handy if at any point in time in the future we want to
// automatically generate metadata for a folder
void CollectionSystemManager::updateCollectionFolderMetadata(SystemData* sys)
{
	FileData* rootFolder = sys->getRootFolder();

	std::string desc = "This collection is empty.";
	std::string rating = "0";
	std::string players = "1";
	std::string releasedate = "N/A";
	std::string developer = "None";
	std::string genre = "None";
	std::string video = "";
	std::string thumbnail = "";
	std::string image = "";

	std::unordered_map<std::string, FileData*> games = rootFolder->getChildrenByFilename();

	if(games.size() > 0)
	{
		std::string games_list = "";
		int games_counter = 0;
		for(std::unordered_map<std::string, FileData*>::const_iterator iter = games.cbegin(); iter != games.cend(); ++iter)
		{
			games_counter++;
			FileData* file = iter->second;

			std::string new_rating = file->metadata.get("rating");
			std::string new_releasedate = file->metadata.get("releasedate");
			std::string new_developer = file->metadata.get("developer");
			std::string new_genre = file->metadata.get("genre");
			std::string new_players = file->metadata.get("players");

			rating = (new_rating > rating ? (new_rating != "" ? new_rating : rating) : rating);
			players = (new_players > players ? (new_players != "" ? new_players : players) : players);
			releasedate = (new_releasedate < releasedate ? (new_releasedate != "" ? new_releasedate : releasedate) : releasedate);
			developer = (developer == "None" ? new_developer : (new_developer != developer ? "Various" : new_developer));
			genre = (genre == "None" ? new_genre : (new_genre != genre ? "Various" : new_genre));

			switch(games_counter)
			{
				case 2:
				case 3:
					games_list += ", ";
				case 1:
					games_list += "'" + file->getName() + "'";
					break;
				case 4:
					games_list += " among other titles.";
			}
		}

		desc = "This collection contains " + std::to_string(games_counter) + " game"
				+ (games_counter == 1 ? "" : "s") + ", including " + games_list;

		FileData* randomGame = sys->getRandomGame();

		video = randomGame->getVideoPath();
		thumbnail = randomGame->getThumbnailPath();
		image = randomGame->getImagePath();
	}


	rootFolder->metadata.set("desc", desc);
	rootFolder->metadata.set("rating", rating);
	rootFolder->metadata.set("players", players);
	rootFolder->metadata.set("genre", genre);
	rootFolder->metadata.set("releasedate", releasedate);
	rootFolder->metadata.set("developer", developer);
	rootFolder->metadata.set("video", video);
	rootFolder->metadata.set("thumbnail", thumbnail);
	rootFolder->metadata.set("image", image);
}

void CollectionSystemManager::initCustomCollectionSystems()
{
	std::vector<std::string> systems = getCollectionsFromConfigFolder();
	for (auto nameIt = systems.cbegin(); nameIt != systems.cend(); nameIt++)
	{
		addNewCustomCollection(*nameIt);
	}
}

SystemData* CollectionSystemManager::getAllGamesCollection()
{
	CollectionSystemData* allSysData = &mAutoCollectionSystemsData["all"];
	if (!allSysData->isPopulated)
	{
		populateAutoCollection(allSysData);
	}
	return allSysData->system;
}

SystemData* CollectionSystemManager::addNewCustomCollection(std::string name)
{
	CollectionSystemDecl decl = mCollectionSystemDeclsIndex[myCollectionsName];
	decl.themeFolder = name;
	decl.name = name;
	decl.longName = name;
	return createNewCollectionEntry(name, decl);
}

// creates a new, empty Collection system, based on the name and declaration
SystemData* CollectionSystemManager::createNewCollectionEntry(std::string name, CollectionSystemDecl sysDecl, bool index)
{
	SystemData* newSys = new SystemData(name, sysDecl.longName, mCollectionEnvData, sysDecl.themeFolder, true);

	CollectionSystemData newCollectionData;
	newCollectionData.system = newSys;
	newCollectionData.decl = sysDecl;
	newCollectionData.isEnabled = false;
	newCollectionData.isPopulated = false;
	newCollectionData.needsSave = false;

	if (index)
	{
		if (!sysDecl.isCustom)
		{
			mAutoCollectionSystemsData[name] = newCollectionData;
		}
		else
		{
			mCustomCollectionSystemsData[name] = newCollectionData;
		}
	}

	return newSys;
}

// populates an Automatic Collection System
void CollectionSystemManager::populateAutoCollection(CollectionSystemData* sysData)
{
	SystemData* newSys = sysData->system;
	CollectionSystemDecl sysDecl = sysData->decl;
	FileData* rootFolder = newSys->getRootFolder();
	FileFilterIndex* index = newSys->getIndex();
	for(auto sysIt = SystemData::sSystemVector.cbegin(); sysIt != SystemData::sSystemVector.cend(); sysIt++)
	{
		// we won't iterate all collections
		if ((*sysIt)->isGameSystem() && !(*sysIt)->isCollection()) {
			std::vector<FileData*> files = (*sysIt)->getRootFolder()->getFilesRecursive(GAME);
			for(auto gameIt = files.cbegin(); gameIt != files.cend(); gameIt++)
			{
				bool include = includeFileInAutoCollections((*gameIt));
				switch(sysDecl.type) {
					case AUTO_LAST_PLAYED:
						include = include && (*gameIt)->metadata.get("playcount") > "0";
						break;
					case AUTO_FAVORITES:
						// we may still want to add files we don't want in auto collections in "favorites"
						include = (*gameIt)->metadata.get("favorite") == "true";
						break;
				}

				if (include) {
					CollectionFileData* newGame = new CollectionFileData(*gameIt, newSys);
					rootFolder->addChild(newGame);
					index->addToIndex(newGame);
				}
			}
		}
	}
	rootFolder->sort(getSortTypeFromString(sysDecl.defaultSort));
	if (sysDecl.type == AUTO_LAST_PLAYED)
		trimCollectionCount(rootFolder, LAST_PLAYED_MAX);
	sysData->isPopulated = true;
}

// populates a Custom Collection System
void CollectionSystemManager::populateCustomCollection(CollectionSystemData* sysData)
{
	SystemData* newSys = sysData->system;
	sysData->isPopulated = true;
	CollectionSystemDecl sysDecl = sysData->decl;
	std::string path = getCustomCollectionConfigPath(newSys->getName());

	if(!Utils::FileSystem::exists(path))
	{
		LOG(LogInfo) << "Couldn't find custom collection config file at " << path;
		return;
	}
	LOG(LogInfo) << "Loading custom collection config file at " << path;

	FileData* rootFolder = newSys->getRootFolder();
	FileFilterIndex* index = newSys->getIndex();

	// get Configuration for this Custom System
	std::ifstream input(path);

	// get all files map
	std::unordered_map<std::string,FileData*> allFilesMap = getAllGamesCollection()->getRootFolder()->getChildrenByFilename();

	// iterate list of files in config file

	for(std::string gameKey; getline(input, gameKey); )
	{
		std::unordered_map<std::string,FileData*>::const_iterator it = allFilesMap.find(gameKey);
		if (it != allFilesMap.cend()) {
			CollectionFileData* newGame = new CollectionFileData(it->second, newSys);
			rootFolder->addChild(newGame);
			index->addToIndex(newGame);
		}
		else
		{
			LOG(LogInfo) << "Couldn't find game referenced at '" << gameKey << "' for system config '" << path << "'";
		}
	}
	rootFolder->sort(getSortTypeFromString(sysDecl.defaultSort));
	updateCollectionFolderMetadata(newSys);
}

/* Handle System View removal and insertion of Collections */
void CollectionSystemManager::removeCollectionsFromDisplayedSystems()
{
	// remove all Collection Systems
	for(auto sysIt = SystemData::sSystemVector.cbegin(); sysIt != SystemData::sSystemVector.cend(); )
	{
		if ((*sysIt)->isCollection())
		{
			sysIt = SystemData::sSystemVector.erase(sysIt);
		}
		else
		{
			sysIt++;
		}
	}

	// remove all custom collections in bundle
	// this should not delete the objects from memory!
	FileData* customRoot = mCustomCollectionsBundle->getRootFolder();
	std::vector<FileData*> mChildren = customRoot->getChildren();
	for(auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
	{
		customRoot->removeChild(*it);
	}
	// clear index
	mCustomCollectionsBundle->getIndex()->resetIndex();
	// remove view so it's re-created as needed
	ViewController::get()->removeGameListView(mCustomCollectionsBundle);
}

void CollectionSystemManager::addEnabledCollectionsToDisplayedSystems(std::map<std::string, CollectionSystemData>* colSystemData)
{
	// add auto enabled ones
	for(std::map<std::string, CollectionSystemData>::iterator it = colSystemData->begin() ; it != colSystemData->end() ; it++ )
	{
		if(it->second.isEnabled)
		{
			// check if populated, otherwise populate
			if (!it->second.isPopulated)
			{
				if(it->second.decl.isCustom)
				{
					populateCustomCollection(&(it->second));
				}
				else
				{
					populateAutoCollection(&(it->second));
				}
			}
			// check if it has its own view
			if(!it->second.decl.isCustom || themeFolderExists(it->first) || !Settings::getInstance()->getBool("UseCustomCollectionsSystem"))
			{
				// exists theme folder, or we chose not to bundle it under the custom-collections system
				// so we need to create a view
				SystemData::sSystemVector.push_back(it->second.system);
			}
			else
			{
				FileData* newSysRootFolder = it->second.system->getRootFolder();
				mCustomCollectionsBundle->getRootFolder()->addChild(newSysRootFolder);
				mCustomCollectionsBundle->getIndex()->importIndex(it->second.system->getIndex());
			}
		}
	}
}

/* Auxiliary methods to get available custom collection possibilities */
std::vector<std::string> CollectionSystemManager::getSystemsFromConfig()
{
	std::vector<std::string> systems;
	std::string path = SystemData::getConfigPath(false);

	if(!Utils::FileSystem::exists(path))
	{
		return systems;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(path.c_str());

	if(!res)
	{
		return systems;
	}

	//actually read the file
	pugi::xml_node systemList = doc.child("systemList");

	if(!systemList)
	{
		return systems;
	}

	for(pugi::xml_node system = systemList.child("system"); system; system = system.next_sibling("system"))
	{
		// theme folder
		std::string themeFolder = system.child("theme").text().get();
		systems.push_back(themeFolder);
	}
	std::sort(systems.begin(), systems.end());
	return systems;
}

// gets all folders from the current theme path
std::vector<std::string> CollectionSystemManager::getSystemsFromTheme()
{
	std::vector<std::string> systems;

	auto themeSets = ThemeData::getThemeSets();
	if(themeSets.empty())
	{
		// no theme sets available
		return systems;
	}

	std::map<std::string, ThemeSet>::const_iterator set = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
	if(set == themeSets.cend())
	{
		// currently selected theme set is missing, so just pick the first available set
		set = themeSets.cbegin();
		Settings::getInstance()->setString("ThemeSet", set->first);
	}

	std::string themePath = set->second.path;

	if (Utils::FileSystem::exists(themePath))
	{
		Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(themePath);

		for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin(); it != dirContent.cend(); ++it)
		{
			if (Utils::FileSystem::isDirectory(*it))
			{
				//... here you have a directory
				std::string folder = *it;
				folder = folder.substr(themePath.size()+1);

				if(Utils::FileSystem::exists(set->second.getThemePath(folder)))
				{
					systems.push_back(folder);
				}
			}
		}
	}
	std::sort(systems.begin(), systems.end());
	return systems;
}

// returns the unused folders from current theme path
std::vector<std::string> CollectionSystemManager::getUnusedSystemsFromTheme()
{
	// get used systems in es_systems.cfg
	std::vector<std::string> systemsInUse = getSystemsFromConfig();
	// get available folders in theme
	std::vector<std::string> themeSys = getSystemsFromTheme();
	// get folders assigned to custom collections
	std::vector<std::string> autoSys = getCollectionThemeFolders(false);
	// get folder assigned to custom collections
	std::vector<std::string> customSys = getCollectionThemeFolders(true);
	// get folders assigned to user collections
	std::vector<std::string> userSys = getUserCollectionThemeFolders();
	// add them all to the list of systems in use
	systemsInUse.insert(systemsInUse.cend(), autoSys.cbegin(), autoSys.cend());
	systemsInUse.insert(systemsInUse.cend(), customSys.cbegin(), customSys.cend());
	systemsInUse.insert(systemsInUse.cend(), userSys.cbegin(), userSys.cend());

	for(auto sysIt = themeSys.cbegin(); sysIt != themeSys.cend(); )
	{
		if (std::find(systemsInUse.cbegin(), systemsInUse.cend(), *sysIt) != systemsInUse.cend())
		{
			sysIt = themeSys.erase(sysIt);
		}
		else
		{
			sysIt++;
		}
	}
	return themeSys;
}

// returns which collection config files exist in the user folder
std::vector<std::string> CollectionSystemManager::getCollectionsFromConfigFolder()
{
	std::vector<std::string> systems;
	std::string configPath = getCollectionsFolder();

	if (Utils::FileSystem::exists(configPath))
	{
		Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(configPath);
		for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin(); it != dirContent.cend(); ++it)
		{
			if (Utils::FileSystem::isRegularFile(*it))
			{
				// it's a file
				std::string filename = Utils::FileSystem::getFileName(*it);

				// need to confirm filename matches config format
				if (filename != "custom-.cfg" && Utils::String::startsWith(filename, "custom-") && Utils::String::endsWith(filename, ".cfg"))
				{
					filename = filename.substr(7, filename.size()-11);
					systems.push_back(filename);
				}
				else
				{
					LOG(LogInfo) << "Found non-collection config file in collections folder: " << filename;
				}
			}
		}
	}
	return systems;
}

// returns the theme folders for Automatic Collections (All, Favorites, Last Played) or generic Custom Collections folder
std::vector<std::string> CollectionSystemManager::getCollectionThemeFolders(bool custom)
{
	std::vector<std::string> systems;
	for(std::map<std::string, CollectionSystemDecl>::const_iterator it = mCollectionSystemDeclsIndex.cbegin() ; it != mCollectionSystemDeclsIndex.cend() ; it++ )
	{
		CollectionSystemDecl sysDecl = it->second;
		if (sysDecl.isCustom == custom)
		{
			systems.push_back(sysDecl.themeFolder);
		}
	}
	return systems;
}

// returns the theme folders in use for the user-defined Custom Collections
std::vector<std::string> CollectionSystemManager::getUserCollectionThemeFolders()
{
	std::vector<std::string> systems;
	for(std::map<std::string, CollectionSystemData>::const_iterator it = mCustomCollectionSystemsData.cbegin() ; it != mCustomCollectionSystemsData.cend() ; it++ )
	{
		systems.push_back(it->second.decl.themeFolder);
	}
	return systems;
}

// returns whether a specific folder exists in the theme
bool CollectionSystemManager::themeFolderExists(std::string folder)
{
	std::vector<std::string> themeSys = getSystemsFromTheme();
	return std::find(themeSys.cbegin(), themeSys.cend(), folder) != themeSys.cend();
}

bool CollectionSystemManager::includeFileInAutoCollections(FileData* file)
{
	// we exclude non-game files from collections (i.e. "kodi", entries from non-game systems)
	// if/when there are more in the future, maybe this can be a more complex method, with a proper list
	// but for now a simple string comparison is more performant
	return file->getName() != "kodi" && file->getSystem()->isGameSystem();
}


bool CollectionSystemManager::needDoublePress(int presscount) {
	if (Settings::getInstance()->getBool("DoublePressRemovesFromFavs") && presscount < 2) {
		GuiInfoPopup* toast = new GuiInfoPopup(mWindow, "Press again to remove from '" + Utils::String::toUpper(mEditingCollection)
		+ "'", ISimpleGameListView::DOUBLE_PRESS_DETECTION_DURATION, 100, 200);
		mWindow->setInfoPopup(toast);
		return true;
	}
	return false;
}

std::string getCustomCollectionConfigPath(std::string collectionName)
{
	return getCollectionsFolder() + "/custom-" + collectionName + ".cfg";
}

std::string getCollectionsFolder()
{
	return Utils::FileSystem::getGenericPath(Utils::FileSystem::getHomePath() + "/.emulationstation/collections");
}

bool systemSort(SystemData* sys1, SystemData* sys2)
{
	std::string name1 = Utils::String::toUpper(sys1->getName());
	std::string name2 = Utils::String::toUpper(sys2->getName());
	return name1.compare(name2) < 0;
}
