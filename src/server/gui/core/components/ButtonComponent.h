#pragma once
#ifndef ES_CORE_COMPONENTS_BUTTON_COMPONENT_H
#define ES_CORE_COMPONENTS_BUTTON_COMPONENT_H

#include "components/NinePatchComponent.h"
#include "GuiComponent.h"

class TextCache;

class ButtonComponent : public GuiComponent
{
public:
	ButtonComponent(Window* window, const std::string& text = "", const std::string& helpText = "", const std::function<void()>& func = nullptr);

	void setPressedFunc(std::function<void()> f);

	void setEnabled(bool enable);

	bool input(InputConfig* config, Input input) override;
	void render(const Transform4x4f& parentTrans) override;

	void setText(const std::string& text, const std::string& helpText);

	inline const std::string& getText() const { return mText; };
	inline const std::function<void()>& getPressedFunc() const { return mPressedFunc; };

	void onSizeChanged() override;
	void onFocusGained() override;
	void onFocusLost() override;

	virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
	std::shared_ptr<Font> mFont;
	std::function<void()> mPressedFunc;

	bool mFocused;
	bool mEnabled;
	unsigned int mTextColorFocused;
	unsigned int mTextColorUnfocused;

	unsigned int getCurTextColor() const;
	void updateImage();

	std::string mText;
	std::string mHelpText;
	std::unique_ptr<TextCache> mTextCache;
	NinePatchComponent mBox;
};

#endif // ES_CORE_COMPONENTS_BUTTON_COMPONENT_H
