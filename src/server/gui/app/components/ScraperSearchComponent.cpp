#include "components/ScraperSearchComponent.h"

#include "components/ComponentList.h"
#include "components/DateTimeEditComponent.h"
#include "components/ImageComponent.h"
#include "components/RatingComponent.h"
#include "components/ScrollableContainer.h"
#include "components/TextComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditPopup.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"
#include "FileData.h"
#include "Log.h"
#include "Window.h"

ScraperSearchComponent::ScraperSearchComponent(Window* window, SearchType type) : GuiComponent(window),
	mGrid(window, Vector2i(4, 3)), mBusyAnim(window),
	mSearchType(type)
{
	addChild(&mGrid);

	mBlockAccept = false;

	// left spacer (empty component, needed for borders)
	mGrid.setEntry(std::make_shared<GuiComponent>(mWindow), Vector2i(0, 0), false, false, Vector2i(1, 3), GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);

	// selected result name
	mResultName = std::make_shared<TextComponent>(mWindow, "Result name", Font::get(FONT_SIZE_MEDIUM), 0x777777FF);

	// selected result thumbnail
	mResultThumbnail = std::make_shared<ImageComponent>(mWindow);
	mGrid.setEntry(mResultThumbnail, Vector2i(1, 1), false, false, Vector2i(1, 1));

	// selected result desc + container
	mDescContainer = std::make_shared<ScrollableContainer>(mWindow);
	mResultDesc = std::make_shared<TextComponent>(mWindow, "Result desc", Font::get(FONT_SIZE_SMALL), 0x777777FF);
	mDescContainer->addChild(mResultDesc.get());
	mDescContainer->setAutoScroll(true);

	// metadata
	auto font = Font::get(FONT_SIZE_SMALL); // this gets replaced in onSizeChanged() so its just a placeholder
	const unsigned int mdColor = 0x777777FF;
	const unsigned int mdLblColor = 0x666666FF;
	mMD_Rating = std::make_shared<RatingComponent>(mWindow);
	mMD_ReleaseDate = std::make_shared<DateTimeEditComponent>(mWindow);
	mMD_ReleaseDate->setColor(mdColor);
	mMD_Developer = std::make_shared<TextComponent>(mWindow, "", font, mdColor);
	mMD_Publisher = std::make_shared<TextComponent>(mWindow, "", font, mdColor);
	mMD_Genre = std::make_shared<TextComponent>(mWindow, "", font, mdColor);
	mMD_Players = std::make_shared<TextComponent>(mWindow, "", font, mdColor);

	mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>(mWindow, "RATING:", font, mdLblColor), mMD_Rating, false));
	mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>(mWindow, "RELEASED:", font, mdLblColor), mMD_ReleaseDate));
	mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>(mWindow, "DEVELOPER:", font, mdLblColor), mMD_Developer));
	mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>(mWindow, "PUBLISHER:", font, mdLblColor), mMD_Publisher));
	mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>(mWindow, "GENRE:", font, mdLblColor), mMD_Genre));
	mMD_Pairs.push_back(MetaDataPair(std::make_shared<TextComponent>(mWindow, "PLAYERS:", font, mdLblColor), mMD_Players));

	mMD_Grid = std::make_shared<ComponentGrid>(mWindow, Vector2i(2, (int)mMD_Pairs.size()*2 - 1));
	unsigned int i = 0;
	for(auto it = mMD_Pairs.cbegin(); it != mMD_Pairs.cend(); it++)
	{
		mMD_Grid->setEntry(it->first, Vector2i(0, i), false, true);
		mMD_Grid->setEntry(it->second, Vector2i(1, i), false, it->resize);
		i += 2;
	}

	mGrid.setEntry(mMD_Grid, Vector2i(2, 1), false, false);

	// result list
	mResultList = std::make_shared<ComponentList>(mWindow);
	mResultList->setCursorChangedCallback([this](CursorState state) { if(state == CURSOR_STOPPED) updateInfoPane(); });

	updateViewStyle();
}

