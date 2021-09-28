#include "views/ViewController.h"

#include "animations/Animation.h"
#include "animations/LambdaAnimation.h"
#include "animations/LaunchAnimation.h"
#include "animations/MoveCameraAnimation.h"
#include "guis/GuiMenu.h"
#include "views/gamelist/DetailedGameListView.h"
#include "views/gamelist/IGameListView.h"
#include "views/gamelist/GridGameListView.h"
#include "views/gamelist/VideoGameListView.h"
#include "views/SystemView.h"
#include "views/UIModeController.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include "Window.h"

ViewController* ViewController::sInstance = NULL;

ViewController* ViewController::get()
{
	assert(sInstance);
	return sInstance;
}

void ViewController::init(Window* window)
{
	assert(!sInstance);
	sInstance = new ViewController(window);
}

ViewController::ViewController(Window* window)
	: GuiComponent(window), mCurrentView(nullptr), mCamera(Transform4x4f::Identity()), mFadeOpacity(0), mLockInput(false)
{
	mState.viewing = NOTHING;
}

ViewController::~ViewController()
{
	assert(sInstance == this);
	sInstance = NULL;
}

void ViewController::goToStart()
{
	// If specific system is requested, go directly to the game list
	auto requestedSystem = Settings::getInstance()->getString("StartupSystem");
	if("" != requestedSystem && "retropie" != requestedSystem)
	{
		for(auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); it++){
			if ((*it)->getName() == requestedSystem)
			{
				goToGameList(*it);
				return;
			}
		}

		// Requested system doesn't exist
		Settings::getInstance()->setString("StartupSystem", "");
	}
	goToSystemView(SystemData::sSystemVector.at(0));
}

void ViewController::ReloadAndGoToStart()
{
	mWindow->renderLoadingScreen("Loading...");
	ViewController::get()->reloadAll();
	ViewController::get()->goToStart();
}

int ViewController::getSystemId(SystemData* system)
{
	std::vector<SystemData*>& sysVec = SystemData::sSystemVector;
	return (int)(std::find(sysVec.cbegin(), sysVec.cend(), system) - sysVec.cbegin());
}

void ViewController::goToSystemView(SystemData* system) {
	// Tell any current view it's about to be hidden
	if (mCurrentView) {
		mCurrentView->onHide();
	}

	mState.viewing = SYSTEM_SELECT;
	mState.system = system;

	std::shared_ptr<SystemView> systemList = getSystemListView();
	systemList->setPosition(getSystemId(system) * (float)Renderer::getScreenWidth(), systemList->getPosition().y());

	systemList->goToSystem(system, false);
	mCurrentView = systemList;
	mCurrentView->onShow();
	PowerSaver::setState(true);

	playViewTransition();
}

void ViewController::goToNextGameList()
{
	assert(mState.viewing == GAME_LIST);
	SystemData* system = getState().getSystem();
	assert(system);
	goToGameList(system->getNext());
}

void ViewController::goToPrevGameList()
{
	assert(mState.viewing == GAME_LIST);
	SystemData* system = getState().getSystem();
	assert(system);
	goToGameList(system->getPrev());
}

void ViewController::goToGameList(SystemData* system) {
	if(mState.viewing == SYSTEM_SELECT) {
		// move system list
		std::shared_ptr<SystemView> sysList = getSystemListView();
		float offX = sysList->getPosition().x();
		int sysId = getSystemId(system);
		sysList->setPosition(sysId * (float)Renderer::getScreenWidth(), sysList->getPosition().y());
		offX = sysList->getPosition().x() - offX;
		mCamera.translation().x() -= offX;
	}

	mState.viewing = GAME_LIST;
	mState.system = system;

	if (mCurrentView) {
		mCurrentView->onHide();
	}
	
	mCurrentView = getGameListView(system);
	if (mCurrentView) {
		mCurrentView->onShow();
	}
	
	playViewTransition();
}

