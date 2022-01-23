#ifdef _RPI_
#include "components/VideoPlayerComponent.h"

#include "renderers/Renderer.h"
#include "utils/StringUtil.h"
#include "AudioManager.h"
#include "Settings.h"
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>

class VolumeControl
{
public:
	static std::shared_ptr<VolumeControl> & getInstance();
	int getVolume() const;
};

VideoPlayerComponent::VideoPlayerComponent(Window* window, std::string path) :
	VideoComponent(window),
	mPlayerPid(-1),
	subtitlePath(path)
{
}

VideoPlayerComponent::~VideoPlayerComponent()
{
	stopVideo();
}

void VideoPlayerComponent::render(const Transform4x4f& parentTrans)
{
	if (!isVisible())
		return;

	VideoComponent::render(parentTrans);

	if (!mIsPlaying || mPlayerPid == -1)
		VideoComponent::renderSnapshot(parentTrans);
}

void VideoPlayerComponent::setResize(float width, float height)
{
	setSize(width, height);
	mTargetSize = Vector2f(width, height);
	mTargetIsMax = false;
	mStaticImage.setResize(width, height);
	onSizeChanged();
}

void VideoPlayerComponent::setMaxSize(float width, float height)
{
	setSize(width, height);
	mTargetSize = Vector2f(width, height);
	mTargetIsMax = true;
	mStaticImage.setMaxSize(width, height);
	onSizeChanged();
}

