#!/bin/sh

# Nymphcast uninstaller for the Linux platform.

# Remove the NymphCast service.
if [ -d "/run/systemd/system" ]; then
	echo "Removing systemd service..."
	#sudo systemctl disable nymphcast.service
	systemctl --user disable nymphcast.service
	sudo rm /etc/systemd/user/nymphcast.service
else
	echo "Removing OpenRC service..."
	sudo make -C src/server/ uninstall-openrc
fi

# Remove the files.
sudo make -C src/server/ uninstall

echo "Uninstalling NymphCast complete."
