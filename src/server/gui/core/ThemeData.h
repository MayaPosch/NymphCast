#pragma once
#ifndef ES_CORE_THEME_DATA_H
#define ES_CORE_THEME_DATA_H

#include "math/Vector2f.h"
#include "math/Vector4f.h"
#include "utils/FileSystemUtil.h"
#include <deque>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

namespace pugi { class xml_node; }

template<typename T>
class TextListComponent;

class GuiComponent;
class ImageComponent;
class NinePatchComponent;
class Sound;
class TextComponent;
class Window;

namespace ThemeFlags
{
	enum PropertyFlags : unsigned int
	{
		PATH = 1,
		POSITION = 2,
		SIZE = 4,
		ORIGIN = 8,
		COLOR = 16,
		FONT_PATH = 32,
		FONT_SIZE = 64,
		SOUND = 128,
		ALIGNMENT = 256,
		TEXT = 512,
		FORCE_UPPERCASE = 1024,
		LINE_SPACING = 2048,
		DELAY = 4096,
		Z_INDEX = 8192,
		ROTATION = 16384,
		VISIBLE = 32768,
		ALL = 0xFFFFFFFF
	};
}

class ThemeException : public std::exception
{
public:
	std::string msg;

	virtual const char* what() const throw() { return msg.c_str(); }

	template<typename T>
	friend ThemeException& operator<<(ThemeException& e, T msg);

	inline void setFiles(const std::deque<std::string>& deque)
	{
		*this << "from theme \"" << deque.front() << "\"\n";
		for(auto it = deque.cbegin() + 1; it != deque.cend(); it++)
			*this << "  (from included file \"" << (*it) << "\")\n";
		*this << "    ";
	}
};

template<typename T>
ThemeException& operator<<(ThemeException& e, T appendMsg)
{
	std::stringstream ss;
	ss << e.msg << appendMsg;
	e.msg = ss.str();
	return e;
}

struct ThemeSet
{
	std::string path;

	inline std::string getName() const { return Utils::FileSystem::getStem(path); }
	inline std::string getThemePath(const std::string& system) const { return path + "/" + system + "/theme.xml"; }
};

class ThemeData
{
public:

	class ThemeElement
	{
	public:
		bool extra;
		std::string type;

		struct Property
		{
			void operator= (const Vector4f& value)     { r = value; v = Vector2f(value.x(), value.y()); }
			void operator= (const Vector2f& value)     { v = value; }
			void operator= (const std::string& value)  { s = value; }
			void operator= (const unsigned int& value) { i = value; }
			void operator= (const float& value)        { f = value; }
			void operator= (const bool& value)         { b = value; }

			Vector4f     r;
			Vector2f     v;
			std::string  s;
			unsigned int i;
			float        f;
			bool         b;
		};

		std::map< std::string, Property > properties;

		template<typename T>
		const T get(const std::string& prop) const
		{
			if(     std::is_same<T, Vector2f>::value)     return *(const T*)&properties.at(prop).v;
			else if(std::is_same<T, std::string>::value)  return *(const T*)&properties.at(prop).s;
			else if(std::is_same<T, unsigned int>::value) return *(const T*)&properties.at(prop).i;
			else if(std::is_same<T, float>::value)        return *(const T*)&properties.at(prop).f;
			else if(std::is_same<T, bool>::value)         return *(const T*)&properties.at(prop).b;
			else if(std::is_same<T, Vector4f>::value)     return *(const T*)&properties.at(prop).r;
			return T();
		}

		inline bool has(const std::string& prop) const { return (properties.find(prop) != properties.cend()); }
	};

private:
	class ThemeView
	{
	public:
		std::map<std::string, ThemeElement> elements;
		std::vector<std::string> orderedKeys;
	};

public:

	ThemeData();

	// throws ThemeException
	void loadFile(std::map<std::string, std::string> sysDataMap, const std::string& path);

	enum ElementPropertyType
	{
		RESOLUTION_RECT,
		RESOLUTION_PAIR,
		RESOLUTION_FLOAT,
		NORMALIZED_RECT,
		NORMALIZED_PAIR,
		PATH,
		STRING,
		COLOR,
		FLOAT,
		BOOLEAN
	};

	bool hasView(const std::string& view);

	// If expectedType is an empty string, will do no type checking.
	const ThemeElement* getElement(const std::string& view, const std::string& element, const std::string& expectedType) const;

	static std::vector<GuiComponent*> makeExtras(const std::shared_ptr<ThemeData>& theme, const std::string& view, Window* window);

	static const std::shared_ptr<ThemeData>& getDefault();

	static std::map<std::string, ThemeSet> getThemeSets();
	static std::string getThemeFromCurrentSet(const std::string& system);

private:
	static std::map< std::string, std::map<std::string, ElementPropertyType> > sElementMap;
	static std::vector<std::string> sSupportedFeatures;
	static std::vector<std::string> sSupportedViews;

	std::deque<std::string> mPaths;
	float mVersion;
	Vector2f mResolution;

	void parseFeatures(const pugi::xml_node& themeRoot);
	void parseIncludes(const pugi::xml_node& themeRoot);
	void parseVariables(const pugi::xml_node& root);
	void parseViews(const pugi::xml_node& themeRoot);
	void parseView(const pugi::xml_node& viewNode, ThemeView& view);
	void parseElement(const pugi::xml_node& elementNode, const std::map<std::string, ElementPropertyType>& typeMap, ThemeElement& element);

	std::map<std::string, ThemeView> mViews;

	std::string resolvePlaceholders(const char* in);
	std::map<std::string, std::string> mVariables;
};

#endif // ES_CORE_THEME_DATA_H
