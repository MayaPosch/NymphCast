#if defined(USE_OPENGLES_10)

#include "renderers/Renderer.h"
#include "math/Transform4x4f.h"
#include "Log.h"
#include "Settings.h"

#include <SDL_opengles.h>
#include <SDL.h>

//////////////////////////////////////////////////////////////////////////

namespace Renderer
{

#if defined(_DEBUG)
#define GL_CHECK_ERROR(Function) (Function, _GLCheckError(#Function))

	static void _GLCheckError(const char* _funcName)
	{
		const GLenum errorCode = glGetError();

		if(errorCode != GL_NO_ERROR)
			LOG(LogError) << "GL error: " << _funcName << " failed with error code: " << errorCode;
	}
#else
#define GL_CHECK_ERROR(Function) (Function)
#endif

//////////////////////////////////////////////////////////////////////////

	static SDL_GLContext sdlContext   = nullptr;
	static GLuint        whiteTexture = 0;

//////////////////////////////////////////////////////////////////////////

	static GLenum convertBlendFactor(const Blend::Factor _blendFactor)
	{
		switch(_blendFactor)
		{
			case Blend::ZERO:                { return GL_ZERO;                } break;
			case Blend::ONE:                 { return GL_ONE;                 } break;
			case Blend::SRC_COLOR:           { return GL_SRC_COLOR;           } break;
			case Blend::ONE_MINUS_SRC_COLOR: { return GL_ONE_MINUS_SRC_COLOR; } break;
			case Blend::SRC_ALPHA:           { return GL_SRC_ALPHA;           } break;
			case Blend::ONE_MINUS_SRC_ALPHA: { return GL_ONE_MINUS_SRC_ALPHA; } break;
			case Blend::DST_COLOR:           { return GL_DST_COLOR;           } break;
			case Blend::ONE_MINUS_DST_COLOR: { return GL_ONE_MINUS_DST_COLOR; } break;
			case Blend::DST_ALPHA:           { return GL_DST_ALPHA;           } break;
			case Blend::ONE_MINUS_DST_ALPHA: { return GL_ONE_MINUS_DST_ALPHA; } break;
			default:                         { return GL_ZERO;                }
		}

	} // convertBlendFactor

//////////////////////////////////////////////////////////////////////////

	static GLenum convertTextureType(const Texture::Type _type)
	{
		switch(_type)
		{
			case Texture::RGBA:  { return GL_RGBA;  } break;
			case Texture::ALPHA: { return GL_ALPHA; } break;
			default:             { return GL_ZERO;  }
		}

	} // convertTextureType

//////////////////////////////////////////////////////////////////////////

	unsigned int convertColor(const unsigned int _color)
	{
		// convert from rgba to abgr
		const unsigned char r = ((_color & 0xff000000) >> 24) & 255;
		const unsigned char g = ((_color & 0x00ff0000) >> 16) & 255;
		const unsigned char b = ((_color & 0x0000ff00) >>  8) & 255;
		const unsigned char a = ((_color & 0x000000ff)      ) & 255;

		return ((a << 24) | (b << 16) | (g << 8) | (r));

	} // convertColor

//////////////////////////////////////////////////////////////////////////

	unsigned int getWindowFlags()
	{
		return SDL_WINDOW_OPENGL;

	} // getWindowFlags

//////////////////////////////////////////////////////////////////////////

	void setupWindow()
	{
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE,           8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,         8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,          8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,        24);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,       1);
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	} // setupWindow