void ScraperSearchComponent::onSizeChanged()
{
	mGrid.setSize(mSize);

	if(mSize.x() == 0 || mSize.y() == 0)
		return;

	// column widths
	if(mSearchType == ALWAYS_ACCEPT_FIRST_RESULT)
		mGrid.setColWidthPerc(0, 0.02f); // looks better when this is higher in auto mode
	else
		mGrid.setColWidthPerc(0, 0.01f);

	mGrid.setColWidthPerc(1, 0.25f);
	mGrid.setColWidthPerc(2, 0.25f);

	// row heights
	if(mSearchType == ALWAYS_ACCEPT_FIRST_RESULT) // show name
		mGrid.setRowHeightPerc(0, (mResultName->getFont()->getHeight() * 1.6f) / mGrid.getSize().y()); // result name
	else
		mGrid.setRowHeightPerc(0, 0.0825f); // hide name but do padding

	if(mSearchType == ALWAYS_ACCEPT_FIRST_RESULT)
	{
		mGrid.setRowHeightPerc(2, 0.2f);
	}else{
		mGrid.setRowHeightPerc(1, 0.505f);
	}

	const float boxartCellScale = 0.9f;

	// limit thumbnail size using setMaxHeight - we do this instead of letting mGrid call setSize because it maintains the aspect ratio
	// we also pad a little so it doesn't rub up against the metadata labels
	mResultThumbnail->setMaxSize(mGrid.getColWidth(1) * boxartCellScale, mGrid.getRowHeight(1));

	// metadata
	resizeMetadata();

	if(mSearchType != ALWAYS_ACCEPT_FIRST_RESULT)
		mDescContainer->setSize(mGrid.getColWidth(1)*boxartCellScale + mGrid.getColWidth(2), mResultDesc->getFont()->getHeight() * 3);
	else
		mDescContainer->setSize(mGrid.getColWidth(3)*boxartCellScale, mResultDesc->getFont()->getHeight() * 8);

	mResultDesc->setSize(mDescContainer->getSize().x(), 0); // make desc text wrap at edge of container

	mGrid.onSizeChanged();

	mBusyAnim.setSize(mSize);
}

void ScraperSearchComponent::resizeMetadata()
{
	mMD_Grid->setSize(mGrid.getColWidth(2), mGrid.getRowHeight(1));
	if(mMD_Grid->getSize().y() > mMD_Pairs.size())
	{
		const int fontHeight = (int)(mMD_Grid->getSize().y() / mMD_Pairs.size() * 0.8f);
		auto fontLbl = Font::get(fontHeight, FONT_PATH_REGULAR);
		auto fontComp = Font::get(fontHeight, FONT_PATH_LIGHT);

		// update label fonts
		float maxLblWidth = 0;
		for(auto it = mMD_Pairs.cbegin(); it != mMD_Pairs.cend(); it++)
		{
			it->first->setFont(fontLbl);
			it->first->setSize(0, 0);
			if(it->first->getSize().x() > maxLblWidth)
				maxLblWidth = it->first->getSize().x() + 6;
		}

		for(unsigned int i = 0; i < mMD_Pairs.size(); i++)
		{
			mMD_Grid->setRowHeightPerc(i*2, (fontLbl->getLetterHeight() + 2) / mMD_Grid->getSize().y());
		}

		// update component fonts
		mMD_ReleaseDate->setFont(fontComp);
		mMD_Developer->setFont(fontComp);
		mMD_Publisher->setFont(fontComp);
		mMD_Genre->setFont(fontComp);
		mMD_Players->setFont(fontComp);

		mMD_Grid->setColWidthPerc(0, maxLblWidth / mMD_Grid->getSize().x());

		// rating is manually sized
		mMD_Rating->setSize(mMD_Grid->getColWidth(1), fontLbl->getHeight() * 0.65f);
		mMD_Grid->onSizeChanged();

		// make result font follow label font
		mResultDesc->setFont(Font::get(fontHeight, FONT_PATH_REGULAR));
	}
}

void ScraperSearchComponent::updateViewStyle()
{
	// unlink description and result list and result name
	mGrid.removeEntry(mResultName);
	mGrid.removeEntry(mResultDesc);
	mGrid.removeEntry(mResultList);

	// add them back depending on search type
	if(mSearchType == ALWAYS_ACCEPT_FIRST_RESULT)
	{
		// show name
		mGrid.setEntry(mResultName, Vector2i(1, 0), false, true, Vector2i(2, 1), GridFlags::BORDER_TOP);

		// need a border on the bottom left
		mGrid.setEntry(std::make_shared<GuiComponent>(mWindow), Vector2i(0, 2), false, false, Vector2i(3, 1), GridFlags::BORDER_BOTTOM);

		// show description on the right
		mGrid.setEntry(mDescContainer, Vector2i(3, 0), false, false, Vector2i(1, 3), GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);
		mResultDesc->setSize(mDescContainer->getSize().x(), 0); // make desc text wrap at edge of container
	}else{
		// fake row where name would be
		mGrid.setEntry(std::make_shared<GuiComponent>(mWindow), Vector2i(1, 0), false, true, Vector2i(2, 1), GridFlags::BORDER_TOP);

		// show result list on the right
		mGrid.setEntry(mResultList, Vector2i(3, 0), true, true, Vector2i(1, 3), GridFlags::BORDER_LEFT | GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);

		// show description under image/info
		mGrid.setEntry(mDescContainer, Vector2i(1, 2), false, false, Vector2i(2, 1), GridFlags::BORDER_BOTTOM);
		mResultDesc->setSize(mDescContainer->getSize().x(), 0); // make desc text wrap at edge of container
	}
}

