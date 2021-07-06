#include "guis/GuiDetectDevice.h"

#include "components/TextComponent.h"
#include "guis/GuiInputConfig.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "InputManager.h"
#include "PowerSaver.h"
#include "Window.h"

#define HOLD_TIME 1000

GuiDetectDevice::GuiDetectDevice(Window* window, bool firstRun, const std::function<void()>& doneCallback) : GuiComponent(window), mFirstRun(firstRun),
	mBackground(window, ":/frame.png"), mGrid(window, Vector2i(1, 5))
{
	mHoldingConfig = NULL;
	mHoldTime = 0;
	mDoneCallback = doneCallback;

	addChild(&mBackground);
	addChild(&mGrid);

	// title
	mTitle = std::make_shared<TextComponent>(mWindow, firstRun ? "WELCOME" : "CONFIGURE INPUT",
		Font::get(FONT_SIZE_LARGE), 0x555555FF, ALIGN_CENTER);
	mGrid.setEntry(mTitle, Vector2i(0, 0), false, true, Vector2i(1, 1), GridFlags::BORDER_BOTTOM);

	// device info
	std::stringstream deviceInfo;
	int numDevices = InputManager::getInstance()->getNumJoysticks();

	if(numDevices > 0)
		deviceInfo << numDevices << " GAMEPAD" << (numDevices > 1 ? "S" : "") << " DETECTED";
	else
		deviceInfo << "NO GAMEPADS DETECTED";
	mDeviceInfo = std::make_shared<TextComponent>(mWindow, deviceInfo.str(), Font::get(FONT_SIZE_SMALL), 0x999999FF, ALIGN_CENTER);
	mGrid.setEntry(mDeviceInfo, Vector2i(0, 1), false, true);

	// message
	mMsg1 = std::make_shared<TextComponent>(mWindow, "HOLD A BUTTON ON YOUR DEVICE TO CONFIGURE IT.", Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_CENTER);
	mGrid.setEntry(mMsg1, Vector2i(0, 2), false, true);

	const char* msg2str = firstRun ? "PRESS F4 TO QUIT AT ANY TIME." : "PRESS ESC TO CANCEL.";
	mMsg2 = std::make_shared<TextComponent>(mWindow, msg2str, Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_CENTER);
	mGrid.setEntry(mMsg2, Vector2i(0, 3), false, true);

	// currently held device
	mDeviceHeld = std::make_shared<TextComponent>(mWindow, "", Font::get(FONT_SIZE_MEDIUM), 0xFFFFFFFF, ALIGN_CENTER);
	mGrid.setEntry(mDeviceHeld, Vector2i(0, 4), false, true);

	setSize(Renderer::getScreenWidth() * 0.6f, Renderer::getScreenHeight() * 0.5f);
	setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);
}

void GuiDetectDevice::onSizeChanged()
{
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	// grid
	mGrid.setSize(mSize);
	mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y());
	//mGrid.setRowHeightPerc(1, mDeviceInfo->getFont()->getHeight() / mSize.y());
	mGrid.setRowHeightPerc(2, mMsg1->getFont()->getHeight() / mSize.y());
	mGrid.setRowHeightPerc(3, mMsg2->getFont()->getHeight() / mSize.y());
	//mGrid.setRowHeightPerc(4, mDeviceHeld->getFont()->getHeight() / mSize.y());
}

bool GuiDetectDevice::input(InputConfig* config, Input input)
{
	PowerSaver::pause();

	if(!mFirstRun && input.device == DEVICE_KEYBOARD && input.type == TYPE_KEY && input.value && input.id == SDLK_ESCAPE)
	{
		// cancel configuring
		PowerSaver::resume();
		delete this;
		return true;
	}

	if(input.type == TYPE_BUTTON || input.type == TYPE_KEY ||input.type == TYPE_CEC_BUTTON)
	{
		if(input.value && mHoldingConfig == NULL)
		{
			// started holding
			mHoldingConfig = config;
			mHoldTime = HOLD_TIME;
			mDeviceHeld->setText(Utils::String::toUpper(config->getDeviceName()));
		}else if(!input.value && mHoldingConfig == config)
		{
			// cancel
			mHoldingConfig = NULL;
			mDeviceHeld->setText("");
		}
	}

	return true;
}

void GuiDetectDevice::update(int deltaTime)
{
	if(mHoldingConfig)
	{
		// If ES starts and if a known device is connected after startup skip controller configuration
		if(mFirstRun && Utils::FileSystem::exists(InputManager::getConfigPath()) && InputManager::getInstance()->getNumConfiguredDevices() > 0)
		{
			if(mDoneCallback)
				mDoneCallback();
			PowerSaver::resume();
			delete this; // delete GUI element
		}
		else
		{
			mHoldTime -= deltaTime;
			const float t = (float)mHoldTime / HOLD_TIME;
			unsigned int c = (unsigned char)(t * 255);
			mDeviceHeld->setColor((c << 24) | (c << 16) | (c << 8) | 0xFF);
			if(mHoldTime <= 0)
			{
				// picked one!
				mWindow->pushGui(new GuiInputConfig(mWindow, mHoldingConfig, true, mDoneCallback));
				PowerSaver::resume();
				delete this;
			}
		}
	}
}
