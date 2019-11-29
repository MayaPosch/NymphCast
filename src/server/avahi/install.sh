#!/bin/sh

sudo cp nymphcast.service /etc/avahi/services/.
sudo systemctl restart avahi-daemon
