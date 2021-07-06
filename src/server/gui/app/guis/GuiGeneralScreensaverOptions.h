#pragma once
#ifndef ES_APP_GUIS_GUI_GENERAL_SCREENSAVER_OPTIONS_H
#define ES_APP_GUIS_GUI_GENERAL_SCREENSAVER_OPTIONS_H

#include "GuiScreensaverOptions.h"

class GuiGeneralScreensaverOptions : public GuiScreensaverOptions
{
public:
	GuiGeneralScreensaverOptions(Window* window, const char* title);
	virtual ~GuiGeneralScreensaverOptions();

private:
	void openVideoScreensaverOptions();
	void openSlideshowScreensaverOptions();
};

#endif // ES_APP_GUIS_GUI_GENERAL_SCREENSAVER_OPTIONS_H
