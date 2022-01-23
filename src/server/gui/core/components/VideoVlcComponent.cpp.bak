#include "components/VideoVlcComponent.h"

#include "renderers/Renderer.h"
#include "resources/TextureResource.h"
#include "utils/StringUtil.h"
#include "PowerSaver.h"
#include "Settings.h"
#include <vlc/vlc.h>
#include <SDL_mutex.h>

#ifdef WIN32
#include <codecvt>
#endif

libvlc_instance_t* VideoVlcComponent::mVLC = NULL;

// VLC prepares to render a video frame.
static void *lock(void *data, void **p_pixels) {
	struct VideoContext *c = (struct VideoContext *)data;
	SDL_LockMutex(c->mutex);
	SDL_LockSurface(c->surface);
	*p_pixels = c->surface->pixels;
	return NULL; // Picture identifier, not needed here.
}

// VLC just rendered a video frame.
static void unlock(void *data, void* /*id*/, void *const* /*p_pixels*/) {
	struct VideoContext *c = (struct VideoContext *)data;
	SDL_UnlockSurface(c->surface);
	SDL_UnlockMutex(c->mutex);
}

// VLC wants to display a video frame.
static void display(void* /*data*/, void* /*id*/) {
	//Data to be displayed
}

VideoVlcComponent::VideoVlcComponent(Window* window, std::string subtitles) :
	VideoComponent(window),
	mMediaPlayer(nullptr)
{
	memset(&mContext, 0, sizeof(mContext));

	// Get an empty texture for rendering the video
	mTexture = TextureResource::get("");

	// Make sure VLC has been initialised
	setupVLC(subtitles);
}

VideoVlcComponent::~VideoVlcComponent()
{
	stopVideo();
}

void VideoVlcComponent::setResize(float width, float height)
{
	mTargetSize = Vector2f(width, height);
	mTargetIsMax = false;
	mStaticImage.setResize(width, height);
	resize();
}

void VideoVlcComponent::setMaxSize(float width, float height)
{
	mTargetSize = Vector2f(width, height);
	mTargetIsMax = true;
	mStaticImage.setMaxSize(width, height);
	resize();
}

void VideoVlcComponent::resize()
{
	if(!mTexture)
		return;

	const Vector2f textureSize((float)mVideoWidth, (float)mVideoHeight);

	if(textureSize == Vector2f::Zero())
		return;

		// SVG rasterization is determined by height (see SVGResource.cpp), and rasterization is done in terms of pixels
		// if rounding is off enough in the rasterization step (for images with extreme aspect ratios), it can cause cutoff when the aspect ratio breaks
		// so, we always make sure the resultant height is an integer to make sure cutoff doesn't happen, and scale width from that
		// (you'll see this scattered throughout the function)
		// this is probably not the best way, so if you're familiar with this problem and have a better solution, please make a pull request!

		if(mTargetIsMax)
		{

			mSize = textureSize;

			Vector2f resizeScale((mTargetSize.x() / mSize.x()), (mTargetSize.y() / mSize.y()));

			if(resizeScale.x() < resizeScale.y())
			{
				mSize[0] *= resizeScale.x();
				mSize[1] *= resizeScale.x();
			}else{
				mSize[0] *= resizeScale.y();
				mSize[1] *= resizeScale.y();
			}

			// for SVG rasterization, always calculate width from rounded height (see comment above)
			mSize[1] = Math::round(mSize[1]);
			mSize[0] = (mSize[1] / textureSize.y()) * textureSize.x();

		}else{
			// if both components are set, we just stretch
			// if no components are set, we don't resize at all
			mSize = mTargetSize == Vector2f::Zero() ? textureSize : mTargetSize;

			// if only one component is set, we resize in a way that maintains aspect ratio
			// for SVG rasterization, we always calculate width from rounded height (see comment above)
			if(!mTargetSize.x() && mTargetSize.y())
			{
				mSize[1] = Math::round(mTargetSize.y());
				mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
			}else if(mTargetSize.x() && !mTargetSize.y())
			{
				mSize[1] = Math::round((mTargetSize.x() / textureSize.x()) * textureSize.y());
				mSize[0] = (mSize.y() / textureSize.y()) * textureSize.x();
			}
		}

	// mSize.y() should already be rounded
	mTexture->rasterizeAt((size_t)Math::round(mSize.x()), (size_t)Math::round(mSize.y()));

	onSizeChanged();
}