//////////////////////////////////////////////////////////////////////////

	void createContext()
	{
		sdlContext = SDL_GL_CreateContext(getSDLWindow());
		SDL_GL_MakeCurrent(getSDLWindow(), sdlContext);

		const std::string vendor     = glGetString(GL_VENDOR)     ? (const char*)glGetString(GL_VENDOR)     : "";
		const std::string renderer   = glGetString(GL_RENDERER)   ? (const char*)glGetString(GL_RENDERER)   : "";
		const std::string version    = glGetString(GL_VERSION)    ? (const char*)glGetString(GL_VERSION)    : "";
		const std::string extensions = glGetString(GL_EXTENSIONS) ? (const char*)glGetString(GL_EXTENSIONS) : "";

		LOG(LogInfo) << "GL vendor:   " << vendor;
		LOG(LogInfo) << "GL renderer: " << renderer;
		LOG(LogInfo) << "GL version:  " << version;
		LOG(LogInfo) << "Checking available OpenGL extensions...";
		LOG(LogInfo) << " ARB_texture_non_power_of_two: " << (extensions.find("ARB_texture_non_power_of_two") != std::string::npos ? "ok" : "MISSING");

		const uint8_t data[4] = {255, 255, 255, 255};
		whiteTexture = createTexture(Texture::RGBA, false, true, 1, 1, data);

		GL_CHECK_ERROR(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
		GL_CHECK_ERROR(glEnable(GL_TEXTURE_2D));
		GL_CHECK_ERROR(glEnable(GL_BLEND));
		GL_CHECK_ERROR(glPixelStorei(GL_PACK_ALIGNMENT, 1));
		GL_CHECK_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
		GL_CHECK_ERROR(glEnableClientState(GL_VERTEX_ARRAY));
		GL_CHECK_ERROR(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		GL_CHECK_ERROR(glEnableClientState(GL_COLOR_ARRAY));

	} // createContext

//////////////////////////////////////////////////////////////////////////

	void destroyContext()
	{
		SDL_GL_DeleteContext(sdlContext);
		sdlContext = nullptr;

	} // destroyContext

//////////////////////////////////////////////////////////////////////////

	unsigned int createTexture(const Texture::Type _type, const bool _linear, const bool _repeat, const unsigned int _width, const unsigned int _height, const void* _data)
	{
		const GLenum type = convertTextureType(_type);
		unsigned int texture;

		GL_CHECK_ERROR(glGenTextures(1, &texture));
		GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));

		GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE));
		GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE));

		GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _linear ? GL_LINEAR : GL_NEAREST));
		GL_CHECK_ERROR(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

		GL_CHECK_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, type, _width, _height, 0, type, GL_UNSIGNED_BYTE, _data));

		return texture;

	} // createTexture

//////////////////////////////////////////////////////////////////////////

	void destroyTexture(const unsigned int _texture)
	{
		GL_CHECK_ERROR(glDeleteTextures(1, &_texture));

	} // destroyTexture

//////////////////////////////////////////////////////////////////////////

	void updateTexture(const unsigned int _texture, const Texture::Type _type, const unsigned int _x, const unsigned _y, const unsigned int _width, const unsigned int _height, const void* _data)
	{
		const GLenum type = convertTextureType(_type);

		GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, _texture));
		GL_CHECK_ERROR(glTexSubImage2D(GL_TEXTURE_2D, 0, _x, _y, _width, _height, type, GL_UNSIGNED_BYTE, _data));
		GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, whiteTexture));

	} // updateTexture

//////////////////////////////////////////////////////////////////////////

	void bindTexture(const unsigned int _texture)
	{
		if(_texture == 0) GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, whiteTexture));
		else              GL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, _texture));

	} // bindTexture

//////////////////////////////////////////////////////////////////////////

	void drawLines(const Vertex* _vertices, const unsigned int _numVertices, const Blend::Factor _srcBlendFactor, const Blend::Factor _dstBlendFactor)
	{
		GL_CHECK_ERROR(glVertexPointer(  2, GL_FLOAT,         sizeof(Vertex), &_vertices[0].pos));
		GL_CHECK_ERROR(glTexCoordPointer(2, GL_FLOAT,         sizeof(Vertex), &_vertices[0].tex));
		GL_CHECK_ERROR(glColorPointer(   4, GL_UNSIGNED_BYTE, sizeof(Vertex), &_vertices[0].col));

		GL_CHECK_ERROR(glBlendFunc(convertBlendFactor(_srcBlendFactor), convertBlendFactor(_dstBlendFactor)));

		GL_CHECK_ERROR(glDrawArrays(GL_LINES, 0, _numVertices));

	} // drawLines

