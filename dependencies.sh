#!/bin/sh

# Install the dependencies.
PLATFORM="unknown"
case "$(uname -s)" in
	Darwin)
		echo 'Mac OS X'
		PLATFORM="macos"
		if [ -x "$(command -v brew)" ]; then
			brew update
			brew install sdl2 sdl2_image poco ffmpeg freetype freeimage rapidjson pkg-config curl
		fi
		;;

	Linux)
		echo 'Linux'
		PLATFORM="linux"
		if [ -x "$(command -v pkg)" ]; then
			pkg upgrade
			pkg install git clang sdl2 sdl2-image libpoco ffmpeg pkg-config freetype freeimage rapidjson libcurl
		elif [ -x "$(command -v apt)" ]; then
			sudo apt update
			sudo apt -y install git g++ libsdl2-image-dev libsdl2-dev libpoco-dev libswscale-dev libavcodec-dev libavdevice-dev libavformat-dev libavutil-dev libpostproc-dev libswresample-dev pkg-config libfreetype6-dev libfreeimage-dev rapidjson-dev libcurl4-gnutls-dev
		elif [ -x "$(command -v apk)" ]; then
			sudo apk update
			sudo apk add poco-dev sdl2-dev sdl2_image-dev ffmpeg-dev openssl-dev freetype-dev freeimage-dev rapidjson-dev alsa-lib-dev glew-dev nymphrpc-dev curl-dev pkgconfig
		elif [ -x "$(command -v pacman)" ]; then
			sudo pacman -Syy 
			sudo pacman -S --noconfirm --needed git sdl2 sdl2_image poco ffmpeg freetype2 freeimage rapidjson pkgconf curl
		fi
		;;

	CYGWIN*|MINGW32*|MSYS*|MINGW*)
		echo 'MS Windows/MinGW'
		PLATFORM="mingw"
		PF=${MINGW_PACKAGE_PREFIX}-
		if [ -x "$(command -v pacman)" ]; then
			pacman -Syy 
			#pacman -S --noconfirm --needed git mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-poco mingw-w64-x86_64-ffmpeg mingw-w64-x86_64-freetype mingw-w64-x86_64-freeimage mingw-w64-x86_64-rapidjson pkgconf curl
			pacman -S --noconfirm --needed git ${PF}gcc ${PF}SDL2 ${PF}SDL2_image ${PF}poco ${PF}ffmpeg ${PF}freetype ${PF}freeimage ${PF}rapidjson pkgconf curl
		fi
		
		# Bail out here for now until MSYS2 support is implemented for the rest.
		#echo 'Install libnymphrpc & libnymphcast before building server.'
		#exit
		;;
		
	FreeBSD)
		echo 'Detected FreeBSD'
		PLATFORM="freebsd"
		pkg install -y gmake gcc git poco sdl2 sdl2_image ffmpeg openssl freetype2 freeimage rapidjson pkgconf curl
		;;
		
	Haiku)
		echo 'Haiku'
		PLATFORM="haiku"
		if [ -x "$(command -v pkgman)" ]; then
			pkgman install git poco poco_devel libsdl2 libsdl2_devel sdl2_image sdl2_image_devel ffmpeg ffmpeg6_devel freetype freetype_devel freeimage freeimage_devel rapidjson curl curl_devel
		fi
		;;

	*)
		echo 'Unsupported OS'
		exit
		;;
esac
