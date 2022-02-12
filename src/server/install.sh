#!/bin/sh

# Installation script for a packaged version of NymphCast Server (NCS).
# To be used with the 'package' target of the NCS Makefile: 
#	- 'make package' copies this installation script into the resulting .tar.gz package.
#	- After unpackaging the archive, running this script will install NCS's components.

# Ensure we got all dependencies in place.
if [ -x "$(command -v apt)" ]; then
	sudo apt update
	sudo apt -y install git g++ libsdl2-image-dev libsdl2-dev libpoco-dev libswscale-dev libavcodec-dev libavdevice-dev libavformat-dev libavutil-dev libpostproc-dev libswresample-dev pkg-config libfreetype6-dev libfreeimage-dev rapidjson-dev libcurl4-gnutls-dev libvlc-dev
elif [ -x "$(command -v apk)" ]; then
	sudo apk update
	sudo apk add poco-dev sdl2-dev sdl2_image-dev ffmpeg-dev openssl-dev freetype-dev freeimage-dev rapidjson-dev alsa-lib-dev glew-dev nymphrpc-dev curl-dev vlc-dev pkg-config
elif [ -x "$(command -v pacman)" ]; then
	sudo pacman -Syy 
	sudo pacman -S --noconfirm --needed git sdl2 sdl2_image poco ffmpeg freetype2 freeimage rapidjson pkgconf curl vlc
fi


# Copy the files to their locations.
sudo cp -a lib/* /usr/lib/.
sudo install -d /usr/local/etc/nymphcast/ \
		-d /usr/local/share/nymphcast/apps/ \
		-d /usr/local/share/nymphcast/wallpapers/
sudo install -m 755 bin/nymphcast_server /usr/local/bin/
sudo install -m 644 *.ini /usr/local/etc/nymphcast/
sudo cp -r apps/* /usr/local/share/nymphcast/apps/
sudo install -m 644 wallpapers/* /usr/local/share/nymphcast/wallpapers/

# Ask to install a system service to start NCS automatically
read -p "Install systemd service for NymphCast? [y/n] (default: n): " choice
if [ "$choice" = "y" ]; then
	echo "Installing systemd service..."
	sudo cp systemd/nymphcast.service /etc/systemd/user/nymphcast.service
	#sudo systemctl enable nymphcast.service
	systemctl --user enable nymphcast.service
else
	echo "Skipping system service installation..."
fi

# Set the requested configuration file.
read -p "Desired NymphCast receiver configuration? [audio/video/screensaver/gui] " choice

case $choice in
	audio)
		echo "Setting audio configuration..."
		sudo cp nymphcast_audio_config.ini /usr/local/etc/nymphcast/nymphcast_config.ini
		;;
		
	video)
		echo "Setting video configuration..."
		sudo cp nymphcast_video_config.ini /usr/local/etc/nymphcast/nymphcast_config.ini
		;;
		
	screensaver)
		echo "Setting screensaver configuration..."
		sudo cp nymphcast_screensaver_config.ini /usr/local/etc/nymphcast/nymphcast_config.ini
		;;
		
	gui)
		echo "Setting GUI configuration..."
		sudo cp nymphcast_gui_config.ini /usr/local/etc/nymphcast/nymphcast_config.ini
		;;
		
	*)
		echo "Unrecognised choice. Please set configuration manually."
		
		;;
esac
