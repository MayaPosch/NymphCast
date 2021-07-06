#include "guis/GuiTextEditPopup.h"

#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"
#include "components/TextEditComponent.h"

GuiTextEditPopup::GuiTextEditPopup(Window* window, const std::string& title, const std::string& initValue,
	const std::function<void(const std::string&)>& okCallback, bool multiLine, const char* acceptBtnText)
	: GuiComponent(window), mBackground(window, ":/frame.png"), mGrid(window, Vector2i(1, 3)), mMultiLine(multiLine)
{
	addChild(&mBackground);
	addChild(&mGrid);

	mTitle = std::make_shared<TextComponent>(mWindow, Utils::String::toUpper(title), Font::get(FONT_SIZE_LARGE), 0x555555FF, ALIGN_CENTER);

	mText = std::make_shared<TextEditComponent>(mWindow);
	mText->setValue(initValue);

	if(!multiLine)
		mText->setCursor(initValue.size());

	std::vector< std::shared_ptr<ButtonComponent> > buttons;
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, acceptBtnText, acceptBtnText, [this, okCallback] { okCallback(mText->getValue()); delete this; }));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, "CANCEL", "discard changes", [this] { delete this; }));

	mButtonGrid = makeButtonGrid(mWindow, buttons);

	mGrid.setEntry(mTitle, Vector2i(0, 0), false, true);
	mGrid.setEntry(mText, Vector2i(0, 1), true, false, Vector2i(1, 1), GridFlags::BORDER_TOP | GridFlags::BORDER_BOTTOM);
	mGrid.setEntry(mButtonGrid, Vector2i(0, 2), true, false);

	float textHeight = mText->getFont()->getHeight();
	if(multiLine)
		textHeight *= 6;
	mText->setSize(0, textHeight);

	setSize(Renderer::getScreenWidth() * 0.5f, mTitle->getFont()->getHeight() + textHeight + mButtonGrid->getSize().y() + 40);
	setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);
}

void GuiTextEditPopup::onSizeChanged()
{
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	mText->setSize(mSize.x() - 40, mText->getSize().y());

	// update grid
	mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y());
	mGrid.setRowHeightPerc(2, mButtonGrid->getSize().y() / mSize.y());
	mGrid.setSize(mSize);
}

bool GuiTextEditPopup::input(InputConfig* config, Input input)
{
	if(GuiComponent::input(config, input))
		return true;

	// pressing back when not text editing closes us
	if(config->isMappedTo("b", input) && input.value)
	{
		delete this;
		return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiTextEditPopup::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mGrid.getHelpPrompts();
	prompts.push_back(HelpPrompt("b", "back"));
	return prompts;
}
