#include "SystemScreenSaver.h"

#ifdef _RPI_
#include "components/VideoPlayerComponent.h"
#endif
#include "components/VideoVlcComponent.h"
#include "CollectionSystemManager.h"
#include "utils/FileSystemUtil.h"
#include "views/gamelist/IGameListView.h"
#include "views/ViewController.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "PowerSaver.h"
#include "Sound.h"
#include "SystemData.h"
#include <unordered_map>
#include <time.h>
#include <chrono>
#define FADE_TIME 			300

static int lastIndex = 0;

SystemScreenSaver::SystemScreenSaver(Window* window) :
	mVideoScreensaver(NULL),
	mImageScreensaver(NULL),
	mWindow(window),
	mVideosCounted(false),
	mVideoCount(0),
	mImagesCounted(false),
	mImageCount(0),
	mState(STATE_INACTIVE),
	mOpacity(0.0f),
	mTimer(0),
	mSystemName(""),
	mGameName(""),
	mCurrentGame(NULL),
	mStopBackgroundAudio(true)
{
	mWindow->setScreenSaver(this);
	std::string path = getTitleFolder();
	if(!Utils::FileSystem::exists(path))
		Utils::FileSystem::createDirectory(path);
	srand((unsigned int)time(NULL));
	mSwapTimeout = 30000;
}

SystemScreenSaver::~SystemScreenSaver()
{
	// Delete subtitle file, if existing
	remove(getTitlePath().c_str());
	mCurrentGame = NULL;
	delete mVideoScreensaver;
	delete mImageScreensaver;
}

bool SystemScreenSaver::allowSleep()
{
	//return false;
	return ((mVideoScreensaver == NULL) && (mImageScreensaver == NULL));
}

bool SystemScreenSaver::isScreenSaverActive()
{
	return (mState != STATE_INACTIVE);
}

void SystemScreenSaver::startScreenSaver()
{
	// if set to index files in background, start thread
	if (Settings::getInstance()->getBool("BackgroundIndexing"))
	{
		mExit = false;
		mThread = new std::thread(&SystemScreenSaver::backgroundIndexing, this);
	}

	std::string screensaver_behavior = Settings::getInstance()->getString("ScreenSaverBehavior");
	if (!mVideoScreensaver && (screensaver_behavior == "random video"))
	{
		// Configure to fade out the windows, Skip Fading if Instant mode
		mState =  PowerSaver::getMode() == PowerSaver::INSTANT
					? STATE_SCREENSAVER_ACTIVE
					: STATE_FADE_OUT_WINDOW;
		mSwapTimeout = Settings::getInstance()->getInt("ScreenSaverSwapVideoTimeout");
		mOpacity = 0.0f;

		// Load a random video
		std::string path = "";
		pickRandomVideo(path);

		int retry = 200;
		while(retry > 0 && ((path.empty() || !Utils::FileSystem::exists(path)) || mCurrentGame == NULL))
		{
			retry--;
			pickRandomVideo(path);
		}

		if (!path.empty() && Utils::FileSystem::exists(path))
		{
#ifdef _RPI_
			// Create the correct type of video component
			if (Settings::getInstance()->getBool("ScreenSaverOmxPlayer"))
				mVideoScreensaver = new VideoPlayerComponent(mWindow, getTitlePath());
			else
				mVideoScreensaver = new VideoVlcComponent(mWindow, getTitlePath());
#else
			mVideoScreensaver = new VideoVlcComponent(mWindow, getTitlePath());
#endif

			mVideoScreensaver->topWindow(true);
			mVideoScreensaver->setOrigin(0.5f, 0.5f);
			mVideoScreensaver->setPosition(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f);

			if (Settings::getInstance()->getBool("StretchVideoOnScreenSaver"))
			{
				mVideoScreensaver->setResize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
			}
			else
			{
				mVideoScreensaver->setMaxSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
			}
			mVideoScreensaver->setVideo(path);
			mVideoScreensaver->setScreensaverMode(true);
			mVideoScreensaver->onShow();
			PowerSaver::runningScreenSaver(true);
			mTimer = 0;
			return;
		}
	}
	else if (screensaver_behavior == "slideshow")
	{
		// Configure to fade out the windows, Skip Fading if Instant mode
		mState =  PowerSaver::getMode() == PowerSaver::INSTANT
					? STATE_SCREENSAVER_ACTIVE
					: STATE_FADE_OUT_WINDOW;
		mSwapTimeout = Settings::getInstance()->getInt("ScreenSaverSwapImageTimeout");
		mOpacity = 0.0f;

		// Load a random image
		std::string path = "";
		if (Settings::getInstance()->getBool("SlideshowScreenSaverCustomImageSource"))
		{
			pickRandomCustomImage(path);
			// Custom images are not tied to the game list
			mCurrentGame = NULL;
		}
		else
		{
			pickRandomGameListImage(path);
		}

		if (!mImageScreensaver)
		{
			mImageScreensaver = new ImageComponent(mWindow, false, false);
		}

		mTimer = 0;

		mImageScreensaver->setImage(path);
		mImageScreensaver->setOrigin(0.5f, 0.5f);
		mImageScreensaver->setPosition(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f);

		if (Settings::getInstance()->getBool("SlideshowScreenSaverStretch"))
		{
			mImageScreensaver->setResize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
		}
		else
		{
			mImageScreensaver->setMaxSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
		}

		std::string bg_audio_file = Settings::getInstance()->getString("SlideshowScreenSaverBackgroundAudioFile");
		if ((!mBackgroundAudio) && (bg_audio_file != ""))
		{
			if (Utils::FileSystem::exists(bg_audio_file))
			{
				// paused PS so that the background audio keeps playing
				PowerSaver::pause();
				mBackgroundAudio = Sound::get(bg_audio_file);
				mBackgroundAudio->play();
			}
		}

		PowerSaver::runningScreenSaver(true);
		mTimer = 0;
		return;
	}
	// No videos. Just use a standard screensaver
	mState = STATE_SCREENSAVER_ACTIVE;
	mCurrentGame = NULL;
}

