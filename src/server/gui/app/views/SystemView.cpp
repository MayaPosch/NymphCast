#include "views/SystemView.h"

#include "animations/LambdaAnimation.h"
#include "guis/GuiMsgBox.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include "Window.h"

// buffer values for scrolling velocity (left, stopped, right)
const int logoBuffersLeft[] = { -5, -2, -1 };
const int logoBuffersRight[] = { 1, 2, 5 };

SystemView::SystemView(Window* window) : IList<SystemViewData, SystemData*>(window, LIST_SCROLL_STYLE_SLOW, LIST_ALWAYS_LOOP),
										 mViewNeedsReload(true),
										 mSystemInfo(window, "SYSTEM INFO", Font::get(FONT_SIZE_SMALL), 0x33333300, ALIGN_CENTER)
{
	mCamOffset = 0;
	mExtrasCamOffset = 0;
	mExtrasFadeOpacity = 0.0f;

	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
	populate();
}

void SystemView::populate()
{
	mEntries.clear();

	for(auto it = SystemData::sSystemVector.cbegin(); it != SystemData::sSystemVector.cend(); it++)
	{
		const std::shared_ptr<ThemeData>& theme = (*it)->getTheme();

		if(mViewNeedsReload)
			getViewElements(theme);

		if((*it)->isVisible())
		{
			Entry e;
			e.name = (*it)->getName();
			e.object = *it;

			// make logo
			const ThemeData::ThemeElement* logoElem = theme->getElement("system", "logo", "image");
			if(logoElem)
			{
				std::string path = logoElem->get<std::string>("path");
				std::string defaultPath = logoElem->has("default") ? logoElem->get<std::string>("default") : "";
				if((!path.empty() && ResourceManager::getInstance()->fileExists(path))
				   || (!defaultPath.empty() && ResourceManager::getInstance()->fileExists(defaultPath)))
				{
					ImageComponent* logo = new ImageComponent(mWindow, false, false);
					logo->setMaxSize(mCarousel.logoSize * mCarousel.logoScale);
					logo->applyTheme(theme, "system", "logo", ThemeFlags::PATH | ThemeFlags::COLOR);
					logo->setRotateByTargetSize(true);
					e.data.logo = std::shared_ptr<GuiComponent>(logo);
				}
			}
			if (!e.data.logo)
			{
				// no logo in theme; use text
				TextComponent* text = new TextComponent(mWindow,
					(*it)->getName(),
					Font::get(FONT_SIZE_LARGE),
					0x000000FF,
					ALIGN_CENTER);
				text->setSize(mCarousel.logoSize * mCarousel.logoScale);
				text->applyTheme((*it)->getTheme(), "system", "logoText", ThemeFlags::FONT_PATH | ThemeFlags::FONT_SIZE | ThemeFlags::COLOR | ThemeFlags::FORCE_UPPERCASE | ThemeFlags::LINE_SPACING | ThemeFlags::TEXT);
				e.data.logo = std::shared_ptr<GuiComponent>(text);

				if (mCarousel.type == VERTICAL || mCarousel.type == VERTICAL_WHEEL)
				{
					text->setHorizontalAlignment(mCarousel.logoAlignment);
					text->setVerticalAlignment(ALIGN_CENTER);
				} else {
					text->setHorizontalAlignment(ALIGN_CENTER);
					text->setVerticalAlignment(mCarousel.logoAlignment);
				}
			}

			if (mCarousel.type == VERTICAL || mCarousel.type == VERTICAL_WHEEL)
			{
				if (mCarousel.logoAlignment == ALIGN_LEFT)
					e.data.logo->setOrigin(0, 0.5);
				else if (mCarousel.logoAlignment == ALIGN_RIGHT)
					e.data.logo->setOrigin(1.0, 0.5);
				else
					e.data.logo->setOrigin(0.5, 0.5);
			} else {
				if (mCarousel.logoAlignment == ALIGN_TOP)
					e.data.logo->setOrigin(0.5, 0);
				else if (mCarousel.logoAlignment == ALIGN_BOTTOM)
					e.data.logo->setOrigin(0.5, 1);
				else
					e.data.logo->setOrigin(0.5, 0.5);
			}

			Vector2f denormalized = mCarousel.logoSize * e.data.logo->getOrigin();
			e.data.logo->setPosition(denormalized.x(), denormalized.y(), 0.0);
			// delete any existing extras
			for (auto extra : e.data.backgroundExtras)
				delete extra;
			e.data.backgroundExtras.clear();

			// make background extras
			e.data.backgroundExtras = ThemeData::makeExtras((*it)->getTheme(), "system", mWindow);

			// sort the extras by z-index
			std::stable_sort(e.data.backgroundExtras.begin(), e.data.backgroundExtras.end(),  [](GuiComponent* a, GuiComponent* b) {
				return b->getZIndex() > a->getZIndex();
			});

			this->add(e);
		}
	}
	if (mEntries.size() == 0)
	{
		// Something is wrong, there is not a single system to show, check if UI mode is not full
		if (!UIModeController::getInstance()->isUIModeFull())
		{
			Settings::getInstance()->setString("UIMode", "Full");
			mWindow->pushGui(new GuiMsgBox(mWindow, "The selected UI mode has nothing to show,\n returning to UI mode: FULL", "OK", nullptr));
		}
	}
}

