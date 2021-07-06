#pragma once
#ifndef ES_APP_VIEWS_GAME_LIST_VIDEO_GAME_LIST_VIEW_H
#define ES_APP_VIEWS_GAME_LIST_VIDEO_GAME_LIST_VIEW_H

#include "components/DateTimeComponent.h"
#include "components/RatingComponent.h"
#include "components/ScrollableContainer.h"
#include "views/gamelist/BasicGameListView.h"

class VideoComponent;

class VideoGameListView : public BasicGameListView
{
public:
	VideoGameListView(Window* window, FileData* root);
	virtual ~VideoGameListView();

	virtual void onShow() override;

	virtual void onThemeChanged(const std::shared_ptr<ThemeData>& theme) override;

	virtual const char* getName() const override { return "video"; }
	virtual void launch(FileData* game) override;

	void onFocusLost(void) override;

protected:
	virtual void update(int deltaTime) override;

private:
	void updateInfoPanel();

	void initMDLabels();
	void initMDValues();

	ImageComponent mThumbnail;
	ImageComponent mMarquee;
	VideoComponent* mVideo;
	ImageComponent mImage;

	TextComponent mLblRating, mLblReleaseDate, mLblDeveloper, mLblPublisher, mLblGenre, mLblPlayers, mLblLastPlayed, mLblPlayCount;

	RatingComponent mRating;
	DateTimeComponent mReleaseDate;
	TextComponent mDeveloper;
	TextComponent mPublisher;
	TextComponent mGenre;
	TextComponent mPlayers;
	DateTimeComponent mLastPlayed;
	TextComponent mPlayCount;
	TextComponent mName;

	std::vector<TextComponent*> getMDLabels();
	std::vector<GuiComponent*> getMDValues();

	ScrollableContainer mDescContainer;
	TextComponent mDescription;

	bool		mVideoPlaying;

};

#endif // ES_APP_VIEWS_GAME_LIST_VIDEO_GAME_LIST_VIEW_H
