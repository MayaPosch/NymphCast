#pragma once
#ifndef ES_APP_GUIS_GUI_SLIDESHOW_SCREENSAVER_OPTIONS_H
#define ES_APP_GUIS_GUI_SLIDESHOW_SCREENSAVER_OPTIONS_H

#include "GuiScreensaverOptions.h"

class GuiSlideshowScreensaverOptions : public GuiScreensaverOptions
{
public:
	GuiSlideshowScreensaverOptions(Window* window, const char* title);
	virtual ~GuiSlideshowScreensaverOptions();

private:
	void addWithLabel(ComponentListRow row, const std::string label, std::shared_ptr<GuiComponent> component);
};

#endif // ES_APP_GUIS_GUI_SLIDESHOW_SCREENSAVER_OPTIONS_H
