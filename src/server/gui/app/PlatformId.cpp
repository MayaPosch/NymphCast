#include "PlatformId.h"

#include <string.h>

namespace PlatformIds
{
	const char* PlatformNames[PLATFORM_COUNT + 1] = {
		"unknown", // nothing set

		"3do",
		"amiga",
		"amstradcpc",
		"apple2",
		"arcade",
		"atari800",
		"atari2600",
		"atari5200",
		"atari7800",
		"atarilynx",
		"atarist",
		"atarijaguar",
		"atarijaguarcd",
		"atarixe",
		"bbcmicro",
		"colecovision",
		"c64", // commodore 64
		"daphne",
		"intellivision",
		"macintosh",
		"xbox",
		"xbox360",
		"msx",
		"neogeo",
		"neogeocd",
		"ngp", // neo geo pocket
		"ngpc", // neo geo pocket color
		"n3ds", // nintendo 3DS
		"n64", // nintendo 64
		"nds", // nintendo DS
		"fds", // Famicom Disk System
		"nes", // nintendo entertainment system
		"pokemini",
		"channelf", // Fairchild ChannelF
		"gb", // game boy
		"gba", // game boy advance
		"gbc", // game boy color
		"gc", // gamecube
		"wii",
		"wiiu",
		"virtualboy",
		"gameandwatch",
		"openbor",
		"pc",
		"sega32x",
		"segacd",
		"dreamcast",
		"gamegear",
		"genesis", // sega genesis
		"mastersystem", // sega master system
		"megadrive", // sega megadrive
		"saturn", // sega saturn
		"sg-1000",
		"psx",
		"ps2",
		"ps3",
		"ps4",
		"psvita",
		"psp", // playstation portable
		"snes", // super nintendo entertainment system
		"scummvm",
		"x1",
		"x68000",
		"solarus",
		"moto", // Thomson MO/TO
		"pc88", // NEC PC-8801
		"pc98", // NEC PC-9801
		"pcengine", // (aka turbografx-16) HuCards only
		"pcenginecd", // (aka turbografx-16) CD-ROMs only
		"wonderswan",
		"wonderswancolor",
		"zxspectrum",
		"zx81",
		"videopac",
		"vectrex",
		"trs-80",
		"coco",

		"ignore", // do not allow scraping for this system
		"invalid"
	};

	PlatformId getPlatformId(const char* str)
	{
		if(str == NULL)
			return PLATFORM_UNKNOWN;

		for(unsigned int i = 1; i < PLATFORM_COUNT; i++)
		{
			if(strcmp(PlatformNames[i], str) == 0)
				return (PlatformId)i;
		}

		return PLATFORM_UNKNOWN;
	}

	const char* getPlatformName(PlatformId id)
	{
		return PlatformNames[id];
	}
}
