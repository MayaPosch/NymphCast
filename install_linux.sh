#!/bin/sh

# NymphCast installer for the Linux platform.
# Requires that the binaries have been compiled with the 'setup.sh' script first.

# Copy the files to the target folder.
sudo make -C src/server/ install

# Install systemd or openrc service.
if [ -d "/run/systemd/system" ]; then
	sudo make -C src/server/ install-systemd
	sudo ln -s /etc/systemd/system/nymphcast.service /etc/systemd/system/multi-user.target.wants/nymphcast.service
else
	sudo make -C src/server/ install-openrc
fi

# Install Avahi service.
sudo install -m666 src/server/avahi/nymphcast.service /etc/avahi/services/.
