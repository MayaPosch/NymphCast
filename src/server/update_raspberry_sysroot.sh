#!/bin/sh
# Update the local sysroot of a Raspberry Pi OS system using rsync.

# Obtain the target (remote) name or IP.
read -p "Target host Raspberry Pi system? [raspberrypi] " target

if [[ -z "$target" ]]; then
	target='raspberrypi'
fi

# Get the local folder to sync into.
folder=$PWD
read -p "Target local sysroot folder? [${folder}] " folder

if [[ -z "$folder" ]]; then
	folder=$PWD
fi

# Output information.
echo "Connecting to host ${target} to copy into folder ${folder}"

#rsync -vR --progress -rl --delete-after --safe-links pi@192.168.1.PI:/{lib,usr,opt/vc/lib} $HOME/raspberrypi/rootfs
rsync -vR --progress -rl --delete-after --safe-links pi@${target}:/{lib,usr/include,usr/lib,opt/vc/lib} $folder