void SystemView::goToSystem(SystemData* system, bool animate)
{
	setCursor(system);

	if(!animate)
		finishAnimation(0);
}

bool SystemView::input(InputConfig* config, Input input)
{
	if(input.value != 0)
	{
		if(config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_r && SDL_GetModState() & KMOD_LCTRL && Settings::getInstance()->getBool("Debug"))
		{
			LOG(LogInfo) << " Reloading all";
			ViewController::get()->reloadAll();
			return true;
		}

		switch (mCarousel.type)
		{
		case VERTICAL:
		case VERTICAL_WHEEL:
			if (config->isMappedLike("up", input))
			{
				listInput(-1);
				return true;
			}
			if (config->isMappedLike("down", input))
			{
				listInput(1);
				return true;
			}
			break;
		case HORIZONTAL:
		case HORIZONTAL_WHEEL:
		default:
			if (config->isMappedLike("left", input))
			{
				listInput(-1);
				return true;
			}
			if (config->isMappedLike("right", input))
			{
				listInput(1);
				return true;
			}
			break;
		}

		if(config->isMappedTo("a", input))
		{
			stopScrolling();
			ViewController::get()->goToGameList(getSelected());
			return true;
		}
		if (config->isMappedTo("x", input))
		{
			// get random system
			// go to system
			setCursor(SystemData::getRandomSystem());
			return true;
		}
	}else{
		if(config->isMappedLike("left", input) ||
			config->isMappedLike("right", input) ||
			config->isMappedLike("up", input) ||
			config->isMappedLike("down", input))
			listInput(0);
		if(!UIModeController::getInstance()->isUIModeKid() && config->isMappedTo("select", input) && Settings::getInstance()->getBool("ScreenSaverControls"))
		{
			mWindow->startScreenSaver();
			mWindow->renderScreenSaver();
			return true;
		}
	}

	return GuiComponent::input(config, input);
}

void SystemView::update(int deltaTime)
{
	listUpdate(deltaTime);
	GuiComponent::update(deltaTime);
}

