#include "guis/GuiGamelistFilter.h"

#include "components/OptionListComponent.h"
#include "views/UIModeController.h"
#include "SystemData.h"

GuiGamelistFilter::GuiGamelistFilter(Window* window, SystemData* system) : GuiComponent(window), mMenu(window, "FILTER GAMELIST BY"), mSystem(system)
{
	initializeMenu();
}

void GuiGamelistFilter::initializeMenu()
{
	addChild(&mMenu);

	// get filters from system

	mFilterIndex = mSystem->getIndex();

	ComponentListRow row;

	// show filtered menu
	row.elements.clear();
	row.addElement(std::make_shared<TextComponent>(mWindow, "RESET ALL FILTERS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
	row.makeAcceptInputHandler(std::bind(&GuiGamelistFilter::resetAllFilters, this));
	mMenu.addRow(row);
	row.elements.clear();

	addFiltersToMenu();

	mMenu.addButton("BACK", "back", std::bind(&GuiGamelistFilter::applyFilters, this));

	mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiGamelistFilter::resetAllFilters()
{
	mFilterIndex->resetFilters();
	for (std::map<FilterIndexType, std::shared_ptr< OptionListComponent<std::string> >>::const_iterator it = mFilterOptions.cbegin(); it != mFilterOptions.cend(); ++it ) {
		std::shared_ptr< OptionListComponent<std::string> > optionList = it->second;
		optionList->selectNone();
	}
}

GuiGamelistFilter::~GuiGamelistFilter()
{
	mFilterOptions.clear();
}

void GuiGamelistFilter::addFiltersToMenu()
{
	std::vector<FilterDataDecl> decls = mFilterIndex->getFilterDataDecls();

	int skip = 0;
	if (!UIModeController::getInstance()->isUIModeFull())
		skip = 1;
	if (UIModeController::getInstance()->isUIModeKid())
		skip = 2;

	for (std::vector<FilterDataDecl>::const_iterator it = decls.cbegin(); it != decls.cend()-skip; ++it ) {

		FilterIndexType type = (*it).type; // type of filter
		std::map<std::string, int>* allKeys = (*it).allIndexKeys; // all possible filters for this type
		std::string menuLabel = (*it).menuLabel; // text to show in menu
		std::shared_ptr< OptionListComponent<std::string> > optionList;


		// add filters (with first one selected)
		ComponentListRow row;

		// add genres
		optionList = std::make_shared< OptionListComponent<std::string> >(mWindow, menuLabel, true);
		for(auto it: *allKeys)
		{
			optionList->add(it.first, it.first, mFilterIndex->isKeyBeingFilteredBy(it.first, type));
		}
		if (allKeys->size() > 0)
			mMenu.addWithLabel(menuLabel, optionList);

		mFilterOptions[type] = optionList;
	}
}

void GuiGamelistFilter::applyFilters()
{
	std::vector<FilterDataDecl> decls = mFilterIndex->getFilterDataDecls();
	for (std::map<FilterIndexType, std::shared_ptr< OptionListComponent<std::string> >>::const_iterator it = mFilterOptions.cbegin(); it != mFilterOptions.cend(); ++it ) {
		std::shared_ptr< OptionListComponent<std::string> > optionList = it->second;
		std::vector<std::string> filters = optionList->getSelectedObjects();
		mFilterIndex->setFilter(it->first, &filters);
	}

	delete this;

}

bool GuiGamelistFilter::input(InputConfig* config, Input input)
{
	bool consumed = GuiComponent::input(config, input);
	if(consumed)
		return true;

	if(config->isMappedTo("b", input) && input.value != 0)
	{
		applyFilters();
	}


	return false;
}

std::vector<HelpPrompt> GuiGamelistFilter::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("b", "back"));
	return prompts;
}
