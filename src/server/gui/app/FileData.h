#pragma once
#ifndef ES_APP_FILE_DATA_H
#define ES_APP_FILE_DATA_H

#include "utils/FileSystemUtil.h"
#include "MetaData.h"

#include <unordered_map>

#include "../gui.h"


class SystemData;
class Window;
struct SystemEnvironmentData;

enum FileType
{
	GAME = 1,   // Cannot have children.
	FOLDER = 2,
	MEDIA = 3,
	PLACEHOLDER = 4
};

enum FileChangeType
{
	FILE_ADDED,
	FILE_METADATA_CHANGED,
	FILE_REMOVED,
	FILE_SORTED
};

// Used for loading/saving gamelist.xml.
const char* fileTypeToString(FileType type);
FileType stringToFileType(const char* str);

// A tree node that holds information for a file.
class FileData
{
public:
	FileData(FileType type, const std::string& path, SystemEnvironmentData* envData, SystemData* system);
	FileData(FileType type, NymphMediaFile file, SystemData* system);
	virtual ~FileData();

	virtual const std::string& getName();
	virtual const std::string& getSortName();
	inline FileType getType() const { return mType; }
	inline const std::string& getPath() const { return mPath; }
	inline FileData* getParent() const { return mParent; }
	inline const std::unordered_map<std::string, FileData*>& getChildrenByFilename() const { return mChildrenByFilename; }
	inline const std::vector<FileData*>& getChildren() const { return mChildren; }
	inline SystemData* getSystem() const { return mSystem; }
	inline SystemEnvironmentData* getSystemEnvData() const { return mEnvData; }
	virtual const std::string getThumbnailPath() const;
	virtual const std::string getVideoPath() const;
	virtual const std::string getMarqueePath() const;
	virtual const std::string getImagePath() const;

	const std::vector<FileData*>& getChildrenListToDisplay();
	std::vector<FileData*> getFilesRecursive(unsigned int typeMask, bool displayedOnly = false) const;

	void addChild(FileData* file); // Error if mType != FOLDER
	void removeChild(FileData* file); //Error if mType != FOLDER

	inline bool isPlaceHolder() { return mType == PLACEHOLDER; };

	virtual inline void refreshMetadata() { return; };

	virtual std::string getKey();
	const bool isArcadeAsset();
	inline std::string getFullPath() { return getPath(); };
	inline std::string getFileName() { return Utils::FileSystem::getFileName(getPath()); };
	virtual FileData* getSourceFileData();
	inline std::string getSystemName() const { return mSystemName; };

	// Returns our best guess at the "real" name for this file (will attempt to perform MAME name translation)
	std::string getDisplayName() const;

	// As above, but also remove parenthesis
	std::string getCleanName() const;

	void launchGame(Window* window);
	void launchItem(Window* window);

	typedef bool ComparisonFunction(const FileData* a, const FileData* b);
	struct SortType
	{
		ComparisonFunction* comparisonFunction;
		bool ascending;
		std::string description;

		SortType(ComparisonFunction* sortFunction, bool sortAscending, const std::string & sortDescription)
			: comparisonFunction(sortFunction), ascending(sortAscending), description(sortDescription) {}
	};

	void sort(ComparisonFunction& comparator, bool ascending = true);
	void sort(const SortType& type);
	MetaDataList metadata;

protected:
	FileData* mSourceFileData;
	FileData* mParent;
	std::string mSystemName;

private:
	FileType mType;
	std::string mPath;
	SystemEnvironmentData* mEnvData;
	SystemData* mSystem;
	NymphMediaFile file;
	std::unordered_map<std::string,FileData*> mChildrenByFilename;
	std::vector<FileData*> mChildren;
	std::vector<FileData*> mFilteredChildren;
};

class CollectionFileData : public FileData
{
public:
	CollectionFileData(FileData* file, SystemData* system);
	~CollectionFileData();
	const std::string& getName();
	void refreshMetadata();
	FileData* getSourceFileData();
	std::string getKey();
private:
	// needs to be updated when metadata changes
	std::string mCollectionFileName;
	bool mDirty;
};

FileData::SortType getSortTypeFromString(std::string desc);

#endif // ES_APP_FILE_DATA_H
