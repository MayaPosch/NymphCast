#pragma once
#ifndef ES_CORE_COMPONENTS_VIDEO_VLC_COMPONENT_H
#define ES_CORE_COMPONENTS_VIDEO_VLC_COMPONENT_H

#include "VideoComponent.h"

struct SDL_mutex;
struct SDL_Surface;
struct libvlc_instance_t;
struct libvlc_media_t;
struct libvlc_media_player_t;

struct VideoContext {
	SDL_Surface*		surface;
	SDL_mutex*			mutex;
	bool				valid;
};

class VideoVlcComponent : public VideoComponent
{
	// Structure that groups together the configuration of the video component
	struct Configuration
	{
		unsigned						startDelay;
		bool							showSnapshotNoVideo;
		bool							showSnapshotDelay;
		std::string						defaultVideoPath;
	};

public:
	static void setupVLC(std::string subtitles);

	VideoVlcComponent(Window* window, std::string subtitles);
	virtual ~VideoVlcComponent();

	void render(const Transform4x4f& parentTrans) override;


	// Resize the video to fit this size. If one axis is zero, scale that axis to maintain aspect ratio.
	// If both are non-zero, potentially break the aspect ratio.  If both are zero, no resizing.
	// Can be set before or after a video is loaded.
	// setMaxSize() and setResize() are mutually exclusive.
	void setResize(float width, float height);

	// Resize the video to be as large as possible but fit within a box of this size.
	// Can be set before or after a video is loaded.
	// Never breaks the aspect ratio. setMaxSize() and setResize() are mutually exclusive.
	void setMaxSize(float width, float height);

private:
	// Calculates the correct mSize from our resizing information (set by setResize/setMaxSize).
	// Used internally whenever the resizing parameters or texture change.
	void resize();
	// Start the video Immediately
	virtual void startVideo();
	// Stop the video
	virtual void stopVideo();
	// Handle looping the video. Must be called periodically
	virtual void handleLooping();

	void setupContext();
	void freeContext();

private:
	static libvlc_instance_t*		mVLC;
	libvlc_media_t*					mMedia;
	libvlc_media_player_t*			mMediaPlayer;
	VideoContext					mContext;
	std::shared_ptr<TextureResource> mTexture;
};

#endif // ES_CORE_COMPONENTS_VIDEO_VLC_COMPONENT_H
