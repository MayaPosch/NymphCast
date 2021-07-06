#include "resources/Font.h"

#include "renderers/Renderer.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Log.h"

#ifdef WIN32
#include <Windows.h>
#endif

FT_Library Font::sLibrary = NULL;

int Font::getSize() const { return mSize; }

std::map< std::pair<std::string, int>, std::weak_ptr<Font> > Font::sFontMap;

Font::FontFace::FontFace(ResourceData&& d, int size) : data(d)
{
	int err = FT_New_Memory_Face(sLibrary, data.ptr.get(), (FT_Long)data.length, 0, &face);
	assert(!err);

	if(!err)
		FT_Set_Pixel_Sizes(face, 0, size);
}

Font::FontFace::~FontFace()
{
	if(face)
		FT_Done_Face(face);
}

void Font::initLibrary()
{
	assert(sLibrary == NULL);
	if(FT_Init_FreeType(&sLibrary))
	{
		sLibrary = NULL;
		LOG(LogError) << "Error initializing FreeType!";
	}
}

size_t Font::getMemUsage() const
{
	size_t memUsage = 0;
	for(auto it = mTextures.cbegin(); it != mTextures.cend(); it++)
		memUsage += it->textureSize.x() * it->textureSize.y() * 4;

	for(auto it = mFaceCache.cbegin(); it != mFaceCache.cend(); it++)
		memUsage += it->second->data.length;

	return memUsage;
}

size_t Font::getTotalMemUsage()
{
	size_t total = 0;

	auto it = sFontMap.cbegin();
	while(it != sFontMap.cend())
	{
		if(it->second.expired())
		{
			it = sFontMap.erase(it);
			continue;
		}

		total += it->second.lock()->getMemUsage();
		it++;
	}

	return total;
}

Font::Font(int size, const std::string& path) : mSize(size), mPath(path)
{
	assert(mSize > 0);

	mLoaded = true;
	mMaxGlyphHeight = 0;

	if(!sLibrary)
		initLibrary();

	// always initialize ASCII characters
	for(unsigned int i = 32; i < 128; i++)
		getGlyph(i);

	clearFaceCache();
}

Font::~Font()
{
	unload();
}

void Font::reload()
{
	if (mLoaded)
		return;

	rebuildTextures();
	mLoaded = true;
}

bool Font::unload()
{
	if (mLoaded)
	{
		unloadTextures();
		mLoaded = false;
		return true;
	}

	return false;
}

std::shared_ptr<Font> Font::get(int size, const std::string& path)
{
	const std::string canonicalPath = Utils::FileSystem::getCanonicalPath(path);

	std::pair<std::string, int> def(canonicalPath.empty() ? getDefaultPath() : canonicalPath, size);
	auto foundFont = sFontMap.find(def);
	if(foundFont != sFontMap.cend())
	{
		if(!foundFont->second.expired())
			return foundFont->second.lock();
	}

	std::shared_ptr<Font> font = std::shared_ptr<Font>(new Font(def.second, def.first));
	sFontMap[def] = std::weak_ptr<Font>(font);
	ResourceManager::getInstance()->addReloadable(font);
	return font;
}

void Font::unloadTextures()
{
	for(auto it = mTextures.begin(); it != mTextures.end(); it++)
	{
		it->deinitTexture();
	}
}

Font::FontTexture::FontTexture()
{
	textureId = 0;
	textureSize = Vector2i(2048, 512);
	writePos = Vector2i::Zero();
	rowHeight = 0;
}

Font::FontTexture::~FontTexture()
{
	deinitTexture();
}

bool Font::FontTexture::findEmpty(const Vector2i& size, Vector2i& cursor_out)
{
	if(size.x() >= textureSize.x() || size.y() >= textureSize.y())
		return false;

	if(writePos.x() + size.x() >= textureSize.x() &&
		writePos.y() + rowHeight + size.y() + 1 < textureSize.y())
	{
		// row full, but it should fit on the next row
		// move cursor to next row
		writePos = Vector2i(0, writePos.y() + rowHeight + 1); // leave 1px of space between glyphs
		rowHeight = 0;
	}

	if(writePos.x() + size.x() >= textureSize.x() ||
		writePos.y() + size.y() >= textureSize.y())
	{
		// nope, still won't fit
		return false;
	}

	cursor_out = writePos;
	writePos[0] += size.x() + 1; // leave 1px of space between glyphs

	if(size.y() > rowHeight)
		rowHeight = size.y();

	return true;
}

