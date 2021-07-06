#include "components/DateTimeEditComponent.h"

#include "resources/Font.h"
#include "utils/StringUtil.h"

DateTimeEditComponent::DateTimeEditComponent(Window* window, DisplayMode dispMode) : GuiComponent(window),
	mEditing(false), mEditIndex(0), mDisplayMode(dispMode), mRelativeUpdateAccumulator(0),
	mColor(0x777777FF), mFont(Font::get(FONT_SIZE_SMALL, FONT_PATH_LIGHT)), mUppercase(false), mAutoSize(true)
{
	updateTextCache();
}

void DateTimeEditComponent::setDisplayMode(DisplayMode mode)
{
	mDisplayMode = mode;
	updateTextCache();
}

bool DateTimeEditComponent::input(InputConfig* config, Input input)
{
	if(input.value == 0)
		return false;

	if(config->isMappedTo("a", input))
	{
		if(mDisplayMode != DISP_RELATIVE_TO_NOW) //don't allow editing for relative times
			mEditing = !mEditing;

		if(mEditing)
		{
			//started editing
			mTimeBeforeEdit = mTime;

			//initialize to now if unset
			if(mTime.getTime() == Utils::Time::NOT_A_DATE_TIME)
			{
				mTime = Utils::Time::now();
				updateTextCache();
			}
		}

		return true;
	}

	if(mEditing)
	{
		if(config->isMappedTo("b", input))
		{
			mEditing = false;
			mTime = mTimeBeforeEdit;
			updateTextCache();
			return true;
		}

		int incDir = 0;
		if(config->isMappedLike("up", input) || config->isMappedLike("leftshoulder", input))
			incDir = 1;
		else if(config->isMappedLike("down", input) || config->isMappedLike("rightshoulder", input))
			incDir = -1;

		if(incDir != 0)
		{
			tm new_tm = mTime;

			if(mEditIndex == 0)
			{
				new_tm.tm_mon += incDir;

				if(new_tm.tm_mon > 11)
					new_tm.tm_mon = 0;
				else if(new_tm.tm_mon < 0)
					new_tm.tm_mon = 11;

			}
			else if(mEditIndex == 1)
			{
				const int days_in_month = Utils::Time::daysInMonth(new_tm.tm_year + 1900, new_tm.tm_mon + 1);
				new_tm.tm_mday += incDir;

				if(new_tm.tm_mday > days_in_month)
					new_tm.tm_mday = 1;
				else if(new_tm.tm_mday < 1)
					new_tm.tm_mday = days_in_month;

			}
			else if(mEditIndex == 2)
			{
				new_tm.tm_year += incDir;

				if(new_tm.tm_year < 0)
					new_tm.tm_year = 0;
			}

			//validate day
			const int days_in_month = Utils::Time::daysInMonth(new_tm.tm_year + 1900, new_tm.tm_mon + 1);
			if(new_tm.tm_mday > days_in_month)
				new_tm.tm_mday = days_in_month;

			mTime = new_tm;

			updateTextCache();
			return true;
		}

		if(config->isMappedLike("right", input))
		{
			mEditIndex++;
			if(mEditIndex >= (int)mCursorBoxes.size())
				mEditIndex--;
			return true;
		}

		if(config->isMappedLike("left", input))
		{
			mEditIndex--;
			if(mEditIndex < 0)
				mEditIndex++;
			return true;
		}
	}

	return GuiComponent::input(config, input);
}

void DateTimeEditComponent::update(int deltaTime)
{
	if(mDisplayMode == DISP_RELATIVE_TO_NOW)
	{
		mRelativeUpdateAccumulator += deltaTime;
		if(mRelativeUpdateAccumulator > 1000)
		{
			mRelativeUpdateAccumulator = 0;
			updateTextCache();
		}
	}

	GuiComponent::update(deltaTime);
}

void DateTimeEditComponent::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();

	if(mTextCache)
	{
		// vertically center
		Vector3f off(0, (mSize.y() - mTextCache->metrics.size.y()) / 2, 0);
		trans.translate(off);

		Renderer::setMatrix(trans);

		std::shared_ptr<Font> font = getFont();

		mTextCache->setColor((mColor & 0xFFFFFF00) | getOpacity());
		font->renderTextCache(mTextCache.get());

		if(mEditing)
		{
			if(mEditIndex >= 0 && (unsigned int)mEditIndex < mCursorBoxes.size())
			{
				Renderer::drawRect(mCursorBoxes[mEditIndex][0], mCursorBoxes[mEditIndex][1],
					mCursorBoxes[mEditIndex][2], mCursorBoxes[mEditIndex][3], 0x00000022, 0x00000022);
			}
		}
	}
}

void DateTimeEditComponent::setValue(const std::string& val)
{
	mTime = val;
	updateTextCache();
}

std::string DateTimeEditComponent::getValue() const
{
	return mTime;
}

