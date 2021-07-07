#include "renderers/Renderer.h"

#include "math/Transform4x4f.h"
#include "math/Vector2i.h"
#include "resources/ResourceManager.h"
#include "ImageIO.h"
#include "Log.h"
#include "Settings.h"

#include <SDL.h>
#include <stack>

//////////////////////////////////////////////////////////////////////////

namespace Renderer
{
	static std::stack<Rect> clipStack;
	static SDL_Window*      sdlWindow          = nullptr;
	static int              windowWidth        = 0;
	static int              windowHeight       = 0;
	static int              screenWidth        = 0;
	static int              screenHeight       = 0;
	static int              screenOffsetX      = 0;
	static int              screenOffsetY      = 0;
	static int              screenRotate       = 0;
	static bool             initialCursorState = 1;

//////////////////////////////////////////////////////////////////////////

	static void setIcon()
	{
		size_t                     width   = 0;
		size_t                     height  = 0;
		const ResourceData         resData = ResourceManager::getInstance()->getFileData(":/window_icon_256.png");
		std::vector<unsigned char> rawData = ImageIO::loadFromMemoryRGBA32(resData.ptr.get(), resData.length, width, height);

		if(!rawData.empty())
		{
			ImageIO::flipPixelsVert(rawData.data(), width, height);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			const unsigned int rmask = 0xFF000000;
			const unsigned int gmask = 0x00FF0000;
			const unsigned int bmask = 0x0000FF00;
			const unsigned int amask = 0x000000FF;
#else
			const unsigned int rmask = 0x000000FF;
			const unsigned int gmask = 0x0000FF00;
			const unsigned int bmask = 0x00FF0000;
			const unsigned int amask = 0xFF000000;
#endif
			// try creating SDL surface from logo data
			SDL_Surface* logoSurface = SDL_CreateRGBSurfaceFrom((void*)rawData.data(), (int)width, (int)height, 32, (int)(width * 4), rmask, gmask, bmask, amask);

			if(logoSurface != nullptr)
			{
				SDL_SetWindowIcon(sdlWindow, logoSurface);
				SDL_FreeSurface(logoSurface);
			}
		}

	} // setIcon

//////////////////////////////////////////////////////////////////////////