void SystemView::onCursorChanged(const CursorState& /*state*/)
{
	// update help style
	updateHelpPrompts();

	float startPos = mCamOffset;

	float posMax = (float)mEntries.size();
	float target = (float)mCursor;

	// what's the shortest way to get to our target?
	// it's one of these...

	float endPos = target; // directly
	float dist = abs(endPos - startPos);

	if(abs(target + posMax - startPos) < dist)
		endPos = target + posMax; // loop around the end (0 -> max)
	if(abs(target - posMax - startPos) < dist)
		endPos = target - posMax; // loop around the start (max - 1 -> -1)


	// animate mSystemInfo's opacity (fade out, wait, fade back in)

	cancelAnimation(1);
	cancelAnimation(2);

	std::string transition_style = Settings::getInstance()->getString("TransitionStyle");
	bool goFast = transition_style == "instant";
	const float infoStartOpacity = mSystemInfo.getOpacity() / 255.f;

	Animation* infoFadeOut = new LambdaAnimation(
		[infoStartOpacity, this] (float t)
	{
		mSystemInfo.setOpacity((unsigned char)(Math::lerp(infoStartOpacity, 0.f, t) * 255));
	}, (int)(infoStartOpacity * (goFast ? 10 : 150)));

	unsigned int gameCount = getSelected()->getDisplayedGameCount();

	// also change the text after we've fully faded out
	setAnimation(infoFadeOut, 0, [this, gameCount] {
		std::stringstream ss;

		if (!getSelected()->isGameSystem())
			ss << "CONFIGURATION";
		else
			ss << gameCount << " GAME" << (gameCount == 1 ? "" : "S") << " AVAILABLE";

		mSystemInfo.setText(ss.str());
	}, false, 1);

	Animation* infoFadeIn = new LambdaAnimation(
		[this](float t)
	{
		mSystemInfo.setOpacity((unsigned char)(Math::lerp(0.f, 1.f, t) * 255));
	}, goFast ? 10 : 300);

	// wait 600ms to fade in
	setAnimation(infoFadeIn, goFast ? 0 : 2000, nullptr, false, 2);

	// no need to animate transition, we're not going anywhere (probably mEntries.size() == 1)
	if(endPos == mCamOffset && endPos == mExtrasCamOffset)
		return;

	Animation* anim;
	bool move_carousel = Settings::getInstance()->getBool("MoveCarousel");
	if(transition_style == "fade")
	{
		float startExtrasFade = mExtrasFadeOpacity;
		anim = new LambdaAnimation(
			[this, startExtrasFade, startPos, endPos, posMax, move_carousel](float t)
		{
			t -= 1;
			float f = Math::lerp(startPos, endPos, t*t*t + 1);
			if(f < 0)
				f += posMax;
			if(f >= posMax)
				f -= posMax;

			this->mCamOffset = move_carousel ? f : endPos;

			t += 1;
			if(t < 0.3f)
				this->mExtrasFadeOpacity = Math::lerp(0.0f, 1.0f, t / 0.3f + startExtrasFade);
			else if(t < 0.7f)
				this->mExtrasFadeOpacity = 1.0f;
			else
				this->mExtrasFadeOpacity = Math::lerp(1.0f, 0.0f, (t - 0.7f) / 0.3f);

			if(t > 0.5f)
				this->mExtrasCamOffset = endPos;

		}, 500);
	} else if (transition_style == "slide") {
		// slide
		anim = new LambdaAnimation(
			[this, startPos, endPos, posMax, move_carousel](float t)
		{
			t -= 1;
			float f = Math::lerp(startPos, endPos, t*t*t + 1);
			if(f < 0)
				f += posMax;
			if(f >= posMax)
				f -= posMax;

			this->mCamOffset = move_carousel ? f : endPos;
			this->mExtrasCamOffset = f;
		}, 500);
	} else {
		// instant
		anim = new LambdaAnimation(
			[this, startPos, endPos, posMax, move_carousel ](float t)
		{
			t -= 1;
			float f = Math::lerp(startPos, endPos, t*t*t + 1);
			if(f < 0)
				f += posMax;
			if(f >= posMax)
				f -= posMax;

			this->mCamOffset = move_carousel ? f : endPos;
			this->mExtrasCamOffset = endPos;
		}, move_carousel ? 500 : 1);
	}


	setAnimation(anim, 0, nullptr, false, 0);
}

void SystemView::render(const Transform4x4f& parentTrans)
{
	if(size() == 0)
		return;  // nothing to render

	Transform4x4f trans = getTransform() * parentTrans;

	auto systemInfoZIndex = mSystemInfo.getZIndex();
	auto minMax = std::minmax(mCarousel.zIndex, systemInfoZIndex);

	renderExtras(trans, INT16_MIN, minMax.first);
	renderFade(trans);

	if (mCarousel.zIndex > mSystemInfo.getZIndex()) {
		renderInfoBar(trans);
	} else {
		renderCarousel(trans);
	}

	renderExtras(trans, minMax.first, minMax.second);

	if (mCarousel.zIndex > mSystemInfo.getZIndex()) {
		renderCarousel(trans);
	} else {
		renderInfoBar(trans);
	}

	renderExtras(trans, minMax.second, INT16_MAX);
}

std::vector<HelpPrompt> SystemView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	if (mCarousel.type == VERTICAL || mCarousel.type == VERTICAL_WHEEL)
		prompts.push_back(HelpPrompt("up/down", "choose"));
	else
		prompts.push_back(HelpPrompt("left/right", "choose"));
	prompts.push_back(HelpPrompt("a", "select"));
	prompts.push_back(HelpPrompt("x", "random"));

	if (!UIModeController::getInstance()->isUIModeKid() && Settings::getInstance()->getBool("ScreenSaverControls"))
		prompts.push_back(HelpPrompt("select", "launch screensaver"));

	return prompts;
}

HelpStyle SystemView::getHelpStyle()
{
	HelpStyle style;
	style.applyTheme(mEntries.at(mCursor).object->getTheme(), "system");
	return style;
}