void ViewController::playViewTransition()
{
	Vector3f target(Vector3f::Zero());
	if(mCurrentView)
		target = mCurrentView->getPosition();

	// no need to animate, we're not going anywhere (probably goToNextGamelist() or goToPrevGamelist() when there's only 1 system)
	if(target == -mCamera.translation() && !isAnimationPlaying(0))
		return;

	std::string transition_style = Settings::getInstance()->getString("TransitionStyle");
	if(transition_style == "fade")
	{
		// fade
		// stop whatever's currently playing, leaving mFadeOpacity wherever it is
		cancelAnimation(0);

		auto fadeFunc = [this](float t) {
			mFadeOpacity = Math::lerp(0, 1, t);
		};

		const static int FADE_DURATION = 240; // fade in/out time
		const static int FADE_WAIT = 320; // time to wait between in/out
		setAnimation(new LambdaAnimation(fadeFunc, FADE_DURATION), 0, [this, fadeFunc, target] {
			this->mCamera.translation() = -target;
			updateHelpPrompts();
			setAnimation(new LambdaAnimation(fadeFunc, FADE_DURATION), FADE_WAIT, nullptr, true);
		});

		// fast-forward animation if we're partway faded
		if(target == -mCamera.translation())
		{
			// not changing screens, so cancel the first half entirely
			advanceAnimation(0, FADE_DURATION);
			advanceAnimation(0, FADE_WAIT);
			advanceAnimation(0, FADE_DURATION - (int)(mFadeOpacity * FADE_DURATION));
		}else{
			advanceAnimation(0, (int)(mFadeOpacity * FADE_DURATION));
		}
	} else if (transition_style == "slide"){
		// slide or simple slide
		setAnimation(new MoveCameraAnimation(mCamera, target));
		updateHelpPrompts(); // update help prompts immediately
	} else {
		// instant
		setAnimation(new LambdaAnimation(
			[this, target](float /*t*/)
		{
			this->mCamera.translation() = -target;
		}, 1));
		updateHelpPrompts();
	}
}

void ViewController::onFileChanged(FileData* file, FileChangeType change)
{
	auto it = mGameListViews.find(file->getSystem());
	if(it != mGameListViews.cend())
		it->second->onFileChanged(file, change);
}

void ViewController::launch(FileData* game, Vector3f center) {
	FileType type = game->getType();
	if (type == GAME) {
		LOG(LogInfo) << "Launching game...";
	}
	else if (type == MEDIA) {
		LOG(LogInfo) << "Launching media file...";
	}
	else {
		LOG(LogError) << "tried to launch something that isn't a game or media file";
		return;
	}

	// Hide the current view
	if (mCurrentView) {
		mCurrentView->onHide();
	}

	Transform4x4f origCamera = mCamera;
	origCamera.translation() = -mCurrentView->getPosition();

	center += mCurrentView->getPosition();
	stopAnimation(1); // make sure the fade in isn't still playing
	mWindow->stopInfoPopup(); // make sure we disable any existing info popup
	mLockInput = true;

	std::string transition_style = Settings::getInstance()->getString("TransitionStyle");
	if (transition_style == "fade") {
		// fade out, launch game, fade back in
		auto fadeFunc = [this](float t) {
			mFadeOpacity = Math::lerp(0.0f, 1.0f, t);
		};
		
		setAnimation(new LambdaAnimation(fadeFunc, 800), 0, [this, game, fadeFunc]
		{
			game->launchGame(mWindow);
			setAnimation(new LambdaAnimation(fadeFunc, 800), 0, [this] { mLockInput = false; }, true);
			this->onFileChanged(game, FILE_METADATA_CHANGED);
			if (mCurrentView)
				mCurrentView->onShow();
		});
	} 
	else if (transition_style == "slide") {
		// move camera to zoom in on center + fade out, launch game, come back in
		setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 1500), 0, [this, origCamera, center, game]
		{
			game->launchGame(mWindow);
			mCamera = origCamera;
			setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 600), 0, [this] { mLockInput = false; }, true);
			this->onFileChanged(game, FILE_METADATA_CHANGED);
			if (mCurrentView)
				mCurrentView->onShow();
		});
	} 
	else { // instant
		setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 10), 0, [this, origCamera, center, game]
		{
			game->launchGame(mWindow);
			mCamera = origCamera;
			setAnimation(new LaunchAnimation(mCamera, mFadeOpacity, center, 10), 0, [this] { mLockInput = false; }, true);
			this->onFileChanged(game, FILE_METADATA_CHANGED);
			if (mCurrentView)
				mCurrentView->onShow();
		});
	}
}

