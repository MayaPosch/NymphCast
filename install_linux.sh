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
if [ -d "/run/systemd/system" ]; then
	sudo cp src/server/systemd/nymphcast.service /etc/systemd/system/.
	sudo ln -s /etc/systemd/system/nymphcast.service /etc/systemd/system/multi-user.target.wants/nymphcast.service
else
	sudo cp src/server/openrc/nymphcast /etc/init.d/nymphcast
fi

# Install Avahi service.
sudo install -m666 src/server/avahi/nymphcast.service /etc/avahi/services/.
