#pragma once
#ifndef ES_APP_VIEWS_GAME_LIST_GRID_GAME_LIST_VIEW_H
#define ES_APP_VIEWS_GAME_LIST_GRID_GAME_LIST_VIEW_H

#include "components/DateTimeComponent.h"
#include "components/RatingComponent.h"
#include "components/ScrollableContainer.h"
#include "components/ImageGridComponent.h"
#include "components/VideoComponent.h"
#include "views/gamelist/ISimpleGameListView.h"

class GridGameListView : public ISimpleGameListView
{
public:
	GridGameListView(Window* window, FileData* root);
	virtual ~GridGameListView();

	virtual void onShow() override;

	virtual void onThemeChanged(const std::shared_ptr<ThemeData>& theme) override;

	virtual FileData* getCursor() override;
	virtual void setCursor(FileData*) override;

	virtual bool input(InputConfig* config, Input input) override;

	virtual const char* getName() const override { return "grid"; }

	virtual std::vector<HelpPrompt> getHelpPrompts() override;
	virtual void launch(FileData* game) override;

	void onFocusLost(void) override;

protected:
	virtual void update(int deltaTime) override;
	virtual std::string getQuickSystemSelectRightButton() override;
	virtual std::string getQuickSystemSelectLeftButton() override;
	virtual void populateList(const std::vector<FileData*>& files) override;
	virtual void remove(FileData* game, bool deleteFile) override;
	virtual void addPlaceholder();

	ImageGridComponent<FileData*> mGrid;

private:
	void updateInfoPanel();
	const std::string getImagePath(FileData* file);

	void initMDLabels();
	void initMDValues();

	TextComponent mLblRating, mLblReleaseDate, mLblDeveloper, mLblPublisher, mLblGenre, mLblPlayers, mLblLastPlayed, mLblPlayCount;

	ImageComponent mMarquee;
	VideoComponent* mVideo;
	bool mVideoPlaying;
	ImageComponent mImage;
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
};

#endif // ES_APP_VIEWS_GAME_LIST_GRID_GAME_LIST_VIEW_H