void VideoVlcComponent::render(const Transform4x4f& parentTrans)
{
	if (!isVisible())
		return;

	VideoComponent::render(parentTrans);
	Transform4x4f trans = parentTrans * getTransform();
	GuiComponent::renderChildren(trans);
	Renderer::setMatrix(trans);

	if (mIsPlaying && mContext.valid)
	{
		const unsigned int fadeIn = (unsigned int)(Math::clamp(0.0f, mFadeIn, 1.0f) * 255.0f);
		const unsigned int color  = Renderer::convertColor((fadeIn << 24) | (fadeIn << 16) | (fadeIn << 8) | 255);
		Renderer::Vertex   vertices[4];

		vertices[0] = { { 0.0f     , 0.0f      }, { 0.0f, 0.0f }, color };
		vertices[1] = { { 0.0f     , mSize.y() }, { 0.0f, 1.0f }, color };
		vertices[2] = { { mSize.x(), 0.0f      }, { 1.0f, 0.0f }, color };
		vertices[3] = { { mSize.x(), mSize.y() }, { 1.0f, 1.0f }, color };

		// round vertices
		for(int i = 0; i < 4; ++i)
			vertices[i].pos.round();

		// Build a texture for the video frame
		mTexture->initFromPixels((unsigned char*)mContext.surface->pixels, mContext.surface->w, mContext.surface->h);
		mTexture->bind();

		// Render it
		Renderer::drawTriangleStrips(&vertices[0], 4);
	}
	else
	{
		VideoComponent::renderSnapshot(parentTrans);
	}
}

