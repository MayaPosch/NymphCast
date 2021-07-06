#include "Window.h"

#include "components/HelpComponent.h"
#include "components/ImageComponent.h"
#include "resources/Font.h"
#include "resources/TextureResource.h"
#include "InputManager.h"
#include "Log.h"
#include "Scripting.h"
#include <algorithm>
#include <iomanip>

#ifdef WIN32
#include <SDL_events.h>
#endif

Window::Window() : mNormalizeNextUpdate(false), mFrameTimeElapsed(0), mFrameCountElapsed(0), mAverageDeltaTime(10),
	mAllowSleep(true), mSleeping(false), mTimeSinceLastInput(0), mScreenSaver(NULL), mRenderScreenSaver(false), mInfoPopup(NULL)
{
	mHelp = new HelpComponent(this);
	mBackgroundOverlay = new ImageComponent(this);
}

Window::~Window()
{
	delete mBackgroundOverlay;

	// delete all our GUIs
	while(peekGui())
		delete peekGui();

	delete mHelp;
}

void Window::pushGui(GuiComponent* gui)
{
	if (mGuiStack.size() > 0)
	{
		auto& top = mGuiStack.back();
		top->topWindow(false);
	}
	mGuiStack.push_back(gui);
	gui->updateHelpPrompts();
}

void Window::removeGui(GuiComponent* gui)
{
	for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
	{
		if(*i == gui)
		{
			i = mGuiStack.erase(i);

			if(i == mGuiStack.cend() && mGuiStack.size()) // we just popped the stack and the stack is not empty
			{
				mGuiStack.back()->updateHelpPrompts();
				mGuiStack.back()->topWindow(true);
			}

			return;
		}
	}
}

GuiComponent* Window::peekGui()
{
	if(mGuiStack.size() == 0)
		return NULL;

	return mGuiStack.back();
}

bool Window::init()
{
	if(!Renderer::init())
	{
		LOG(LogError) << "Renderer failed to initialize!";
		return false;
	}

	InputManager::getInstance()->init();

	ResourceManager::getInstance()->reloadAll();

	//keep a reference to the default fonts, so they don't keep getting destroyed/recreated
	if(mDefaultFonts.empty())
	{
		mDefaultFonts.push_back(Font::get(FONT_SIZE_SMALL));
		mDefaultFonts.push_back(Font::get(FONT_SIZE_MEDIUM));
		mDefaultFonts.push_back(Font::get(FONT_SIZE_LARGE));
	}

	mBackgroundOverlay->setImage(":/scroll_gradient.png");
	mBackgroundOverlay->setResize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	// update our help because font sizes probably changed
	if(peekGui())
		peekGui()->updateHelpPrompts();

	return true;
}

void Window::deinit()
{
	// Hide all GUI elements on uninitialisation - this disable
	for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
	{
		(*i)->onHide();
	}
	InputManager::getInstance()->deinit();
	ResourceManager::getInstance()->unloadAll();
	Renderer::deinit();
}

void Window::textInput(const char* text)
{
	if(peekGui())
		peekGui()->textInput(text);
}

