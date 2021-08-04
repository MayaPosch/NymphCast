#include "FileData.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "AudioManager.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Log.h"
#include "MameNames.h"
#include "platform.h"
#include "Scripting.h"
#include "SystemData.h"
#include "VolumeControl.h"
#include "Window.h"
#include <assert.h>


#include <map>
#include <chrono>
#include <Poco/Net/DNS.h>
#include <Poco/Net/NetworkInterface.h>
#include <Poco/Net/NetException.h>
#include <nyansd.h>


FileData::FileData(FileType type, const std::string& path, SystemEnvironmentData* envData, SystemData* system)
	: mType(type), mPath(path), mSystem(system), mEnvData(envData), mSourceFileData(NULL), mParent(NULL), metadata(type == GAME ? GAME_METADATA : FOLDER_METADATA) // metadata is REALLY set in the constructor!
{
	// metadata needs at least a name field (since that's what getName() will return)
	if(metadata.get("name").empty())
		metadata.set("name", getDisplayName());
	mSystemName = system->getName();
	metadata.resetChangedFlag();
}


FileData::FileData(FileType type, NymphMediaFile file, SystemData* system) : 
	file(file), mType(type), mParent(NULL), mPath(file.name), 
	mSystem(system), metadata(type == MEDIA ? GAME_METADATA : FOLDER_METADATA) // metadata is REALLY set in the constructor! 
	{
	// metadata needs at least a name field (since that's what getName() will return)
	if (metadata.get("name").empty()) {
		//metadata.set("name", getDisplayName());
		metadata.set("name", file.name);
	}
	
	mSystemName = system->getName();
	metadata.resetChangedFlag();
}


FileData::~FileData()
{
	if(mParent)
		mParent->removeChild(this);

	if(mType == GAME)
		mSystem->getIndex()->removeFromIndex(this);

	mChildren.clear();
}

std::string FileData::getDisplayName() const
{
	std::string stem = Utils::FileSystem::getStem(mPath);
	if(mSystem && mSystem->hasPlatformId(PlatformIds::ARCADE) || mSystem->hasPlatformId(PlatformIds::NEOGEO))
		stem = MameNames::getInstance()->getRealName(stem);

	return stem;
}

std::string FileData::getCleanName() const
{
	return Utils::String::removeParenthesis(this->getDisplayName());
}

const std::string FileData::getThumbnailPath() const
{
	std::string thumbnail = metadata.get("thumbnail");

	// no thumbnail, try image
	if(thumbnail.empty())
	{
		thumbnail = metadata.get("image");

		// no image, try to use local image
		if(thumbnail.empty() && Settings::getInstance()->getBool("LocalArt"))
		{
			const char* extList[2] = { ".png", ".jpg" };
			for(int i = 0; i < 2; i++)
			{
				if(thumbnail.empty())
				{
					std::string path = mEnvData->mStartPath + "/images/" + getDisplayName() + "-image" + extList[i];
					if(Utils::FileSystem::exists(path))
						thumbnail = path;
				}
			}
		}
	}

	return thumbnail;
}

const std::string& FileData::getName()
{
	return metadata.get("name");
}

const std::string& FileData::getSortName()
{
	if (metadata.get("sortname").empty())
		return metadata.get("name");
	else
		return metadata.get("sortname");
}