void SystemScreenSaver::stopScreenSaver()
{
	if ((mBackgroundAudio) && (mStopBackgroundAudio))
	{
		mBackgroundAudio->stop();
		mBackgroundAudio.reset();
		// if we were playing audio, we paused PS
		PowerSaver::resume();
	}

	// so that we stop the background audio next time, unless we're restarting the screensaver
	mStopBackgroundAudio = true;

	delete mVideoScreensaver;
	mVideoScreensaver = NULL;
	delete mImageScreensaver;
	mImageScreensaver = NULL;

	// Exit the indexing thread
	if (Settings::getInstance()->getBool("BackgroundIndexing"))
	{
		mExit = true;
		mThread->join();
		delete mThread;
	}

	// we need this to loop through different videos
	mState = STATE_INACTIVE;
	PowerSaver::runningScreenSaver(false);
}

void SystemScreenSaver::renderScreenSaver()
{
	std::string screensaver_behavior = Settings::getInstance()->getString("ScreenSaverBehavior");
	if (mVideoScreensaver && screensaver_behavior == "random video")
	{
		// Render black background
		Renderer::setMatrix(Transform4x4f::Identity());
		Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0x000000FF, 0x000000FF);

		// Only render the video if the state requires it
		if ((int)mState >= STATE_FADE_IN_VIDEO)
		{
			Transform4x4f transform = Transform4x4f::Identity();
			mVideoScreensaver->render(transform);
		}
	}
	else if (mImageScreensaver && screensaver_behavior == "slideshow")
	{
		// Render black background
		Renderer::setMatrix(Transform4x4f::Identity());
		Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0x000000FF, 0x000000FF);

		// Only render the image if the state requires it
		if ((int)mState >= STATE_FADE_IN_VIDEO)
		{
			if (mImageScreensaver->hasImage())
			{
				mImageScreensaver->setOpacity(255- (unsigned char) (mOpacity * 255));

				Transform4x4f transform = Transform4x4f::Identity();
				mImageScreensaver->render(transform);
			}
		}

		// Check if we need to restart the background audio
		if ((mBackgroundAudio) && (Settings::getInstance()->getString("SlideshowScreenSaverBackgroundAudioFile") != ""))
		{
			if (!mBackgroundAudio->isPlaying())
			{
				mBackgroundAudio->play();
			}
		}
	}
	else if (mState != STATE_INACTIVE)
	{
		Renderer::setMatrix(Transform4x4f::Identity());
		unsigned char color = screensaver_behavior == "dim" ? 0x000000A0 : 0x000000FF;
		Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(), color, color);
	}
}