void Window::input(InputConfig* config, Input input)
{
	if (mScreenSaver && mScreenSaver->isScreenSaverActive() && Settings::getInstance()->getBool("ScreenSaverControls")
		&& inputDuringScreensaver(config, input))
	{
		return;
	}

	if (mSleeping)
	{
		mSleeping = false;
		mTimeSinceLastInput = 0;
		onWake();
		return;
	}

	mTimeSinceLastInput = 0;
	if (cancelScreenSaver())
		return;

	bool dbg_keyboard_key_press = Settings::getInstance()->getBool("Debug") && config->getDeviceId() == DEVICE_KEYBOARD && input.value;
	if (dbg_keyboard_key_press && input.id == SDLK_g && SDL_GetModState() & KMOD_LCTRL)
	{
		// toggle debug grid with Ctrl-G
		Settings::getInstance()->setBool("DebugGrid", !Settings::getInstance()->getBool("DebugGrid"));
	}
	else if (dbg_keyboard_key_press && input.id == SDLK_t && SDL_GetModState() & KMOD_LCTRL)
	{
		// toggle TextComponent debug view with Ctrl-T
		Settings::getInstance()->setBool("DebugText", !Settings::getInstance()->getBool("DebugText"));
	}
	else if (dbg_keyboard_key_press && input.id == SDLK_i && SDL_GetModState() & KMOD_LCTRL)
	{
		// toggle TextComponent debug view with Ctrl-I
		Settings::getInstance()->setBool("DebugImage", !Settings::getInstance()->getBool("DebugImage"));
	}
	else if (peekGui())
	{
		this->peekGui()->input(config, input); // this is where the majority of inputs will be consumed: the GuiComponent Stack
	}
}

bool Window::inputDuringScreensaver(InputConfig* config, Input input)
{
	bool input_consumed = false;
	std::string screensaver_type = Settings::getInstance()->getString("ScreenSaverBehavior");

	if (screensaver_type == "random video" || screensaver_type == "slideshow")
	{
		bool is_select_input = config->isMappedLike("right", input) || config->isMappedTo("select", input);
		bool is_start_input = config->isMappedTo("start", input);

		if (is_select_input)
		{
			if (input.value) {
				mScreenSaver->nextMediaItem();
				// user input resets sleep time counter
				mTimeSinceLastInput = 0;
			}
			input_consumed = true;
		}
		else if (is_start_input)
		{
			bool slideshow_custom_images = Settings::getInstance()->getBool("SlideshowScreenSaverCustomImageSource");
			if (screensaver_type == "random video" || !slideshow_custom_images)
			{
				mScreenSaver->launchGame();
			}
		}
	}
	return input_consumed;
}

void Window::update(int deltaTime)
{
	if(mNormalizeNextUpdate)
	{
		mNormalizeNextUpdate = false;
		if(deltaTime > mAverageDeltaTime)
			deltaTime = mAverageDeltaTime;
	}

	mFrameTimeElapsed += deltaTime;
	mFrameCountElapsed++;
	if(mFrameTimeElapsed > 500)
	{
		mAverageDeltaTime = mFrameTimeElapsed / mFrameCountElapsed;

		if(Settings::getInstance()->getBool("DrawFramerate"))
		{
			std::stringstream ss;

			// fps
			ss << std::fixed << std::setprecision(1) << (1000.0f * (float)mFrameCountElapsed / (float)mFrameTimeElapsed) << "fps, ";
			ss << std::fixed << std::setprecision(2) << ((float)mFrameTimeElapsed / (float)mFrameCountElapsed) << "ms";

			// vram
			float textureVramUsageMb = TextureResource::getTotalMemUsage() / 1000.0f / 1000.0f;
			float textureTotalUsageMb = TextureResource::getTotalTextureSize() / 1000.0f / 1000.0f;
			float fontVramUsageMb = Font::getTotalMemUsage() / 1000.0f / 1000.0f;

			ss << "\nFont VRAM: " << fontVramUsageMb << " Tex VRAM: " << textureVramUsageMb <<
				  " Tex Max: " << textureTotalUsageMb;
			mFrameDataText = std::unique_ptr<TextCache>(mDefaultFonts.at(1)->buildTextCache(ss.str(), 50.f, 50.f, 0xFF00FFFF));
		}

		mFrameTimeElapsed = 0;
		mFrameCountElapsed = 0;
	}

	mTimeSinceLastInput += deltaTime;

	if(peekGui())
		peekGui()->update(deltaTime);

	// Update the screensaver
	if (mScreenSaver)
		mScreenSaver->update(deltaTime);
}

