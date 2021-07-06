#include "guis/GuiSettings.h"

#include "views/ViewController.h"
#include "Settings.h"
#include "SystemData.h"
#include "Window.h"

GuiSettings::GuiSettings(Window* window, const char* title) : GuiComponent(window), mMenu(window, title)
{
	addChild(&mMenu);

	mMenu.addButton("BACK", "go back", [this] { delete this; });

	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
	mMenu.setPosition((mSize.x() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

GuiSettings::~GuiSettings()
{
	save();
}

void GuiSettings::save()
{
	if(!mSaveFuncs.size())
		return;

	for(auto it = mSaveFuncs.cbegin(); it != mSaveFuncs.cend(); it++)
		(*it)();

	Settings::getInstance()->saveFile();
}

bool GuiSettings::input(InputConfig* config, Input input)
{
	if(config->isMappedTo("b", input) && input.value != 0)
	{
		delete this;
		return true;
	}

	if(config->isMappedTo("start", input) && input.value != 0)
	{
		// close everything
		Window* window = mWindow;
		while(window->peekGui() && window->peekGui() != ViewController::get())
			delete window->peekGui();
		return true;
	}

	return GuiComponent::input(config, input);
}

HelpStyle GuiSettings::getHelpStyle()
{
	HelpStyle style = HelpStyle();
	style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
	return style;
}

std::vector<HelpPrompt> GuiSettings::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();

	prompts.push_back(HelpPrompt("b", "back"));
	prompts.push_back(HelpPrompt("start", "close"));

	return prompts;
}