void  SystemView::onThemeChanged(const std::shared_ptr<ThemeData>& /*theme*/)
{
	LOG(LogDebug) << "SystemView::onThemeChanged()";
	mViewNeedsReload = true;
	populate();
}

//  Get the ThemeElements that make up the SystemView.
void  SystemView::getViewElements(const std::shared_ptr<ThemeData>& theme)
{
	LOG(LogDebug) << "SystemView::getViewElements()";

	getDefaultElements();

	if (!theme->hasView("system"))
		return;

	const ThemeData::ThemeElement* carouselElem = theme->getElement("system", "systemcarousel", "carousel");
	if (carouselElem)
		getCarouselFromTheme(carouselElem);

	const ThemeData::ThemeElement* sysInfoElem = theme->getElement("system", "systemInfo", "text");
	if (sysInfoElem)
		mSystemInfo.applyTheme(theme, "system", "systemInfo", ThemeFlags::ALL);

	mViewNeedsReload = false;
}

//  Render system carousel
void SystemView::renderCarousel(const Transform4x4f& trans)
{
	// background box behind logos
	Transform4x4f carouselTrans = trans;
	carouselTrans.translate(Vector3f(mCarousel.pos.x(), mCarousel.pos.y(), 0.0));
	carouselTrans.translate(Vector3f(mCarousel.origin.x() * mCarousel.size.x() * -1, mCarousel.origin.y() * mCarousel.size.y() * -1, 0.0f));

	Vector2f clipPos(carouselTrans.translation().x(), carouselTrans.translation().y());
	Renderer::pushClipRect(Vector2i((int)clipPos.x(), (int)clipPos.y()), Vector2i((int)mCarousel.size.x(), (int)mCarousel.size.y()));

	Renderer::setMatrix(carouselTrans);
	Renderer::drawRect(0.0f, 0.0f, mCarousel.size.x(), mCarousel.size.y(), mCarousel.color, mCarousel.colorEnd, mCarousel.colorGradientHorizontal);

	// draw logos
	Vector2f logoSpacing(0.0, 0.0); // NB: logoSpacing will include the size of the logo itself as well!
	float xOff = 0.0;
	float yOff = 0.0;

	switch (mCarousel.type)
	{
		case VERTICAL_WHEEL:
			yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2.f - (mCamOffset * logoSpacing[1]);
			if (mCarousel.logoAlignment == ALIGN_LEFT)
				xOff = mCarousel.logoSize.x() / 10.f;
			else if (mCarousel.logoAlignment == ALIGN_RIGHT)
				xOff = mCarousel.size.x() - (mCarousel.logoSize.x() * 1.1f);
			else
				xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2.f;
			break;
		case VERTICAL:
			logoSpacing[1] = ((mCarousel.size.y() - (mCarousel.logoSize.y() * mCarousel.maxLogoCount)) / (mCarousel.maxLogoCount)) + mCarousel.logoSize.y();
			yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2.f - (mCamOffset * logoSpacing[1]);

			if (mCarousel.logoAlignment == ALIGN_LEFT)
				xOff = mCarousel.logoSize.x() / 10.f;
			else if (mCarousel.logoAlignment == ALIGN_RIGHT)
				xOff = mCarousel.size.x() - (mCarousel.logoSize.x() * 1.1f);
			else
				xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2;
			break;
		case HORIZONTAL_WHEEL:
			xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2 - (mCamOffset * logoSpacing[1]);
			if (mCarousel.logoAlignment == ALIGN_TOP)
				yOff = mCarousel.logoSize.y() / 10;
			else if (mCarousel.logoAlignment == ALIGN_BOTTOM)
				yOff = mCarousel.size.y() - (mCarousel.logoSize.y() * 1.1f);
			else
				yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2;
			break;
		case HORIZONTAL:
		default:
			logoSpacing[0] = ((mCarousel.size.x() - (mCarousel.logoSize.x() * mCarousel.maxLogoCount)) / (mCarousel.maxLogoCount)) + mCarousel.logoSize.x();
			xOff = (mCarousel.size.x() - mCarousel.logoSize.x()) / 2.f - (mCamOffset * logoSpacing[0]);

			if (mCarousel.logoAlignment == ALIGN_TOP)
				yOff = mCarousel.logoSize.y() / 10.f;
			else if (mCarousel.logoAlignment == ALIGN_BOTTOM)
				yOff = mCarousel.size.y() - (mCarousel.logoSize.y() * 1.1f);
			else
				yOff = (mCarousel.size.y() - mCarousel.logoSize.y()) / 2.f;
			break;
	}

	int center = (int)(mCamOffset);
	int logoCount = Math::min(mCarousel.maxLogoCount, (int)mEntries.size());

	// Adding texture loading buffers depending on scrolling speed and status
	int bufferIndex = getScrollingVelocity() + 1;
	int bufferLeft = logoBuffersLeft[bufferIndex];
	int bufferRight = logoBuffersRight[bufferIndex];
	if (logoCount == 1)
	{
		bufferLeft = 0;
		bufferRight = 0;
	}

	for (int i = center - logoCount / 2 + bufferLeft; i <= center + logoCount / 2 + bufferRight; i++)
	{
		int index = i;
		while (index < 0)
			index += (int)mEntries.size();
		while (index >= (int)mEntries.size())
			index -= (int)mEntries.size();

		Transform4x4f logoTrans = carouselTrans;
		logoTrans.translate(Vector3f(i * logoSpacing[0] + xOff, i * logoSpacing[1] + yOff, 0));

		float distance = i - mCamOffset;

		float scale = 1.0f + ((mCarousel.logoScale - 1.0f) * (1.0f - fabs(distance)));
		scale = Math::min(mCarousel.logoScale, Math::max(1.0f, scale));
		scale /= mCarousel.logoScale;

		int opacity = (int)Math::round(0x80 + ((0xFF - 0x80) * (1.0f - fabs(distance))));
		opacity = Math::max((int) 0x80, opacity);

		const std::shared_ptr<GuiComponent> &comp = mEntries.at(index).data.logo;
		if (mCarousel.type == VERTICAL_WHEEL || mCarousel.type == HORIZONTAL_WHEEL) {
			comp->setRotationDegrees(mCarousel.logoRotation * distance);
			comp->setRotationOrigin(mCarousel.logoRotationOrigin);
		}
		comp->setScale(scale);
		comp->setOpacity((unsigned char)opacity);
		comp->render(logoTrans);
	}
	Renderer::popClipRect();
}

