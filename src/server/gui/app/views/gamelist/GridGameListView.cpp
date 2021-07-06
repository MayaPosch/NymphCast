#include "views/gamelist/GridGameListView.h"

#include "animations/LambdaAnimation.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "SystemData.h"
#ifdef _RPI_
#include "components/VideoPlayerComponent.h"
#endif
#include "components/VideoVlcComponent.h"

GridGameListView::GridGameListView(Window* window, FileData* root) :
	ISimpleGameListView(window, root),
	mGrid(window), mMarquee(window),
	mImage(window),
	mVideo(nullptr),
	mVideoPlaying(false),
	mDescContainer(window, DESCRIPTION_SCROLL_DELAY), mDescription(window),

	mLblRating(window), mLblReleaseDate(window), mLblDeveloper(window), mLblPublisher(window),
	mLblGenre(window), mLblPlayers(window), mLblLastPlayed(window), mLblPlayCount(window),

	mRating(window), mReleaseDate(window), mDeveloper(window), mPublisher(window),
	mGenre(window), mPlayers(window), mLastPlayed(window), mPlayCount(window),
	mName(window)
{
	const float padding = 0.01f;

// Create the correct type of video window
#ifdef _RPI_
	if (Settings::getInstance()->getBool("VideoOmxPlayer"))
		mVideo = new VideoPlayerComponent(window, "");
	else
		mVideo = new VideoVlcComponent(window, getTitlePath());
#else
	mVideo = new VideoVlcComponent(window, getTitlePath());
#endif

	mGrid.setPosition(mSize.x() * 0.1f, mSize.y() * 0.1f);
	mGrid.setDefaultZIndex(20);
	mGrid.setCursorChangedCallback([&](const CursorState& /*state*/) { updateInfoPanel(); });
	addChild(&mGrid);

	populateList(root->getChildrenListToDisplay());

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

	// Image
	// Default to off the screen
	mImage.setOrigin(0.5f, 0.5f);
	mImage.setPosition(2.0f, 2.0f);
	mImage.setMaxSize(mSize.x(), mSize.y());
	mImage.setDefaultZIndex(30);
	mImage.setVisible(false);
	addChild(&mImage);

	// Video
	// Default to off the screen
	mVideo->setOrigin(0.5f, 0.5f);
	mVideo->setPosition(2.0f, 2.0f);
	mVideo->setSize(mSize.x(), mSize.y());
	mVideo->setDefaultZIndex(30);
	mVideo->setVisible(false);
	addChild(mVideo);

	// Marquee
	// Default to off the screen
	mMarquee.setOrigin(0.5f, 0.5f);
	mMarquee.setPosition(2.0f, 2.0f);
	mMarquee.setMaxSize(mSize.x(), mSize.y());
	mMarquee.setDefaultZIndex(35);
	mMarquee.setVisible(false);
	addChild(&mMarquee);

	initMDLabels();
	initMDValues();
	updateInfoPanel();
}

GridGameListView::~GridGameListView()
{
	delete mVideo;
}

FileData* GridGameListView::getCursor()
{
	return mGrid.getSelected();
}

void GridGameListView::setCursor(FileData* file)
{
	if(!mGrid.setCursor(file) && (!file->isPlaceHolder()))
	{
		populateList(file->getParent()->getChildrenListToDisplay());
		mGrid.setCursor(file);
	}
}

std::string GridGameListView::getQuickSystemSelectRightButton()
{
	return "rightshoulder";
}

std::string GridGameListView::getQuickSystemSelectLeftButton()
{
	return "leftshoulder";
}

bool GridGameListView::input(InputConfig* config, Input input)
{
	if(config->isMappedLike("left", input) || config->isMappedLike("right", input))
		return GuiComponent::input(config, input);

	return ISimpleGameListView::input(config, input);
}

const std::string GridGameListView::getImagePath(FileData* file)
{
	ImageSource src = mGrid.getImageSource();

	if (src == ImageSource::IMAGE)
		return file->getImagePath();
	else if (src == ImageSource::MARQUEE)
		return file->getMarqueePath();

	return file->getThumbnailPath();
}

void GridGameListView::populateList(const std::vector<FileData*>& files)
{
	mGrid.clear();
	mHeaderText.setText(mRoot->getSystem()->getFullName());
	if (files.size() > 0)
	{
		for (auto it = files.cbegin(); it != files.cend(); it++)
		{
			mGrid.add((*it)->getName(), getImagePath(*it), *it);
		}
	}
	else
	{
		addPlaceholder();
	}
}

void GridGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ISimpleGameListView::onThemeChanged(theme);

	using namespace ThemeFlags;

	mGrid.applyTheme(theme, getName(), "gamegrid", ALL);
	mName.applyTheme(theme, getName(), "md_name", ALL);
	mMarquee.applyTheme(theme, getName(), "md_marquee", POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
	mImage.applyTheme(theme, getName(), "md_image", POSITION | ThemeFlags::SIZE | Z_INDEX | ROTATION | VISIBLE);
	mVideo->applyTheme(theme, getName(), "md_video", POSITION | ThemeFlags::SIZE | ThemeFlags::DELAY | Z_INDEX | ROTATION | VISIBLE);

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

	// Repopulate list in case new theme is displaying a different image.  Preserve selection.
	FileData* file = mGrid.getSelected();
	populateList(mRoot->getChildrenListToDisplay());
	mGrid.setCursor(file);

	sortChildren();
}