	static bool createWindow()
	{
		LOG(LogInfo) << "Creating window...";

		if(SDL_Init(SDL_INIT_VIDEO) != 0)
		{
			LOG(LogError) << "Error initializing SDL!\n	" << SDL_GetError();
			return false;
		}

		initialCursorState = (SDL_ShowCursor(0) != 0);

		SDL_DisplayMode dispMode;
		SDL_GetDesktopDisplayMode(0, &dispMode);
		windowWidth   = Settings::getInstance()->getInt("WindowWidth")   ? Settings::getInstance()->getInt("WindowWidth")   : dispMode.w;
		windowHeight  = Settings::getInstance()->getInt("WindowHeight")  ? Settings::getInstance()->getInt("WindowHeight")  : dispMode.h;
		screenWidth   = Settings::getInstance()->getInt("ScreenWidth")   ? Settings::getInstance()->getInt("ScreenWidth")   : windowWidth;
		screenHeight  = Settings::getInstance()->getInt("ScreenHeight")  ? Settings::getInstance()->getInt("ScreenHeight")  : windowHeight;
		screenOffsetX = Settings::getInstance()->getInt("ScreenOffsetX") ? Settings::getInstance()->getInt("ScreenOffsetX") : 0;
		screenOffsetY = Settings::getInstance()->getInt("ScreenOffsetY") ? Settings::getInstance()->getInt("ScreenOffsetY") : 0;
		screenRotate  = Settings::getInstance()->getInt("ScreenRotate")  ? Settings::getInstance()->getInt("ScreenRotate")  : 0;

		setupWindow();

		const unsigned int windowFlags = (Settings::getInstance()->getBool("Windowed") ? 0 : (Settings::getInstance()->getBool("FullscreenBorderless") ? SDL_WINDOW_BORDERLESS : SDL_WINDOW_FULLSCREEN)) | getWindowFlags();

		if((sdlWindow = SDL_CreateWindow("EmulationStation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, windowFlags)) == nullptr)
		{
			LOG(LogError) << "Error creating SDL window!\n\t" << SDL_GetError();
			return false;
		}

		LOG(LogInfo) << "Created window successfully.";

		createContext();
		setIcon();
		setSwapInterval();

		return true;

	} // createWindow

//////////////////////////////////////////////////////////////////////////

	static void destroyWindow()
	{
		destroyContext();

		SDL_DestroyWindow(sdlWindow);
		sdlWindow = nullptr;

		SDL_ShowCursor(initialCursorState);

		SDL_Quit();

	} // destroyWindow

//////////////////////////////////////////////////////////////////////////

	bool init()
	{
		if(!createWindow())
			return false;

		Transform4x4f projection = Transform4x4f::Identity();
		Rect          viewport   = Rect(0, 0, 0, 0);

		switch(screenRotate)
		{
			case 0:
			{
				viewport.x = screenOffsetX;
				viewport.y = screenOffsetY;
				viewport.w = screenWidth;
				viewport.h = screenHeight;

				projection.orthoProjection(0, screenWidth, screenHeight, 0, -1.0, 1.0);
			}
			break;

			case 1:
			{
				viewport.x = windowWidth - screenOffsetY - screenHeight;
				viewport.y = screenOffsetX;
				viewport.w = screenHeight;
				viewport.h = screenWidth;

				projection.orthoProjection(0, screenHeight, screenWidth, 0, -1.0, 1.0);
				projection.rotate((float)ES_DEG_TO_RAD(90), {0, 0, 1});
				projection.translate({0, screenHeight * -1.0f, 0});
			}
			break;

			case 2:
			{
				viewport.x = windowWidth  - screenOffsetX - screenWidth;
				viewport.y = windowHeight - screenOffsetY - screenHeight;
				viewport.w = screenWidth;
				viewport.h = screenHeight;

				projection.orthoProjection(0, screenWidth, screenHeight, 0, -1.0, 1.0);
				projection.rotate((float)ES_DEG_TO_RAD(180), {0, 0, 1});
				projection.translate({screenWidth * -1.0f, screenHeight * -1.0f, 0});
			}
			break;

			case 3:
			{
				viewport.x = screenOffsetY;
				viewport.y = windowHeight - screenOffsetX - screenWidth;
				viewport.w = screenHeight;
				viewport.h = screenWidth;

				projection.orthoProjection(0, screenHeight, screenWidth, 0, -1.0, 1.0);
				projection.rotate((float)ES_DEG_TO_RAD(270), {0, 0, 1});
				projection.translate({screenWidth * -1.0f, 0, 0});
			}
			break;
		}

		setViewport(viewport);
		setProjection(projection);
		swapBuffers();

		return true;

	} // init

//////////////////////////////////////////////////////////////////////////

	void deinit()
	{
		destroyWindow();

	} // deinit

//////////////////////////////////////////////////////////////////////////

	void pushClipRect(const Vector2i& _pos, const Vector2i& _size)
	{
		Rect box(_pos.x(), _pos.y(), _size.x(), _size.y());

		if(box.w == 0) box.w = screenWidth  - box.x;
		if(box.h == 0) box.h = screenHeight - box.y;

		switch(screenRotate)
		{
			case 0: { box = Rect(screenOffsetX + box.x,                       screenOffsetY + box.y,                        box.w, box.h); } break;
			case 1: { box = Rect(windowWidth - screenOffsetY - box.y - box.h, screenOffsetX + box.x,                        box.h, box.w); } break;
			case 2: { box = Rect(windowWidth - screenOffsetX - box.x - box.w, windowHeight - screenOffsetY - box.y - box.h, box.w, box.h); } break;
			case 3: { box = Rect(screenOffsetY + box.y,                       windowHeight - screenOffsetX - box.x - box.w, box.h, box.w); } break;
		}

		// make sure the box fits within clipStack.top(), and clip further accordingly
		if(clipStack.size())
		{
			const Rect& top = clipStack.top();
			if( top.x          >  box.x)          box.x = top.x;
			if( top.y          >  box.y)          box.y = top.y;
			if((top.x + top.w) < (box.x + box.w)) box.w = (top.x + top.w) - box.x;
			if((top.y + top.h) < (box.y + box.h)) box.h = (top.y + top.h) - box.y;
		}

		if(box.w < 0) box.w = 0;
		if(box.h < 0) box.h = 0;

		clipStack.push(box);

		setScissor(box);

	} // pushClipRect

//////////////////////////////////////////////////////////////////////////

	void popClipRect()
	{
		if(clipStack.empty())
		{
			LOG(LogError) << "Tried to popClipRect while the stack was empty!";
			return;
		}

		clipStack.pop();

		if(clipStack.empty()) setScissor(Rect(0, 0, 0, 0));
		else                  setScissor(clipStack.top());

	} // popClipRect

//////////////////////////////////////////////////////////////////////////

	void drawRect(const float _x, const float _y, const float _w, const float _h, const unsigned int _color, const unsigned int _colorEnd, bool horizontalGradient, const Blend::Factor _srcBlendFactor, const Blend::Factor _dstBlendFactor)
	{
		const unsigned int color    = convertColor(_color);
		const unsigned int colorEnd = convertColor(_colorEnd);
		Vertex             vertices[4];

		vertices[0] = { { _x     ,_y      }, { 0.0f, 0.0f }, color };
		vertices[1] = { { _x     ,_y + _h }, { 0.0f, 0.0f }, horizontalGradient ? colorEnd : color };
		vertices[2] = { { _x + _w,_y      }, { 0.0f, 0.0f }, horizontalGradient ? color : colorEnd };
		vertices[3] = { { _x + _w,_y + _h }, { 0.0f, 0.0f }, colorEnd };

		// round vertices
		for(int i = 0; i < 4; ++i)
			vertices[i].pos.round();

		bindTexture(0);
		drawTriangleStrips(vertices, 4, _srcBlendFactor, _dstBlendFactor);

	} // drawRect

//////////////////////////////////////////////////////////////////////////

	SDL_Window* getSDLWindow()     { return sdlWindow; }
	int         getWindowWidth()   { return windowWidth; }
	int         getWindowHeight()  { return windowHeight; }
	int         getScreenWidth()   { return screenWidth; }
	int         getScreenHeight()  { return screenHeight; }
	int         getScreenOffsetX() { return screenOffsetX; }
	int         getScreenOffsetY() { return screenOffsetY; }
	int         getScreenRotate()  { return screenRotate; }

} // Renderer::