void SystemView::renderInfoBar(const Transform4x4f& trans)
{
	Renderer::setMatrix(trans);
	mSystemInfo.render(trans);
}

// Draw background extras
void SystemView::renderExtras(const Transform4x4f& trans, float lower, float upper)
{
	int extrasCenter = (int)mExtrasCamOffset;

	// Adding texture loading buffers depending on scrolling speed and status
	int bufferIndex = getScrollingVelocity() + 1;

	Renderer::pushClipRect(Vector2i::Zero(), Vector2i((int)mSize.x(), (int)mSize.y()));

	for (int i = extrasCenter + logoBuffersLeft[bufferIndex]; i <= extrasCenter + logoBuffersRight[bufferIndex]; i++)
	{
		int index = i;
		while (index < 0)
			index += (int)mEntries.size();
		while (index >= (int)mEntries.size())
			index -= (int)mEntries.size();

		//Only render selected system when not showing
		if (mShowing || index == mCursor)
		{
			Transform4x4f extrasTrans = trans;
			if (mCarousel.type == HORIZONTAL || mCarousel.type == HORIZONTAL_WHEEL)
				extrasTrans.translate(Vector3f((i - mExtrasCamOffset) * mSize.x(), 0, 0));
			else
				extrasTrans.translate(Vector3f(0, (i - mExtrasCamOffset) * mSize.y(), 0));

			Renderer::pushClipRect(Vector2i((int)extrasTrans.translation()[0], (int)extrasTrans.translation()[1]),
								   Vector2i((int)mSize.x(), (int)mSize.y()));
			SystemViewData data = mEntries.at(index).data;
			for (unsigned int j = 0; j < data.backgroundExtras.size(); j++) {
				GuiComponent *extra = data.backgroundExtras[j];
				if (extra->getZIndex() >= lower && extra->getZIndex() < upper) {
					extra->render(extrasTrans);
				}
			}
			Renderer::popClipRect();
		}
	}
	Renderer::popClipRect();
}

void SystemView::renderFade(const Transform4x4f& trans)
{
	// fade extras if necessary
	if (mExtrasFadeOpacity)
	{
		unsigned int fadeColor = 0x00000000 | (unsigned char)(mExtrasFadeOpacity * 255);
		Renderer::setMatrix(trans);
		Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), fadeColor, fadeColor);
	}
}