void ScraperSearchComponent::search(const ScraperSearchParams& params)
{
	mBlockAccept = true;

	mResultList->clear();
	mScraperResults.clear();
	mThumbnailReq.reset();
	mMDResolveHandle.reset();
	updateInfoPane();

	mLastSearch = params;
	mSearchHandle = startScraperSearch(params);
}

void ScraperSearchComponent::stop()
{
	mThumbnailReq.reset();
	mSearchHandle.reset();
	mMDResolveHandle.reset();
	mBlockAccept = false;
}

void ScraperSearchComponent::onSearchDone(const std::vector<ScraperSearchResult>& results)
{
	mResultList->clear();

	mScraperResults = results;

	auto font = Font::get(FONT_SIZE_MEDIUM);
	unsigned int color = 0x777777FF;
	if(results.empty())
	{
		// Check if the scraper used is still valid
		if (!isValidConfiguredScraper())
		{
			mWindow->pushGui(new GuiMsgBox(mWindow, Utils::String::toUpper("Configured scraper is no longer available.\nPlease change the scraping source in the settings."),
				"FINISH", mSkipCallback));
		}
		else
		{
			ComponentListRow row;
			row.addElement(std::make_shared<TextComponent>(mWindow, "NO GAMES FOUND - SKIP", font, color), true);

			if(mSkipCallback)
				row.makeAcceptInputHandler(mSkipCallback);

			mResultList->addRow(row);
			mGrid.resetCursor();
		}
	}else{
		ComponentListRow row;
		for(size_t i = 0; i < results.size(); i++)
		{
			row.elements.clear();
			row.addElement(std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(results.at(i).mdl.get("name")), font, color), true);
			row.makeAcceptInputHandler([this, i] { returnResult(mScraperResults.at(i)); });
			mResultList->addRow(row);
		}
		mGrid.resetCursor();
	}

	mBlockAccept = false;
	updateInfoPane();

	if(mSearchType == ALWAYS_ACCEPT_FIRST_RESULT)
	{
		if(mScraperResults.size() == 0)
			mSkipCallback();
		else
			returnResult(mScraperResults.front());
	}else if(mSearchType == ALWAYS_ACCEPT_MATCHING_CRC)
	{
		// TODO
	}
}

void ScraperSearchComponent::onSearchError(const std::string& error)
{
	LOG(LogInfo) << "ScraperSearchComponent search error: " << error;
	mWindow->pushGui(new GuiMsgBox(mWindow, Utils::String::toUpper(error),
		"RETRY", std::bind(&ScraperSearchComponent::search, this, mLastSearch),
		"SKIP", mSkipCallback,
		"CANCEL", mCancelCallback));
}

int ScraperSearchComponent::getSelectedIndex()
{
	if(!mScraperResults.size() || mGrid.getSelectedComponent() != mResultList)
		return -1;

	return mResultList->getCursorId();
}

void ScraperSearchComponent::updateInfoPane()
{
	int i = getSelectedIndex();
	if(mSearchType == ALWAYS_ACCEPT_FIRST_RESULT && mScraperResults.size())
	{
		i = 0;
	}

	if(i != -1 && (int)mScraperResults.size() > i)
	{
		ScraperSearchResult& res = mScraperResults.at(i);
		mResultName->setText(Utils::String::toUpper(res.mdl.get("name")));
		mResultDesc->setText(Utils::String::toUpper(res.mdl.get("desc")));
		mDescContainer->reset();

		mResultThumbnail->setImage("");
		const std::string& thumb = res.thumbnailUrl.empty() ? res.imageUrl : res.thumbnailUrl;
		if(!thumb.empty())
		{
			mThumbnailReq = std::unique_ptr<HttpReq>(new HttpReq(thumb));
		}else{
			mThumbnailReq.reset();
		}

		// metadata
		mMD_Rating->setValue(Utils::String::toUpper(res.mdl.get("rating")));
		mMD_ReleaseDate->setValue(Utils::String::toUpper(res.mdl.get("releasedate")));
		mMD_Developer->setText(Utils::String::toUpper(res.mdl.get("developer")));
		mMD_Publisher->setText(Utils::String::toUpper(res.mdl.get("publisher")));
		mMD_Genre->setText(Utils::String::toUpper(res.mdl.get("genre")));
		mMD_Players->setText(Utils::String::toUpper(res.mdl.get("players")));
		mGrid.onSizeChanged();
	}else{
		mResultName->setText("");
		mResultDesc->setText("");
		mResultThumbnail->setImage("");

		// metadata
		mMD_Rating->setValue("");
		mMD_ReleaseDate->setValue("");
		mMD_Developer->setText("");
		mMD_Publisher->setText("");
		mMD_Genre->setText("");
		mMD_Players->setText("");
	}
}

