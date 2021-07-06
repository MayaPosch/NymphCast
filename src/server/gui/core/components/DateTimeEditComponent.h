#pragma once
#ifndef ES_CORE_COMPONENTS_DATE_TIME_EDIT_COMPONENT_H
#define ES_CORE_COMPONENTS_DATE_TIME_EDIT_COMPONENT_H

#include "utils/TimeUtil.h"
#include "GuiComponent.h"

class TextCache;

// Used to enter or display a specific point in time.
class DateTimeEditComponent : public GuiComponent
{
public:
	enum DisplayMode
	{
		DISP_DATE,
		DISP_DATE_TIME,
		DISP_RELATIVE_TO_NOW
	};

	DateTimeEditComponent(Window* window, DisplayMode dispMode = DISP_DATE);

	void setValue(const std::string& val) override;
	std::string getValue() const override;

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;
	void onSizeChanged() override;

	// Set how the point in time will be displayed:
	//  * DISP_DATE - only display the date.
	//  * DISP_DATE_TIME - display both the date and the time on that date.
	//  * DISP_RELATIVE_TO_NOW - intelligently display the point in time relative to right now (e.g. "5 secs ago", "3 minutes ago", "1 day ago".  Automatically updates as time marches on.
	// The initial value is DISP_DATE.
	void setDisplayMode(DisplayMode mode);

	void setColor(unsigned int color); // Text color.
	void setFont(std::shared_ptr<Font> font); // Font to display with. Default is Font::get(FONT_SIZE_MEDIUM).
	void setUppercase(bool uppercase); // Force text to be uppercase when in DISP_RELATIVE_TO_NOW mode.

	virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

private:
	std::shared_ptr<Font> getFont() const;

	std::string getDisplayString(DisplayMode mode) const;
	DisplayMode getCurrentDisplayMode() const;

	void updateTextCache();

	Utils::Time::DateTime mTime;
	Utils::Time::DateTime mTimeBeforeEdit;

	bool mEditing;
	int mEditIndex;
	DisplayMode mDisplayMode;

	int mRelativeUpdateAccumulator;

	std::unique_ptr<TextCache> mTextCache;
	std::vector<Vector4f> mCursorBoxes;

	unsigned int mColor;
	std::shared_ptr<Font> mFont;
	bool mUppercase;

	bool mAutoSize;
};

#endif // ES_CORE_COMPONENTS_DATE_TIME_COMPONENT_H
