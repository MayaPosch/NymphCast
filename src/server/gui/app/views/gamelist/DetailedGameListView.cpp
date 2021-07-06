#include "views/gamelist/DetailedGameListView.h"

#include "animations/LambdaAnimation.h"
#include "views/ViewController.h"

DetailedGameListView::DetailedGameListView(Window* window, FileData* root) :
	BasicGameListView(window, root),
	mDescContainer(window, DESCRIPTION_SCROLL_DELAY), mDescription(window),
	mThumbnail(window),
	mMarquee(window),
	mImage(window),

	mLblRating(window), mLblReleaseDate(window), mLblDeveloper(window), mLblPublisher(window),
	mLblGenre(window), mLblPlayers(window), mLblLastPlayed(window), mLblPlayCount(window),

	mRating(window), mReleaseDate(window), mDeveloper(window), mPublisher(window),
	mGenre(window), mPlayers(window), mLastPlayed(window), mPlayCount(window),
	mName(window)
{
	//mHeaderImage.setPosition(mSize.x() * 0.25f, 0);

	const float padding = 0.01f;

	mList.setPosition(mSize.x() * (0.50f + padding), mList.getPosition().y());
	mList.setSize(mSize.x() * (0.50f - padding), mList.getSize().y());
	mList.setAlignment(TextListComponent<FileData*>::ALIGN_LEFT);
	mList.setCursorChangedCallback([&](const CursorState& /*state*/) { updateInfoPanel(); });

	// Image
	mImage.setOrigin(0.5f, 0.5f);
	mImage.setPosition(mSize.x() * 0.25f, mList.getPosition().y() + mSize.y() * 0.2125f);
	mImage.setMaxSize(mSize.x() * (0.50f - 2*padding), mSize.y() * 0.4f);
	mImage.setDefaultZIndex(30);
	addChild(&mImage);

	// Thumbnail
	// Default to off the screen
	mThumbnail.setOrigin(0.5f, 0.5f);
	mThumbnail.setPosition(2.0f, 2.0f);
	mThumbnail.setMaxSize(mSize.x(), mSize.y());
	mThumbnail.setDefaultZIndex(35);
	mThumbnail.setVisible(false);
	addChild(&mThumbnail);

	// Marquee
	// Default to off the screen
	mMarquee.setOrigin(0.5f, 0.5f);
	mMarquee.setPosition(2.0f, 2.0f);
	mMarquee.setMaxSize(mSize.x(), mSize.y());
	mMarquee.setDefaultZIndex(35);
	mMarquee.setVisible(false);
	addChild(&mMarquee);

	// metadata labels + values
	mLblRating.setText("Rating: ");
	addChild(&mLblRating);
	addChild(&mRating);
	mLblReleaseDate.setText("Released: ");
	addChild(&mLblReleaseDate);
	addChild(&mReleaseDate);
	mLblDeveloper.setText("Developer: ");
	addChild(&mLblDeveloper);
	addChild(&mDeveloper);
	mLblPublisher.setText("Publisher: ");
	addChild(&mLblPublisher);
	addChild(&mPublisher);
	mLblGenre.setText("Genre: ");
	addChild(&mLblGenre);
	addChild(&mGenre);
	mLblPlayers.setText("Players: ");
	addChild(&mLblPlayers);
	addChild(&mPlayers);
	mLblLastPlayed.setText("Last played: ");
	addChild(&mLblLastPlayed);
	mLastPlayed.setDisplayRelative(true);
	addChild(&mLastPlayed);
	mLblPlayCount.setText("Times played: ");
	addChild(&mLblPlayCount);
	addChild(&mPlayCount);

	mName.setPosition(mSize.x(), mSize.y());
	mName.setDefaultZIndex(40);
	mName.setColor(0xAAAAAAFF);
	mName.setFont(Font::get(FONT_SIZE_MEDIUM));
	mName.setHorizontalAlignment(ALIGN_CENTER);
	addChild(&mName);

	mDescContainer.setPosition(mSize.x() * padding, mSize.y() * 0.65f);
	mDescContainer.setSize(mSize.x() * (0.50f - 2*padding), mSize.y() - mDescContainer.getPosition().y());
	mDescContainer.setAutoScroll(true);
	mDescContainer.setDefaultZIndex(40);
	addChild(&mDescContainer);

	mDescription.setFont(Font::get(FONT_SIZE_SMALL));
	mDescription.setSize(mDescContainer.getSize().x(), 0);
	mDescContainer.addChild(&mDescription);


	initMDLabels();
	initMDValues();
	updateInfoPanel();
}

void DetailedGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	BasicGameListView::onThemeChanged(theme);

	using namespace ThemeFlags;
	mThumbnail.applyTheme(theme, getName(), "md_thumbnail", POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
	mMarquee.applyTheme(theme, getName(), "md_marquee", POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
	mImage.applyTheme(theme, getName(), "md_image", POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
	mName.applyTheme(theme, getName(), "md_name", ALL);

	initMDLabels();
	std::vector<TextComponent*> labels = getMDLabels();
	assert(labels.size() == 8);
	const char* lblElements[8] = {
		"md_lbl_rating", "md_lbl_releasedate", "md_lbl_developer", "md_lbl_publisher",
		"md_lbl_genre", "md_lbl_players", "md_lbl_lastplayed", "md_lbl_playcount"
	};

	for(unsigned int i = 0; i < labels.size(); i++)
	{
		labels[i]->applyTheme(theme, getName(), lblElements[i], ALL);
	}


	initMDValues();
	std::vector<GuiComponent*> values = getMDValues();
	assert(values.size() == 8);
	const char* valElements[8] = {
		"md_rating", "md_releasedate", "md_developer", "md_publisher",
		"md_genre", "md_players", "md_lastplayed", "md_playcount"
	};

	for(unsigned int i = 0; i < values.size(); i++)
	{
		values[i]->applyTheme(theme, getName(), valElements[i], ALL ^ ThemeFlags::TEXT);
	}

	mDescContainer.applyTheme(theme, getName(), "md_description", POSITION | ThemeFlags::SIZE | Z_INDEX | VISIBLE);
	mDescription.setSize(mDescContainer.getSize().x(), 0);
	mDescription.applyTheme(theme, getName(), "md_description", ALL ^ (POSITION | ThemeFlags::SIZE | ThemeFlags::ORIGIN | TEXT | ROTATION));

	sortChildren();
}

void DetailedGameListView::initMDLabels()
{
	std::vector<TextComponent*> components = getMDLabels();

	const unsigned int colCount = 2;
	const unsigned int rowCount = (int)(components.size() / 2);

	Vector3f start(mSize.x() * 0.01f, mSize.y() * 0.625f, 0.0f);

	const float colSize = (mSize.x() * 0.48f) / colCount;
	const float rowPadding = 0.01f * mSize.y();

	for(unsigned int i = 0; i < components.size(); i++)
	{
		const unsigned int row = i % rowCount;
		Vector3f pos(0.0f, 0.0f, 0.0f);
		if(row == 0)
		{
			pos = start + Vector3f(colSize * (i / rowCount), 0, 0);
		}else{
			// work from the last component
			GuiComponent* lc = components[i-1];
			pos = lc->getPosition() + Vector3f(0, lc->getSize().y() + rowPadding, 0);
		}

		components[i]->setFont(Font::get(FONT_SIZE_SMALL));
		components[i]->setPosition(pos);
		components[i]->setDefaultZIndex(40);
	}
}

void DetailedGameListView::initMDValues()
{
	std::vector<TextComponent*> labels = getMDLabels();
	std::vector<GuiComponent*> values = getMDValues();

	std::shared_ptr<Font> defaultFont = Font::get(FONT_SIZE_SMALL);
	mRating.setSize(defaultFont->getHeight() * 5.0f, (float)defaultFont->getHeight());
	mReleaseDate.setFont(defaultFont);
	mDeveloper.setFont(defaultFont);
	mPublisher.setFont(defaultFont);
	mGenre.setFont(defaultFont);
	mPlayers.setFont(defaultFont);
	mLastPlayed.setFont(defaultFont);
	mPlayCount.setFont(defaultFont);

	float bottom = 0.0f;

	const float colSize = (mSize.x() * 0.48f) / 2;
	for(unsigned int i = 0; i < labels.size(); i++)
	{
		const float heightDiff = (labels[i]->getSize().y() - values[i]->getSize().y()) / 2;
		values[i]->setPosition(labels[i]->getPosition() + Vector3f(labels[i]->getSize().x(), heightDiff, 0));
		values[i]->setSize(colSize - labels[i]->getSize().x(), values[i]->getSize().y());
		values[i]->setDefaultZIndex(40);

		float testBot = values[i]->getPosition().y() + values[i]->getSize().y();
		if(testBot > bottom)
			bottom = testBot;
	}

	mDescContainer.setPosition(mDescContainer.getPosition().x(), bottom + mSize.y() * 0.01f);
	mDescContainer.setSize(mDescContainer.getSize().x(), mSize.y() - mDescContainer.getPosition().y());
}

void DetailedGameListView::updateInfoPanel()
{
	FileData* file = (mList.size() == 0 || mList.isScrolling()) ? NULL : mList.getSelected();

	bool fadingOut;
	if(file == NULL)
	{
		//mImage.setImage("");
		//mDescription.setText("");
		fadingOut = true;
	}else{
		mThumbnail.setImage(file->getThumbnailPath());
		mMarquee.setImage(file->getMarqueePath());
		mImage.setImage(file->getImagePath());
		mDescription.setText(file->metadata.get("desc"));
		mDescContainer.reset();

		mRating.setValue(file->metadata.get("rating"));
		mReleaseDate.setValue(file->metadata.get("releasedate"));
		mDeveloper.setValue(file->metadata.get("developer"));
		mPublisher.setValue(file->metadata.get("publisher"));
		mGenre.setValue(file->metadata.get("genre"));
		mPlayers.setValue(file->metadata.get("players"));
		mName.setValue(file->metadata.get("name"));

		if(file->getType() == GAME)
		{
			mLastPlayed.setValue(file->metadata.get("lastplayed"));
			mPlayCount.setValue(file->metadata.get("playcount"));
		}

		fadingOut = false;
	}

	std::vector<GuiComponent*> comps = getMDValues();
	comps.push_back(&mThumbnail);
	comps.push_back(&mMarquee);
	comps.push_back(&mImage);
	comps.push_back(&mDescription);
	comps.push_back(&mName);
	std::vector<TextComponent*> labels = getMDLabels();
	comps.insert(comps.cend(), labels.cbegin(), labels.cend());

	for(auto it = comps.cbegin(); it != comps.cend(); it++)
	{
		GuiComponent* comp = *it;
		// an animation is playing
		//   then animate if reverse != fadingOut
		// an animation is not playing
		//   then animate if opacity != our target opacity
		if((comp->isAnimationPlaying(0) && comp->isAnimationReversed(0) != fadingOut) ||
			(!comp->isAnimationPlaying(0) && comp->getOpacity() != (fadingOut ? 0 : 255)))
		{
			auto func = [comp](float t)
			{
				comp->setOpacity((unsigned char)(Math::lerp(0.0f, 1.0f, t)*255));
			};
			comp->setAnimation(new LambdaAnimation(func, 150), 0, nullptr, fadingOut);
		}
	}
}

void DetailedGameListView::launch(FileData* game)
{
	Vector3f target(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f, 0);
	if(mImage.hasImage())
		target = Vector3f(mImage.getCenter().x(), mImage.getCenter().y(), 0);

	ViewController::get()->launch(game, target);
}

std::vector<TextComponent*> DetailedGameListView::getMDLabels()
{
	std::vector<TextComponent*> ret;
	ret.push_back(&mLblRating);
	ret.push_back(&mLblReleaseDate);
	ret.push_back(&mLblDeveloper);
	ret.push_back(&mLblPublisher);
	ret.push_back(&mLblGenre);
	ret.push_back(&mLblPlayers);
	ret.push_back(&mLblLastPlayed);
	ret.push_back(&mLblPlayCount);
	return ret;
}

std::vector<GuiComponent*> DetailedGameListView::getMDValues()
{
	std::vector<GuiComponent*> ret;
	ret.push_back(&mRating);
	ret.push_back(&mReleaseDate);
	ret.push_back(&mDeveloper);
	ret.push_back(&mPublisher);
	ret.push_back(&mGenre);
	ret.push_back(&mPlayers);
	ret.push_back(&mLastPlayed);
	ret.push_back(&mPlayCount);
	return ret;
}

void DetailedGameListView::onFocusLost() {
	mDescContainer.reset();
}