void GridGameListView::initMDLabels()
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

void GridGameListView::initMDValues()
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

void GridGameListView::updateInfoPanel()
{
	FileData* file = (mGrid.size() == 0 || mGrid.isScrolling()) ? NULL : mGrid.getSelected();

	bool fadingOut;
	if(file == NULL)
	{
		mVideo->setVideo("");
		mVideo->setImage("");
		mVideoPlaying = false;

		//mDescription.setText("");
		fadingOut = true;
	}else{
		if (!mVideo->setVideo(file->getVideoPath()))
		{
			mVideo->setDefaultVideo();
		}
		mVideoPlaying = true;

		mVideo->setImage(file->getThumbnailPath());
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
	comps.push_back(&mDescription);
	comps.push_back(&mName);
	comps.push_back(&mMarquee);
	comps.push_back(mVideo);
	comps.push_back(&mImage);
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

void GridGameListView::addPlaceholder()
{
	// empty grid - add a placeholder
	FileData* placeholder = new FileData(PLACEHOLDER, "<No Entries Found>", this->mRoot->getSystem()->getSystemEnvData(), this->mRoot->getSystem());
	mGrid.add(placeholder->getName(), "", placeholder);
}

void GridGameListView::launch(FileData* game)
{
	float screenWidth = (float) Renderer::getScreenWidth();
	float screenHeight = (float) Renderer::getScreenHeight();

	Vector3f target(screenWidth / 2.0f, screenHeight / 2.0f, 0);

	if(mMarquee.hasImage() &&
		(mMarquee.getPosition().x() < screenWidth && mMarquee.getPosition().x() > 0.0f &&
		 mMarquee.getPosition().y() < screenHeight && mMarquee.getPosition().y() > 0.0f))
	{
		target = Vector3f(mMarquee.getCenter().x(), mMarquee.getCenter().y(), 0);
	}
	else if(mImage.hasImage() &&
		(mImage.getPosition().x() < screenWidth && mImage.getPosition().x() > 2.0f &&
		 mImage.getPosition().y() < screenHeight && mImage.getPosition().y() > 2.0f))
	{
		target = Vector3f(mImage.getCenter().x(), mImage.getCenter().y(), 0);
	}
	else if(mVideo->getPosition().x() < screenWidth && mVideo->getPosition().x() > 0.0f &&
		 mVideo->getPosition().y() < screenHeight && mVideo->getPosition().y() > 0.0f)
	{
		target = Vector3f(mVideo->getCenter().x(), mVideo->getCenter().y(), 0);
	}

	ViewController::get()->launch(game, target);

}

void GridGameListView::remove(FileData *game, bool deleteFile)
{
	if (deleteFile)
		Utils::FileSystem::removeFile(game->getPath());  // actually delete the file on the filesystem
	FileData* parent = game->getParent();
	if (getCursor() == game)                     // Select next element in list, or prev if none
	{
		std::vector<FileData*> siblings = parent->getChildrenListToDisplay();
		auto gameIter = std::find(siblings.cbegin(), siblings.cend(), game);
		int gamePos = (int)std::distance(siblings.cbegin(), gameIter);
		if (gameIter != siblings.cend())
		{
			if ((gamePos + 1) < (int)siblings.size())
			{
				setCursor(siblings.at(gamePos + 1));
			} else if ((gamePos - 1) > 0) {
				setCursor(siblings.at(gamePos - 1));
			}
		}
	}
	mGrid.remove(game);
	if(mGrid.size() == 0)
	{
		addPlaceholder();
	}
	delete game;                                 // remove before repopulating (removes from parent)
	onFileChanged(parent, FILE_REMOVED);           // update the view, with game removed
}

std::vector<TextComponent*> GridGameListView::getMDLabels()
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

std::vector<GuiComponent*> GridGameListView::getMDValues()
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

std::vector<HelpPrompt> GridGameListView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;

	if(Settings::getInstance()->getBool("QuickSystemSelect"))
		prompts.push_back(HelpPrompt("lr", "system"));
	prompts.push_back(HelpPrompt("up/down/left/right", "choose"));
	prompts.push_back(HelpPrompt("a", "launch"));
	prompts.push_back(HelpPrompt("b", "back"));
	if(!UIModeController::getInstance()->isUIModeKid())
		prompts.push_back(HelpPrompt("select", "options"));
	if(mRoot->getSystem()->isGameSystem())
		prompts.push_back(HelpPrompt("x", "random"));
	if(mRoot->getSystem()->isGameSystem() && !UIModeController::getInstance()->isUIModeKid())
	{
		std::string prompt = CollectionSystemManager::get()->getEditingCollection();
		prompts.push_back(HelpPrompt("y", prompt));
	}
	return prompts;
}

void GridGameListView::update(int deltaTime)
{
	ISimpleGameListView::update(deltaTime);
	mVideo->update(deltaTime);
}

void GridGameListView::onShow()
{
	GuiComponent::onShow();
	updateInfoPanel();
}

void GridGameListView::onFocusLost() {
	mDescContainer.reset();
}