// Populate the system carousel with the legacy values
void  SystemView::getDefaultElements(void)
{
	// Carousel
	mCarousel.type = HORIZONTAL;
	mCarousel.logoAlignment = ALIGN_CENTER;
	mCarousel.size.x() = mSize.x();
	mCarousel.size.y() = 0.2325f * mSize.y();
	mCarousel.pos.x() = 0.0f;
	mCarousel.pos.y() = 0.5f * (mSize.y() - mCarousel.size.y());
	mCarousel.origin.x() = 0.0f;
	mCarousel.origin.y() = 0.0f;
	mCarousel.color = 0xFFFFFFD8;
	mCarousel.colorEnd = 0xFFFFFFD8;
	mCarousel.colorGradientHorizontal = true;
	mCarousel.logoScale = 1.2f;
	mCarousel.logoRotation = 7.5;
	mCarousel.logoRotationOrigin.x() = -5;
	mCarousel.logoRotationOrigin.y() = 0.5;
	mCarousel.logoSize.x() = 0.25f * mSize.x();
	mCarousel.logoSize.y() = 0.155f * mSize.y();
	mCarousel.maxLogoCount = 3;
	mCarousel.zIndex = 40;

	// System Info Bar
	mSystemInfo.setSize(mSize.x(), mSystemInfo.getFont()->getLetterHeight()*2.2f);
	mSystemInfo.setPosition(0, (mCarousel.pos.y() + mCarousel.size.y() - 0.2f));
	mSystemInfo.setBackgroundColor(0xDDDDDDD8);
	mSystemInfo.setRenderBackground(true);
	mSystemInfo.setFont(Font::get((int)(0.035f * mSize.y()), Font::getDefaultPath()));
	mSystemInfo.setColor(0x000000FF);
	mSystemInfo.setZIndex(50);
	mSystemInfo.setDefaultZIndex(50);
}

void SystemView::getCarouselFromTheme(const ThemeData::ThemeElement* elem)
{
	if (elem->has("type"))
	{
		if (!(elem->get<std::string>("type").compare("vertical")))
			mCarousel.type = VERTICAL;
		else if (!(elem->get<std::string>("type").compare("vertical_wheel")))
			mCarousel.type = VERTICAL_WHEEL;
		else if (!(elem->get<std::string>("type").compare("horizontal_wheel")))
			mCarousel.type = HORIZONTAL_WHEEL;
		else
			mCarousel.type = HORIZONTAL;
	}
	if (elem->has("size"))
		mCarousel.size = elem->get<Vector2f>("size") * mSize;
	if (elem->has("pos"))
		mCarousel.pos = elem->get<Vector2f>("pos") * mSize;
	if (elem->has("origin"))
		mCarousel.origin = elem->get<Vector2f>("origin");
	if (elem->has("color"))
	{
		mCarousel.color = elem->get<unsigned int>("color");
		mCarousel.colorEnd = mCarousel.color;
	}
	if (elem->has("colorEnd"))
		mCarousel.colorEnd = elem->get<unsigned int>("colorEnd");
	if (elem->has("gradientType"))
		mCarousel.colorGradientHorizontal = !(elem->get<std::string>("gradientType").compare("horizontal"));
	if (elem->has("logoScale"))
		mCarousel.logoScale = elem->get<float>("logoScale");
	if (elem->has("logoSize"))
		mCarousel.logoSize = elem->get<Vector2f>("logoSize") * mSize;
	if (elem->has("maxLogoCount"))
		mCarousel.maxLogoCount = (int)Math::round(elem->get<float>("maxLogoCount"));
	if (elem->has("zIndex"))
		mCarousel.zIndex = elem->get<float>("zIndex");
	if (elem->has("logoRotation"))
		mCarousel.logoRotation = elem->get<float>("logoRotation");
	if (elem->has("logoRotationOrigin"))
		mCarousel.logoRotationOrigin = elem->get<Vector2f>("logoRotationOrigin");
	if (elem->has("logoAlignment"))
	{
		if (!(elem->get<std::string>("logoAlignment").compare("left")))
			mCarousel.logoAlignment = ALIGN_LEFT;
		else if (!(elem->get<std::string>("logoAlignment").compare("right")))
			mCarousel.logoAlignment = ALIGN_RIGHT;
		else if (!(elem->get<std::string>("logoAlignment").compare("top")))
			mCarousel.logoAlignment = ALIGN_TOP;
		else if (!(elem->get<std::string>("logoAlignment").compare("bottom")))
			mCarousel.logoAlignment = ALIGN_BOTTOM;
		else
			mCarousel.logoAlignment = ALIGN_CENTER;
	}
}

void SystemView::onShow()
{
	mShowing = true;
}

void SystemView::onHide()
{
	mShowing = false;
}