void SystemScreenSaver::backgroundIndexing()
{
	LOG(LogDebug) << "Background indexing starting.";

	// get the list of all games
	SystemData* all = CollectionSystemManager::get()->getAllGamesCollection();
	std::vector<FileData*> files = all->getRootFolder()->getFilesRecursive(GAME);
	
	const auto startTs = std::chrono::system_clock::now();
	for (lastIndex; lastIndex < files.size(); lastIndex++)
	{
		if(mExit)
			break;
		Utils::FileSystem::exists(files.at(lastIndex)->getVideoPath());
		Utils::FileSystem::exists(files.at(lastIndex)->getMarqueePath());
		Utils::FileSystem::exists(files.at(lastIndex)->getThumbnailPath());
		Utils::FileSystem::exists(files.at(lastIndex)->getImagePath());
	}
	auto endTs = std::chrono::system_clock::now();
	LOG(LogDebug) << "Indexed a total of " << lastIndex << " entries in " << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count() << " ms. Stopping.";		
}

unsigned long SystemScreenSaver::countGameListNodes(const char *nodeName)
{
	unsigned long nodeCount = 0;
	std::vector<SystemData*>::const_iterator it;
	for (it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); ++it)
	{
		// We only want nodes from game systems that are not collections
		if (!(*it)->isGameSystem() || (*it)->isCollection())
			continue;

		FileData* rootFileData = (*it)->getRootFolder();

		FileType type = GAME;
		std::vector<FileData*> allFiles = rootFileData->getFilesRecursive(type, true);
		std::vector<FileData*>::const_iterator itf;  // declare an iterator to a vector of strings

		for(itf=allFiles.cbegin() ; itf < allFiles.cend(); itf++) {
			if ((strcmp(nodeName, "video") == 0 && (*itf)->getVideoPath() != "") ||
				(strcmp(nodeName, "image") == 0 && (*itf)->getImagePath() != ""))
			{
				nodeCount++;
			}
		}
	}
	return nodeCount;
}

void SystemScreenSaver::countVideos()
{
	if (!mVideosCounted)
	{
		mVideoCount = countGameListNodes("video");
		mVideosCounted = true;
	}
}

void SystemScreenSaver::countImages()
{
	if (!mImagesCounted)
	{
		mImageCount = countGameListNodes("image");
		mImagesCounted = true;
	}
}

void SystemScreenSaver::pickGameListNode(unsigned long index, const char *nodeName, std::string& path)
{
	std::vector<SystemData*>::const_iterator it;
	for (it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); ++it)
	{
		// We only want nodes from game systems that are not collections
		if (!(*it)->isGameSystem() || (*it)->isCollection())
			continue;

		FileData* rootFileData = (*it)->getRootFolder();

		FileType type = GAME;
		std::vector<FileData*> allFiles = rootFileData->getFilesRecursive(type, true);
		std::vector<FileData*>::const_iterator itf;  // declare an iterator to a vector of strings

		for(itf=allFiles.cbegin() ; itf < allFiles.cend(); itf++) {
			if ((strcmp(nodeName, "video") == 0 && (*itf)->getVideoPath() != "") ||
				(strcmp(nodeName, "image") == 0 && (*itf)->getImagePath() != ""))
			{
				if (index-- == 0)
				{
					// We have it
					path = "";
					if (strcmp(nodeName, "video") == 0)
						path = (*itf)->getVideoPath();
					else if (strcmp(nodeName, "image") == 0)
						path = (*itf)->getImagePath();
					mSystemName = (*it)->getFullName();
					mGameName = (*itf)->getName();
					mCurrentGame = (*itf);

					// end of getting FileData
					if (Settings::getInstance()->getString("ScreenSaverGameInfo") != "never")
						writeSubtitle(mGameName.c_str(), mSystemName.c_str(),
							(Settings::getInstance()->getString("ScreenSaverGameInfo") == "always"));
					return;
				}
			}
		}
	}
}

