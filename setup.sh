#!/bin/sh

# Compilation script for the NymphCast server
echo "UPDATE: $UPDATE"
echo "PACKAGE: $PACKAGE"

# Install the dependencies.
if [ -x "$(command -v apt)" ]; then
	sudo apt update
	sudo apt -y install libsdl2-image-dev libsdl2-dev libpoco-dev libswscale-dev libavcodec-dev libavdevice-dev libavformat-dev libavutil-dev libpostproc-dev libswresample-dev
elif [ -x "$(command -v apk)" ]; then
	sudo apk update
	sudo apk add poco-dev sdl2-dev sdl2_image-dev ffmpeg-dev openssl-dev
fi

if [ ! -z "${UPDATE}" ]; then
	if [ -f "/usr/local/lib/libnymphrpc.a" ]; then
		sudo rm /usr/local/lib/libnymphrpc.a
		sudo rm -rf /usr/local/include/nymph
	fi
fi

if [ -f "/usr/local/lib/libnymphrpc.a" ]; then
	echo "NymphRPC library found in /usr/local/lib. Skipping installation."
else
	# Obtain current version of NymphRPC
	git clone --depth 1 https://github.com/MayaPosch/NymphRPC.git
	
	# Build NymphRPC and install it.
	echo "Installing NymphRPC..."
	make -C NymphRPC/ lib
	sudo mkdir -p /usr/local/include/nymph
	sudo cp NymphRPC/src/*.h /usr/local/include/nymph/.
	sudo chmod 766 /usr/local/include/nymph/*
	sudo cp NymphRPC/lib/libnymphrpc.a /usr/local/lib/.
	sudo cp NymphRPC/lib/libnymphrpc.so.* /usr/lib/.
	sudo chmod 766 /usr/local/lib/libnymphrpc.a
	sudo chmod 766 /usr/lib/libnymphrpc.so*
fi

# Remove NymphRPC folder.
rm -rf NymphRPC

# Build NymphCast server.
if [ -f "src/server/bin/nymphcast_server" ]; then
	rm src/server/bin/nymphcast_server
fi

make -C src/server/ clean
make -C src/server/

# Copy the wallpaper files into the bin folder.
mkdir -p src/server/bin/wallpapers
cp src/server/*.jpg src/server/bin/wallpapers/.

# Copy the configuration file into the bin folder.
cp src/server/*.ini src/server/bin/.

# Copy the NymphCast apps into the bin folder.
cp -r src/server/apps src/server/bin/.

if [ ! -z "${PACKAGE}" ]; then
	# Package into a tar.gz
	echo "Packaging into tar.gz file."
	tar -czC src/server/bin -f nymphcast_server.tar.gz .
fi
