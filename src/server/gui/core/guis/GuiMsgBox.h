#pragma once
#ifndef ES_CORE_GUIS_GUI_MSG_BOX_H
#define ES_CORE_GUIS_GUI_MSG_BOX_H

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "GuiComponent.h"

class ButtonComponent;
class TextComponent;

class GuiMsgBox : public GuiComponent
{
public:
	GuiMsgBox(Window* window, const std::string& text,
		const std::string& name1 = "OK", const std::function<void()>& func1 = nullptr,
		const std::string& name2 = "", const std::function<void()>& func2 = nullptr,
		const std::string& name3 = "", const std::function<void()>& func3 = nullptr);

	bool input(InputConfig* config, Input input) override;
	void onSizeChanged() override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void deleteMeAndCall(const std::function<void()>& func);

	NinePatchComponent mBackground;
	ComponentGrid mGrid;


	std::shared_ptr<TextComponent> mMsg;
	std::vector< std::shared_ptr<ButtonComponent> > mButtons;
	std::shared_ptr<ComponentGrid> mButtonGrid;
	std::function<void()> mAcceleratorFunc;
};

#endif // ES_CORE_GUIS_GUI_MSG_BOX_H