void SystemScreenSaver::pickRandomVideo(std::string& path)
{
	countVideos();
	mCurrentGame = NULL;
	if (mVideoCount > 0)
	{
		int video = (int)(((float)rand() / float(RAND_MAX)) * (float)mVideoCount);

		pickGameListNode(video, "video", path);
	}
}

void SystemScreenSaver::pickRandomGameListImage(std::string& path)
{
	countImages();
	mCurrentGame = NULL;
	if (mImageCount > 0)
	{
		int image = (int)(((float)rand() / float(RAND_MAX)) * (float)mImageCount);

		pickGameListNode(image, "image", path);
	}
}

void SystemScreenSaver::pickRandomCustomImage(std::string& path)
{
	std::string imageDir = Settings::getInstance()->getString("SlideshowScreenSaverImageDir");
	if ((imageDir != "") && (Utils::FileSystem::exists(imageDir)))
	{
		std::string                   imageFilter = Settings::getInstance()->getString("SlideshowScreenSaverImageFilter");
		std::vector<std::string>      matchingFiles;
		Utils::FileSystem::stringList dirContent  = Utils::FileSystem::getDirContent(imageDir, Settings::getInstance()->getBool("SlideshowScreenSaverRecurse"));

		for(Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin(); it != dirContent.cend(); ++it)
		{
			if (Utils::FileSystem::isRegularFile(*it))
			{
				// If the image filter is empty, or the file extension is in the filter string,
				//  add it to the matching files list
				if ((imageFilter.length() <= 0) ||
					(imageFilter.find(Utils::FileSystem::getExtension(*it)) != std::string::npos))
				{
					matchingFiles.push_back(*it);
				}
			}
		}

		int fileCount = (int)matchingFiles.size();
		if (fileCount > 0)
		{
			// get a random index in the range 0 to fileCount (exclusive)
			int randomIndex = rand() % fileCount;
			path = matchingFiles[randomIndex];
		}
		else
		{
			LOG(LogError) << "Slideshow Screensaver - No image files found\n";
		}
	}
	else
	{
		LOG(LogError) << "Slideshow Screensaver - Image directory does not exist: " << imageDir << "\n";
	}
}

void SystemScreenSaver::update(int deltaTime)
{
	// Use this to update the fade value for the current fade stage
	if (mState == STATE_FADE_OUT_WINDOW)
	{
		mOpacity += (float)deltaTime / FADE_TIME;
		if (mOpacity >= 1.0f)
		{
			mOpacity = 1.0f;

			// Update to the next state
			mState = STATE_FADE_IN_VIDEO;
		}
	}
	else if (mState == STATE_FADE_IN_VIDEO)
	{
		mOpacity -= (float)deltaTime / FADE_TIME;
		if (mOpacity <= 0.0f)
		{
			mOpacity = 0.0f;
			// Update to the next state
			mState = STATE_SCREENSAVER_ACTIVE;
		}
	}
	else if (mState == STATE_SCREENSAVER_ACTIVE)
	{
		// Update the timer that swaps the videos/images
		mTimer += deltaTime;
		if (mTimer > mSwapTimeout)
		{
			nextMediaItem();
		}
	}

	// If we have a loaded video/image then update it
	if (mVideoScreensaver)
		mVideoScreensaver->update(deltaTime);
	else if (mImageScreensaver)
		mImageScreensaver->update(deltaTime);
}

void SystemScreenSaver::nextMediaItem() {
	mStopBackgroundAudio = false;
	stopScreenSaver();
	startScreenSaver();
	mState = STATE_SCREENSAVER_ACTIVE;
}

FileData* SystemScreenSaver::getCurrentGame()
{
	return mCurrentGame;
}

void SystemScreenSaver::launchGame()
{
	if (mCurrentGame != NULL)
	{
		// launching Game
		ViewController::get()->goToGameList(mCurrentGame->getSystem());
		IGameListView* view = ViewController::get()->getGameListView(mCurrentGame->getSystem()).get();
		view->setCursor(mCurrentGame);
		view->launch(mCurrentGame);
	}
}
