#pragma once
#ifndef ES_CORE_COMPONENTS_TEXT_COMPONENT_H
#define ES_CORE_COMPONENTS_TEXT_COMPONENT_H

#include "resources/Font.h"
#include "GuiComponent.h"

class ThemeData;

// Used to display text.
// TextComponent::setSize(x, y) works a little differently than most components:
//  * (0, 0)                     - will automatically calculate a size that fits the text on one line (expand horizontally)
//  * (x != 0, 0)                - wrap text so that it does not reach beyond x. Will automatically calculate a vertical size (expand vertically).
//  * (x != 0, y <= fontHeight)  - will truncate text so it fits within this box.
class TextComponent : public GuiComponent
{
public:
	TextComponent(Window* window);
	TextComponent(Window* window, const std::string& text, const std::shared_ptr<Font>& font, unsigned int color = 0x000000FF, Alignment align = ALIGN_LEFT,
		Vector3f pos = Vector3f::Zero(), Vector2f size = Vector2f::Zero(), unsigned int bgcolor = 0x00000000);

	void setFont(const std::shared_ptr<Font>& font);
	void setUppercase(bool uppercase);
	void onSizeChanged() override;
	void setText(const std::string& text);
	void setColor(unsigned int color);
	void setHorizontalAlignment(Alignment align);
	void setVerticalAlignment(Alignment align);
	void setLineSpacing(float spacing);
	void setBackgroundColor(unsigned int color);
	void setRenderBackground(bool render);

	void render(const Transform4x4f& parentTrans) override;

	std::string getValue() const override;
	void setValue(const std::string& value) override;

	unsigned char getOpacity() const override;
	void setOpacity(unsigned char opacity) override;

	inline std::shared_ptr<Font> getFont() const { return mFont; }

	virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

protected:
	virtual void onTextChanged();

	std::string mText;
	std::shared_ptr<Font> mFont;

private:
	void calculateExtent();

	void onColorChanged();

	unsigned int mColor;
	unsigned int mBgColor;
	unsigned char mColorOpacity;
	unsigned char mBgColorOpacity;
	bool mRenderBackground;

	bool mUppercase;
	Vector2i mAutoCalcExtent;
	std::shared_ptr<TextCache> mTextCache;
	Alignment mHorizontalAlignment;
	Alignment mVerticalAlignment;
	float mLineSpacing;
};

#endif // ES_CORE_COMPONENTS_TEXT_COMPONENT_H
