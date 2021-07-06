#include "guis/GuiMsgBox.h"

#include "components/ButtonComponent.h"
#include "components/MenuComponent.h"

#define HORIZONTAL_PADDING_PX 20

GuiMsgBox::GuiMsgBox(Window* window, const std::string& text,
	const std::string& name1, const std::function<void()>& func1,
	const std::string& name2, const std::function<void()>& func2,
	const std::string& name3, const std::function<void()>& func3) : GuiComponent(window),
	mBackground(window, ":/frame.png"), mGrid(window, Vector2i(1, 2))
{
	float width = Renderer::getScreenWidth() * 0.6f; // max width
	float minWidth = Renderer::getScreenWidth() * 0.3f; // minimum width

	mMsg = std::make_shared<TextComponent>(mWindow, text, Font::get(FONT_SIZE_MEDIUM), 0x777777FF, ALIGN_CENTER);
	mGrid.setEntry(mMsg, Vector2i(0, 0), false, false);

	// create the buttons
	mButtons.push_back(std::make_shared<ButtonComponent>(mWindow, name1, name1, std::bind(&GuiMsgBox::deleteMeAndCall, this, func1)));
	if(!name2.empty())
		mButtons.push_back(std::make_shared<ButtonComponent>(mWindow, name2, name3, std::bind(&GuiMsgBox::deleteMeAndCall, this, func2)));
	if(!name3.empty())
		mButtons.push_back(std::make_shared<ButtonComponent>(mWindow, name3, name3, std::bind(&GuiMsgBox::deleteMeAndCall, this, func3)));

	// set accelerator automatically (button to press when "b" is pressed)
	if(mButtons.size() == 1)
	{
		mAcceleratorFunc = mButtons.front()->getPressedFunc();
	}else{
		for(auto it = mButtons.cbegin(); it != mButtons.cend(); it++)
		{
			if(Utils::String::toUpper((*it)->getText()) == "OK" || Utils::String::toUpper((*it)->getText()) == "NO")
			{
				mAcceleratorFunc = (*it)->getPressedFunc();
				break;
			}
		}
	}

	// put the buttons into a ComponentGrid
	mButtonGrid = makeButtonGrid(mWindow, mButtons);
	mGrid.setEntry(mButtonGrid, Vector2i(0, 1), true, false, Vector2i(1, 1), GridFlags::BORDER_TOP);

	// decide final width
	if(mMsg->getSize().x() < width && mButtonGrid->getSize().x() < width)
	{
		// mMsg and buttons are narrower than width
		width = Math::max(mButtonGrid->getSize().x(), mMsg->getSize().x());
		width = Math::max(width, minWidth);
	}

	// now that we know width, we can find height
	mMsg->setSize(width, 0); // mMsg->getSize.y() now returns the proper length
	const float msgHeight = Math::max(Font::get(FONT_SIZE_LARGE)->getHeight(), mMsg->getSize().y()*1.225f);
	setSize(width + HORIZONTAL_PADDING_PX*2, msgHeight + mButtonGrid->getSize().y());

	// center for good measure
	setPosition((Renderer::getScreenWidth() - mSize.x()) / 2.0f, (Renderer::getScreenHeight() - mSize.y()) / 2.0f);

	addChild(&mBackground);
	addChild(&mGrid);
}

bool GuiMsgBox::input(InputConfig* config, Input input)
{
	// special case for when GuiMsgBox comes up to report errors before anything has been configured
	if(config->getDeviceId() == DEVICE_KEYBOARD && !config->isConfigured() && input.value &&
		(input.id == SDLK_RETURN || input.id == SDLK_ESCAPE || input.id == SDLK_SPACE))
	{
		mAcceleratorFunc();
		return true;
	}

	if(mAcceleratorFunc && config->isMappedTo("b", input) && input.value != 0)
	{
		mAcceleratorFunc();
		return true;
	}

	return GuiComponent::input(config, input);
}

void GuiMsgBox::onSizeChanged()
{
	mGrid.setSize(mSize);
	mGrid.setRowHeightPerc(1, mButtonGrid->getSize().y() / mSize.y());

	// update messagebox size
	mMsg->setSize(mSize.x() - HORIZONTAL_PADDING_PX*2, mGrid.getRowHeight(0));
	mGrid.onSizeChanged();

	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
}

void GuiMsgBox::deleteMeAndCall(const std::function<void()>& func)
{
	auto funcCopy = func;
	delete this;

	if(funcCopy)
		funcCopy();

}

std::vector<HelpPrompt> GuiMsgBox::getHelpPrompts()
{
	return mGrid.getHelpPrompts();
}