void Font::FontTexture::initTexture()
{
	assert(textureId == 0);
	textureId = Renderer::createTexture(Renderer::Texture::ALPHA, false, false, textureSize.x(), textureSize.y(), nullptr);
}

void Font::FontTexture::deinitTexture()
{
	if(textureId != 0)
	{
		Renderer::destroyTexture(textureId);
		textureId = 0;
	}
}

void Font::getTextureForNewGlyph(const Vector2i& glyphSize, FontTexture*& tex_out, Vector2i& cursor_out)
{
	if(mTextures.size())
	{
		// check if the most recent texture has space
		tex_out = &mTextures.back();

		// will this one work?
		if(tex_out->findEmpty(glyphSize, cursor_out))
			return; // yes
	}

	// current textures are full,
	// make a new one
	mTextures.push_back(FontTexture());
	tex_out = &mTextures.back();
	tex_out->initTexture();

	bool ok = tex_out->findEmpty(glyphSize, cursor_out);
	if(!ok)
	{
		LOG(LogError) << "Glyph too big to fit on a new texture (glyph size > " << tex_out->textureSize.x() << ", " << tex_out->textureSize.y() << ")!";
		tex_out = NULL;
	}
}

std::vector<std::string> getFallbackFontPaths()
{
#ifdef WIN32
	// Windows

	// get this system's equivalent of "C:\Windows" (might be on a different drive or in a different folder)
	// so we can check the Fonts subdirectory for fallback fonts
	TCHAR winDir[MAX_PATH];
	GetWindowsDirectory(winDir, MAX_PATH);
	std::string fontDir = winDir;
	fontDir += "\\Fonts\\";

	const char* fontNames[] = {
		"meiryo.ttc", // japanese
		"simhei.ttf", // chinese
		"arial.ttf"   // latin
	};

	//prepend to font file names
	std::vector<std::string> fontPaths;
	fontPaths.reserve(sizeof(fontNames) / sizeof(fontNames[0]));

	for(unsigned int i = 0; i < sizeof(fontNames) / sizeof(fontNames[0]); i++)
	{
		std::string path = fontDir + fontNames[i];
		if(ResourceManager::getInstance()->fileExists(path))
			fontPaths.push_back(path);
	}

	fontPaths.shrink_to_fit();
	return fontPaths;

#else
	// Linux

	const char* paths[] = {
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/truetype/freefont/FreeMono.ttf",
		"/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf" // japanese, chinese, present on Debian
	};

	std::vector<std::string> fontPaths;
	for(unsigned int i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
	{
		if(ResourceManager::getInstance()->fileExists(paths[i]))
			fontPaths.push_back(paths[i]);
	}

	fontPaths.shrink_to_fit();
	return fontPaths;

#endif
}

FT_Face Font::getFaceForChar(unsigned int id)
{
	static const std::vector<std::string> fallbackFonts = getFallbackFontPaths();

	// look through our current font + fallback fonts to see if any have the glyph we're looking for
	for(unsigned int i = 0; i < fallbackFonts.size() + 1; i++)
	{
		auto fit = mFaceCache.find(i);

		if(fit == mFaceCache.cend()) // doesn't exist yet
		{
			// i == 0 -> mPath
			// otherwise, take from fallbackFonts
			const std::string& path = (i == 0 ? mPath : fallbackFonts.at(i - 1));
			ResourceData data = ResourceManager::getInstance()->getFileData(path);
			mFaceCache[i] = std::unique_ptr<FontFace>(new FontFace(std::move(data), mSize));
			fit = mFaceCache.find(i);
		}

		if(FT_Get_Char_Index(fit->second->face, id) != 0)
			return fit->second->face;
	}

	// nothing has a valid glyph - return the "real" face so we get a "missing" character
	return mFaceCache.cbegin()->second->face;
}

void Font::clearFaceCache()
{
	mFaceCache.clear();
}

Font::Glyph* Font::getGlyph(unsigned int id)
{
	// is it already loaded?
	auto it = mGlyphMap.find(id);
	if(it != mGlyphMap.cend())
		return &it->second;

	// nope, need to make a glyph
	FT_Face face = getFaceForChar(id);
	if(!face)
	{
		LOG(LogError) << "Could not find appropriate font face for character " << id << " for font " << mPath;
		return NULL;
	}

	FT_GlyphSlot g = face->glyph;

	if(FT_Load_Char(face, id, FT_LOAD_RENDER))
	{
		LOG(LogError) << "Could not find glyph for character " << id << " for font " << mPath << ", size " << mSize << "!";
		return NULL;
	}

	Vector2i glyphSize(g->bitmap.width, g->bitmap.rows);

	FontTexture* tex = NULL;
	Vector2i cursor;
	getTextureForNewGlyph(glyphSize, tex, cursor);

	// getTextureForNewGlyph can fail if the glyph is bigger than the max texture size (absurdly large font size)
	if(tex == NULL)
	{
		LOG(LogError) << "Could not create glyph for character " << id << " for font " << mPath << ", size " << mSize << " (no suitable texture found)!";
		return NULL;
	}

	// create glyph
	Glyph& glyph = mGlyphMap[id];

	glyph.texture = tex;
	glyph.texPos = Vector2f(cursor.x() / (float)tex->textureSize.x(), cursor.y() / (float)tex->textureSize.y());
	glyph.texSize = Vector2f(glyphSize.x() / (float)tex->textureSize.x(), glyphSize.y() / (float)tex->textureSize.y());

	glyph.advance = Vector2f((float)g->metrics.horiAdvance / 64.0f, (float)g->metrics.vertAdvance / 64.0f);
	glyph.bearing = Vector2f((float)g->metrics.horiBearingX / 64.0f, (float)g->metrics.horiBearingY / 64.0f);

	// upload glyph bitmap to texture
	Renderer::updateTexture(tex->textureId, Renderer::Texture::ALPHA, cursor.x(), cursor.y(), glyphSize.x(), glyphSize.y(), g->bitmap.buffer);

	// update max glyph height
	if(glyphSize.y() > mMaxGlyphHeight)
		mMaxGlyphHeight = glyphSize.y();

	// done
	return &glyph;
}

// completely recreate the texture data for all textures based on mGlyphs information
void Font::rebuildTextures()
{
	// recreate OpenGL textures
	for(auto it = mTextures.begin(); it != mTextures.end(); it++)
	{
		it->initTexture();
	}

	// reupload the texture data
	for(auto it = mGlyphMap.cbegin(); it != mGlyphMap.cend(); it++)
	{
		FT_Face face = getFaceForChar(it->first);
		FT_GlyphSlot glyphSlot = face->glyph;

		// load the glyph bitmap through FT
		FT_Load_Char(face, it->first, FT_LOAD_RENDER);

		FontTexture* tex = it->second.texture;

		// find the position/size
		Vector2i cursor((int)(it->second.texPos.x() * tex->textureSize.x()), (int)(it->second.texPos.y() * tex->textureSize.y()));
		Vector2i glyphSize((int)(it->second.texSize.x() * tex->textureSize.x()), (int)(it->second.texSize.y() * tex->textureSize.y()));

		// upload to texture
		Renderer::updateTexture(tex->textureId, Renderer::Texture::ALPHA, cursor.x(), cursor.y(), glyphSize.x(), glyphSize.y(), glyphSlot->bitmap.buffer);
	}
}

void Font::renderTextCache(TextCache* cache)
{
	if(cache == NULL)
	{
		LOG(LogError) << "Attempted to draw NULL TextCache!";
		return;
	}

	for(auto it = cache->vertexLists.cbegin(); it != cache->vertexLists.cend(); it++)
	{
		assert(*it->textureIdPtr != 0);

		auto vertexList = *it;

		Renderer::bindTexture(*it->textureIdPtr);
		Renderer::drawTriangleStrips(&it->verts[0], (int)it->verts.size());
	}
}

Vector2f Font::sizeText(std::string text, float lineSpacing)
{
	float lineWidth = 0.0f;
	float highestWidth = 0.0f;

	const float lineHeight = getHeight(lineSpacing);

	float y = lineHeight;

	size_t i = 0;
	while(i < text.length())
	{
		unsigned int character = Utils::String::chars2Unicode(text, i); // advances i

		if(character == '\n')
		{
			if(lineWidth > highestWidth)
				highestWidth = lineWidth;

			lineWidth = 0.0f;
			y += lineHeight;
		}

		Glyph* glyph = getGlyph(character);
		if(glyph)
			lineWidth += glyph->advance.x();
	}

	if(lineWidth > highestWidth)
		highestWidth = lineWidth;

	return Vector2f(highestWidth, y);
}

float Font::getHeight(float lineSpacing) const
{
	return mMaxGlyphHeight * lineSpacing;
}

float Font::getLetterHeight()
{
	Glyph* glyph = getGlyph('S');
	assert(glyph);
	return glyph->texSize.y() * glyph->texture->textureSize.y();
}

//the worst algorithm ever written
//breaks up a normal string with newlines to make it fit xLen
std::string Font::wrapText(std::string text, float xLen)
{
	std::string out;

	std::string line, word, temp;
	size_t space;

	Vector2f textSize;

	while(text.length() > 0) //while there's text or we still have text to render
	{
		space = text.find_first_of(" \t\n");
		if(space == std::string::npos)
			space = text.length() - 1;

		word = text.substr(0, space + 1);
		text.erase(0, space + 1);

		temp = line + word;

		textSize = sizeText(temp);

		// if the word will fit on the line, add it to our line, and continue
		if(textSize.x() <= xLen)
		{
			line = temp;
			continue;
		}else{
			// the next word won't fit, so break here
			out += line + '\n';
			line = word;
		}
	}

	// whatever's left should fit
	out += line;

	return out;
}

Vector2f Font::sizeWrappedText(std::string text, float xLen, float lineSpacing)
{
	text = wrapText(text, xLen);
	return sizeText(text, lineSpacing);
}

Vector2f Font::getWrappedTextCursorOffset(std::string text, float xLen, size_t stop, float lineSpacing)
{
	std::string wrappedText = wrapText(text, xLen);

	float lineWidth = 0.0f;
	float y = 0.0f;

	size_t wrapCursor = 0;
	size_t cursor = 0;
	while(cursor < stop)
	{
		unsigned int wrappedCharacter = Utils::String::chars2Unicode(wrappedText, wrapCursor);
		unsigned int character = Utils::String::chars2Unicode(text, cursor);

		if(wrappedCharacter == '\n' && character != '\n')
		{
			//this is where the wordwrap inserted a newline
			//reset lineWidth and increment y, but don't consume a cursor character
			lineWidth = 0.0f;
			y += getHeight(lineSpacing);

			cursor = Utils::String::prevCursor(text, cursor); // unconsume
			continue;
		}

		if(character == '\n')
		{
			lineWidth = 0.0f;
			y += getHeight(lineSpacing);
			continue;
		}

		Glyph* glyph = getGlyph(character);
		if(glyph)
			lineWidth += glyph->advance.x();
	}

	return Vector2f(lineWidth, y);
}

//=============================================================================================================
//TextCache
//=============================================================================================================

float Font::getNewlineStartOffset(const std::string& text, const unsigned int& charStart, const float& xLen, const Alignment& alignment)
{
	switch(alignment)
	{
	case ALIGN_LEFT:
		return 0;
	case ALIGN_CENTER:
		{
			unsigned int endChar = (unsigned int)text.find('\n', charStart);
			return (xLen - sizeText(text.substr(charStart, endChar != std::string::npos ? endChar - charStart : endChar)).x()) / 2.0f;
		}
	case ALIGN_RIGHT:
		{
			unsigned int endChar = (unsigned int)text.find('\n', charStart);
			return xLen - (sizeText(text.substr(charStart, endChar != std::string::npos ? endChar - charStart : endChar)).x());
		}
	default:
		return 0;
	}
}

TextCache* Font::buildTextCache(const std::string& text, Vector2f offset, unsigned int color, float xLen, Alignment alignment, float lineSpacing)
{
	float x = offset[0] + (xLen != 0 ? getNewlineStartOffset(text, 0, xLen, alignment) : 0);

	float yTop = getGlyph('S')->bearing.y();
	float yBot = getHeight(lineSpacing);
	float y = offset[1] + (yBot + yTop)/2.0f;

	// vertices by texture
	std::map< FontTexture*, std::vector<Renderer::Vertex> > vertMap;

	size_t cursor = 0;
	while(cursor < text.length())
	{
		unsigned int character = Utils::String::chars2Unicode(text, cursor); // also advances cursor
		Glyph* glyph;

		// invalid character
		if(character == 0)
			continue;

		if(character == '\n')
		{
			y += getHeight(lineSpacing);
			x = offset[0] + (xLen != 0 ? getNewlineStartOffset(text, (const unsigned int)cursor /* cursor is already advanced */, xLen, alignment) : 0);
			continue;
		}

		glyph = getGlyph(character);
		if(glyph == NULL)
			continue;

		std::vector<Renderer::Vertex>& verts = vertMap[glyph->texture];
		size_t oldVertSize = verts.size();
		verts.resize(oldVertSize + 6);
		Renderer::Vertex* vertices = verts.data() + oldVertSize;

		const float        glyphStartX    = x + glyph->bearing.x();
		const Vector2i&    textureSize    = glyph->texture->textureSize;
		const unsigned int convertedColor = Renderer::convertColor(color);

		vertices[1] = { { glyphStartX                                       , y - glyph->bearing.y()                                          }, { glyph->texPos.x(),                      glyph->texPos.y()                      }, convertedColor };
		vertices[2] = { { glyphStartX                                       , y - glyph->bearing.y() + (glyph->texSize.y() * textureSize.y()) }, { glyph->texPos.x(),                      glyph->texPos.y() + glyph->texSize.y() }, convertedColor };
		vertices[3] = { { glyphStartX + glyph->texSize.x() * textureSize.x(), y - glyph->bearing.y()                                          }, { glyph->texPos.x() + glyph->texSize.x(), glyph->texPos.y()                      }, convertedColor };
		vertices[4] = { { glyphStartX + glyph->texSize.x() * textureSize.x(), y - glyph->bearing.y() + (glyph->texSize.y() * textureSize.y()) }, { glyph->texPos.x() + glyph->texSize.x(), glyph->texPos.y() + glyph->texSize.y() }, convertedColor };

		// round vertices
		for(int i = 1; i < 5; ++i)
			vertices[i].pos.round();

		// make duplicates of first and last vertex so this can be rendered as a triangle strip
		vertices[0] = vertices[1];
		vertices[5] = vertices[4];

		// advance
		x += glyph->advance.x();
	}

	//TextCache::CacheMetrics metrics = { sizeText(text, lineSpacing) };

	TextCache* cache = new TextCache();
	cache->vertexLists.resize(vertMap.size());
	cache->metrics = { sizeText(text, lineSpacing) };

	unsigned int i = 0;
	for(auto it = vertMap.cbegin(); it != vertMap.cend(); it++)
	{
		TextCache::VertexList& vertList = cache->vertexLists.at(i);

		vertList.textureIdPtr = &it->first->textureId;
		vertList.verts = it->second;
	}

	clearFaceCache();

	return cache;
}

TextCache* Font::buildTextCache(const std::string& text, float offsetX, float offsetY, unsigned int color)
{
	return buildTextCache(text, Vector2f(offsetX, offsetY), color, 0.0f);
}

void TextCache::setColor(unsigned int color)
{
	const unsigned int convertedColor = Renderer::convertColor(color);

	for(auto it = vertexLists.begin(); it != vertexLists.end(); it++)
		for(auto it2 = it->verts.begin(); it2 != it->verts.end(); it2++)
			it2->col = convertedColor;
}

std::shared_ptr<Font> Font::getFromTheme(const ThemeData::ThemeElement* elem, unsigned int properties, const std::shared_ptr<Font>& orig)
{
	using namespace ThemeFlags;
	if(!(properties & FONT_PATH) && !(properties & FONT_SIZE))
		return orig;

	std::shared_ptr<Font> font;
	int size = (orig ? orig->mSize : FONT_SIZE_MEDIUM);
	std::string path = (orig ? orig->mPath : getDefaultPath());

	float sh = (float)Renderer::getScreenHeight();
	if(properties & FONT_SIZE && elem->has("fontSize"))
		size = (int)(sh * elem->get<float>("fontSize"));
	if(properties & FONT_PATH && elem->has("fontPath"))
		path = elem->get<std::string>("fontPath");

	return get(size, path);
}
