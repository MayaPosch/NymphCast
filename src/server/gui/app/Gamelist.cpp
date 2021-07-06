#include "Gamelist.h"

#include <chrono>

#include "utils/FileSystemUtil.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include <pugixml/src/pugixml.hpp>

FileData* findOrCreateFile(SystemData* system, const std::string& path, FileType type)
{
	// first, verify that path is within the system's root folder
	FileData* root = system->getRootFolder();
	bool contains = false;
	std::string relative = Utils::FileSystem::removeCommonPath(path, root->getPath(), contains);

	if(!contains)
	{
		LOG(LogError) << "File path \"" << path << "\" is outside system path \"" << system->getStartPath() << "\"";
		return NULL;
	}

	Utils::FileSystem::stringList pathList = Utils::FileSystem::getPathList(relative);
	auto path_it = pathList.begin();
	FileData* treeNode = root;
	bool found = false;
	while(path_it != pathList.end())
	{
		const std::unordered_map<std::string, FileData*>& children = treeNode->getChildrenByFilename();

		std::string key = *path_it;
		found = children.find(key) != children.cend();
		if (found) {
			treeNode = children.at(key);
		}

		// this is the end
		if(path_it == --pathList.end())
		{
			if(found)
				return treeNode;

			if(type == FOLDER)
			{
				LOG(LogWarning) << "gameList: folder doesn't already exist, won't create";
				return NULL;
			}

			FileData* file = new FileData(type, path, system->getSystemEnvData(), system);

			// skipping arcade assets from gamelist
			if(!file->isArcadeAsset())
			{
				treeNode->addChild(file);
			}
			return file;
		}

		if(!found)
		{
			// don't create folders unless it's leading up to a game
			// if type is a folder it's gonna be empty, so don't bother
			if(type == FOLDER)
			{
				LOG(LogWarning) << "gameList: folder doesn't already exist, won't create";
				return NULL;
			}

			// create missing folder
			FileData* folder = new FileData(FOLDER, Utils::FileSystem::getStem(treeNode->getPath()) + "/" + *path_it, system->getSystemEnvData(), system);
			treeNode->addChild(folder);
			treeNode = folder;
		}

		path_it++;
	}

	return NULL;
}

void parseGamelist(SystemData* system)
{
	bool trustGamelist = Settings::getInstance()->getBool("ParseGamelistOnly");
	std::string xmlpath = system->getGamelistPath(false);

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

	pugi::xml_node root = doc.child("gameList");
	if(!root)
	{
		LOG(LogError) << "Could not find <gameList> node in gamelist \"" << xmlpath << "\"!";
		return;
	}

	std::string relativeTo = system->getStartPath();

	const char* tagList[2] = { "game", "folder" };
	FileType typeList[2] = { GAME, FOLDER };
	for(int i = 0; i < 2; i++)
	{
		const char* tag = tagList[i];
		FileType type = typeList[i];
		for(pugi::xml_node fileNode = root.child(tag); fileNode; fileNode = fileNode.next_sibling(tag))
		{
			const std::string path = Utils::FileSystem::resolveRelativePath(fileNode.child("path").text().get(), relativeTo, false);

			if(!trustGamelist && !Utils::FileSystem::exists(path))
			{
				LOG(LogWarning) << "File \"" << path << "\" does not exist! Ignoring.";
				continue;
			}

			FileData* file = findOrCreateFile(system, path, type);
			if(!file)
			{
				LOG(LogError) << "Error finding/creating FileData for \"" << path << "\", skipping.";
				continue;
			}
			else if(!file->isArcadeAsset())
			{
				std::string defaultName = file->metadata.get("name");
				file->metadata = MetaDataList::createFromXML(GAME_METADATA, fileNode, relativeTo);

				//make sure name gets set if one didn't exist
				if(file->metadata.get("name").empty())
					file->metadata.set("name", defaultName);

				file->metadata.resetChangedFlag();
			}
		}
	}
}

void addFileDataNode(pugi::xml_node& parent, const FileData* file, const char* tag, SystemData* system)
{
	//create game and add to parent node
	pugi::xml_node newNode = parent.append_child(tag);

	//write metadata
	file->metadata.appendToXML(newNode, true, system->getStartPath());

	if(newNode.children().begin() == newNode.child("name") //first element is name
		&& ++newNode.children().begin() == newNode.children().end() //theres only one element
		&& newNode.child("name").text().get() == file->getDisplayName()) //the name is the default
	{
		//if the only info is the default name, don't bother with this node
		//delete it and ultimately do nothing
		parent.remove_child(newNode);
	}else{
		//there's something useful in there so we'll keep the node, add the path

		// try and make the path relative if we can so things still work if we change the rom folder location in the future
		newNode.prepend_child("path").text().set(Utils::FileSystem::createRelativePath(file->getPath(), system->getStartPath(), false).c_str());
	}
}

