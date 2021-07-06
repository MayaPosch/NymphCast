#include "guis/GuiGeneralScreensaverOptions.h"

#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiSlideshowScreensaverOptions.h"
#include "guis/GuiVideoScreensaverOptions.h"
#include "Settings.h"

GuiGeneralScreensaverOptions::GuiGeneralScreensaverOptions(Window* window, const char* title) : GuiScreensaverOptions(window, title)
{
	// screensaver time
	auto screensaver_time = std::make_shared<SliderComponent>(mWindow, 0.f, 30.f, 1.f, "m");
	screensaver_time->setValue((float)(Settings::getInstance()->getInt("ScreenSaverTime") / Settings::ONE_MINUTE_IN_MS));
	addWithLabel("SCREENSAVER AFTER", screensaver_time);
	addSaveFunc([screensaver_time] {
		Settings::getInstance()->setInt("ScreenSaverTime", (int)Math::round(screensaver_time->getValue()) * Settings::ONE_MINUTE_IN_MS);
		PowerSaver::updateTimeouts();
	});

	// Allow ScreenSaver Controls - ScreenSaverControls
	auto ss_controls = std::make_shared<SwitchComponent>(mWindow);
	ss_controls->setState(Settings::getInstance()->getBool("ScreenSaverControls"));
	addWithLabel("SCREENSAVER CONTROLS", ss_controls);
	addSaveFunc([ss_controls] { Settings::getInstance()->setBool("ScreenSaverControls", ss_controls->getState()); });

	// screensaver behavior
	auto screensaver_behavior = std::make_shared< OptionListComponent<std::string> >(mWindow, "SCREENSAVER BEHAVIOR", false);
	std::vector<std::string> screensavers;
	screensavers.push_back("dim");
	screensavers.push_back("black");
	screensavers.push_back("random video");
	screensavers.push_back("slideshow");
	for(auto it = screensavers.cbegin(); it != screensavers.cend(); it++)
		screensaver_behavior->add(*it, *it, Settings::getInstance()->getString("ScreenSaverBehavior") == *it);
	addWithLabel("SCREENSAVER BEHAVIOR", screensaver_behavior);
	addSaveFunc([this, screensaver_behavior] {
		if (Settings::getInstance()->getString("ScreenSaverBehavior") != "random video" && screensaver_behavior->getSelected() == "random video") {
			// if before it wasn't risky but now there's a risk of problems, show warning
			mWindow->pushGui(new GuiMsgBox(mWindow,
			"The \"Random Video\" screensaver shows videos from your gamelist.\n\nIf you do not have videos, or if in several consecutive attempts the games it selects don't have videos it will default to black.\n\nMore options in the \"UI Settings\" > \"Video Screensaver\" menu.",
				"OK", [] { return; }));
		}
		Settings::getInstance()->setString("ScreenSaverBehavior", screensaver_behavior->getSelected());
		PowerSaver::updateTimeouts();
	});

	ComponentListRow row;

	// show filtered menu
	row.elements.clear();
	row.addElement(std::make_shared<TextComponent>(mWindow, "VIDEO SCREENSAVER SETTINGS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
	row.addElement(makeArrow(mWindow), false);
	row.makeAcceptInputHandler(std::bind(&GuiGeneralScreensaverOptions::openVideoScreensaverOptions, this));
	addRow(row);

	row.elements.clear();
	row.addElement(std::make_shared<TextComponent>(mWindow, "SLIDESHOW SCREENSAVER SETTINGS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
	row.addElement(makeArrow(mWindow), false);
	row.makeAcceptInputHandler(std::bind(&GuiGeneralScreensaverOptions::openSlideshowScreensaverOptions, this));
	addRow(row);

	// system sleep time
	float stepw = 5.f;
	float max =  120.f;
	auto system_sleep_time = std::make_shared<SliderComponent>(mWindow, 0.f, max, stepw, "m");
	system_sleep_time->setValue((float)(Settings::getInstance()->getInt("SystemSleepTime") / Settings::ONE_MINUTE_IN_MS));
	addWithLabel("SYSTEM SLEEP AFTER", system_sleep_time);
	addSaveFunc([this, system_sleep_time, screensaver_time, max, stepw] {
		if (screensaver_time->getValue() > system_sleep_time->getValue() && system_sleep_time->getValue() > 0) {
			int steps = Math::min(1 + (int)(screensaver_time->getValue() / stepw), (int)(max/stepw));
			int adj_system_sleep_time = steps*stepw;
			system_sleep_time->setValue((float)adj_system_sleep_time);
			std::string msg = "";
			if (!Settings::getInstance()->getBool("SystemSleepTimeHintDisplayed")) {
				msg += "One time note: Enabling the system sleep time will trigger user-defined scripts.";
				msg += "\nPlease see Retropie/Emulationstation Wiki on events for details.";
				Settings::getInstance()->setBool("SystemSleepTimeHintDisplayed", true);
			}
			if (msg.length() > 0) {
				msg += "\n\n";
			}
			msg += "The system sleep delay is enabled, but is less than or equal to the screen saver start delay.";
			msg	+= "\n\nAdjusted system sleep time to " + std::to_string(adj_system_sleep_time) + " minutes.";
			mWindow->pushGui(new GuiMsgBox(mWindow, msg, "OK", [] { return; }));
		}
		Settings::getInstance()->setInt("SystemSleepTime", (int)Math::round(system_sleep_time->getValue()) * Settings::ONE_MINUTE_IN_MS);
	});
}

GuiGeneralScreensaverOptions::~GuiGeneralScreensaverOptions()
{
}

void GuiGeneralScreensaverOptions::openVideoScreensaverOptions() {
	mWindow->pushGui(new GuiVideoScreensaverOptions(mWindow, "VIDEO SCREENSAVER"));
}

void GuiGeneralScreensaverOptions::openSlideshowScreensaverOptions() {
    mWindow->pushGui(new GuiSlideshowScreensaverOptions(mWindow, "SLIDESHOW SCREENSAVER"));
}
