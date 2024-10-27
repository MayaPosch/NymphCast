#!/bin/sh

# NymphCast installer for the Linux platform.
if [ "$(uname -s)" = "Linux" ]; then
	echo "Detected Linux system. Proceeding with installation..."
else
	echo "This installer requires a Linux system. Exiting..."
	exit
fi

# Requires that the binaries have been compiled with the 'setup.sh' script first.
PLATFORM=`g++ -dumpmachine`
if [ -f "src/server/bin/${PLATFORM}/nymphcast_server" ]; then
	echo "NymphCast Server binary found, skipping compilation..."
else
	echo "Compiling NymphCast server..."
	./setup.sh
fi

# Copy files to the target folders.
sudo make -C src/server/ install

# Set the requested configuration file.
read -p "Desired NymphCast receiver configuration? [audio/video/screensaver/gui] " choice

DESKTOP_INSTALL=true
case $choice in
	audio)
		echo "Setting Audio configuration..."
		sudo cp src/server/nymphcast_audio_config.ini /usr/local/etc/nymphcast/nymphcast_config.ini
		DESKTOP_INSTALL=false
		;;
		
	video)
		echo "Setting video configuration..."
		sudo cp src/server/nymphcast_video_config.ini /usr/local/etc/nymphcast/nymphcast_config.ini
		;;
		
	screensaver)
		echo "Setting screensaver configuration..."
		sudo cp src/server/nymphcast_screensaver_config.ini /usr/local/etc/nymphcast/nymphcast_config.ini
		;;
		
	gui)
		echo "Setting GUI configuration..."
		sudo cp src/server/nymphcast_gui_config.ini /usr/local/etc/nymphcast/nymphcast_config.ini
		;;
		
	*)
		echo "Unrecognised choice. Please set configuration manually."
		
		;;
esac

# Confirm installation of autostart/service.
read -p "Install autostart file/system service? (Y/N)" install_auto

case $install_auto in
	Y)
		echo "Installing..."
		;;
	*)
		echo "Installation finished."
		;;
esac

# If GUI, screensaver or video mode, install Desktop file for auto-start.
if [ "${DESKTOP_INSTALL}" = "true" ]; then
	# TODO: Install desktop file into $XDG_CONFIG_DIRS/autostart (/etc/xdg/autostart).
	#sudo cp src/server/autostart/nymphcast_server.desktop $XDG_CONFIG_DIRS/autostart
	echo "Installing .desktop file to /etc/xdg/autostart..."
	sudo cp src/server/autostart/nymphcast_server.desktop /etc/xdg/autostart
else
	# Install systemd or openrc service.
	if [ -d "/run/systemd/system" ]; then
		echo "Installing systemd service..."
		sudo make -C src/server/ install-systemd
		sudo chmod 644 /etc/systemd/user/nymphcast.service
		#sudo systemctl enable nymphcast.service
		systemctl --user enable nymphcast.service
	else
		echo "Installing OpenRC service..."
		sudo make -C src/server/ install-openrc
	fi
fi