void updateGamelist(SystemData* system)
{
	//We do this by reading the XML again, adding changes and then writing it back,
	//because there might be information missing in our systemdata which would then miss in the new XML.
	//We have the complete information for every game though, so we can simply remove a game
	//we already have in the system from the XML, and then add it back from its GameData information...

	if(Settings::getInstance()->getBool("IgnoreGamelist"))
		return;

	pugi::xml_document doc;
	pugi::xml_node root;
	std::string xmlReadPath = system->getGamelistPath(false);

	std::string relativeTo = system->getStartPath();

	if(Utils::FileSystem::exists(xmlReadPath))
	{
		//parse an existing file first
		pugi::xml_parse_result result = doc.load_file(xmlReadPath.c_str());

		if(!result)
		{
			LOG(LogError) << "Error parsing XML file \"" << xmlReadPath << "\"!\n	" << result.description();
			return;
		}

		root = doc.child("gameList");
		if(!root)
		{
			LOG(LogError) << "Could not find <gameList> node in gamelist \"" << xmlReadPath << "\"!";
			return;
		}
	}else{
		//set up an empty gamelist to append to
		root = doc.append_child("gameList");
	}

	std::vector<FileData*> changedGames;
	std::vector<FileData*> changedFolders;

	//now we have all the information from the XML. now iterate through all our games and add information from there
	FileData* rootFolder = system->getRootFolder();
	if (rootFolder != nullptr)
	{
		int numUpdated = 0;

		//get only files, no folders
		std::vector<FileData*> files = rootFolder->getFilesRecursive(GAME | FOLDER);
		
		// Stage 1: iterate through all files in memory, checking for changes
		for(std::vector<FileData*>::const_iterator fit = files.cbegin(); fit != files.cend(); ++fit)
		{

			// do not touch if it wasn't changed anyway
			if (!(*fit)->metadata.wasChanged())
				continue;
			
			// adding item to changed list
			if ((*fit)->getType() == GAME) 
			{
				changedGames.push_back((*fit));	
			}
			else 
			{
				changedFolders.push_back((*fit));
			}
		}


		// Stage 2: iterate XML if needed, to remove and add changed items
		const char* tagList[2] = { "game", "folder" };
		FileType typeList[2] = { GAME, FOLDER };
		std::vector<FileData*> changedList[2] = { changedGames, changedFolders };
		
		for(int i = 0; i < 2; i++)
		{
			const char* tag = tagList[i];
			std::vector<FileData*> changes = changedList[i];

			// if changed items of this type
			if (changes.size() > 0) {
				// check if the item already exists in the XML
				// if it does, remove all corresponding items before adding
				for(pugi::xml_node fileNode = root.child(tag); fileNode; )
				{
					pugi::xml_node pathNode = fileNode.child("path");

					// we need this as we were deleting the iterator and things would become inconsistent
					pugi::xml_node nextNode = fileNode.next_sibling(tag);

					if(!pathNode)
					{
						LOG(LogError) << "<" << tag << "> node contains no <path> child!";
						continue;
					}

					// apply the same transformation as in Gamelist::parseGamelist
					std::string xmlpath = Utils::FileSystem::resolveRelativePath(pathNode.text().get(), relativeTo, false);
					
					for(std::vector<FileData*>::const_iterator cfit = changes.cbegin(); cfit != changes.cend(); ++cfit)
					{
						if(xmlpath == (*cfit)->getPath())
						{
							// found it
							root.remove_child(fileNode);
							break;
						}
					}
					fileNode = nextNode;

				}

				// add items to XML
				for(std::vector<FileData*>::const_iterator cfit = changes.cbegin(); cfit != changes.cend(); ++cfit)
				{
					// it was either removed or never existed to begin with; either way, we can add it now
					addFileDataNode(root, *cfit, tag, system);
					++numUpdated;
				}
			}
		}

		// now write the file

		if (numUpdated > 0) {
			const auto startTs = std::chrono::system_clock::now();

			//make sure the folders leading up to this path exist (or the write will fail)
			std::string xmlWritePath(system->getGamelistPath(true));
			Utils::FileSystem::createDirectory(Utils::FileSystem::getParent(xmlWritePath));

			LOG(LogInfo) << "Added/Updated " << numUpdated << " entities in '" << xmlReadPath << "'";

			if (!doc.save_file(xmlWritePath.c_str())) {
				LOG(LogError) << "Error saving gamelist.xml to \"" << xmlWritePath << "\" (for system " << system->getName() << ")!";
			}

			const auto endTs = std::chrono::system_clock::now();
			LOG(LogInfo) << "Saved gamelist.xml for system \"" << system->getName() << "\" in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count() << " ms";
		}
	}else{
		LOG(LogError) << "Found no root folder for system \"" << system->getName() << "\"!";
	}
}
