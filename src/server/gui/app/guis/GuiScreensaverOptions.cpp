#include "guis/GuiScreensaverOptions.h"

#include "guis/GuiTextEditPopup.h"
#include "views/ViewController.h"
#include "Settings.h"
#include "SystemData.h"
#include "Window.h"

GuiScreensaverOptions::GuiScreensaverOptions(Window* window, const char* title) : GuiComponent(window), mMenu(window, title)
{
	addChild(&mMenu);

	mMenu.addButton("BACK", "go back", [this] { delete this; });

	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
	mMenu.setPosition((mSize.x() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

GuiScreensaverOptions::~GuiScreensaverOptions()
{
	save();
}

void GuiScreensaverOptions::save()
{
	if(!mSaveFuncs.size())
		return;

	for(auto it = mSaveFuncs.cbegin(); it != mSaveFuncs.cend(); it++)
		(*it)();

	Settings::getInstance()->saveFile();
}

bool GuiScreensaverOptions::input(InputConfig* config, Input input)
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

HelpStyle GuiScreensaverOptions::getHelpStyle()
{
	HelpStyle style = HelpStyle();
	style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
	return style;
}

std::vector<HelpPrompt> GuiScreensaverOptions::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();

	prompts.push_back(HelpPrompt("b", "back"));
	prompts.push_back(HelpPrompt("start", "close"));

	return prompts;
}

void GuiScreensaverOptions::addEditableTextComponent(ComponentListRow row, const std::string label, std::shared_ptr<GuiComponent> ed, std::string value)
{
	row.elements.clear();

	auto lbl = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(label), Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
	row.addElement(lbl, true); // label

	row.addElement(ed, true);

	auto spacer = std::make_shared<GuiComponent>(mWindow);
	spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
	row.addElement(spacer, false);

	auto bracket = std::make_shared<ImageComponent>(mWindow);
	bracket->setImage(":/arrow.svg");
	bracket->setResize(Vector2f(0, lbl->getFont()->getLetterHeight()));
	row.addElement(bracket, false);

	auto updateVal = [ed](const std::string& newVal) { ed->setValue(newVal); }; // ok callback (apply new value to ed)
	row.makeAcceptInputHandler([this, label, ed, updateVal] {
		mWindow->pushGui(new GuiTextEditPopup(mWindow, label, ed->getValue(), updateVal, false));
	});
	assert(ed);
	addRow(row);
	ed->setValue(value);
}