void ViewController::removeGameListView(SystemData* system)
{
	//if we already made one, return that one
	auto exists = mGameListViews.find(system);
	if(exists != mGameListViews.cend())
	{
		exists->second.reset();
		mGameListViews.erase(system);
	}
}

std::shared_ptr<IGameListView> ViewController::getGameListView(SystemData* system) {
	//if we already made one, return that one
	auto exists = mGameListViews.find(system);
	if (exists != mGameListViews.cend()) {
		return exists->second;
	}

	system->getIndex()->setUIModeFilters();
	//if we didn't, make it, remember it, and return it
	std::shared_ptr<IGameListView> view;

	bool themeHasVideoView = system->getTheme()->hasView("video");

	//decide type
	GameListViewType selectedViewType = AUTOMATIC;

	std::string viewPreference = Settings::getInstance()->getString("GamelistViewStyle");
	if (viewPreference.compare("basic") == 0)
		selectedViewType = BASIC;
	if (viewPreference.compare("detailed") == 0)
		selectedViewType = DETAILED;
	if (viewPreference.compare("grid") == 0)
		selectedViewType = GRID;
	if (viewPreference.compare("video") == 0)
		selectedViewType = VIDEO;

	if (selectedViewType == AUTOMATIC) {
		std::vector<FileData*> files = system->getRootFolder()->getFilesRecursive(GAME | FOLDER);
		for (auto it = files.cbegin(); it != files.cend(); it++) {
			if (themeHasVideoView && !(*it)->getVideoPath().empty()) {
				selectedViewType = VIDEO;
				break;
			}
			else if (!(*it)->getThumbnailPath().empty()) {
				selectedViewType = DETAILED;
				// Don't break out in case any subsequent files have video
			}
		}
	}

	// Create the view
	switch (selectedViewType) {
		case VIDEO:
			view = std::shared_ptr<IGameListView>(new VideoGameListView(mWindow, system->getRootFolder()));
			break;
		case DETAILED:
			view = std::shared_ptr<IGameListView>(new DetailedGameListView(mWindow, system->getRootFolder()));
			break;
		case GRID:
			view = std::shared_ptr<IGameListView>(new GridGameListView(mWindow, system->getRootFolder()));
			break;
		case BASIC:
		default:
			view = std::shared_ptr<IGameListView>(new BasicGameListView(mWindow, system->getRootFolder()));
			break;
	}

	view->setTheme(system->getTheme());

	std::vector<SystemData*>& sysVec = SystemData::sSystemVector;
	int id = (int)(std::find(sysVec.cbegin(), sysVec.cend(), system) - sysVec.cbegin());
	view->setPosition(id * (float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight() * 2);

	addChild(view.get());

	mGameListViews[system] = view;
	return view;
}

std::shared_ptr<SystemView> ViewController::getSystemListView()
{
	//if we already made one, return that one
	if(mSystemListView)
		return mSystemListView;

	mSystemListView = std::shared_ptr<SystemView>(new SystemView(mWindow));
	addChild(mSystemListView.get());
	mSystemListView->setPosition(0, (float)Renderer::getScreenHeight());
	return mSystemListView;
}


bool ViewController::input(InputConfig* config, Input input)
{
	if(mLockInput)
		return true;

	// open menu
	if(!(UIModeController::getInstance()->isUIModeKid() && Settings::getInstance()->getBool("DisableKidStartMenu")) && config->isMappedTo("start", input) && input.value != 0)
	{
		// open menu
		mWindow->pushGui(new GuiMenu(mWindow));
		return true;
	}

	if(UIModeController::getInstance()->listen(config, input))  // check if UI mode has changed due to passphrase completion
	{
		return true;
	}

	if(mCurrentView)
		return mCurrentView->input(config, input);

	return false;
}

void ViewController::update(int deltaTime)
{
	if(mCurrentView)
	{
		mCurrentView->update(deltaTime);
	}

	updateSelf(deltaTime);
}

void ViewController::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = mCamera * parentTrans;
	Transform4x4f transInverse;
	transInverse.invert(trans);

	// camera position, position + size
	Vector3f viewStart = transInverse.translation();
	Vector3f viewEnd = transInverse * Vector3f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight(), 0);

	// Keep track of UI mode changes.
	UIModeController::getInstance()->monitorUIMode();

	// draw systemview
	getSystemListView()->render(trans);

	// draw gamelists
	for(auto it = mGameListViews.cbegin(); it != mGameListViews.cend(); it++)
	{
		// clipping
		Vector3f guiStart = it->second->getPosition();
		Vector3f guiEnd = it->second->getPosition() + Vector3f(it->second->getSize().x(), it->second->getSize().y(), 0);

		if(guiEnd.x() >= viewStart.x() && guiEnd.y() >= viewStart.y() &&
			guiStart.x() <= viewEnd.x() && guiStart.y() <= viewEnd.y())
			it->second->render(trans);
	}

	if(mWindow->peekGui() == this)
		mWindow->renderHelpPromptsEarly();

	// fade out
	if(mFadeOpacity)
	{
		unsigned int fadeColor = 0x00000000 | (unsigned char)(mFadeOpacity * 255);
		Renderer::setMatrix(parentTrans);
		Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(), fadeColor, fadeColor);
	}
}

