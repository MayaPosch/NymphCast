#pragma once
#ifndef ES_APP_GUIS_GUI_INFO_POPUP_H
#define ES_APP_GUIS_GUI_INFO_POPUP_H

#include "GuiComponent.h"
#include "Window.h"

class ComponentGrid;
class NinePatchComponent;

class GuiInfoPopup : public GuiComponent, public Window::InfoPopup
{
public:
	GuiInfoPopup(Window* window, std::string message, int duration, int fadein = 500, int fadeout = 500);
	~GuiInfoPopup();
	void render(const Transform4x4f& parentTrans) override;
	inline void stop() { running = false; };
private:
	std::string mMessage;
	int mDuration;
	int mFadein;
	int mFadeout;
	int alpha;
	bool updateState();
	int mStartTime;
	ComponentGrid* mGrid;
	NinePatchComponent* mFrame;
	bool running;
};

#endif // ES_APP_GUIS_GUI_INFO_POPUP_H
