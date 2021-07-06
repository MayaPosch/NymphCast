#include "SwitchComponent.h"

#include "resources/Font.h"

SwitchComponent::SwitchComponent(Window* window, bool state) : GuiComponent(window), mImage(window), mState(state)
{
	mImage.setImage(":/off.svg");
	mImage.setResize(0, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight());
	mSize = mImage.getSize();
}

void SwitchComponent::onSizeChanged()
{
	mImage.setSize(mSize);
}

bool SwitchComponent::input(InputConfig* config, Input input)
{
	if(config->isMappedTo("a", input) && input.value)
	{
		mState = !mState;
		onStateChanged();
		return true;
	}

	return false;
}

void SwitchComponent::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();

	mImage.render(trans);

	renderChildren(trans);
}

bool SwitchComponent::getState() const
{
	return mState;
}

void SwitchComponent::setState(bool state)
{
	mState = state;
	onStateChanged();
}

std::string SwitchComponent::getValue() const
{
	return mState ?  "true" : "false";
}

void SwitchComponent::setValue(const std::string& statestring)
{
	if (statestring == "true")
	{
		mState = true;
	}else
	{
		mState = false;
	}
	onStateChanged();
}

void SwitchComponent::onStateChanged()
{
	mImage.setImage(mState ? ":/on.svg" : ":/off.svg");
}

std::vector<HelpPrompt> SwitchComponent::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt("a", "change"));
	return prompts;
}
