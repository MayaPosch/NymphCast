#include "guis/GuiSlideshowScreensaverOptions.h"

#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiTextEditPopup.h"
#include "utils/StringUtil.h"
#include "Settings.h"
#include "Window.h"

GuiSlideshowScreensaverOptions::GuiSlideshowScreensaverOptions(Window* window, const char* title) : GuiScreensaverOptions(window, title)
{
	ComponentListRow row;

	// image duration (seconds)
	auto sss_image_sec = std::make_shared<SliderComponent>(mWindow, 1.f, 60.f, 1.f, "s");
	sss_image_sec->setValue((float)(Settings::getInstance()->getInt("ScreenSaverSwapImageTimeout") / (1000)));
	addWithLabel(row, "SWAP IMAGE AFTER (SECS)", sss_image_sec);
	addSaveFunc([sss_image_sec] {
		int playNextTimeout = (int)Math::round(sss_image_sec->getValue()) * (1000);
		Settings::getInstance()->setInt("ScreenSaverSwapImageTimeout", playNextTimeout);
		PowerSaver::updateTimeouts();
	});

	// stretch
	auto sss_stretch = std::make_shared<SwitchComponent>(mWindow);
	sss_stretch->setState(Settings::getInstance()->getBool("SlideshowScreenSaverStretch"));
	addWithLabel(row, "STRETCH IMAGES", sss_stretch);
	addSaveFunc([sss_stretch] {
		Settings::getInstance()->setBool("SlideshowScreenSaverStretch", sss_stretch->getState());
	});

	// background audio file
	auto sss_bg_audio_file = std::make_shared<TextComponent>(mWindow, "", Font::get(FONT_SIZE_SMALL), 0x777777FF);
	addEditableTextComponent(row, "BACKGROUND AUDIO", sss_bg_audio_file, Settings::getInstance()->getString("SlideshowScreenSaverBackgroundAudioFile"));
	addSaveFunc([sss_bg_audio_file] {
		Settings::getInstance()->setString("SlideshowScreenSaverBackgroundAudioFile", sss_bg_audio_file->getValue());
	});

	// image source
	auto sss_custom_source = std::make_shared<SwitchComponent>(mWindow);
	sss_custom_source->setState(Settings::getInstance()->getBool("SlideshowScreenSaverCustomImageSource"));
	addWithLabel(row, "USE CUSTOM IMAGES", sss_custom_source);
	addSaveFunc([sss_custom_source] { Settings::getInstance()->setBool("SlideshowScreenSaverCustomImageSource", sss_custom_source->getState()); });

	// custom image directory
	auto sss_image_dir = std::make_shared<TextComponent>(mWindow, "", Font::get(FONT_SIZE_SMALL), 0x777777FF);
	addEditableTextComponent(row, "CUSTOM IMAGE DIR", sss_image_dir, Settings::getInstance()->getString("SlideshowScreenSaverImageDir"));
	addSaveFunc([sss_image_dir] {
		Settings::getInstance()->setString("SlideshowScreenSaverImageDir", sss_image_dir->getValue());
	});

	// recurse custom image directory
	auto sss_recurse = std::make_shared<SwitchComponent>(mWindow);
	sss_recurse->setState(Settings::getInstance()->getBool("SlideshowScreenSaverRecurse"));
	addWithLabel(row, "CUSTOM IMAGE DIR RECURSIVE", sss_recurse);
	addSaveFunc([sss_recurse] {
		Settings::getInstance()->setBool("SlideshowScreenSaverRecurse", sss_recurse->getState());
	});

	// custom image filter
	auto sss_image_filter = std::make_shared<TextComponent>(mWindow, "", Font::get(FONT_SIZE_SMALL), 0x777777FF);
	addEditableTextComponent(row, "CUSTOM IMAGE FILTER", sss_image_filter, Settings::getInstance()->getString("SlideshowScreenSaverImageFilter"));
	addSaveFunc([sss_image_filter] {
		Settings::getInstance()->setString("SlideshowScreenSaverImageFilter", sss_image_filter->getValue());
	});
}

GuiSlideshowScreensaverOptions::~GuiSlideshowScreensaverOptions()
{
}

void GuiSlideshowScreensaverOptions::addWithLabel(ComponentListRow row, const std::string label, std::shared_ptr<GuiComponent> component)
{
	row.elements.clear();

	auto lbl = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(label), Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
	row.addElement(lbl, true); // label

	row.addElement(component, false, true);

	addRow(row);
}
