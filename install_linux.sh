#!/bin/sh

# NymphCast installer for the Linux platform.
# Requires that the binaries have been compiled with the 'setup.sh' script first.

TARGET_FOLDER="/opt/nymphcast"

# Check for the presence of the target folder
if [ -f "${TARGET_FOLDER}" ]; then
	echo "Existing NymphCast folder found. Upgrading..."
else
	sudo mkdir -p "${TARGET_FOLDER}"
fi

# Copy the files to the target folder.
sudo cp -r src/server/bin/* "${TARGET_FOLDER}"

# Install systemd service.
#if [ ! -f "/etc/systemd/system/nymphcast.service" ]; then
	sudo cp src/server/systemd/nymphcast.service /etc/systemd/system/.
	sudo ln -s /etc/systemd/system/nymphcast.service /etc/systemd/system/multi-user.target.wants/nymphcast.service
	# sudo systemctl start nymphcast.service
# else
	# echo "SystemD service was already installed. Skipping..."
# fi

# Install Avahi service.
#if [ ! -f "/etc/systemd/system/nymphcast.service" ]; then
	sudo cp src/server/avahi/nymphcast.service /etc/avahi/services/.
	#sudo systemctl restart avahi-daemon
# else
	# echo "Avahi service was already installed. Skipping..."
# fi
