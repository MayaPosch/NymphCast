#if defined(_WIN32)
#  include "config_sdl_win32.h"

#elif defined(__APPLE__) || defined(__ANDROID__) || defined(__unix__)
#  include "config_sdl_unix.h"

#else
#error generate a harfbuzz config for your platform
#endif