DateTimeEditComponent::DisplayMode DateTimeEditComponent::getCurrentDisplayMode() const
{
	/*if(mEditing)
	{
		if(mDisplayMode == DISP_RELATIVE_TO_NOW)
		{
			//TODO: if time component == 00:00:00, return DISP_DATE, else return DISP_DATE_TIME
			return DISP_DATE;
		}
	}*/

	return mDisplayMode;
}

std::string DateTimeEditComponent::getDisplayString(DisplayMode mode) const
{
	std::string fmt;
	switch(mode)
	{
	case DISP_DATE:
		fmt = "%m/%d/%Y";
		break;
	case DISP_DATE_TIME:
		if(mTime.getTime() == 0)
			return "unknown";
		fmt = "%m/%d/%Y %H:%M:%S";
		break;
	case DISP_RELATIVE_TO_NOW:
		{
			//relative time
			if(mTime.getTime() == 0)
				return "never";

			Utils::Time::DateTime now(Utils::Time::now());
			Utils::Time::Duration dur(now.getTime() - mTime.getTime());

			char buf[64];

			if(dur.getDays() > 0)
				sprintf(buf, "%d day%s ago", dur.getDays(), (dur.getDays() > 1) ? "s" : "");
			else if(dur.getHours() > 0)
				sprintf(buf, "%d hour%s ago", dur.getHours(), (dur.getHours() > 1) ? "s" : "");
			else if(dur.getMinutes() > 0)
				sprintf(buf, "%d minute%s ago", dur.getMinutes(), (dur.getMinutes() > 1) ? "s" : "");
			else
				sprintf(buf, "%d second%s ago", dur.getSeconds(), (dur.getSeconds() > 1) ? "s" : "");

			return std::string(buf);
		}
		break;
	}

	return Utils::Time::timeToString(mTime, fmt);
}

std::shared_ptr<Font> DateTimeEditComponent::getFont() const
{
	if(mFont)
		return mFont;

	return Font::get(FONT_SIZE_MEDIUM);
}

void DateTimeEditComponent::updateTextCache()
{
	DisplayMode mode = getCurrentDisplayMode();
	const std::string dispString = mUppercase ? Utils::String::toUpper(getDisplayString(mode)) : getDisplayString(mode);
	std::shared_ptr<Font> font = getFont();
	mTextCache = std::unique_ptr<TextCache>(font->buildTextCache(dispString, 0, 0, mColor));

	if(mAutoSize)
	{
		mSize = mTextCache->metrics.size;

		mAutoSize = false;
		if(getParent())
			getParent()->onSizeChanged();
	}

	//set up cursor positions
	mCursorBoxes.clear();

	if(dispString.empty() || mode == DISP_RELATIVE_TO_NOW)
		return;

	//month
	Vector2f start(0, 0);
	Vector2f end = font->sizeText(dispString.substr(0, 2));
	Vector2f diff = end - start;
	mCursorBoxes.push_back(Vector4f(start[0], start[1], diff[0], diff[1]));

	//day
	start[0] = font->sizeText(dispString.substr(0, 3)).x();
	end = font->sizeText(dispString.substr(0, 5));
	diff = end - start;
	mCursorBoxes.push_back(Vector4f(start[0], start[1], diff[0], diff[1]));

	//year
	start[0] = font->sizeText(dispString.substr(0, 6)).x();
	end = font->sizeText(dispString.substr(0, 10));
	diff = end - start;
	mCursorBoxes.push_back(Vector4f(start[0], start[1], diff[0], diff[1]));

	//if mode == DISP_DATE_TIME do times too but I don't wanna do the logic for editing times because no one will ever use it so screw it
}

void DateTimeEditComponent::setColor(unsigned int color)
{
	mColor = color;
	if(mTextCache)
		mTextCache->setColor(color);
}

void DateTimeEditComponent::setFont(std::shared_ptr<Font> font)
{
	mFont = font;
	updateTextCache();
}

void DateTimeEditComponent::onSizeChanged()
{
	mAutoSize = false;
	updateTextCache();
}

void DateTimeEditComponent::setUppercase(bool uppercase)
{
	mUppercase = uppercase;
	updateTextCache();
}

void DateTimeEditComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	const ThemeData::ThemeElement* elem = theme->getElement(view, element, "datetime");
	if(!elem)
		return;

	// We set mAutoSize BEFORE calling GuiComponent::applyTheme because it calls
	// setSize(), which will call updateTextCache(), which will reset mSize if
	// mAutoSize == true, ignoring the theme's value.
	if(properties & ThemeFlags::SIZE)
		mAutoSize = !elem->has("size");

	GuiComponent::applyTheme(theme, view, element, properties);

	using namespace ThemeFlags;

	if(properties & COLOR && elem->has("color"))
		setColor(elem->get<unsigned int>("color"));

	if(properties & FORCE_UPPERCASE && elem->has("forceUppercase"))
		setUppercase(elem->get<bool>("forceUppercase"));

	setFont(Font::getFromTheme(elem, properties, mFont));
}