//////////////////////////////////////////////////////////////////////////

	void drawTriangleStrips(const Vertex* _vertices, const unsigned int _numVertices, const Blend::Factor _srcBlendFactor, const Blend::Factor _dstBlendFactor)
	{
		GL_CHECK_ERROR(glVertexPointer(  2, GL_FLOAT,         sizeof(Vertex), &_vertices[0].pos));
		GL_CHECK_ERROR(glTexCoordPointer(2, GL_FLOAT,         sizeof(Vertex), &_vertices[0].tex));
		GL_CHECK_ERROR(glColorPointer(   4, GL_UNSIGNED_BYTE, sizeof(Vertex), &_vertices[0].col));

		GL_CHECK_ERROR(glBlendFunc(convertBlendFactor(_srcBlendFactor), convertBlendFactor(_dstBlendFactor)));

		GL_CHECK_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, _numVertices));

	} // drawTriangleStrips

//////////////////////////////////////////////////////////////////////////

	void setProjection(const Transform4x4f& _projection)
	{
		GL_CHECK_ERROR(glMatrixMode(GL_PROJECTION));
		GL_CHECK_ERROR(glLoadMatrixf((GLfloat*)&_projection));

	} // setProjection

//////////////////////////////////////////////////////////////////////////

	void setMatrix(const Transform4x4f& _matrix)
	{
		Transform4x4f matrix = _matrix;
		matrix.round();

		GL_CHECK_ERROR(glMatrixMode(GL_MODELVIEW));
		GL_CHECK_ERROR(glLoadMatrixf((GLfloat*)&matrix));

	} // setMatrix

//////////////////////////////////////////////////////////////////////////

	void setViewport(const Rect& _viewport)
	{
		// glViewport starts at the bottom left of the window
		GL_CHECK_ERROR(glViewport( _viewport.x, getWindowHeight() - _viewport.y - _viewport.h, _viewport.w, _viewport.h));

	} // setViewport

//////////////////////////////////////////////////////////////////////////

	void setScissor(const Rect& _scissor)
	{
		if((_scissor.x == 0) && (_scissor.y == 0) && (_scissor.w == 0) && (_scissor.h == 0))
		{
			GL_CHECK_ERROR(glDisable(GL_SCISSOR_TEST));
		}
		else
		{
			// glScissor starts at the bottom left of the window
			GL_CHECK_ERROR(glScissor(_scissor.x, getWindowHeight() - _scissor.y - _scissor.h, _scissor.w, _scissor.h));
			GL_CHECK_ERROR(glEnable(GL_SCISSOR_TEST));
		}

	} // setScissor

//////////////////////////////////////////////////////////////////////////

	void setSwapInterval()
	{
		// vsync
		if(Settings::getInstance()->getBool("VSync"))
		{
			// SDL_GL_SetSwapInterval(0) for immediate updates (no vsync, default),
			// 1 for updates synchronized with the vertical retrace,
			// or -1 for late swap tearing.
			// SDL_GL_SetSwapInterval returns 0 on success, -1 on error.
			// if vsync is requested, try normal vsync; if that doesn't work, try late swap tearing
			// if that doesn't work, report an error
			if(SDL_GL_SetSwapInterval(1) != 0 && SDL_GL_SetSwapInterval(-1) != 0)
				LOG(LogWarning) << "Tried to enable vsync, but failed! (" << SDL_GetError() << ")";
		}
		else
			SDL_GL_SetSwapInterval(0);

	} // setSwapInterval

//////////////////////////////////////////////////////////////////////////

	void swapBuffers()
	{
		SDL_GL_SwapWindow(getSDLWindow());
		GL_CHECK_ERROR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	} // swapBuffers

} // Renderer::

#endif // USE_OPENGLES_10