void Window::render()
{
	Transform4x4f transform = Transform4x4f::Identity();

	mRenderedHelpPrompts = false;

	// draw only bottom and top of GuiStack (if they are different)
	if(mGuiStack.size())
	{
		auto& bottom = mGuiStack.front();
		auto& top = mGuiStack.back();

		bottom->render(transform);
		if(bottom != top)
		{
			mBackgroundOverlay->render(transform);
			top->render(transform);
		}
	}

	if(!mRenderedHelpPrompts)
		mHelp->render(transform);

	if(Settings::getInstance()->getBool("DrawFramerate") && mFrameDataText)
	{
		Renderer::setMatrix(Transform4x4f::Identity());
		mDefaultFonts.at(1)->renderTextCache(mFrameDataText.get());
	}

	unsigned int screensaverTime = (unsigned int)Settings::getInstance()->getInt("ScreenSaverTime");
	if(mTimeSinceLastInput >= screensaverTime && screensaverTime != 0)
		startScreenSaver();

	// Always call the screensaver render function regardless of whether the screensaver is active
	// or not because it may perform a fade on transition
	renderScreenSaver();

	if(!mRenderScreenSaver && mInfoPopup)
	{
		mInfoPopup->render(transform);
	}

	if(mTimeSinceLastInput >= screensaverTime && screensaverTime != 0)
	{
		unsigned int systemSleepTime = (unsigned int)Settings::getInstance()->getInt("SystemSleepTime");
		if(!isProcessing() && mAllowSleep && systemSleepTime != 0 && mTimeSinceLastInput >= systemSleepTime) {
			mSleeping = true;
			onSleep();
		}
	}
}

void Window::normalizeNextUpdate()
{
	mNormalizeNextUpdate = true;
}

bool Window::getAllowSleep()
{
	return mAllowSleep;
}

void Window::setAllowSleep(bool sleep)
{
	mAllowSleep = sleep;
}

void Window::renderLoadingScreen(std::string text, float percent, unsigned char opacity)
{
	Transform4x4f trans = Transform4x4f::Identity();
	Renderer::setMatrix(trans);
	Renderer::drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0x000000FF, 0x000000FF);

	if (percent >= 0)
	{
		float baseHeight = 0.04f;

		float w = Renderer::getScreenWidth() / 2;
		float h = Renderer::getScreenHeight() * baseHeight;

		float x = Renderer::getScreenWidth() / 2 - w / 2;
		float y = Renderer::getScreenHeight() - (Renderer::getScreenHeight() * 3 * baseHeight);

		Renderer::drawRect(x, y, w, h, 0x25252500 | opacity, 0x25252500 | opacity);
		Renderer::drawRect(x, y, (w*percent), h, 0x006C9E00 | opacity, 0x006C9E00 | opacity); // 0xFFFFFFFF
	}

	ImageComponent splash(this, true);
	splash.setResize(Renderer::getScreenWidth() * 0.6f, 0.0f);
	splash.setImage(":/splash.svg");
	splash.setPosition((Renderer::getScreenWidth() - splash.getSize().x()) / 2, (Renderer::getScreenHeight() - splash.getSize().y()) / 2 * 0.6f);
	splash.render(trans);

	auto& font = mDefaultFonts.at(1);
	TextCache* cache = font->buildTextCache(text, 0, 0, 0x656565FF);

	float x = Math::round((Renderer::getScreenWidth() - cache->metrics.size.x()) / 2.0f);
	float y = Math::round(Renderer::getScreenHeight() * 0.78f);
	trans = trans.translate(Vector3f(x, y, 0.0f));
	Renderer::setMatrix(trans);
	font->renderTextCache(cache);
	delete cache;

	Renderer::swapBuffers();

#ifdef WIN32
	// Avoid Window Freezing on Windows
	SDL_Event event;
	while (SDL_PollEvent(&event));
#endif
}

void Window::renderHelpPromptsEarly()
{
	mHelp->render(Transform4x4f::Identity());
	mRenderedHelpPrompts = true;
}