void ViewController::preload()
{
	int i = 1;
	int max = SystemData::sSystemVector.size() + 1;

	bool splash = Settings::getInstance()->getBool("SplashScreen") && Settings::getInstance()->getBool("SplashScreenProgress");
	if (splash) {
		mWindow->renderLoadingScreen("Preloading UI", (float)i / (float)max);
	}

	for(auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); it++) {
		if (splash) {
			i++;
			mWindow->renderLoadingScreen("Preloading UI", (float)i / (float)max);
		}

		(*it)->getIndex()->resetFilters();
		getGameListView(*it);
	}
}

void ViewController::reloadGameListView(IGameListView* view, bool reloadTheme) {
	for(auto it = mGameListViews.cbegin(); it != mGameListViews.cend(); it++) {
		if(it->second.get() == view) {
			bool isCurrent = (mCurrentView == it->second);
			SystemData* system = it->first;
			FileData* cursor = view->getCursor();
			mGameListViews.erase(it);

			if(reloadTheme)
				system->loadTheme();
			system->getIndex()->setUIModeFilters();
			std::shared_ptr<IGameListView> newView = getGameListView(system);

			// to counter having come from a placeholder
			if (!cursor->isPlaceHolder()) {
				newView->setCursor(cursor);
			}
			if(isCurrent)
				mCurrentView = newView;

			break;
		}
	}
	// Redisplay the current view
	if (mCurrentView)
		mCurrentView->onShow();

}

void ViewController::reloadAll()
{
	// clear all gamelistviews
	std::map<SystemData*, FileData*> cursorMap;
	for(auto it = mGameListViews.cbegin(); it != mGameListViews.cend(); it++)
	{
		cursorMap[it->first] = it->second->getCursor();
	}
	mGameListViews.clear();


	// load themes, create gamelistviews and reset filters
	for(auto it = cursorMap.cbegin(); it != cursorMap.cend(); it++)
	{
		it->first->loadTheme();
		it->first->getIndex()->resetFilters();
		getGameListView(it->first)->setCursor(it->second);
	}

	// Rebuild SystemListView
	mSystemListView.reset();
	getSystemListView();

	// update mCurrentView since the pointers changed
	if(mState.viewing == GAME_LIST)
	{
		mCurrentView = getGameListView(mState.getSystem());
	}else if(mState.viewing == SYSTEM_SELECT)
	{
		SystemData* system = mState.getSystem();
		goToSystemView(SystemData::sSystemVector.front());
		mSystemListView->goToSystem(system, false);
		mCurrentView = mSystemListView;
	}else{
		goToSystemView(SystemData::sSystemVector.front());
	}

	updateHelpPrompts();
}

std::vector<HelpPrompt> ViewController::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	if(!mCurrentView)
		return prompts;

	prompts = mCurrentView->getHelpPrompts();
	if(!(UIModeController::getInstance()->isUIModeKid() && Settings::getInstance()->getBool("DisableKidStartMenu")))
		prompts.push_back(HelpPrompt("start", "menu"));

	return prompts;
}

HelpStyle ViewController::getHelpStyle()
{
	if(!mCurrentView)
		return GuiComponent::getHelpStyle();

	return mCurrentView->getHelpStyle();
}
