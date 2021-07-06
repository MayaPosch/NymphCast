#pragma once
#ifndef ES_APP_GUIS_GUI_GAME_LIST_OPTIONS_H
#define ES_APP_GUIS_GUI_GAME_LIST_OPTIONS_H

#include "components/MenuComponent.h"
#include "components/OptionListComponent.h"
#include "FileData.h"
#include "GuiComponent.h"

class IGameListView;
class SystemData;

class GuiGamelistOptions : public GuiComponent
{
public:
	GuiGamelistOptions(Window* window, SystemData* system);
	virtual ~GuiGamelistOptions();

	virtual bool input(InputConfig* config, Input input) override;
	virtual std::vector<HelpPrompt> getHelpPrompts() override;
	virtual HelpStyle getHelpStyle() override;

private:
	void openGamelistFilter();
	void openMetaDataEd();
	void startEditMode();
	void exitEditMode();
	void jumpToLetter();

	MenuComponent mMenu;

	typedef OptionListComponent<char> LetterList;
	std::shared_ptr<LetterList> mJumpToLetterList;

	typedef OptionListComponent<const FileData::SortType*> SortList;
	std::shared_ptr<SortList> mListSort;

	SystemData* mSystem;
	IGameListView* getGamelist();
	bool fromPlaceholder;
	bool mFiltersChanged;
};

#endif // ES_APP_GUIS_GUI_GAME_LIST_OPTIONS_H