void Window::setHelpPrompts(const std::vector<HelpPrompt>& prompts, const HelpStyle& style)
{
	mHelp->clearPrompts();
	mHelp->setStyle(style);

	std::vector<HelpPrompt> addPrompts;

	std::map<std::string, bool> inputSeenMap;
	std::map<std::string, int> mappedToSeenMap;
	for(auto it = prompts.cbegin(); it != prompts.cend(); it++)
	{
		// only add it if the same icon hasn't already been added
		if(inputSeenMap.emplace(it->first, true).second)
		{
			// this symbol hasn't been seen yet, what about the action name?
			auto mappedTo = mappedToSeenMap.find(it->second);
			if(mappedTo != mappedToSeenMap.cend())
			{
				// yes, it has!

				// can we combine? (dpad only)
				if((it->first == "up/down" && addPrompts.at(mappedTo->second).first != "left/right") ||
					(it->first == "left/right" && addPrompts.at(mappedTo->second).first != "up/down"))
				{
					// yes!
					addPrompts.at(mappedTo->second).first = "up/down/left/right";
					// don't need to add this to addPrompts since we just merged
				}else{
					// no, we can't combine!
					addPrompts.push_back(*it);
				}
			}else{
				// no, it hasn't!
				mappedToSeenMap.emplace(it->second, (int)addPrompts.size());
				addPrompts.push_back(*it);
			}
		}
	}

	// sort prompts so it goes [dpad_all] [dpad_u/d] [dpad_l/r] [a/b/x/y/l/r] [start/select]
	std::sort(addPrompts.begin(), addPrompts.end(), [](const HelpPrompt& a, const HelpPrompt& b) -> bool {

		static const char* map[] = {
			"up/down/left/right",
			"up/down",
			"left/right",
			"a", "b", "x", "y", "l", "r",
			"start", "select",
			NULL
		};

		int i = 0;
		int aVal = 0;
		int bVal = 0;
		while(map[i] != NULL)
		{
			if(a.first == map[i])
				aVal = i;
			if(b.first == map[i])
				bVal = i;
			i++;
		}

		return aVal > bVal;
	});

	mHelp->setPrompts(addPrompts);
}


void Window::onSleep()
{
	if (Settings::getInstance()->getBool("Windowed")) {
		LOG(LogInfo) << "running windowed. No further onSleep() processing.";
		return;
	}

	int gotErrors = Scripting::fireEvent("sleep");

	if (gotErrors == 0 && mScreenSaver && mRenderScreenSaver)
	{
		mScreenSaver->stopScreenSaver();
		mRenderScreenSaver = false;
		mScreenSaver->resetCounts();
	}
}

void Window::onWake()
{
	Scripting::fireEvent("wake");
}

bool Window::isProcessing()
{
	return count_if(mGuiStack.cbegin(), mGuiStack.cend(), [](GuiComponent* c) { return c->isProcessing(); }) > 0;
}

void Window::startScreenSaver()
{
	if (mScreenSaver && !mRenderScreenSaver)
	{
		// Tell the GUI components the screensaver is starting
		for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
			(*i)->onScreenSaverActivate();

		mScreenSaver->startScreenSaver();
		mRenderScreenSaver = true;
		Scripting::fireEvent("screensaver-start");
	}
}

bool Window::cancelScreenSaver()
{
	if (mScreenSaver && mRenderScreenSaver)
	{
		mScreenSaver->stopScreenSaver();
		mRenderScreenSaver = false;
		mScreenSaver->resetCounts();
		Scripting::fireEvent("screensaver-stop");

		// Tell the GUI components the screensaver has stopped
		for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
			(*i)->onScreenSaverDeactivate();

		return true;
	}

	return false;
}

void Window::renderScreenSaver()
{
	if (mScreenSaver)
		mScreenSaver->renderScreenSaver();
}
