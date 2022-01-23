#include "components/VideoComponent.h"

#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "PowerSaver.h"
#include "ThemeData.h"
#include "Window.h"
#include <SDL_timer.h>

#define FADE_TIME_MS	200

std::string getTitlePath() {
	std::string titleFolder = getTitleFolder();
	return titleFolder + "last_title.srt";
}

std::string getTitleFolder() {
	std::string home = Utils::FileSystem::getHomePath();
	return home + "/.emulationstation/tmp/";
}

void writeSubtitle(const char* gameName, const char* systemName, bool always)
{
	FILE* file = fopen(getTitlePath().c_str(), "w");
	int end = (int)(Settings::getInstance()->getInt("ScreenSaverSwapVideoTimeout") / (1000));
	if (always) {
		fprintf(file, "1\n00:00:01,000 --> 00:00:%d,000\n", end);
	}
	else
	{
		fprintf(file, "1\n00:00:01,000 --> 00:00:08,000\n");
	}
	fprintf(file, "%s\n", gameName);
	fprintf(file, "<i>%s</i>\n\n", systemName);

	if (!always) {
		if (end > 12)
		{
			fprintf(file, "2\n00:00:%d,000 --> 00:00:%d,000\n%s\n<i>%s</i>\n", end-4, end, gameName, systemName);
		}
	}

	fflush(file);
	fclose(file);
	file = NULL;
}

void VideoComponent::setScreensaverMode(bool isScreensaver)
{
	mScreensaverMode = isScreensaver;
}

VideoComponent::VideoComponent(Window* window) :
	GuiComponent(window),
	mStaticImage(window),
	mVideoHeight(0),
	mVideoWidth(0),
	mStartDelayed(false),
	mIsPlaying(false),
	mShowing(false),
	mScreensaverActive(false),
	mDisable(false),
	mScreensaverMode(false),
	mTargetIsMax(false),
	mTargetSize(0, 0)
{
	// Setup the default configuration
	mConfig.showSnapshotDelay 		= false;
	mConfig.showSnapshotNoVideo		= false;
	mConfig.startDelay				= 0;
	if (mWindow->getGuiStackSize() > 1) {
		topWindow(false);
	}

	std::string path = getTitleFolder();
	if(!Utils::FileSystem::exists(path))
		Utils::FileSystem::createDirectory(path);
}

VideoComponent::~VideoComponent()
{
	// Stop any currently running video
	stopVideo();
	// Delete subtitle file, if existing
	remove(getTitlePath().c_str());
}

void VideoComponent::onOriginChanged()
{
	// Update the embeded static image
	mStaticImage.setOrigin(mOrigin);
}
void VideoComponent::onPositionChanged()
{
	// Update the embeded static image
	mStaticImage.setPosition(mPosition);
}

void VideoComponent::onSizeChanged()
{
	// Update the embeded static image
	mStaticImage.onSizeChanged();
}

bool VideoComponent::setVideo(std::string path)
{
	// Convert the path into a generic format
	std::string fullPath = path.empty() ? path : Utils::FileSystem::getAbsolutePath(path);

	// Check that it's changed
	if (fullPath == mVideoPath)
		return !path.empty();

	// Store the path
	mVideoPath = fullPath;

	// If the file exists then set the new video
	if (!fullPath.empty() && ResourceManager::getInstance()->fileExists(fullPath))
	{
		// Return true to show that we are going to attempt to play a video
		return true;
	}
	// Return false to show that no video will be displayed
	return false;
}

void VideoComponent::setImage(std::string path)
{
	// Check that the image has changed
	if (path == mStaticImagePath)
		return;

	mStaticImage.setImage(path);
	mFadeIn = 0.0f;
	mStaticImagePath = path;
}

void VideoComponent::setDefaultVideo()
{
	setVideo(mConfig.defaultVideoPath);
}

void VideoComponent::setOpacity(unsigned char opacity)
{
	mOpacity = opacity;
	// Update the embeded static image
	mStaticImage.setOpacity(opacity);
}

void VideoComponent::render(const Transform4x4f& parentTrans)
{
	if (!isVisible())
		return;

	Transform4x4f trans = parentTrans * getTransform();
	GuiComponent::renderChildren(trans);

	Renderer::setMatrix(trans);

	// Handle the case where the video is delayed
	handleStartDelay();

	// Handle looping of the video
	handleLooping();
}

void VideoComponent::renderSnapshot(const Transform4x4f& parentTrans)
{
	// This is the case where the video is not currently being displayed. Work out
	// if we need to display a static image
	if ((mConfig.showSnapshotNoVideo && mVideoPath.empty()) || (mStartDelayed && mConfig.showSnapshotDelay))
	{
		// Display the static image instead
		mStaticImage.setOpacity((unsigned char)(mFadeIn * 255.0f));
		mStaticImage.render(parentTrans);
	}
}

void VideoComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	using namespace ThemeFlags;

	GuiComponent::applyTheme(theme, view, element, (properties ^ SIZE) | ((properties & (SIZE | POSITION)) ? ORIGIN : 0));

	const ThemeData::ThemeElement* elem = theme->getElement(view, element, "video");
	if(!elem)
		return;

	Vector2f scale = getParent() ? getParent()->getSize() : Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	if(properties & ThemeFlags::SIZE)
	{
		if(elem->has("size"))
			setResize(elem->get<Vector2f>("size") * scale);
		else if(elem->has("maxSize"))
			setMaxSize(elem->get<Vector2f>("maxSize") * scale);
	}

	if(elem->has("default"))
		mConfig.defaultVideoPath = elem->get<std::string>("default");

	if((properties & ThemeFlags::DELAY) && elem->has("delay"))
		mConfig.startDelay = (unsigned)(elem->get<float>("delay") * 1000.0f);

	if (elem->has("showSnapshotNoVideo"))
		mConfig.showSnapshotNoVideo = elem->get<bool>("showSnapshotNoVideo");

	if (elem->has("showSnapshotDelay"))
		mConfig.showSnapshotDelay = elem->get<bool>("showSnapshotDelay");
}

std::vector<HelpPrompt> VideoComponent::getHelpPrompts()
{
	std::vector<HelpPrompt> ret;
	ret.push_back(HelpPrompt("a", "select"));
	return ret;
}

void VideoComponent::handleStartDelay()
{
	// Only play if any delay has timed out
	if (mStartDelayed)
	{
		if (mStartTime > SDL_GetTicks())
		{
			// Timeout not yet completed
			return;
		}
		// Completed
		mStartDelayed = false;
		// Clear the playing flag so startVideo works
		mIsPlaying = false;
		startVideo();
	}
}

void VideoComponent::handleLooping()
{
}

void VideoComponent::startVideoWithDelay()
{
	// If not playing then either start the video or initiate the delay
	if (!mIsPlaying)
	{
		// Set the video that we are going to be playing so we don't attempt to restart it
		mPlayingVideoPath = mVideoPath;

		if (mConfig.startDelay == 0 || PowerSaver::getMode() == PowerSaver::INSTANT)
		{
			// No delay. Just start the video
			mStartDelayed = false;
			startVideo();
		}
		else
		{
			// Configure the start delay
			mStartDelayed = true;
			mFadeIn = 0.0f;
			mStartTime = SDL_GetTicks() + mConfig.startDelay;
		}
		mIsPlaying = true;
	}
}

void VideoComponent::update(int deltaTime)
{
	manageState();

	// If the video start is delayed and there is less than the fade time then set the image fade
	// accordingly
	if (mStartDelayed)
	{
		Uint32 ticks = SDL_GetTicks();
		if (mStartTime > ticks)
		{
			Uint32 diff = mStartTime - ticks;
			if (diff < FADE_TIME_MS)
			{
				mFadeIn = (float)diff / (float)FADE_TIME_MS;
				return;
			}
		}
	}
	// If the fade in is less than 1 then increment it
	if (mFadeIn < 1.0f)
	{
		mFadeIn += deltaTime / (float)FADE_TIME_MS;
		if (mFadeIn > 1.0f)
			mFadeIn = 1.0f;
	}
	GuiComponent::update(deltaTime);
}

void VideoComponent::manageState()
{
	// We will only show if the component is on display and the screensaver
	// is not active and the component is visible
	bool show = mShowing && !mScreensaverActive && !mDisable && mVisible;

	// See if we're already playing
	if (mIsPlaying)
	{
		// If we are not on display then stop the video from playing
		if (!show)
		{
			stopVideo();
		}
		else
		{
			if (mVideoPath != mPlayingVideoPath)
			{
				// Path changed. Stop the video. We will start it again below because
				// mIsPlaying will be modified by stopVideo to be false
				stopVideo();
			}
		}
	}
	// Need to recheck variable rather than 'else' because it may be modified above
	if (!mIsPlaying)
	{
		// If we are on display then see if we should start the video
		if (show && !mVideoPath.empty())
		{
			startVideoWithDelay();
		}
	}
}

void VideoComponent::onShow()
{
	mShowing = true;
	manageState();
}

void VideoComponent::onHide()
{
	mShowing = false;
	manageState();
}

void VideoComponent::onScreenSaverActivate()
{
	mScreensaverActive = true;
	manageState();
}

void VideoComponent::onScreenSaverDeactivate()
{
	mScreensaverActive = false;
	manageState();
}

void VideoComponent::topWindow(bool isTop)
{
	mDisable = !isTop;
	manageState();
}