void VideoVlcComponent::setupContext()
{
	if (!mContext.valid)
	{
		// Create an RGBA surface to render the video into
		mContext.surface = SDL_CreateRGBSurface(SDL_SWSURFACE, (int)mVideoWidth, (int)mVideoHeight, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
		mContext.mutex = SDL_CreateMutex();
		mContext.valid = true;
		resize();
	}
}

void VideoVlcComponent::freeContext()
{
	if (mContext.valid)
	{
		SDL_FreeSurface(mContext.surface);
		SDL_DestroyMutex(mContext.mutex);
		mContext.valid = false;
	}
}

void VideoVlcComponent::setupVLC(std::string subtitles)
{
	// If VLC hasn't been initialised yet then do it now
	if (!mVLC)
	{
		const char** args;
		const char* newargs[] = { "--quiet", "--sub-file", subtitles.c_str() };
		const char* singleargs[] = { "--quiet" };
		int argslen = 0;

		if (!subtitles.empty())
		{
			argslen = sizeof(newargs) / sizeof(newargs[0]);
			args = newargs;
		}
		else
		{
			argslen = sizeof(singleargs) / sizeof(singleargs[0]);
			args = singleargs;
		}
		mVLC = libvlc_new(argslen, args);
	}
}

void VideoVlcComponent::handleLooping()
{
	if (mIsPlaying && mMediaPlayer)
	{
		libvlc_state_t state = libvlc_media_player_get_state(mMediaPlayer);
		if (state == libvlc_Ended)
		{
			if (!Settings::getInstance()->getBool("VideoAudio") ||
				(Settings::getInstance()->getBool("ScreenSaverVideoMute") && mScreensaverMode))
			{
				libvlc_audio_set_mute(mMediaPlayer, 1);
			}
			//libvlc_media_player_set_position(mMediaPlayer, 0.0f);
			libvlc_media_player_set_media(mMediaPlayer, mMedia);
			libvlc_media_player_play(mMediaPlayer);
		}
	}
}

void VideoVlcComponent::startVideo()
{
	if (!mIsPlaying) {
		mVideoWidth = 0;
		mVideoHeight = 0;

#ifdef WIN32
		std::string path(Utils::String::replace(mVideoPath, "/", "\\"));
#else
		std::string path(mVideoPath);
#endif
		// Make sure we have a video path
		if (mVLC && (path.size() > 0))
		{
			// Set the video that we are going to be playing so we don't attempt to restart it
			mPlayingVideoPath = mVideoPath;

			// Open the media
			mMedia = libvlc_media_new_path(mVLC, path.c_str());
			if (mMedia)
			{
				unsigned track_count;
				// Get the media metadata so we can find the aspect ratio
				libvlc_media_parse(mMedia);
				libvlc_media_track_t** tracks;
				track_count = libvlc_media_tracks_get(mMedia, &tracks);
				for (unsigned track = 0; track < track_count; ++track)
				{
					if (tracks[track]->i_type == libvlc_track_video)
					{
						mVideoWidth = tracks[track]->video->i_width;
						mVideoHeight = tracks[track]->video->i_height;
						break;
					}
				}
				libvlc_media_tracks_release(tracks, track_count);

				// Make sure we found a valid video track
				if ((mVideoWidth > 0) && (mVideoHeight > 0))
				{
					if (mScreensaverMode)
					{
						std::string resolution = Settings::getInstance()->getString("VlcScreenSaverResolution");
						if(resolution != "original") {
							float scale = 1;			
							if (resolution == "low")
								// 25% of screen resolution
								scale = 0.25;
							if (resolution == "medium")
								// 50% of screen resolution
								scale = 0.5;
							if (resolution == "high")
								// 75% of screen resolution
								scale = 0.75;

							Vector2f resizeScale((Renderer::getScreenWidth() / (float)mVideoWidth) * scale, (Renderer::getScreenHeight() / (float)mVideoHeight) * scale);

							if(resizeScale.x() < resizeScale.y())
							{
								mVideoWidth = (unsigned int) (mVideoWidth * resizeScale.x());
								mVideoHeight = (unsigned int) (mVideoHeight * resizeScale.x());
							}else{
								mVideoWidth = (unsigned int) (mVideoWidth * resizeScale.y());
								mVideoHeight = (unsigned int) (mVideoHeight * resizeScale.y());
							}
						}
					}
					PowerSaver::pause();
					setupContext();

					// Setup the media player
					mMediaPlayer = libvlc_media_player_new_from_media(mMedia);

					if (!Settings::getInstance()->getBool("VideoAudio") ||
						(Settings::getInstance()->getBool("ScreenSaverVideoMute") && mScreensaverMode))
					{
						libvlc_audio_set_mute(mMediaPlayer, 1);
					}

					libvlc_media_player_play(mMediaPlayer);
					libvlc_video_set_callbacks(mMediaPlayer, lock, unlock, display, (void*)&mContext);
					libvlc_video_set_format(mMediaPlayer, "RGBA", (int)mVideoWidth, (int)mVideoHeight, (int)mVideoWidth * 4);

					// Update the playing state
					mIsPlaying = true;
					mFadeIn = 0.0f;
				}
			}
		}
	}
}

void VideoVlcComponent::stopVideo()
{
	mIsPlaying = false;
	mStartDelayed = false;
	// Release the media player so it stops calling back to us
	if (mMediaPlayer)
	{
		libvlc_media_player_stop(mMediaPlayer);
		libvlc_media_player_release(mMediaPlayer);
		libvlc_media_release(mMedia);
		mMediaPlayer = NULL;
		freeContext();
		PowerSaver::resume();
	}
}