bool ScraperSearchComponent::input(InputConfig* config, Input input)
{
	if(config->isMappedTo("a", input) && input.value != 0)
	{
		if(mBlockAccept)
			return true;
	}

	return GuiComponent::input(config, input);
}

void ScraperSearchComponent::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();

	renderChildren(trans);

	if(mBlockAccept)
	{
		Renderer::setMatrix(trans);
		Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), 0x00000011, 0x00000011);

		mBusyAnim.render(trans);
	}
}

void ScraperSearchComponent::returnResult(ScraperSearchResult result)
{
	mBlockAccept = true;

	// resolve metadata image before returning
	if(!result.imageUrl.empty())
	{
		mMDResolveHandle = resolveMetaDataAssets(result, mLastSearch);
		return;
	}

	mAcceptCallback(result);
}

void ScraperSearchComponent::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	if(mBlockAccept)
	{
		mBusyAnim.update(deltaTime);
	}

	if(mThumbnailReq && mThumbnailReq->status() != HttpReq::REQ_IN_PROGRESS)
	{
		updateThumbnail();
	}

	if(mSearchHandle && mSearchHandle->status() != ASYNC_IN_PROGRESS)
	{
		auto status = mSearchHandle->status();
		auto results = mSearchHandle->getResults();
		auto statusString = mSearchHandle->getStatusString();

		// we reset here because onSearchDone in auto mode can call mSkipCallback() which can call
		// another search() which will set our mSearchHandle to something important
		mSearchHandle.reset();

		if(status == ASYNC_DONE)
		{
			onSearchDone(results);
		}else if(status == ASYNC_ERROR)
		{
			onSearchError(statusString);
		}
	}

	if(mMDResolveHandle && mMDResolveHandle->status() != ASYNC_IN_PROGRESS)
	{
		if(mMDResolveHandle->status() == ASYNC_DONE)
		{
			ScraperSearchResult result = mMDResolveHandle->getResult();
			mMDResolveHandle.reset();

			// this might end in us being deleted, depending on mAcceptCallback - so make sure this is the last thing we do in update()
			returnResult(result);
		}else if(mMDResolveHandle->status() == ASYNC_ERROR)
		{
			onSearchError(mMDResolveHandle->getStatusString());
			mMDResolveHandle.reset();
		}
	}
}

void ScraperSearchComponent::updateThumbnail()
{
	if(mThumbnailReq && mThumbnailReq->status() == HttpReq::REQ_SUCCESS)
	{
		std::string content = mThumbnailReq->getContent();
		mResultThumbnail->setImage(content.data(), content.length());
		mGrid.onSizeChanged(); // a hack to fix the thumbnail position since its size changed
	}else{
		LOG(LogWarning) << "thumbnail req failed: " << mThumbnailReq->getErrorMsg();
		mResultThumbnail->setImage("");
	}

	mThumbnailReq.reset();
}

void ScraperSearchComponent::openInputScreen(ScraperSearchParams& params)
{
	auto searchForFunc = [&](const std::string& name)
	{
		params.nameOverride = name;
		search(params);
	};

	stop();
	mWindow->pushGui(new GuiTextEditPopup(mWindow, "SEARCH FOR",
		// initial value is last search if there was one, otherwise the clean path name
		params.nameOverride.empty() ? params.game->getCleanName() : params.nameOverride,
		searchForFunc, false, "SEARCH"));
}

std::vector<HelpPrompt> ScraperSearchComponent::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
	if(getSelectedIndex() != -1)
		prompts.push_back(HelpPrompt("a", "accept result"));

	return prompts;
}

void ScraperSearchComponent::onFocusGained()
{
	mGrid.onFocusGained();
}

void ScraperSearchComponent::onFocusLost()
{
	mGrid.onFocusLost();
}
