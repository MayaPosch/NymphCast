#pragma once
#ifndef ES_CORE_GUIS_GUI_DETECT_DEVICE_H
#define ES_CORE_GUIS_GUI_DETECT_DEVICE_H

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "GuiComponent.h"

class TextComponent;

class GuiDetectDevice : public GuiComponent
{
public:
	GuiDetectDevice(Window* window, bool firstRun, const std::function<void()>& doneCallback);

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void onSizeChanged() override;

private:
	bool mFirstRun;
	InputConfig* mHoldingConfig;
	int mHoldTime;

	NinePatchComponent mBackground;
	ComponentGrid mGrid;

	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<TextComponent> mMsg1;
	std::shared_ptr<TextComponent> mMsg2;
	std::shared_ptr<TextComponent> mDeviceInfo;
	std::shared_ptr<TextComponent> mDeviceHeld;

	std::function<void()> mDoneCallback;
};

#endif // ES_CORE_GUIS_GUI_DETECT_DEVICE_H