const std::vector<FileData*>& FileData::getChildrenListToDisplay() {

	FileFilterIndex* idx = CollectionSystemManager::get()->getSystemToView(mSystem)->getIndex();
	if (idx->isFiltered()) {
		mFilteredChildren.clear();
		for(auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
		{
			if (idx->showFile((*it))) {
				mFilteredChildren.push_back(*it);
			}
		}

		return mFilteredChildren;
	}
	else
	{
		return mChildren;
	}
}

const std::string FileData::getVideoPath() const
{
	std::string video = metadata.get("video");

	// no video, try to use local video
	if(video.empty() && Settings::getInstance()->getBool("LocalArt"))
	{
		std::string path = mEnvData->mStartPath + "/images/" + getDisplayName() + "-video.mp4";
		if(Utils::FileSystem::exists(path))
			video = path;
	}

	return video;
}

const std::string FileData::getMarqueePath() const
{
	std::string marquee = metadata.get("marquee");

	// no marquee, try to use local marquee
	if(marquee.empty() && Settings::getInstance()->getBool("LocalArt"))
	{
		const char* extList[2] = { ".png", ".jpg" };
		for(int i = 0; i < 2; i++)
		{
			if(marquee.empty())
			{
				std::string path = mEnvData->mStartPath + "/images/" + getDisplayName() + "-marquee" + extList[i];
				if(Utils::FileSystem::exists(path))
					marquee = path;
			}
		}
	}

	return marquee;
}

const std::string FileData::getImagePath() const
{
	std::string image = metadata.get("image");

	// no image, try to use local image
	if(image.empty())
	{
		const char* extList[2] = { ".png", ".jpg" };
		for(int i = 0; i < 2; i++)
		{
			if(image.empty())
			{
				std::string path = mEnvData->mStartPath + "/images/" + getDisplayName() + "-image" + extList[i];
				if(Utils::FileSystem::exists(path))
					image = path;
			}
		}
	}

	return image;
}

std::vector<FileData*> FileData::getFilesRecursive(unsigned int typeMask, bool displayedOnly) const
{
	std::vector<FileData*> out;
	FileFilterIndex* idx = mSystem->getIndex();

	for(auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
	{
		if((*it)->getType() & typeMask)
		{
			if (!displayedOnly || !idx->isFiltered() || idx->showFile(*it))
				out.push_back(*it);
		}

		if((*it)->getChildren().size() > 0)
		{
			std::vector<FileData*> subchildren = (*it)->getFilesRecursive(typeMask, displayedOnly);
			out.insert(out.cend(), subchildren.cbegin(), subchildren.cend());
		}
	}

	return out;
}

std::string FileData::getKey() {
	if (mType == MEDIA) {
		return mPath;
	}
	
	// Return local file path for local files only.
	return getFileName();
}

const bool FileData::isArcadeAsset()
{
	const std::string stem = Utils::FileSystem::getStem(mPath);
	return (
		(mSystem && (mSystem->hasPlatformId(PlatformIds::ARCADE) || mSystem->hasPlatformId(PlatformIds::NEOGEO)))
		&&
		(MameNames::getInstance()->isBios(stem) || MameNames::getInstance()->isDevice(stem))
	);
}

FileData* FileData::getSourceFileData()
{
	return this;
}

void FileData::addChild(FileData* file)
{
	assert(mType == FOLDER);
	assert(file->getParent() == NULL);

	const std::string key = file->getKey();
	if (mChildrenByFilename.find(key) == mChildrenByFilename.cend())
	{
		mChildrenByFilename[key] = file;
		mChildren.push_back(file);
		file->mParent = this;
	}
}

void FileData::removeChild(FileData* file)
{
	assert(mType == FOLDER);
	assert(file->getParent() == this);
	mChildrenByFilename.erase(file->getKey());
	for(auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
	{
		if(*it == file)
		{
			file->mParent = NULL;
			mChildren.erase(it);
			return;
		}
	}

	// File somehow wasn't in our children.
	assert(false);

}

void FileData::sort(ComparisonFunction& comparator, bool ascending)
{
	std::stable_sort(mChildren.begin(), mChildren.end(), comparator);

	for(auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
	{
		if((*it)->getChildren().size() > 0)
			(*it)->sort(comparator, ascending);
	}

	if(!ascending)
		std::reverse(mChildren.begin(), mChildren.end());
}

void FileData::sort(const SortType& type)
{
	sort(*type.comparisonFunction, type.ascending);
}


// --- REMOTE TO LOCAL IP ---
// Defined in NyanSD.
bool remoteToLocalIP(Poco::Net::SocketAddress &sa, uint32_t &ipv4, std::string &ipv6);


// --- LAUNCH GAME ---
void FileData::launchGame(Window* window) {
	LOG(LogInfo) << "Attempting to launch game...";

	AudioManager::getInstance()->deinit();
	VolumeControl::getInstance()->deinit();
	window->deinit();
	
	// Check whether media file or game.
	if (mType == MEDIA) {
		// Initiate playback.
		// FIXME: move to a more logical location.
		NymphCastRemote receiver;
		receiver.name = Poco::Net::DNS::hostName();
		receiver.port = 4004; // FIXME: don't use hardcoded port.
		Poco::Net::SocketAddress sa(file.mediaserver.ipv6, receiver.port);
		uint32_t ipv4;
		if (!remoteToLocalIP(sa, ipv4, receiver.ipv6)) {
			LOG(LogError) << "Failed to convert remote IP to local.";
		}
		
		receiver.ipv4 = NyanSD::ipv4_uintToString(ipv4);
		
		Gui::active = false;
		
		std::vector<NymphCastRemote> receivers;
		receivers.push_back(receiver);
		if (!Gui::client->playShare(file, receivers)) {
			LOG(LogError) << "Failed to play back file...";
		}
		else {
			// Wait until playback has finished.
			std::unique_lock<std::mutex> lk(Gui::resumeMtx);
			using namespace std::chrono_literals;
			while (Gui::resumeCv.wait_for(lk, 10s) != std::cv_status::timeout) {
				// FIXME: handle playback issues. May end up in a loop here.
				if (Gui::active) { break; }
			}
		}
	}
	else {
		std::string command = mEnvData->mLaunchCommand;

		const std::string rom      = Utils::FileSystem::getEscapedPath(getPath());
		const std::string basename = Utils::FileSystem::getStem(getPath());
		const std::string rom_raw  = Utils::FileSystem::getPreferredPath(getPath());

		command = Utils::String::replace(command, "%ROM%", rom);
		command = Utils::String::replace(command, "%BASENAME%", basename);
		command = Utils::String::replace(command, "%ROM_RAW%", rom_raw);

		Scripting::fireEvent("game-start", rom, basename);

		LOG(LogInfo) << "	" << command;
		int exitCode = runSystemCommand(command);

		if(exitCode != 0)
		{
			LOG(LogWarning) << "...launch terminated with nonzero exit code " << exitCode << "!";
		}

		Scripting::fireEvent("game-end");
	}

	window->init();
	VolumeControl::getInstance()->init();
	window->normalizeNextUpdate();

	if (mType == GAME) {
		//update number of times the game has been launched
		FileData* gameToUpdate = getSourceFileData();

		int timesPlayed = gameToUpdate->metadata.getInt("playcount") + 1;
		gameToUpdate->metadata.set("playcount", std::to_string(static_cast<long long>(timesPlayed)));

		//update last played time
		gameToUpdate->metadata.set("lastplayed", Utils::Time::DateTime(Utils::Time::now()));
		CollectionSystemManager::get()->refreshCollectionSystems(gameToUpdate);

		gameToUpdate->mSystem->onMetaDataSavePoint();
	}
}

CollectionFileData::CollectionFileData(FileData* file, SystemData* system)
	: FileData(file->getSourceFileData()->getType(), file->getSourceFileData()->getPath(), file->getSourceFileData()->getSystemEnvData(), system)
{
	// we use this constructor to create a clone of the filedata, and change its system
	mSourceFileData = file->getSourceFileData();
	refreshMetadata();
	mParent = NULL;
	metadata = mSourceFileData->metadata;
	mSystemName = mSourceFileData->getSystem()->getName();
}

CollectionFileData::~CollectionFileData()
{
	// need to remove collection file data at the collection object destructor
	if(mParent)
		mParent->removeChild(this);
	mParent = NULL;
}

std::string CollectionFileData::getKey() {
	return getFullPath();
}

FileData* CollectionFileData::getSourceFileData()
{
	return mSourceFileData;
}

void CollectionFileData::refreshMetadata()
{
	metadata = mSourceFileData->metadata;
	mDirty = true;
}

const std::string& CollectionFileData::getName()
{
	if (mDirty) {
		mCollectionFileName = Utils::String::removeParenthesis(mSourceFileData->metadata.get("name"));
		mCollectionFileName += " [" + Utils::String::toUpper(mSourceFileData->getSystem()->getName()) + "]";
		mDirty = false;
	}

	if (Settings::getInstance()->getBool("CollectionShowSystemInfo"))
		return mCollectionFileName;
	return mSourceFileData->metadata.get("name");
}

// returns Sort Type based on a string description
FileData::SortType getSortTypeFromString(std::string desc) {
	std::vector<FileData::SortType> SortTypes = FileSorts::SortTypes;
	// find it
	for(unsigned int i = 0; i < FileSorts::SortTypes.size(); i++)
	{
		const FileData::SortType& sort = FileSorts::SortTypes.at(i);
		if(sort.description == desc)
		{
			return sort;
		}
	}
	// if not found default to name, ascending
	return FileSorts::SortTypes.at(0);
}