void VideoPlayerComponent::startVideo()
{
	if (!mIsPlaying)
	{
		mVideoWidth = 0;
		mVideoHeight = 0;

		std::string path(mVideoPath.c_str());

		// Make sure we have a video path
		if ((path.size() > 0) && (mPlayerPid == -1))
		{
			// Set the video that we are going to be playing so we don't attempt to restart it
			mPlayingVideoPath = mVideoPath;

			// Disable AudioManager so video can play, in case we're requesting ALSA
			if (Utils::String::startsWith(Settings::getInstance()->getString("OMXAudioDev").c_str(), "alsa"))
			{
				AudioManager::getInstance()->deinit();
			}

			// Start the player process
			pid_t pid = fork();
			if (pid == -1)
			{
				// Failed
				mPlayingVideoPath = "";
			}
			else if (pid > 0)
			{
				mPlayerPid = pid;
				// Update the playing state
				signal(SIGCHLD, catch_child);
				mIsPlaying = true;
				mFadeIn = 0.0f;
			}
			else
			{

				// Find out the pixel position of the video view and build a command line for
				// omxplayer to position it in the right place
				char buf1[32];
				char buf2[32];
				float x = mPosition.x() - (mOrigin.x() * mSize.x());
				float y = mPosition.y() - (mOrigin.y() * mSize.y());

				// fix x and y
				switch(Renderer::getScreenRotate())
				{
					case 0:
					{
						const int x1 = (int)(Renderer::getScreenOffsetX() + x);
						const int y1 = (int)(Renderer::getScreenOffsetY() + y);
						const int x2 = (int)(x1 + mSize.x());
						const int y2 = (int)(y1 + mSize.y());
						sprintf(buf1, "%d,%d,%d,%d", x1, y1, x2, y2);
					}
					break;

					case 1:
					{
						const int x1 = (int)(Renderer::getWindowWidth() - Renderer::getScreenOffsetY() - y - mSize.y());
						const int y1 = (int)(Renderer::getScreenOffsetX() + x);
						const int x2 = (int)(x1 + mSize.y());
						const int y2 = (int)(y1 + mSize.x());
						sprintf(buf1, "%d,%d,%d,%d", x1, y1, x2, y2);
					}
					break;

					case 2:
					{
						const int x1 = (int)(Renderer::getWindowWidth()  - Renderer::getScreenOffsetX() - x - mSize.x());
						const int y1 = (int)(Renderer::getWindowHeight() - Renderer::getScreenOffsetY() - y - mSize.y());
						const int x2 = (int)(x1 + mSize.x());
						const int y2 = (int)(y1 + mSize.y());
						sprintf(buf1, "%d,%d,%d,%d", x1, y1, x2, y2);
					}
					break;

					case 3:
					{
						const int x1 = (int)(Renderer::getScreenOffsetY() + y);
						const int y1 = (int)(Renderer::getWindowHeight() - Renderer::getScreenOffsetX() - x - mSize.x());
						const int x2 = (int)(x1 + mSize.y());
						const int y2 = (int)(y1 + mSize.x());
						sprintf(buf1, "%d,%d,%d,%d", x1, y1, x2, y2);
					}
					break;
				}

				// rotate the video
				switch(Renderer::getScreenRotate())
				{
					case 0: { sprintf(buf2, "%d", (int)  0); } break;
					case 1: { sprintf(buf2, "%d", (int) 90); } break;
					case 2: { sprintf(buf2, "%d", (int)180); } break;
					case 3: { sprintf(buf2, "%d", (int)270); } break;
				}

				// We need to specify the layer of 10000 or above to ensure the video is displayed on top
				// of our SDL display

				const char* argv[] = { "", "--layer", "10010", "--loop", "--no-osd", "--aspect-mode", "letterbox", "--vol", "0", "-o", "both","--win", buf1, "--orientation", buf2, "", "", "", "", "", "", "", "", "", "", "", NULL };

				// check if we want to mute the audio
				if ((!Settings::getInstance()->getBool("VideoAudio") || (float)VolumeControl::getInstance()->getVolume() == 0) ||
					(Settings::getInstance()->getBool("ScreenSaverVideoMute") && mScreensaverMode))
				{
					argv[8] = "-1000000";
				}
				else
				{
					float percentVolume = (float)VolumeControl::getInstance()->getVolume();
					int OMXVolume = (int)((percentVolume-98)*105);
					argv[8] = std::to_string(OMXVolume).c_str();
				}

				// test if there's a path for possible subtitles, meaning we're a screensaver video
				if (!subtitlePath.empty())
				{
					// if we are rendering a screensaver

					// check if we want to stretch the image
					if (Settings::getInstance()->getBool("StretchVideoOnScreenSaver"))
					{
						argv[6] = "stretch";
					}

					if (Settings::getInstance()->getString("ScreenSaverGameInfo") != "never")
					{
						// if we have chosen to render subtitles
						argv[15] = "--subtitles";
						argv[16] = subtitlePath.c_str();
						argv[17] = mPlayingVideoPath.c_str();
						argv[18] = "--font";
						argv[19] = Settings::getInstance()->getString("SubtitleFont").c_str();
						argv[20] = "--italic-font";
						argv[21] = Settings::getInstance()->getString("SubtitleItalicFont").c_str();
						argv[22] = "--font-size";
						argv[23] = std::to_string(Settings::getInstance()->getInt("SubtitleSize")).c_str();
						argv[24] = "--align";
						argv[25] = Settings::getInstance()->getString("SubtitleAlignment").c_str();
					}
					else
					{
						// if we have chosen NOT to render subtitles in the screensaver
						argv[15] = mPlayingVideoPath.c_str();
					}
				}
				else
				{
					// if we are rendering a video gamelist
					if (!mTargetIsMax)
					{
						argv[6] = "stretch";
					}
					argv[15] = mPlayingVideoPath.c_str();
				}

				argv[10] = Settings::getInstance()->getString("OMXAudioDev").c_str();

				//const char* argv[] = args;
				const char* env[] = { "LD_LIBRARY_PATH=/opt/vc/libs:/usr/lib/omxplayer", NULL };

				// Redirect stdout
				int fdin = open("/dev/null", O_RDONLY);
				int fdout = open("/dev/null", O_WRONLY);
				dup2(fdin, 0);
				dup2(fdout, 1);
				// Run the omxplayer binary
				execve("/usr/bin/omxplayer.bin", (char**)argv, (char**)env);

				_exit(EXIT_FAILURE);
			}
		}
	}
}

void catch_child(int sig_num)
{
    /* when we get here, we know there's a zombie child waiting */
    int child_status;
    wait(&child_status);
}

void VideoPlayerComponent::stopVideo()
{
	mIsPlaying = false;
	mStartDelayed = false;

	// Stop the player process
	if (mPlayerPid != -1)
	{
		int status;
		kill(mPlayerPid, SIGKILL);
		waitpid(mPlayerPid, &status, WNOHANG);
		mPlayerPid = -1;
	}
}

#endif

