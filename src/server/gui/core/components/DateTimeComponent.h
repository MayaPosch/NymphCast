#pragma once
#ifndef ES_CORE_COMPONENTS_DATE_TIME_COMPONENT_H
#define ES_CORE_COMPONENTS_DATE_TIME_COMPONENT_H

#include "utils/TimeUtil.h"
#include "TextComponent.h"

class ThemeData;

// Used to display date times.
class DateTimeComponent : public TextComponent
{
public:
	DateTimeComponent(Window* window);
	DateTimeComponent(Window* window, const std::string& text, const std::shared_ptr<Font>& font, unsigned int color = 0x000000FF, Alignment align = ALIGN_LEFT,
		Vector3f pos = Vector3f::Zero(), Vector2f size = Vector2f::Zero(), unsigned int bgcolor = 0x00000000);

	void render(const Transform4x4f& parentTrans) override;

	void setValue(const std::string& val) override;
	std::string getValue() const override;

	void setFormat(const std::string& format);
	void setDisplayRelative(bool displayRelative);

	virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

protected:
	void onTextChanged() override;

private:
	std::string getDisplayString() const;

	Utils::Time::DateTime mTime;
	std::string mFormat;
	bool mDisplayRelative;
};

#endif // ES_CORE_COMPONENTS_DATE_TIME_COMPONENT_H
