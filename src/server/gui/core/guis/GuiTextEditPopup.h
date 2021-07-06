#pragma once
#ifndef ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H
#define ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "GuiComponent.h"

class TextComponent;
class TextEditComponent;

class GuiTextEditPopup : public GuiComponent
{
public:
	GuiTextEditPopup(Window* window, const std::string& title, const std::string& initValue,
		const std::function<void(const std::string&)>& okCallback, bool multiLine, const char* acceptBtnText = "OK");

	bool input(InputConfig* config, Input input);
	void onSizeChanged();
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	NinePatchComponent mBackground;
	ComponentGrid mGrid;

	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<TextEditComponent> mText;
	std::shared_ptr<ComponentGrid> mButtonGrid;

	bool mMultiLine;
};

#endif // ES_CORE_GUIS_GUI_TEXT_EDIT_POPUP_H
