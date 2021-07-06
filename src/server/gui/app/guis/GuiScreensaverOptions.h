#pragma once
#ifndef ES_APP_GUIS_GUI_SCREENSAVER_OPTIONS_H
#define ES_APP_GUIS_GUI_SCREENSAVER_OPTIONS_H

#include "components/MenuComponent.h"

// This is just a really simple template for a GUI that calls some save functions when closed.
class GuiScreensaverOptions : public GuiComponent
{
public:
	GuiScreensaverOptions(Window* window, const char* title);
	virtual ~GuiScreensaverOptions(); // just calls save();

	virtual void save();
	inline void addRow(const ComponentListRow& row) { mMenu.addRow(row); };
	inline void addWithLabel(const std::string& label, const std::shared_ptr<GuiComponent>& comp) { mMenu.addWithLabel(label, comp); };
	inline void addSaveFunc(const std::function<void()>& func) { mSaveFuncs.push_back(func); };
	void addEditableTextComponent(ComponentListRow row, const std::string label, std::shared_ptr<GuiComponent> ed, std::string value);

	bool input(InputConfig* config, Input input) override;
	std::vector<HelpPrompt> getHelpPrompts() override;
	HelpStyle getHelpStyle() override;

protected:
	MenuComponent mMenu;
	std::vector< std::function<void()> > mSaveFuncs;
};

#endif // ES_APP_GUIS_GUI_SCREENSAVER_OPTIONS_H
