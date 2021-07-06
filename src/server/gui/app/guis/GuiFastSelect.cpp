#include "guis/GuiFastSelect.h"

#include "views/gamelist/IGameListView.h"
#include "FileSorts.h"
#include "SystemData.h"

static const std::string LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

GuiFastSelect::GuiFastSelect(Window* window, IGameListView* gamelist) : GuiComponent(window),
	mBackground(window), mSortText(window), mLetterText(window), mGameList(gamelist)
{
	setPosition(Renderer::getScreenWidth() * 0.2f, Renderer::getScreenHeight() * 0.2f);
	setSize(Renderer::getScreenWidth() * 0.6f, Renderer::getScreenHeight() * 0.6f);

	const std::shared_ptr<ThemeData>& theme = mGameList->getTheme();
	using namespace ThemeFlags;

	mBackground.applyTheme(theme, "fastSelect", "windowBackground", PATH);
	mBackground.fitTo(mSize);
	addChild(&mBackground);

	mLetterText.setSize(mSize.x(), mSize.y() * 0.75f);
	mLetterText.setHorizontalAlignment(ALIGN_CENTER);
	mLetterText.applyTheme(theme, "fastSelect", "letter", FONT_PATH | COLOR);
	// TODO - set font size
	addChild(&mLetterText);

	mSortText.setPosition(0, mSize.y() * 0.75f);
	mSortText.setSize(mSize.x(), mSize.y() * 0.25f);
	mSortText.setHorizontalAlignment(ALIGN_CENTER);
	mSortText.applyTheme(theme, "fastSelect", "subtext", FONT_PATH | COLOR);
	// TODO - set font size
	addChild(&mSortText);

	mSortId = 0; // TODO
	updateSortText();

	mLetterId = LETTERS.find(mGameList->getCursor()->getName()[0]);
	if(mLetterId == std::string::npos)
		mLetterId = 0;

	mScrollDir = 0;
	mScrollAccumulator = 0;
	scroll(); // initialize the letter value
}

bool GuiFastSelect::input(InputConfig* config, Input input)
{
	if(input.value == 0 && config->isMappedTo("select", input))
	{
		// the user let go of select; make our changes to the gamelist and close this gui
		updateGameListSort();
		updateGameListCursor();
		delete this;
		return true;
	}

	if(config->isMappedLike("up", input))
	{
		if(input.value != 0)
			setScrollDir(-1);
		else
			setScrollDir(0);

		return true;
	}else if(config->isMappedLike("down", input))
	{
		if(input.value != 0)
			setScrollDir(1);
		else
			setScrollDir(0);

		return true;
	}else if(config->isMappedLike("left", input) && input.value != 0)
	{
		mSortId = (mSortId + 1) % FileSorts::SortTypes.size();
		updateSortText();
		return true;
	}else if(config->isMappedLike("right", input) && input.value != 0)
	{
		mSortId--;
		if(mSortId < 0)
			mSortId += (int)FileSorts::SortTypes.size();

		updateSortText();
		return true;
	}

	return GuiComponent::input(config, input);
}

void GuiFastSelect::setScrollDir(int dir)
{
	mScrollDir = dir;
	scroll();
	mScrollAccumulator = -500;
}

void GuiFastSelect::update(int deltaTime)
{
	if(mScrollDir != 0)
	{
		mScrollAccumulator += deltaTime;
		while(mScrollAccumulator >= 150)
		{
			scroll();
			mScrollAccumulator -= 150;
		}
	}

	GuiComponent::update(deltaTime);
}

void GuiFastSelect::scroll()
{
	mLetterId += mScrollDir;
	if(mLetterId < 0)
		mLetterId += LETTERS.length();
	else if(mLetterId >= LETTERS.length())
		mLetterId -= LETTERS.length();

	mLetterText.setText(LETTERS.substr(mLetterId, 1));
}

void GuiFastSelect::updateSortText()
{
	std::stringstream ss;
	ss << "<- " << FileSorts::SortTypes.at(mSortId).description << " ->";
	mSortText.setText(ss.str());
}

void GuiFastSelect::updateGameListSort()
{
	const FileData::SortType& sort = FileSorts::SortTypes.at(mSortId);

	FileData* root = mGameList->getCursor()->getSystem()->getRootFolder();
	root->sort(sort); // will also recursively sort children

	// notify that the root folder was sorted
	mGameList->onFileChanged(root, FILE_SORTED);
}

void GuiFastSelect::updateGameListCursor()
{
	const std::vector<FileData*>& list = mGameList->getCursor()->getParent()->getChildren();

	// only skip by letter when the sort mode is alphabetical
	const FileData::SortType& sort = FileSorts::SortTypes.at(mSortId);
	if(sort.comparisonFunction != &FileSorts::compareName)
		return;

	// find the first entry in the list that either exactly matches our target letter or is beyond our target letter
	for(auto it = list.cbegin(); it != list.cend(); it++)
	{
		char check = (*it)->getName().empty() ? 'A' : (*it)->getName()[0];

		// if there's an exact match or we've passed it, set the cursor to this one
		if(check == LETTERS[mLetterId] || (sort.ascending && check > LETTERS[mLetterId]) || (!sort.ascending && check < LETTERS[mLetterId]))
		{
			mGameList->setCursor(*it);
			break;
		}
	}
}
