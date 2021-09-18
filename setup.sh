#!/bin/sh

# Compilation script for the NymphCast server
echo "UPDATE: $UPDATE"
echo "PACKAGE: $PACKAGE"

# Install the dependencies.
PLATFORM="unknown"
case "$(uname -s)" in
	Darwin)
		echo 'Mac OS X'
		PLATFORM="macos"
		if [ -x "$(command -v brew)" ]; then
			brew update
			brew install sdl2 sdl2_image poco ffmpeg freetype freeimage rapidjson pkg-config curl 
			brew install --cask vlc
		fi
		;;

	Linux)
		echo 'Linux'
		PLATFORM="linux"
		if [ -x "$(command -v apt)" ]; then
			sudo apt update
			sudo apt -y install git g++ libsdl2-image-dev libsdl2-dev libpoco-dev libswscale-dev libavcodec-dev libavdevice-dev libavformat-dev libavutil-dev libpostproc-dev libswresample-dev pkg-config libfreetype6-dev libfreeimage-dev rapidjson-dev libcurl4-gnutls-dev libvlc-dev
		elif [ -x "$(command -v apk)" ]; then
			sudo apk update
			sudo apk add poco-dev sdl2-dev sdl2_image-dev ffmpeg-dev openssl-dev freetype-dev freeimage-dev rapidjson-dev alsa-lib-dev glew-dev nymphrpc-dev curl-dev vlc-dev
		elif [ -x "$(command -v pacman)" ]; then
			sudo pacman -Syy 
			sudo pacman -S --noconfirm --needed git sdl2 sdl2_image poco ffmpeg freetype2 freeimage rapidjson pkgconf curl vlc
		fi
		;;

	CYGWIN*|MINGW32*|MSYS*|MINGW*)
		echo 'MS Windows/MinGW'
		PLATFORM="mingw"
		if [ -x "$(command -v pacman)" ]; then
			pacman -Syy 
			pacman -S --noconfirm --needed git mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-poco mingw-w64-x86_64-ffmpeg mingw-w64-x86_64-freetype mingw-w64-x86_64-freeimage mingw-w64-x86_64-rapidjson pkgconf curl mingw-w64-x86_64-vlc
		fi
		
		# Bail out here for now until MSYS2 support is implemented for the rest.
		#echo 'Install libnymphrpc & libnymphcast before building server.'
		#exit
		;;

	*)
		echo 'Unsupported OS'
		exit
		;;
esac


if [ -n "${UPDATE}" ]; then
	if [ "${PLATFORM}" == "linux" ]; then
		if [ -f "/usr/local/lib/libnymphrpc.a" ]; then
			sudo rm /usr/local/lib/libnymphrpc.*
			sudo rm -rf /usr/local/include/nymph
		fi
	elif [ "${PLATFORM}" == "mingw" ]; then
		if [ -f "/mingw64/lib/libnymphrpc.a" ]; then
			rm /mingw64/lib/libnymphrpc.a
		fi
	fi
fi

if [ -f "/usr/lib/libnymphrpc.so" ]; then
	echo "NymphRPC dynamic library found in /usr/lib. Skipping installation."
elif [ -f "/mingw64/lib/libnymphrpc.so" ]; then
	echo "NymphRPC dynamic library found in /mingw64/lib. Skipping installation."
else
	# Obtain current version of NymphRPC
	git clone --depth 1 https://github.com/MayaPosch/NymphRPC.git
	
	# Build NymphRPC and install it.
	echo "Installing NymphRPC..."
	make -C NymphRPC/ lib
	if [ "${PLATFORM}" == "mingw" ]; then
		make -C NymphRPC/ install
	else
		sudo make -C NymphRPC/ install
	fi
fi

# Remove NymphRPC folder.
rm -rf NymphRPC

# Build NymphCast client library.
#make -C src/client_lib/ clean
#make -C src/client_lib/
if [ -f "/usr/lib/libnymphcast.so" ]; then
	echo "LibNymphCast dynamic library found in /usr/lib. Skipping installation."
elif [ -f "/mingw64/lib/libnymphcast.so" ]; then
	echo "LibNymphCast dynamic library found in /mingw64/lib. Skipping installation."
else
	# Obtain current version of LibNymphCast
	git clone --depth 1 https://github.com/MayaPosch/libnymphcast.git
	
	# Build libnymphcast and install it.
	echo "Installing LibNymphCast..."
	make -C libnymphcast/ lib
	if [ "${PLATFORM}" == "mingw" ]; then
		make -C libnymphcast/ install
	else 
		sudo make -C libnymphcast/ install
	fi
fi

# Install client library
#sudo make -C src/client_lib/ install

# Build NymphCast server.
#make -C src/server/ clean
make -C src/server/

# Copy the wallpaper files into the bin folder.
#mkdir -p src/server/bin/wallpapers
#cp src/server/*.jpg src/server/bin/wallpapers/.

# Copy the configuration file into the bin folder.
#cp src/server/*.ini src/server/bin/.

# Copy the NymphCast apps into the bin folder.
#cp -r src/server/apps src/server/bin/.

if [ ! -z "${PACKAGE}" ]; then
	# Package into a tar.gz
	echo "Packaging into tar.gz file."
	tar -czC src/server/bin -f nymphcast_server.tar.gz .
fi
