#pragma once
#ifndef ES_APP_GUIS_GUI_GAME_LIST_FILTER_H
#define ES_APP_GUIS_GUI_GAME_LIST_FILTER_H

#include "components/MenuComponent.h"
#include "FileFilterIndex.h"
#include "GuiComponent.h"

template<typename T>
class OptionListComponent;
class SystemData;

class GuiGamelistFilter : public GuiComponent
{
public:
	GuiGamelistFilter(Window* window, SystemData* system);
	~GuiGamelistFilter();
	bool input(InputConfig* config, Input input) override;

	virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void initializeMenu();
	void applyFilters();
	void resetAllFilters();
	void addFiltersToMenu();

	std::map<FilterIndexType, std::shared_ptr< OptionListComponent<std::string> >> mFilterOptions;

	MenuComponent mMenu;
	SystemData* mSystem;
	FileFilterIndex* mFilterIndex;
};

#endif // ES_APP_GUIS_GUI_GAME_LIST_FILTER_H
